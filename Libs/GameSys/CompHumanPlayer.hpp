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

#ifndef CAFU_GAMESYS_COMPONENT_HUMAN_PLAYER_HPP_INCLUDED
#define CAFU_GAMESYS_COMPONENT_HUMAN_PLAYER_HPP_INCLUDED

#include "CompBase.hpp"
#include "../../Games/PlayerCommand.hpp"      // TODO: This file must be moved (and/or its contents completely redesigned).
#include "PhysicsWorld.hpp"


namespace cf
{
    namespace GameSys
    {
        /// Entities with this component are associated with a client connection
        /// at whose ends is a human player who provides input to control the entity.
        class ComponentHumanPlayerT : public ComponentBaseT
        {
            public:

            /// The constructor.
            ComponentHumanPlayerT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentHumanPlayerT(const ComponentHumanPlayerT& Comp);

            /// A temporary method for compatibility with old code.
            ArrayT<PlayerCommandT>& GetPlayerCommands() { return m_PlayerCommands; }

            /// A convenience method for use by the `CarriedWeaponT` method implementations.
            Vector3dT GetPlayerVelocity() const;

            /// Another convenience method for use by the `CarriedWeaponT` method implementations.
            Vector3dT GetOriginWS() const;

            /// Another convenience method for use by the `CarriedWeaponT` method implementations.
            Vector3dT GetViewDirWS(double Random = 0.0) const;

            /// Another convenience method for use by the `CarriedWeaponT` method implementations:
            /// It traces a ray in the given direction and that originates at the player origin
            /// through the world.
            RayResultT TracePlayerRay(const Vector3dT& Dir) const;

            /// Another convenience method for use by the `CarriedWeaponT` method implementations.
            void InflictDamage(EntityT* OtherEnt, float Amount, const Vector3dT& Dir) const;


            // Base class overrides.
            ComponentHumanPlayerT* Clone() const;
            const char* GetName() const { return "HumanPlayer"; }


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

            ArrayT<PlayerCommandT> m_PlayerCommands;    ///< The commands to be processed in the next Think() step.
        };
    }
}

#endif
