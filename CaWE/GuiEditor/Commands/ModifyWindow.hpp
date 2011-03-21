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

#ifndef _GUIEDITOR_COMMAND_MODIFY_WINDOW_HPP_
#define _GUIEDITOR_COMMAND_MODIFY_WINDOW_HPP_

#include "../CommandPattern.hpp"

#include "GuiSys/Window.hpp"


namespace GuiEditor
{
    class GuiDocumentT;

    class CommandModifyWindowT : public CommandT
    {
        public:

        CommandModifyWindowT(GuiDocumentT* GuiDocument, cf::GuiSys::WindowT* Window, const wxString& PropertyName, cf::GuiSys::WindowT::MemberVarT& MemberVar, const wxString& NewValue);
        CommandModifyWindowT(GuiDocumentT* GuiDocument, cf::GuiSys::WindowT* Window, const wxString& PropertyName, cf::GuiSys::WindowT::MemberVarT& MemberVar, const float* NewValue);
        CommandModifyWindowT(GuiDocumentT* GuiDocument, cf::GuiSys::WindowT* Window, const wxString& PropertyName, cf::GuiSys::WindowT::MemberVarT& MemberVar, const int NewValue);


        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        GuiDocumentT*                   m_GuiDocument;
        cf::GuiSys::WindowT*            m_Window;
        wxString                        m_PropertyName;
        cf::GuiSys::WindowT::MemberVarT m_MemberVar;

        std::string                     m_NewString;
        int                             m_NewInt;
        float                           m_NewFloat[4];

        std::string                     m_OldString;
        int                             m_OldInt;
        float                           m_OldFloat[4];
    };
}

#endif
