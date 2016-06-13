/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
