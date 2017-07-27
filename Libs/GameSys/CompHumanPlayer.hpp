/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GAMESYS_COMPONENT_HUMAN_PLAYER_HPP_INCLUDED
#define CAFU_GAMESYS_COMPONENT_HUMAN_PLAYER_HPP_INCLUDED

#include "CompBase.hpp"
#include "GuiSys/GuiImpl.hpp"

class ParticleMaterialSetT;
struct PlayerCommandT;


namespace cf
{
    namespace GameSys
    {
        class CarriedWeaponT;
        class ComponentCarriedWeaponT;


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

            /// The destructor.
            ~ComponentHumanPlayerT();

            /// Returns the player's current velocity (as learned from its PlayerPhysics component).
            Vector3dT GetPlayerVelocity() const;

            /// Returns the origin of the entity's camera, in world-space.
            Vector3dT GetCameraOriginWS() const;

            /// Returns the view (forward) vector of the entity's camera, in world space, optionally deviated by some random amount.
            Vector3dT GetCameraViewDirWS(double Random = 0.0) const;

            /// Traces a ray that originates at the player camera's origin in the given direction through the world.
            bool TraceCameraRay(const Vector3dT& Dir, Vector3dT& HitPoint, ComponentBaseT*& HitComp) const;

            /// A helper function for Think().
            bool CheckGUIs(const PlayerCommandT& PrevPC, const PlayerCommandT& PC, bool ThinkingOnServerSide) const;

            /// The method that does the actual work for DoServerFrame() *and* the (re-)prediction in the client.
            void Think(const PlayerCommandT& PrevPC, const PlayerCommandT& PC, bool ThinkingOnServerSide);

            /// Returns the ComponentCarriedWeaponT component of the currently active weapon,
            /// or NULL if currently no weapon is active.
            IntrusivePtrT<ComponentCarriedWeaponT> GetActiveWeapon() const;

            /// This method initiates the holstering of the currently active weapon and the subsequent drawing
            /// of the given weapon.
            ///
            /// If the current weapon is none, unknown or not available to the player (e.g. because it has never been picked up),
            /// or if it turns out that the weapon does not support holstering (e.g. because there is no holstering
            /// sequence available), the holstering is skipped and the next weapon is drawn immediately.
            /// If the current weapon is fine but is not idle at the time that this method is called (e.g. reloading
            /// or firing), the call is *ignored*, that is, the weapon is *not* changed.
            ///
            /// @param NextWeaponNr   The index number into the CarriedWeapon components of this entity, starting at 1.
            ///                       Use 0 to select "no" weapon.
            /// @param Force          If `true`, forces the drawing of the next weapon immediately, ignoring the idle
            ///                       state and holstering sequence of the current weapon. This is normally only used
            ///                       if, for example, the last hand grenade has been thrown and bare-handed animation
            ///                       sequences for idle and holster are not available.
            void SelectWeapon(uint8_t NextWeaponNr, bool Force = false);

            /// This method draws the next weapon as previously prepared by SelectWeapon().
            ///
            /// It is intended to be called at the end of the holstering sequence of the previous weapon, either
            /// directly from SelectWeapon() when it found that holstering can entirely be skipped, or indirectly
            /// when SelectWeapon() calls the previous weapon's `Holster()` callback, the end of the holster
            /// sequence is detected in the `OnSequenceWrap_Sv()` script callback, and its implementation in turn
            /// calls this method.
            void SelectNextWeapon();

            /// Returns a pseudo-random integer in range `0 ... n-1`.
            ///
            /// The important aspect of this method is that it returns pseudo-random numbers that are reproducible
            /// in the context of the "client prediction" feature of the Cafu Engine. All random numbers that are
            /// used in human player code must be obtained from one of the GetRandom() methods in this class.
            unsigned int GetRandom(unsigned int n);

            /// Returns a pseudo-random double in range `[0.0, 1.0]` (inclusive).
            ///
            /// The important aspect of this method is that it returns pseudo-random numbers that are reproducible
            /// in the context of the "client prediction" feature of the Cafu Engine. All random numbers that are
            /// used in human player code must be obtained from one of the GetRandom() methods in this class.
            double GetRandom();


            // Base class overrides.
            ComponentHumanPlayerT* Clone() const override;
            const char* GetName() const override { return "HumanPlayer"; }
            BoundingBox3fT GetCullingBB() const override;
            void PostRender(bool FirstPersonView) override;
            void DoServerFrame(float t) override;
            void DoClientFrame(float t) override;


            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const override { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            protected:

            // The Lua API methods of this class.
            static int GetActiveWeapon(lua_State* LuaState);
            static int SelectWeapon(lua_State* LuaState);
            static int SelectNextWeapon(lua_State* LuaState);
            static int FireRay(lua_State* LuaState);
            static int GetRandom(lua_State* LuaState);
            static int SpawnWeaponChild(lua_State* LuaState);
            static int RegisterParticle(lua_State* LuaState);
            static int toString(lua_State* LuaState);

            static const luaL_Reg               MethodsList[];  ///< The list of Lua methods for this class.
            static const char*                  DocClass;
            static const cf::TypeSys::MethsDocT DocMethods[];
            static const cf::TypeSys::VarsDocT  DocVars[];


            private:

            void FillMemberVars();                              ///< A helper method for the constructors.
            bool IsPlayerPrototype() const;                     ///< Is this component in a player prototype entity, or in a concrete player instance?
            IntrusivePtrT<cf::GuiSys::GuiImplT> GetHUD();       ///< Returns the GUI instance for the player's Head-Up Display.

            TypeSys::VarT<std::string>   m_PlayerName;          ///< The name that the player chose for himself.
            TypeSys::VarT<uint16_t>      m_RandomCount;         ///< Keeps track of the next random number that is returned by the GetRandom() methods.
            TypeSys::VarT<uint8_t>       m_StateOfExistence;    ///< For the player's main state machine, e.g. "spectator, dead, alive, ...".
            TypeSys::VarT<uint8_t>       m_Health;              ///< Health.
            TypeSys::VarT<uint8_t>       m_Armor;               ///< Armor.
            TypeSys::VarT<uint8_t>       m_Frags;               ///< Frags.
            TypeSys::VarT<uint8_t>       m_ActiveWeaponNr;      ///< The index number into the CarriedWeapon components of this entity, starting at 1, indicating the currently active weapon. The weapon must also be available (have been picked up) before the player can use it. A value of 0 means that "no" weapon is currently active.
            TypeSys::VarT<uint8_t>       m_NextWeaponNr;        ///< The next weapon to be drawn by SelectNextWeapon(). Like m_ActiveWeaponNr, this is an index number into the CarriedWeapon components of this entity, starting at 1. A value of 0 means "none".
            TypeSys::VarT<float>         m_HeadSway;            ///< The progress of one "head swaying" cycle in state FrozenSpectator.

            IntrusivePtrT<GuiSys::GuiImplT> m_HUD;              ///< The GUI instance for the player's Head-Up Display.
            ParticleMaterialSetT*           m_GenericMatSet;    ///< Resources needed to implement the temporary RegisterParticle() method.
            ParticleMaterialSetT*           m_WhiteSmokeMatSet; ///< Resources needed to implement the temporary RegisterParticle() method.
        };
    }
}

#endif
