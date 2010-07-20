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

#ifndef _CF_GUISYS_WINDOW_EDIT_HPP_
#define _CF_GUISYS_WINDOW_EDIT_HPP_

#include "Window.hpp"


namespace cf
{
    namespace GuiSys
    {
        /// A window whose text can be edited by the user.
        /// EditWindowTs behave different from the WindowT base class in the following points:
        /// - Key press events that have not been dealt with by the Lua OnKeyPress() script function
        ///   are used to modify the windows Text directly.
        /// - A text cursor is rendered.
        class EditWindowT : public WindowT
        {
            public:

            /// Constructor for creating an edit window.
            EditWindowT(const cf::GuiSys::WindowCreateParamsT& Params);

            /// The Copy Constructor.
            EditWindowT(const EditWindowT& Window, bool Recursive=false);

            virtual EditWindowT* Clone(bool Recursive=false) const;

            // Overloaded methods from the base class.
            void Render() const;
            bool OnInputEvent(const CaKeyboardEventT& KE);
            bool OnClockTickEvent(float t);
            void EditorFillInPG(wxPropertyGridManager* PropMan);
            bool UpdateProperty(wxPGProperty* Property);
            bool EditorHandlePGChange(wxPropertyGridEvent& Event, GuiEditor::ChildFrameT* ChildFrame);
            bool WriteInitMethod(std::ostream& OutFile);
            void EditorRender() const;

            // The TypeSys related declarations for this class.
            virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            protected:

            void FillMemberVars(); ///< Helper method that fills the MemberVars array with entries for each class member.


            private:

            unsigned int TextCursorPos;       ///< The character position of the text cursor in the text. Valid values are 0 to Text.length().
            unsigned int TextCursorType;      ///< The type of the text cursor. 0 is a vertical bar cursor '|', 1 is an underline cursor'_'. Any other types are not supported and default to the '|' cursor type.
            float        TextCursorRate;      ///< The rate in seconds at which the text cursor completes one blink cycle (on/off).
            float        TextCursorTime;      ///< The current time in the cursor blink cycle.
            float        TextCursorColor[4];  ///< The color of the text cursor.


            // Lua script methods.
            static int Set(lua_State* LuaState);    ///< An overload of the base class method.
            static int GetTextCursorPos(lua_State* LuaState);
            static int SetTextCursorPos(lua_State* LuaState);
            static int SetTextCursorType(lua_State* LuaState);
            static int SetTextCursorRate(lua_State* LuaState);
            static int SetTextCursorColor(lua_State* LuaState);

            static const luaL_Reg MethodsList[];
        };
    }
}

#endif
