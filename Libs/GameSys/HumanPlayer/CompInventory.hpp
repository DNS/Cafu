/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
