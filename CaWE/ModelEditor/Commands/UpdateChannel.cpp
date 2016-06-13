/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "UpdateChannel.hpp"
#include "../ModelDocument.hpp"
#include "Models/Model_cmdl.hpp"


using namespace ModelEditor;


CommandUpdateChannelT::CommandUpdateChannelT(ModelDocumentT* ModelDoc, unsigned int ChannelNr, unsigned int JointNr, bool IsMember)
    : m_ModelDoc(ModelDoc),
      m_ChannelNr(ChannelNr),
      m_JointNr(JointNr),
      m_NewIsMember(IsMember),
      m_OldIsMember(m_ModelDoc->GetModel()->GetChannels()[m_ChannelNr].IsMember(JointNr))
{
}


bool CommandUpdateChannelT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // If the joints membership in the channel didn't really change, don't put this command into the command history.
    if (m_NewIsMember==m_OldIsMember) return false;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_Channels[m_ChannelNr],
    // because it's bound to become invalid whenever another command meddles with the array of channels.
    m_ModelDoc->GetModel()->m_Channels[m_ChannelNr].SetMember(m_JointNr, m_NewIsMember);

    m_ModelDoc->UpdateAllObservers_ChannelChanged(m_ChannelNr);
    m_Done=true;
    return true;
}


void CommandUpdateChannelT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Cannot keep a reference to m_ModelDoc->GetModel()->m_Channels[m_ChannelNr],
    // because it's bound to become invalid whenever another command meddles with the array of channels.
    m_ModelDoc->GetModel()->m_Channels[m_ChannelNr].SetMember(m_JointNr, m_OldIsMember);

    m_ModelDoc->UpdateAllObservers_ChannelChanged(m_ChannelNr);
    m_Done=false;
}


wxString CommandUpdateChannelT::GetName() const
{
    return m_NewIsMember ? "Add joint to channel" : "Remove joint from channel";
}
