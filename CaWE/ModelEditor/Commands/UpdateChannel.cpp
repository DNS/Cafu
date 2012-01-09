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
