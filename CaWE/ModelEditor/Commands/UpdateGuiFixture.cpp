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

#include "UpdateGuiFixture.hpp"
#include "../ModelDocument.hpp"
#include "Models/Model_cmdl.hpp"


using namespace ModelEditor;


CommandUpdateGuiFixtureT::CommandUpdateGuiFixtureT(ModelDocumentT* ModelDoc, unsigned int GFNr, const CafuModelT::GuiFixtureT& GF)
    : m_ModelDoc(ModelDoc),
      m_GFNr(GFNr),
      m_NewGF(GF),
      m_OldGF(m_ModelDoc->GetModel()->GetGuiFixtures()[m_GFNr])
{
}


bool CommandUpdateGuiFixtureT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // If the GUI fixture didn't really change, don't put this command into the command history.
    if (m_NewGF.Name              ==m_OldGF.Name               &&
        m_NewGF.Points[0].MeshNr  ==m_OldGF.Points[0].MeshNr   &&
        m_NewGF.Points[0].VertexNr==m_OldGF.Points[0].VertexNr &&
        m_NewGF.Points[1].MeshNr  ==m_OldGF.Points[1].MeshNr   &&
        m_NewGF.Points[1].VertexNr==m_OldGF.Points[1].VertexNr &&
        m_NewGF.Points[2].MeshNr  ==m_OldGF.Points[2].MeshNr   &&
        m_NewGF.Points[2].VertexNr==m_OldGF.Points[2].VertexNr &&
        m_NewGF.Trans[0]          ==m_OldGF.Trans[0]           &&
        m_NewGF.Trans[1]          ==m_OldGF.Trans[1]           &&
        m_NewGF.Scale[0]          ==m_OldGF.Scale[0]           &&
        m_NewGF.Scale[1]          ==m_OldGF.Scale[1]) return false;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_GuiFixtures[m_GFNr],
    // because it's bound to become invalid whenever another command meddles with the array of GUI fixtures.
    m_ModelDoc->GetModel()->m_GuiFixtures[m_GFNr]=m_NewGF;

    m_ModelDoc->UpdateAllObservers_GuiFixtureChanged(m_GFNr);
    m_Done=true;
    return true;
}


void CommandUpdateGuiFixtureT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_GuiFixtures[m_GFNr],
    // because it's bound to become invalid whenever another command meddles with the array of GUI fixtures.
    m_ModelDoc->GetModel()->m_GuiFixtures[m_GFNr]=m_OldGF;

    m_ModelDoc->UpdateAllObservers_GuiFixtureChanged(m_GFNr);
    m_Done=false;
}


wxString CommandUpdateGuiFixtureT::GetName() const
{
    return "Update GUI fixture";
}
