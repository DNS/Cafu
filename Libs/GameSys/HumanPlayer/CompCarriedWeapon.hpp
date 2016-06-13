/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
            ComponentCarriedWeaponT* Clone() const override;
            const char* GetName() const override { return "CarriedWeapon"; }
            void PreCache() override;
            void OnPostLoad(bool OnlyStatic) override;


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
