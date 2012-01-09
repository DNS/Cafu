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

#include "SetAnimNext.hpp"
#include "../ModelDocument.hpp"
#include "Models/Model_cmdl.hpp"


using namespace ModelEditor;


CommandSetAnimNextT::CommandSetAnimNextT(ModelDocumentT* ModelDoc, unsigned int AnimNr, int NewNext)
    : m_ModelDoc(ModelDoc),
      m_AnimNr(AnimNr),
      m_NewNext(NewNext),
      m_OldNext(m_ModelDoc->GetModel()->GetAnims()[m_AnimNr].Next)
{
}


bool CommandSetAnimNextT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // If the next sequence didn't really change, don't put this command into the command history.
    if (m_NewNext==m_OldNext) return false;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_Anims[m_AnimNr] as a member,
    // because it's bound to become invalid whenever another command meddles with the array of anims.
    CafuModelT::AnimT& Anim=m_ModelDoc->GetModel()->m_Anims[m_AnimNr];

    Anim.Next=m_NewNext;

    m_ModelDoc->UpdateAllObservers_AnimChanged(m_AnimNr);
    m_Done=true;
    return true;
}


void CommandSetAnimNextT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_Anims[m_AnimNr] as a member,
    // because it's bound to become invalid whenever another command meddles with the array of anims.
    CafuModelT::AnimT& Anim=m_ModelDoc->GetModel()->m_Anims[m_AnimNr];

    Anim.Next=m_OldNext;

    m_ModelDoc->UpdateAllObservers_AnimChanged(m_AnimNr);
    m_Done=false;
}


wxString CommandSetAnimNextT::GetName() const
{
    return "Assign next anim sequence";
}
