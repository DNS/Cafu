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

#ifndef _GUIEDITOR_COMMAND_CHANGE_WINDOW_HIERARCHY_HPP_
#define _GUIEDITOR_COMMAND_CHANGE_WINDOW_HIERARCHY_HPP_

#include "../CommandPattern.hpp"


namespace cf { namespace GuiSys { class WindowT; } }


namespace GuiEditor
{
    class GuiDocumentT;

    class CommandChangeWindowHierarchyT : public CommandT
    {
        public:

        CommandChangeWindowHierarchyT(GuiDocumentT* GuiDocument, cf::GuiSys::WindowT* Window, cf::GuiSys::WindowT* NewParent, unsigned long NewPosition);

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        GuiDocumentT*        m_GuiDocument;
        cf::GuiSys::WindowT* m_Window;
        cf::GuiSys::WindowT* m_NewParent;
        unsigned long        m_NewPosition;
        cf::GuiSys::WindowT* m_OldParent;
        unsigned long        m_OldPosition;
    };
}

#endif