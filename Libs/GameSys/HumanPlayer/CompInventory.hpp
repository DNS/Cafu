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

#ifndef CAFU_GAMESYS_COMPONENT_INVENTORY_HPP_INCLUDED
#define CAFU_GAMESYS_COMPONENT_INVENTORY_HPP_INCLUDED

#include "../CompBase.hpp"


namespace cf
{
    namespace GameSys
    {
        /// This component keeps an inventory count for an arbitrary set of items.
        /// An item can be anything that can be described with a string.
        /// Contrary to other components, an inventory is flexible regarding the "kind" and number
        /// of items that it keeps the counts for. However, it is focused on being used by script code;
        /// it is not possible to inspect and edit the contained items in the Map Editor at this time.
        class ComponentInventoryT : public ComponentBaseT
        {
            public:

            /// The constructor.
            ComponentInventoryT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentInventoryT(const ComponentInventoryT& Comp);

            // Base class overrides.
            ComponentInventoryT* Clone() const;
            const char* GetName() const { return "Inventory"; }


            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            protected:

            // The Lua API methods of this class.
            static int Get(lua_State* LuaState);
            static int Set(lua_State* LuaState);
            static int Add(lua_State* LuaState);
            static int CheckMax(lua_State* LuaState);
            static int toString(lua_State* LuaState);

            static const luaL_Reg               MethodsList[];  ///< The list of Lua methods for this class.
            static const char*                  DocClass;
            static const cf::TypeSys::MethsDocT DocMethods[];
            static const cf::TypeSys::VarsDocT  DocVars[];


            private:

            void DoSerialize(cf::Network::OutStreamT& Stream) const /*override*/;
            void DoDeserialize(cf::Network::InStreamT& Stream, bool IsIniting) /*override*/;

            std::map<std::string, uint16_t> m_Items;
        };
    }
}

#endif
