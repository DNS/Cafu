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

#ifndef _GUIEDITOR_COMMAND_MODIFY_GUI_HPP_
#define _GUIEDITOR_COMMAND_MODIFY_GUI_HPP_

#include "../../CommandPattern.hpp"


namespace cf { namespace GuiSys { class WindowT; } }


namespace GuiEditor
{
    class GuiDocumentT;

    class CommandModifyGuiT : public CommandT
    {
        public:

        static CommandModifyGuiT* Create(GuiDocumentT* GuiDocument, const wxString& PropertyName, const wxString& NewValue);
        static CommandModifyGuiT* Create(GuiDocumentT* GuiDocument, const wxString& PropertyName, int             NewValue);
        static CommandModifyGuiT* Create(GuiDocumentT* GuiDocument, const wxString& PropertyName, long            NewValue);
        static CommandModifyGuiT* Create(GuiDocumentT* GuiDocument, const wxString& PropertyName, float           NewValue);
        static CommandModifyGuiT* Create(GuiDocumentT* GuiDocument, const wxString& PropertyName, bool            NewValue);
        static CommandModifyGuiT* Create(GuiDocumentT* GuiDocument, const wxString& PropertyName, const wxColour& NewValue);

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        CommandModifyGuiT(GuiDocumentT* GuiDocument, const wxString& PropertyName);

        GuiDocumentT*        m_GuiDocument;
        wxString             m_PropertyName;

        wxString             m_NewString;
        int                  m_NewInt;
        long                 m_NewLong;
        float                m_NewFloat;
        bool                 m_NewBool;
        wxColour             m_NewColor;

        wxString             m_OldString;
        int                  m_OldInt;
        long                 m_OldLong;
        float                m_OldFloat;
        bool                 m_OldBool;
        wxColour             m_OldColor;
    };
}

#endif
