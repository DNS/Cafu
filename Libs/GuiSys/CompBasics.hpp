/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#ifndef CAFU_GUISYS_COMPONENT_WINDOW_BASICS_HPP_INCLUDED
#define CAFU_GUISYS_COMPONENT_WINDOW_BASICS_HPP_INCLUDED

#include "CompBase.hpp"


namespace cf
{
    namespace GuiSys
    {
        /// This components adds the basics of the window (its name and the "is shown?" flag);
        /// every window must have exactly one.
        class ComponentBasicsT : public ComponentBaseT
        {
            public:

            /// The constructor.
            ComponentBasicsT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentBasicsT(const ComponentBasicsT& Comp);

            const std::string& GetWindowName() const { return m_Name.Get(); }
            bool IsShown() const { return m_Show.Get(); }

            void SetWindowName(const std::string& Name) { m_Name.Set(Name); }
            void Show(bool b=true) { m_Show.Set(b); }

            // Base class overrides.
            ComponentBasicsT* Clone() const;
            const char* GetName() const { return "Basics"; }


            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            private:

            /// A variable of type std::string, specifically for window names.
            /// Window names must be valid Lua script identifiers and unique among their siblings.
            class WindowNameT : public TypeSys::VarT<std::string>
            {
                public:

                WindowNameT(const char* Name, const std::string& Value, const char* Flags[], ComponentBasicsT& CompBasics);
                WindowNameT(const WindowNameT& Var, ComponentBasicsT& CompBasics);

                // Base class overrides.
                void Set(const std::string& v);


                private:

                ComponentBasicsT& m_CompBasics; ///< The parent ComponentBasicsT that contains this variable.
            };

            // The Lua API methods of this class.
            static const luaL_Reg MethodsList[];        ///< The list of Lua methods for this class.
            static int toString(lua_State* LuaState);   ///< Returns a string representation of this object.

            WindowNameT         m_Name;     ///< The name of the window. Window names must be valid Lua script identifiers and unique among their siblings.
            TypeSys::VarT<bool> m_Show;     ///< Is this window currently shown?
        };
    }
}

#endif
