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
#include "GuiSys/GuiImpl.hpp"
#include "PhysicsWorld.hpp"


namespace cf
{
    namespace GameSys
    {
        class CarriedWeaponT;


        /// Entities with this component are associated with a client connection
        /// at whose ends is a human player who provides input to control the entity.
        class ComponentHumanPlayerT : public ComponentBaseT
        {
            public:

            static const unsigned int EVENT_TYPE_PRIMARY_FIRE;
            static const unsigned int EVENT_TYPE_SECONDARY_FIRE;


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
            Vector3dT GetCameraOriginWS() const;

            /// Another convenience method for use by the `CarriedWeaponT` method implementations.
            Vector3dT GetCameraViewDirWS(double Random = 0.0) const;

            /// Another convenience method for use by the `CarriedWeaponT` method implementations:
            /// It traces a ray in the given direction and that originates at the player origin
            /// through the world.
            RayResultT TraceCameraRay(const Vector3dT& Dir) const;

            /// Another convenience method for use by the `CarriedWeaponT` method implementations.
            void InflictDamage(EntityT* OtherEnt, float Amount, const Vector3dT& Dir) const;

            /// Another convenience method for use by the `CarriedWeaponT` method implementations.
            /// It calls the PostEvent() method of the players Script component.
            void PostEvent(unsigned int EventType) const;

            /// Another convenience method for use by the `CarriedWeaponT` method implementations.
            const CarriedWeaponT* GetCarriedWeapon(unsigned int ActiveWeaponSlot) const;

            // Temporary methods, so that obsolete DeathMatch code can access our data.
            uint8_t GetStateOfExistence() const { return m_StateOfExistence.Get(); }
            void SetStateOfExistence(uint8_t s) { m_StateOfExistence.Set(s); }
            uint8_t GetHealth() const { return m_Health.Get(); }
            void SetHealth(uint8_t h) { m_Health.Set(h); }
            uint8_t GetArmor() const { return m_Armor.Get(); }
            void SetArmor(uint8_t a) { m_Armor.Set(a); }
            unsigned int GetHaveItems() const { return m_HaveItems.Get(); }
            void SetHaveItems(unsigned int i) { m_HaveItems.Set(i); }
            unsigned int GetHaveWeapons() const { return m_HaveWeapons.Get(); }
            void SetHaveWeapons(unsigned int w) { m_HaveWeapons.Set(w); }
            uint8_t GetActiveWeaponSlot() const { return m_ActiveWeaponSlot.Get(); }
            void SetActiveWeaponSlot(uint8_t ws) { m_ActiveWeaponSlot.Set(ws); }
            uint8_t GetActiveWeaponSequNr() const { return m_ActiveWeaponSequNr.Get(); }
            void SetActiveWeaponSequNr(uint8_t ws) { m_ActiveWeaponSequNr.Set(ws); }
            float GetActiveWeaponFrameNr() const { return m_ActiveWeaponFrameNr.Get(); }
            void SetActiveWeaponFrameNr(float f) { m_ActiveWeaponFrameNr.Set(f); }
            TypeSys::VarArrayT<uint16_t>& GetHaveAmmo() { return m_HaveAmmo; }
            TypeSys::VarArrayT<uint8_t>& GetHaveAmmoInWeapons() { return m_HaveAmmoInWeapons; }

            /// A helper function for Think().
            void CheckGUIs(bool ThinkingOnServerSide, bool HaveButtonClick) const;

            /// A helper method (that does the actual work) for DoServerFrame() *and* the (re-)prediction in the client.
            void Think(const PlayerCommandT& PlayerCommand, bool ThinkingOnServerSide);


            // Base class overrides.
            ComponentHumanPlayerT* Clone() const;
            const char* GetName() const { return "HumanPlayer"; }
            BoundingBox3fT GetVisualBB() const;
            void Render(bool FirstPersonView, float LodDist) const;
            void DoServerFrame(float t);
            void DoClientFrame(float t);


            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            protected:

            // The Lua API methods of this class.
            static int GetCrosshairInfo(lua_State* LuaState);
            static int GetAmmoString(lua_State* LuaState);
            static int ProcessEvent(lua_State* LuaState);
            static int PickUpItem(lua_State* LuaState);
            static int toString(lua_State* LuaState);

            static const luaL_Reg               MethodsList[];  ///< The list of Lua methods for this class.
            static const char*                  DocClass;
            static const cf::TypeSys::MethsDocT DocMethods[];
            static const cf::TypeSys::VarsDocT  DocVars[];


            private:

            void FillMemberVars();                              ///< A helper method for the constructors.
            IntrusivePtrT<cf::GuiSys::GuiImplT> GetGuiHUD();    ///< Returns the GUI instance for the player's Head-Up Display.

            TypeSys::VarT<std::string>   m_PlayerName;          ///< The name that the player chose for himself.
            TypeSys::VarT<uint8_t>       m_StateOfExistence;    ///< For the player's main state machine, e.g. "spectator, dead, alive, ...".
            TypeSys::VarT<uint8_t>       m_Health;              ///< Health.
            TypeSys::VarT<uint8_t>       m_Armor;               ///< Armor.
            TypeSys::VarT<unsigned int>  m_HaveItems;           ///< Bit field, entity can carry 32 different items.
            TypeSys::VarT<unsigned int>  m_HaveWeapons;         ///< Bit field, entity can carry 32 different weapons.
            TypeSys::VarT<uint8_t>       m_ActiveWeaponSlot;    ///< Index into m_HaveWeapons, m_HaveAmmoInWeapons, and for determining the weapon model index.
            TypeSys::VarT<uint8_t>       m_ActiveWeaponSequNr;  ///< The weapon anim sequence that we see (the local clients 1st person ("view") weapon model).
            TypeSys::VarT<float>         m_ActiveWeaponFrameNr; ///< Respectively, this is the frame number of the current weapon sequence.
            TypeSys::VarArrayT<uint16_t> m_HaveAmmo;            ///< Entity can carry 16 different types of ammo (weapon independent). This is the amount of each.
            TypeSys::VarArrayT<uint8_t>  m_HaveAmmoInWeapons;   ///< Entity can carry ammo in each of the 32 weapons. This is the amount of each.
            TypeSys::VarT<float>         m_HeadSway;            ///< The progress of one "head swaying" cycle in state FrozenSpectator.

            ArrayT<PlayerCommandT>          m_PlayerCommands;   ///< The commands to be processed in the next Think() step.
            IntrusivePtrT<GuiSys::GuiImplT> m_GuiHUD;           ///< The GUI instance for the player's Head-Up Display.
        };
    }
}

#endif
