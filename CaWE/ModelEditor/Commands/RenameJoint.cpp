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

#include "RenameJoint.hpp"
#include "../ModelDocument.hpp"
#include "Models/Model_cmdl.hpp"


using namespace ModelEditor;


CommandRenameJointT::CommandRenameJointT(ModelDocumentT* ModelDoc, unsigned int JointNr, const wxString& NewName)
    : m_ModelDoc(ModelDoc),
      m_JointNr(JointNr),
      m_NewName(std::string(NewName)),
      m_OldName(m_ModelDoc->GetModel()->m_Joints[m_JointNr].Name)
{
}


bool CommandRenameJointT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // If the name didn't really change, don't put this command into the command history.
    if (m_NewName==m_OldName) return false;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_Joints[m_JointNr],
    // because it's bound to become invalid whenever another command meddles with the array of joints.
    m_ModelDoc->GetModel()->m_Joints[m_JointNr].Name=m_NewName;

    m_ModelDoc->UpdateAllObservers_JointChanged(m_JointNr);
    m_Done=true;
    return true;
}


void CommandRenameJointT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_Joints[m_JointNr],
    // because it's bound to become invalid whenever another command meddles with the array of joints.
    m_ModelDoc->GetModel()->m_Joints[m_JointNr].Name=m_OldName;

    m_ModelDoc->UpdateAllObservers_JointChanged(m_JointNr);
    m_Done=false;
}


wxString CommandRenameJointT::GetName() const
{
    return "Rename Joint";
}
