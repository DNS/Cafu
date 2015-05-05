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
            TypeSys::VarT<bool>            m_OldWishJump;   ///< Only jump if the jump key was *not* pressed in the previous frame.

            const cf::ClipSys::ClipWorldT* m_ClipWorld;
            const cf::ClipSys::ClipModelT* m_IgnoreClipModel;
            Vector3dT                      m_Origin;
            Vector3dT                      m_Vel;
            cf::ClipSys::TraceBoxT         m_DimSolid;
        };
    }
}

#endif
