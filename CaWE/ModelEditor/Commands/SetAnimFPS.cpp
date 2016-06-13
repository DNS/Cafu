/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "SetAnimFPS.hpp"
#include "../ModelDocument.hpp"
#include "Models/Model_cmdl.hpp"


using namespace ModelEditor;


CommandSetAnimFPST::CommandSetAnimFPST(ModelDocumentT* ModelDoc, unsigned int AnimNr, float NewFPS)
    : m_ModelDoc(ModelDoc),
      m_AnimNr(AnimNr),
      m_NewFPS(NewFPS),
      m_OldFPS(m_ModelDoc->GetModel()->GetAnims()[m_AnimNr].FPS)
{
}


bool CommandSetAnimFPST::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // If the FPS didn't really change, don't put this command into the command history.
    if (m_NewFPS==m_OldFPS) return false;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_Anims[m_AnimNr] as a member,
    // because it's bound to become invalid whenever another command meddles with the array of anims.
    CafuModelT::AnimT& Anim=m_ModelDoc->GetModel()->m_Anims[m_AnimNr];

    Anim.FPS=m_NewFPS;

    m_ModelDoc->UpdateAllObservers_AnimChanged(m_AnimNr);
    m_Done=true;
    return true;
}


void CommandSetAnimFPST::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_Anims[m_AnimNr] as a member,
    // because it's bound to become invalid whenever another command meddles with the array of anims.
    CafuModelT::AnimT& Anim=m_ModelDoc->GetModel()->m_Anims[m_AnimNr];

    Anim.FPS=m_OldFPS;

    m_ModelDoc->UpdateAllObservers_AnimChanged(m_AnimNr);
    m_Done=false;
}


wxString CommandSetAnimFPST::GetName() const
{
    return "Assign anim FPS";
}
