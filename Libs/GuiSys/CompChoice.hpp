/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2014 Carsten Fuchs Software.

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

#ifndef CAFU_GUISYS_COMPONENT_CHOICE_HPP_INCLUDED
#define CAFU_GUISYS_COMPONENT_CHOICE_HPP_INCLUDED

#include "CompBase.hpp"


namespace cf
{
    namespace GuiSys
    {
        class ComponentTextT;


        /// This components add the behaviour of a choice field to its window.
        /// It requires that the window also has a text component, whose value it updates according to user
        /// interaction to one of the available choices.
        class ComponentChoiceT : public ComponentBaseT
        {
            public:

            /// The constructor.
            ComponentChoiceT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentChoiceT(const ComponentChoiceT& Comp);

            // Base class overrides.
            ComponentChoiceT* Clone() const;
            const char* GetName() const { return "Choice"; }
            void UpdateDependencies(WindowT* Window);
            void OnPostLoad(bool OnlyStatic);
            bool OnInputEvent(const CaKeyboardEventT& KE);
            bool OnInputEvent(const CaMouseEventT& ME, float PosX, float PosY);

            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            protected:

            // The Lua API methods of this class.
            static int Set(lua_State* LuaState);
            static int GetSelItem(lua_State* LuaState);
            static int toString(lua_State* LuaState);

            static const luaL_Reg               MethodsList[];  ///< The list of Lua methods for this class.
            static const char*                  DocClass;
            static const cf::TypeSys::MethsDocT DocMethods[];
            static const cf::TypeSys::VarsDocT  DocVars[];


            private:

            void Sync();    ///< Sets the text component to the currently selected choice.

            IntrusivePtrT<ComponentTextT>   m_TextComp;     ///< The sibling text component whose value we're updating.
            TypeSys::VarArrayT<std::string> m_Choices;      ///< The list of available choices.
            TypeSys::VarT<unsigned int>     m_Selection;    ///< The index number of the currently selected choice, where 1 corresponds to the first choice (as per Lua convention). Use 0 for "no selection".
        };
    }
}

#endif
