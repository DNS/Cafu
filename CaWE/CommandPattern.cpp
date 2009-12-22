/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#include "CommandPattern.hpp"
#include "MapElement.hpp"


/****************/
/*** CommandT ***/
/****************/


static unsigned long IDCounter=1;


CommandT::CommandT(bool ShowInHistory, bool SuggestsSave)
    : m_Done(false),
      m_ShowInHistory(ShowInHistory),
      m_SuggestsSave(SuggestsSave),
      m_ID(IDCounter++)
{
}


/*********************/
/*** CommandMacroT ***/
/*********************/

CommandMacroT::CommandMacroT(const ArrayT<CommandT*>& SubCommands, const wxString& Name, bool ShowInHistory, bool SuggestsSave)
    : CommandT(ShowInHistory, SuggestsSave),
      m_SubCommands(SubCommands),
      m_Name(Name)
{
    // Assume that m_Done is the same for all SubCommands.
    // That is, either they are all done or all not done. Everything else (any mix) is a bug.
    if (m_SubCommands.Size()>0) m_Done=m_SubCommands[0]->IsDone();
}


CommandMacroT::~CommandMacroT()
{
    for (unsigned long i=0; i<m_SubCommands.Size(); i++)
        delete m_SubCommands[i];
}


bool CommandMacroT::Do()
{
    wxASSERT(!m_Done);

    if (m_Done) return false;

    for (unsigned long CommandNr=0; CommandNr<m_SubCommands.Size(); CommandNr++)
        if (!m_SubCommands[CommandNr]->Do())
        {
            // The last command failed.
            // Now un-do the previous commands in opposite order.
            for (int UndoNr=CommandNr-1; UndoNr>=0; UndoNr--)
                m_SubCommands[UndoNr]->Undo();

            return false;
        }

    m_Done=true;

    return true;
}


void CommandMacroT::Undo()
{
    wxASSERT(m_Done);

    if (!m_Done) return;

    for (int CommandNr=m_SubCommands.Size()-1; CommandNr>=0; CommandNr--)
        m_SubCommands[CommandNr]->Undo();

    m_Done=false;
}


wxString CommandMacroT::GetName() const
{
    return m_Name;
}
