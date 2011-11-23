/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

Cafu is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

Cafu is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Cafu. If not, see <http://www.gnu.org/licenses/>.

For support and more information about Cafu, visit us at <http://www.cafu.de>.
=================================================================================
*/

#include "UpdateTriangle.hpp"
#include "../ModelDocument.hpp"
#include "Models/Model_cmdl.hpp"


using namespace ModelEditor;


CommandUpdateTriangleT::CommandUpdateTriangleT(ModelDocumentT* ModelDoc, unsigned int MeshNr, unsigned int TriNr, bool SkipDraw)
    : m_ModelDoc(ModelDoc),
      m_MeshNr(MeshNr),
      m_TriNr(TriNr),
      m_NewSkipDraw(SkipDraw),
      m_OldSkipDraw(m_ModelDoc->GetModel()->GetMeshes()[m_MeshNr].Triangles[m_TriNr].SkipDraw)
{
}


bool CommandUpdateTriangleT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // If the triangle didn't really change, don't put this command into the command history.
    if (m_NewSkipDraw==m_OldSkipDraw) return false;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_Meshes[m_MeshNr].Triangles[m_TriNr].SkipDraw,
    // because it's bound to become invalid whenever another command meddles with the arrays.
    m_ModelDoc->GetModel()->m_Meshes[m_MeshNr].Triangles[m_TriNr].SkipDraw=m_NewSkipDraw;

    // Make sure that the draw cache is refreshed.
    m_ModelDoc->GetAnimState().Pose.SetNeedsRecache();

    m_ModelDoc->UpdateAllObservers_MeshChanged(m_MeshNr);
    m_Done=true;
    return true;
}


void CommandUpdateTriangleT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_Meshes[m_MeshNr].Triangles[m_TriNr].SkipDraw,
    // because it's bound to become invalid whenever another command meddles with the arrays.
    m_ModelDoc->GetModel()->m_Meshes[m_MeshNr].Triangles[m_TriNr].SkipDraw=m_OldSkipDraw;

    // Make sure that the draw cache is refreshed.
    m_ModelDoc->GetAnimState().Pose.SetNeedsRecache();

    m_ModelDoc->UpdateAllObservers_MeshChanged(m_MeshNr);
    m_Done=false;
}


wxString CommandUpdateTriangleT::GetName() const
{
    return m_NewSkipDraw ? "Hide Triangle" : "Show Triangle";
}
