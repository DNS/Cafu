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

#ifndef CAFU_GAMESYS_COMPONENT_ENTITY_BASICS_HPP_INCLUDED
#define CAFU_GAMESYS_COMPONENT_ENTITY_BASICS_HPP_INCLUDED

#include "CompBase.hpp"


namespace cf
{
    namespace GameSys
    {
        /// This component adds the basics of the entity (its name and the "is shown?" flag).
        /// It is one of the components that is "fundamental" to an entity (cf::GameSys::IsFundamental() returns `true`).
        /// Every entity must have exactly one.
        class ComponentBasicsT : public ComponentBaseT
        {
            public:

            /// The constructor.
            ComponentBasicsT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentBasicsT(const ComponentBasicsT& Comp);

            /// Returns the name of the entity.
            const std::string& GetEntityName() const { return m_Name.Get(); }

            /// Sets a new name for the entity.
            /// Entity names must be valid Lua script identifiers and unique among their siblings, and the method
            /// modifies the given string as necessary. As a result, GetEntityName() can return a string that is
            /// different from the string given to a preceeding call to SetEntityName().
            /// This is equivalent to calling `SetMember("Name", Name);`, but is provided for the more explicit
            /// documentation of the side-effects.
            void SetEntityName(const std::string& Name) { m_Name.Set(Name); }

            /// Returns `true` if the entity is currently shown. Returns `false` if the entity is currently hidden.
            bool IsShown() const { return m_Show.Get(); }

            /// Returns `true` if the entity is declared as static. Returns `false` if (the primitives of) the entity can move.
            bool IsStatic() const { return m_Static.Get(); }

            // Base class overrides.
            ComponentBasicsT* Clone() const;
            const char* GetName() const { return "Basics"; }


            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            protected:

            // The Lua API methods of this class.
            static int toString(lua_State* LuaState);

            static const luaL_Reg               MethodsList[];  ///< The list of Lua methods for this class.
            static const char*                  DocClass;
            static const cf::TypeSys::MethsDocT DocMethods[];
            static const cf::TypeSys::VarsDocT  DocVars[];


            private:

            /// A variable of type `std::string`, specifically for entity names.
            /// Entity names must be valid Lua script identifiers and unique among their siblings.
            class EntityNameT : public TypeSys::VarT<std::string>
            {
                public:

                EntityNameT(const char* Name, const std::string& Value, const char* Flags[], ComponentBasicsT& CompBasics);
                EntityNameT(const EntityNameT& Var, ComponentBasicsT& CompBasics);

                // Base class overrides.
                void Set(const std::string& v);


                private:

                ComponentBasicsT& m_CompBasics; ///< The parent ComponentBasicsT that contains this variable.
            };

            /// A variable of type `bool`, indicating whether this entity is currently shown or hidden.
            /// Besides keeping the boolean flag, this variable calls the `OnShow()` script callback whenever its value changes.
            class EntityShowT : public TypeSys::VarT<bool>
            {
                public:

                EntityShowT(const char* Name, const bool& Value, const char* Flags[], ComponentBasicsT& CompBasics);
                EntityShowT(const EntityShowT& Var, ComponentBasicsT& CompBasics);

                // Base class overrides.
                void Set(const bool& v);


                private:

                ComponentBasicsT& m_CompBasics; ///< The parent ComponentBasicsT that contains this variable.
            };


            EntityNameT         m_Name;     ///< The name of the entity. Entity names must be valid Lua script identifiers and unique among their siblings.
            EntityShowT         m_Show;     ///< Is this entity currently shown?
            TypeSys::VarT<bool> m_Static;   ///< Are the map primitives of this entity fixed and immovable, never moving around in the game world?
        };
    }
}

#endif
