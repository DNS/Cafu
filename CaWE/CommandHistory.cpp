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

#include "Options.hpp"
#include "CommandHistory.hpp"

#if defined(_WIN32) && defined(_MSC_VER)
    #if (_MSC_VER<1300)
        #define for if (false) ; else for
    #endif
#endif


CommandHistoryT::CommandHistoryT()
    : m_CurrentIndex(-1),
      m_InvalidCommandID(0)
{
}


CommandHistoryT::~CommandHistoryT()
{
    for (unsigned long CommandNr=0; CommandNr<m_Commands.Size(); CommandNr++)
        delete m_Commands[CommandNr];

    for (unsigned long CommandNr=0; CommandNr<m_InvisCommands.Size(); CommandNr++)
        delete m_InvisCommands[CommandNr];
}


const CommandT* CommandHistoryT::GetUndoCommand() const
{
    for (int CommandIndex=m_CurrentIndex; CommandIndex>-1; CommandIndex--)
        if (m_Commands[CommandIndex]->ShowInHistory())
            return m_Commands[CommandIndex];

    return NULL;
}


const CommandT* CommandHistoryT::GetRedoCommand() const
{
    for (int CommandIndex=m_CurrentIndex+1; CommandIndex<int(m_Commands.Size()); CommandIndex++)
        if (m_Commands[CommandIndex]->ShowInHistory())
            return m_Commands[CommandIndex];

    return NULL;
}


unsigned long CommandHistoryT::GetLastSaveSuggestedCommandID() const
{
    // Return the ID of the last executed command that suggest to save the document.
    for (int CommandIndex=m_CurrentIndex; CommandIndex>-1; CommandIndex--)
        if (m_Commands[CommandIndex]->SuggestsSave()) return m_Commands[CommandIndex]->GetID();

    return m_InvalidCommandID;
}


void CommandHistoryT::Undo()
{
    if (m_CurrentIndex<0) return;
    wxASSERT(m_CurrentIndex<(int)m_Commands.Size());

    // Undo all commands from the invisible commands list and delete them.
    while (m_InvisCommands.Size()>0)
    {
        m_InvisCommands[m_InvisCommands.Size()-1]->Undo();
        delete m_InvisCommands[m_InvisCommands.Size()-1];
        m_InvisCommands.DeleteBack();
    }

    // Undo all commands succeeding the current undo command starting at the current array position.
    // These commands are not visible in the undo/redo history but must be undone before undoing the
    // current command.
    while (m_CurrentIndex>=0 && !m_Commands[m_CurrentIndex]->ShowInHistory())
    {
        m_Commands[m_CurrentIndex]->Undo();
        m_CurrentIndex--;
    }

    // Complete the undo.
    if (m_CurrentIndex>=0)
    {
        m_Commands[m_CurrentIndex]->Undo();
        m_CurrentIndex--;
    }
}


void CommandHistoryT::Redo()
{
    if (m_CurrentIndex+1>=int(m_Commands.Size())) return;
    wxASSERT(m_CurrentIndex<(int)m_Commands.Size()-1);

    // Undo all commands from the invisible commands list and delete them.
    while (m_InvisCommands.Size()>0)
    {
        m_InvisCommands[m_InvisCommands.Size()-1]->Undo();
        delete m_InvisCommands[m_InvisCommands.Size()-1];
        m_InvisCommands.DeleteBack();
    }

    // Redo all commands preceeding the current redo command starting at the current array position +1.
    // These commands are not visible in the undo/redo history but must be redone before redoing the
    // current command.
    while (m_CurrentIndex+1<int(m_Commands.Size()) && !m_Commands[m_CurrentIndex+1]->ShowInHistory())
    {
        m_Commands[m_CurrentIndex+1]->Do();
        m_CurrentIndex++;
    }

    // Complete the redo.
    if (m_CurrentIndex+1<int(m_Commands.Size()))
    {
        m_Commands[m_CurrentIndex+1]->Do();
        m_CurrentIndex++;
    }
}


bool CommandHistoryT::SubmitCommand(CommandT* Command)
{
    if (!Command->IsDone() && !Command->Do())
    {
        delete Command;
        return false;
    }

    // Check # of undo levels and remove some.
    while (int(m_Commands.Size())>Options.general.UndoLevels)
    {
        if (m_Commands[0]->ShowInHistory()) m_InvalidCommandID=m_Commands[0]->GetID();

        delete m_Commands[0];
        m_Commands.RemoveAtAndKeepOrder(0);
        m_CurrentIndex--;
        if (m_CurrentIndex<-1) m_CurrentIndex=-1;
    }

    if (!Command->ShowInHistory())
    {
        // If the command should not be visible in the history add it to the invisible list.
        m_InvisCommands.PushBack(Command);
    }
    else
    {
        // If we have redo commands in the array delete them.
        while (m_CurrentIndex<(int)m_Commands.Size()-1)
        {
            delete m_Commands[m_CurrentIndex+1];
            m_Commands.RemoveAtAndKeepOrder(m_CurrentIndex+1);
        }

        // First add all invisible commands to the list then the real command so the history will stay intact.
        m_Commands.PushBack(m_InvisCommands);
        m_Commands.PushBack(Command);

        m_CurrentIndex+=m_InvisCommands.Size()+1; // Increase current index by the amount of commands added to the history.

        m_InvisCommands.Clear(); // Clear the list of invisible commands (they are now part of the real command list).
    }

    return true;
}
