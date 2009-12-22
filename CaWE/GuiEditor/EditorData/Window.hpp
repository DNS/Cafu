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

#ifndef _GUIEDITOR_EDITORDATA_WINDOW_
#define _GUIEDITOR_EDITORDATA_WINDOW_

#include "GuiSys/EditorData.hpp"

#include "wx/wx.h"


namespace GuiEditor
{
    class GuiDocumentT;

    class EditorDataWindowT : public cf::GuiSys::EditorDataT
    {
        public:

        EditorDataWindowT(cf::GuiSys::WindowT* Window, GuiDocumentT* GuiDocument);

        /// Sets the name for this window.
        /// This method checks if the name is valid in the sense of LUA compatibility
        /// (window name is used as a LUA variable name for this window) and uniqueness.
        /// This function should always be called instead of setting the name member
        /// of the window directly.
        /// @param NewName The name to be set.
        /// @return Whether the name has been successfully set.
        bool SetName(const wxString& NewName);

        /// Checks the name uniqueness of a new name string within the windows siblings.
        /// @param Name Name to check for uniqueness.
        /// @return Whether this name is unique.
        bool CheckNameUniqueness(wxString Name);

        GuiDocumentT* GetGuiDoc() { return m_GuiDocument; }

        bool Selected;


        private:

        GuiDocumentT* m_GuiDocument;

        /// Helper method to check and repair the uniqueness of the name of this window.
        void RepairNameUniqueness();
    };
}

#endif
