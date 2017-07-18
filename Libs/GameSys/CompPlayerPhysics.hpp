/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GAMESYS_COMPONENT_PLAYER_PHYSICS_HPP_INCLUDED
#define CAFU_GAMESYS_COMPONENT_PLAYER_PHYSICS_HPP_INCLUDED

#include "CompBase.hpp"
#include "ClipSys/TraceSolid.hpp"


namespace cf { namespace ClipSys { class ClipModelT; } }
namespace cf { namespace ClipSys { class ClipWorldT; } }


namespace cf
{
    namespace GameSys
    {
        /// This component implements human player physics for its entity.
        /// It updates the entity's origin according to the laws of simple physics
        /// that are appropriate for player movement.
        /// The component does not act on its own in a server's Think() step, but is
        /// only a helper to other C++ or script code that must drive it explicitly.
        class ComponentPlayerPhysicsT : public ComponentBaseT
        {
            public:

            /// The constructor.
            ComponentPlayerPhysicsT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentPlayerPhysicsT(const ComponentPlayerPhysicsT& Comp);

            /// This is the main method of this component: It advances the entity's origin
            /// according to the laws of simple physics and the given state and parameters.
            /// Other C++ or script code of the entity typically calls this method on each
            /// clock-tick (frame) of the server.
            /// @param FrameTime       The time across which the entity is to be advanced.
            /// @param WishVelocity    The desired velocity of the entity as per user input.
            /// @param WishVelLadder   The desired velocity on a ladder as per user input.
            /// @param WishJump        Does the user want the entity to jump?
            void MoveHuman(float FrameTime, const Vector3fT& WishVelocity, const Vector3fT& WishVelLadder, bool WishJump);

            /// Returns the current velocity.
            const Vector3dT& GetVelocity() const { return m_Velocity.Get(); }

            // Base class overrides.
            ComponentPlayerPhysicsT* Clone() const;
            const char* GetName() const { return "PlayerPhysics"; }
            void UpdateDependencies(EntityT* Entity);


            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            protected:

            // The Lua API methods of this class.
            static int MoveHuman(lua_State* LuaState);
            static int toString(lua_State* LuaState);

            static const luaL_Reg               MethodsList[];  ///< The list of Lua methods for this class.
            static const char*                  DocClass;
            static const cf::TypeSys::MethsDocT DocMethods[];
            static const cf::TypeSys::VarsDocT  DocVars[];


            private:

            enum PosCatT { InAir, OnSolid };

            PosCatT CategorizePosition() const;
            void ApplyFriction(double FrameTime, PosCatT PosCat);
            void ApplyAcceleration(double FrameTime, PosCatT PosCat, const Vector3dT& WishVelocity);
            void ApplyGravity(double FrameTime, PosCatT PosCat);
            void FlyMove(double TimeLeft);
            void GroundMove(double FrameTime);
            void MoveHuman(float FrameTime, const Vector3dT& WishVelocity, const VectorT& WishVelLadder, bool WishJump);

            TypeSys::VarT<Vector3dT>       m_Velocity;      ///< The current velocity of the entity.
            TypeSys::VarT<BoundingBox3dT>  m_Dimensions;    ///< The bounding box of the entity (relative to the origin).
            TypeSys::VarT<double>          m_StepHeight;    ///< The maximum height that the entity can climb in one step.

            const cf::ClipSys::ClipWorldT* m_ClipWorld;
            const cf::ClipSys::ClipModelT* m_IgnoreClipModel;
            Vector3dT                      m_Origin;
            Vector3dT                      m_Vel;
            cf::ClipSys::TraceBoxT         m_DimSolid;
        };
    }
}

#endif
