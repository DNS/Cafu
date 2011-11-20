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

#include "TransformJoint.hpp"
#include "../ModelDocument.hpp"
#include "Models/Model_cmdl.hpp"


using namespace ModelEditor;


CommandTransformJointT::CommandTransformJointT(ModelDocumentT* ModelDoc, unsigned int JointNr, char Type, const Vector3fT& v)
    : m_ModelDoc(ModelDoc),
      m_JointNr(JointNr),
      m_Type(Type),
      m_NewVec(v),
      m_OldVec(GetModelVec())
{
}


bool CommandTransformJointT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // If the vector didn't really change, don't put this command into the command history.
    if (m_NewVec==m_OldVec) return false;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_Joints[m_JointNr],
    // because it's bound to become invalid whenever another command meddles with the array of joints.
    GetModelVec()=m_NewVec;

    // Make sure that the caches are refreshed.
    m_ModelDoc->GetAnimState().Pose.SetNeedsRecache();

    m_ModelDoc->UpdateAllObservers_JointChanged(m_JointNr);
    m_Done=true;
    return true;
}


void CommandTransformJointT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_Joints[m_JointNr],
    // because it's bound to become invalid whenever another command meddles with the array of joints.
    GetModelVec()=m_OldVec;

    // Make sure that the caches are refreshed.
    m_ModelDoc->GetAnimState().Pose.SetNeedsRecache();

    m_ModelDoc->UpdateAllObservers_JointChanged(m_JointNr);
    m_Done=false;
}


wxString CommandTransformJointT::GetName() const
{
    switch (m_Type)
    {
        case 'p': return "Translate Joint";
        case 'q': return "Rotate Joint";
    }

    return "Scale Joint";
}


Vector3fT& CommandTransformJointT::GetModelVec()
{
    switch (m_Type)
    {
        case 'p': return m_ModelDoc->GetModel()->m_Joints[m_JointNr].Pos;
        case 'q': return m_ModelDoc->GetModel()->m_Joints[m_JointNr].Qtr;
    }

    return m_ModelDoc->GetModel()->m_Joints[m_JointNr].Scale;
}
