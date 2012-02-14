/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#include "SetMeshTSMethod.hpp"
#include "../ModelDocument.hpp"


using namespace ModelEditor;


CommandSetMeshTSMethodT::CommandSetMeshTSMethodT(ModelDocumentT* ModelDoc, unsigned int MeshNr, CafuModelT::MeshT::TangentSpaceMethodT NewTSMethod)
    : m_ModelDoc(ModelDoc),
      m_MeshNr(MeshNr),
      m_NewTSMethod(NewTSMethod),
      m_OldTSMethod(m_ModelDoc->GetModel()->GetMeshes()[m_MeshNr].TSMethod)
{
}


bool CommandSetMeshTSMethodT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // If the TSMethod didn't really change, don't put this command into the command history.
    if (m_NewTSMethod==m_OldTSMethod) return false;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_Meshes[m_MeshNr] as a member,
    // because it's bound to become invalid whenever another command meddles with the array of meshes.
    CafuModelT::MeshT& Mesh=m_ModelDoc->GetModel()->m_Meshes[m_MeshNr];

    Mesh.TSMethod=m_NewTSMethod;

    // Make sure that the draw cache is refreshed.
    m_ModelDoc->GetAnimState().Pose.SetNeedsRecache();

    m_ModelDoc->UpdateAllObservers_MeshChanged(m_MeshNr);
    m_Done=true;
    return true;
}


void CommandSetMeshTSMethodT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_Meshes[m_MeshNr] as a member,
    // because it's bound to become invalid whenever another command meddles with the array of meshes.
    CafuModelT::MeshT& Mesh=m_ModelDoc->GetModel()->m_Meshes[m_MeshNr];

    Mesh.TSMethod=m_OldTSMethod;

    // Make sure that the draw cache is refreshed.
    m_ModelDoc->GetAnimState().Pose.SetNeedsRecache();

    m_ModelDoc->UpdateAllObservers_MeshChanged(m_MeshNr);
    m_Done=false;
}


wxString CommandSetMeshTSMethodT::GetName() const
{
    return "Set tangent-space method";
}
