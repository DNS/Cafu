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

#ifndef CAFU_GUISYS_COMPONENT_BORDER_HPP_INCLUDED
#define CAFU_GUISYS_COMPONENT_BORDER_HPP_INCLUDED

#include "CompBase.hpp"


namespace cf
{
    namespace GuiSys
    {
        /// This components adds a border to its window.
        class ComponentBorderT : public ComponentBaseT
        {
            public:

            /// The constructor.
            ComponentBorderT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentBorderT(const ComponentBorderT& Comp);

            // Base class overrides.
            ComponentBorderT* Clone() const;
            const char* GetName() const { return "Border"; }
            void Render() const;


            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            private:

            void FillMemberVars();                      ///< A helper method for the constructors.

            // The Lua API methods of this class.
            static const luaL_Reg MethodsList[];        ///< The list of Lua methods for this class.
            static int toString(lua_State* LuaState);   ///< Returns a string representation of this object.

            TypeSys::VarT<float>     m_Width;           ///< The width of the border.
            TypeSys::VarT<Vector3fT> m_Color;           ///< The border color.
            TypeSys::VarT<float>     m_Alpha;           ///< The alpha component of the color.
        };
    }
}

#endif
