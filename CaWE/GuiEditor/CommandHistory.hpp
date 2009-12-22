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

#ifndef _GUIEDITOR_COMMAND_HISTORY_HPP_
#define _GUIEDITOR_COMMAND_HISTORY_HPP_

#include "CommandPattern.hpp"


namespace GuiEditor
{
    class CommandHistoryT
    {
        public:

        CommandHistoryT();
        ~CommandHistoryT();

        bool SubmitCommand(CommandT* Command);

        void Undo();
        void Redo();

        const CommandT* GetUndoCommand() const;     ///< Returns the next command in the history that can be undone, or NULL if there is none. Only commands that are shown in history are taken into account.
        const CommandT* GetRedoCommand() const;     ///< Returns the next command in the history that can be redone, or NULL if there is none. Only commands that are shown in history are taken into account.

        unsigned long GetLastSaveSuggestedCommandID() const;


        private:

        ArrayT<CommandT*> m_Commands;
        ArrayT<CommandT*> m_InvisCommands;    ///< Stores all commands not visible in the history until a visible command is added to the history (then they are moved into the normal history).
        int               m_CurrentIndex;     ///< The current index inside the commands array. -1 means no valid index.

        /// The command id returned when there is no current command (when the current index is -1).
        /// On creation this value is 0. It becomes the command ID of the last command removed from the history due to
        /// size limitations as set in the CaWE options by the user.
        unsigned long     m_InvalidCommandID;
    };
}

#endif
