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

#ifndef CAFU_GAMESYS_COMPONENT_CARRIED_WEAPON_HPP_INCLUDED
#define CAFU_GAMESYS_COMPONENT_CARRIED_WEAPON_HPP_INCLUDED

#include "../CompBase.hpp"


namespace cf
{
    namespace GameSys
    {
        /// This component represents a weapon that a player can pick up and use.
        class ComponentCarriedWeaponT : public ComponentBaseT
        {
            public:

            /// The constructor.
            ComponentCarriedWeaponT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentCarriedWeaponT(const ComponentCarriedWeaponT& Comp);

            /// Returns whether this weapon is available to the player.
            /// A weapon is usually available to the player only after it has been picked up in the world.
            /// Alternatively, it is possible to configure the player prototype to spawn the player with the
            /// weapon readily available.
            /// Only weapons that are available can be selected and drawn.
            bool IsAvail() const { return m_IsAvail.Get(); }

            // Base class overrides.
            ComponentCarriedWeaponT* Clone() const;
            const char* GetName() const { return "CarriedWeapon"; }
            void OnPostLoad(bool OnlyStatic);


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
            static const cf::TypeSys::MethsDocT DocCallbacks[];
            static const cf::TypeSys::VarsDocT  DocVars[];


            private:

            void FillMemberVars();  ///< A helper method for the constructors.

            TypeSys::VarT<std::string> m_Label;             ///< A short informational name for this weapon. Used for reference e.g. in the Map Editor, in log output, or in script code.
            TypeSys::VarT<bool>        m_IsAvail;           ///< Is this weapon available to the player? A weapon is usually available to the player only after it has been picked up in the world. Alternatively, it is possible to configure the player prototype to spawn the player with the weapon readily available. Only weapons that are available can be selected and drawn.
            TypeSys::VarT<std::string> m_Script;            ///< The filename of the script that implements the behaviour of this weapon.
            TypeSys::VarT<uint16_t>    m_PrimaryAmmo;       ///< The current amount of ammo for the primary fire of this weapon.
            TypeSys::VarT<uint16_t>    m_MaxPrimaryAmmo;    ///< The maximum amount of ammo for the primary fire of this weapon.
            TypeSys::VarT<uint16_t>    m_SecondaryAmmo;     ///< The current amount of ammo for the secondary fire of this weapon.
            TypeSys::VarT<uint16_t>    m_MaxSecondaryAmmo;  ///< The maximum amount of ammo for the secondary fire of this weapon.
        };
    }
}

#endif
