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

#ifndef CAFU_GAMESYS_COMPONENT_PLAYER_START_HPP_INCLUDED
#define CAFU_GAMESYS_COMPONENT_PLAYER_START_HPP_INCLUDED

#include "CompBase.hpp"


namespace cf
{
    namespace GameSys
    {
        /// This component marks its entity as possible spawn point for human players
        /// that begin a single-player level or join a multi-player game.
        class ComponentPlayerStartT : public ComponentBaseT
        {
            public:

            /// The constructor.
            ComponentPlayerStartT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentPlayerStartT(const ComponentPlayerStartT& Comp);

            /// Returns whether players can be spawned here in single-player games.
            bool IsSinglePlayerStart() const { return m_SinglePlayer.Get(); }

            /// Returns whether players can be spawned here in multi-player games.
            bool IsMultiPlayerStart() const { return m_MultiPlayer.Get(); }


            // Base class overrides.
            ComponentPlayerStartT* Clone() const;
            const char* GetName() const { return "PlayerStart"; }


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

            TypeSys::VarT<bool> m_SinglePlayer;     ///< If true, players can be spawned here in single-player games.
            TypeSys::VarT<bool> m_MultiPlayer;      ///< If true, players can be spawned here in multi-player games.
        };
    }
}

#endif
