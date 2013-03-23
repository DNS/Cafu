/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2013 Carsten Fuchs Software.

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

#ifndef CAFU_GUISYS_COMPONENT_TEXT_EDIT_HPP_INCLUDED
#define CAFU_GUISYS_COMPONENT_TEXT_EDIT_HPP_INCLUDED

#include "CompBase.hpp"


namespace cf
{
    namespace GuiSys
    {
        class ComponentTextT;


        /// With this component, the user can edit the text in a sibling text component.
        /// The component requires that the window also has a text component, whose value it updates according to
        /// user edits.
        class ComponentTextEditT : public ComponentBaseT
        {
            public:

            /// The constructor.
            ComponentTextEditT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentTextEditT(const ComponentTextEditT& Comp);

            // Base class overrides.
            ComponentTextEditT* Clone() const;
            const char* GetName() const { return "TextEdit"; }
            void UpdateDependencies(WindowT* Window);
            void Render() const;
            bool OnInputEvent(const CaKeyboardEventT& KE);
            void OnClockTickEvent(float t);

            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            private:

            /// A variable of type int, specifically for the cursor type, "|" vs. "_".
            class VarCursorTypeT : public TypeSys::VarT<int>
            {
                public:

                VarCursorTypeT(const char* Name, const int& Value, const char* Flags[]=NULL);

                // Base class overrides.
                void GetChoices(ArrayT<std::string>& Strings, ArrayT<int>& Values) const;
            };


            // The Lua API methods of this class.
            static const luaL_Reg MethodsList[];        ///< The list of Lua methods for this class.
            static int SetText(lua_State* LuaState);    ///< Sets the given text in the related Text sibling component and moves the cursor position to its end.
            static int toString(lua_State* LuaState);   ///< Returns a string representation of this object.

            IntrusivePtrT<ComponentTextT> m_TextComp;       ///< The sibling text component whose value we're editing.
            float                         m_CursorTime;     ///< The current time in the cursor blink cycle.

            TypeSys::VarT<unsigned int>   m_CursorPos;      ///< The character position of the text cursor in the text. Valid values are 0 to Text.length().
            VarCursorTypeT                m_CursorType;     ///< The type of the text cursor. 0 is a vertical bar cursor '|', 1 is an underline cursor'_'. Any other types default to the '|' cursor type.
            TypeSys::VarT<float>          m_CursorRate;     ///< The rate in seconds at which the text cursor completes one blink cycle (on/off).
            TypeSys::VarT<Vector3fT>      m_CursorColor;    ///< The color of the text cursor.
            TypeSys::VarT<float>          m_CursorAlpha;    ///< The alpha component of the color.
        };
    }
}

#endif
