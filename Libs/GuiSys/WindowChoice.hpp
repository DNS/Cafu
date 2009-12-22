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

#ifndef _CF_GUISYS_WINDOW_CHOICE_HPP_
#define _CF_GUISYS_WINDOW_CHOICE_HPP_

#include "Window.hpp"


namespace cf
{
    namespace GuiSys
    {
        /// A choice window.
        /// This was initially intended to look-n-feel like a combobox under Windows,
        /// but it turns out that for speed and ease of implementation, an entirely
        /// different approach is more feasible (for now).
        class ChoiceT : public WindowT
        {
            public:

            /// Constructor for creating a listbox.
            /// @param Params Choice creation parameters.
            ChoiceT(const cf::GuiSys::WindowCreateParamsT& Params);

            /// The Copy Constructor.
            /// @param Window Window to copy.
            /// @param Recursive Whether to copy the whole window hierarchy of the window and its children.
            ChoiceT(const ChoiceT& Window, bool Recursive=false);

            virtual ChoiceT* Clone(bool Recursive=false) const;

            /// Destructor.
            ~ChoiceT();

            /// Returns an array of the choices  of this window.
            ArrayT<std::string>& GetChoices() { return Choices; }

            // Overloaded methods from the base class.
            void Render() const;
            bool OnInputEvent(const CaKeyboardEventT& KE);
            bool OnInputEvent(const CaMouseEventT&    ME, float PosX, float PosY);
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

            ArrayT<std::string> Choices;        ///< The available choices that the user can choose from.
            unsigned long       SelectedChoice; ///< Number of the currently selected choice, 0xFFFFFFFF for none.
         // bool                IsListOpen;     ///< Whether the dropdown list of choices is currently shown.
         // float               ListMaxHeight;  ///< The maximum height of the dropdown list, when opened.


            // Lua script methods.
            // These first basic methods are analogous to those in WindowListBoxT.
            static int Clear(lua_State* LuaState);
            static int Append(lua_State* LuaState);
            static int Insert(lua_State* LuaState);
            static int GetNumChoices(lua_State* LuaState);
            static int GetChoice(lua_State* LuaState);
            static int SetChoice(lua_State* LuaState);
            static int GetSelection(lua_State* LuaState);
            static int SetSelection(lua_State* LuaState);

            static const luaL_Reg MethodsList[];
        };
    }
}

#endif
