/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "UpdateAnim.hpp"
#include "../ModelDocument.hpp"
#include "Models/Model_cmdl.hpp"


using namespace ModelEditor;


CommandUpdateAnimT::CommandUpdateAnimT(ModelDocumentT* ModelDoc, unsigned int AnimNr, const CafuModelT::AnimT& Anim)
    : m_ModelDoc(ModelDoc),
      m_AnimNr(AnimNr),
      m_NewAnim(Anim),
      m_OldAnim(m_ModelDoc->GetModel()->GetAnims()[m_AnimNr])
{
}


bool CommandUpdateAnimT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // If the anim didn't really change, don't put this command into the command history.
    // if (m_NewAnim==m_OldAnim) return false;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_Anims[m_AnimNr],
    // because it's bound to become invalid whenever another command meddles with the array of anims.
    m_ModelDoc->GetModel()->m_Anims[m_AnimNr]=m_NewAnim;

    m_ModelDoc->UpdateAllObservers_AnimChanged(m_AnimNr);
    m_Done=true;
    return true;
}


void CommandUpdateAnimT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_Anims[m_AnimNr],
    // because it's bound to become invalid whenever another command meddles with the array of anims.
    m_ModelDoc->GetModel()->m_Anims[m_AnimNr]=m_OldAnim;

    m_ModelDoc->UpdateAllObservers_AnimChanged(m_AnimNr);
    m_Done=false;
}


wxString CommandUpdateAnimT::GetName() const
{
    return "Update anim sequence";
}
