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

#ifndef CAFU_GAMESYS_COMPONENT_PHYSICS_HPP_INCLUDED
#define CAFU_GAMESYS_COMPONENT_PHYSICS_HPP_INCLUDED

#include "CompBase.hpp"
#include "btBulletDynamicsCommon.h"


namespace cf
{
    namespace GameSys
    {
        /// This component includes the body of this entity in the dynamic simulation of physics.
        ///
        /// Without this component, the entity is either *static* (it doesn't move at all), *kinematic*
        /// (it is moved by script or program code), or it doesn't participate in physics computations
        /// at all.
        ///
        /// With this component, the entity's body is subject to gravity, impulses, and generally to
        /// the dynamic simulation of physics effects in the game world.
        class ComponentPhysicsT : public ComponentBaseT, public btMotionState
        {
            public:

            /// The constructor.
            ComponentPhysicsT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentPhysicsT(const ComponentPhysicsT& Comp);

            /// The destructor.
            ~ComponentPhysicsT();

            float GetMass() const { return m_Mass.Get(); }
            const btRigidBody* GetRigidBody() const { return m_RigidBody; }


            // Base class overrides.
            ComponentPhysicsT* Clone() const;
            const char* GetName() const { return "Physics"; }
            void UpdateDependencies(EntityT* Entity);


            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            protected:

            // The Lua API methods of this class.
            static int ApplyImpulse(lua_State* LuaState);
            static int SetGravity(lua_State* LuaState);
            static int toString(lua_State* LuaState);

            static const luaL_Reg               MethodsList[];  ///< The list of Lua methods for this class.
            static const char*                  DocClass;
            static const cf::TypeSys::MethsDocT DocMethods[];
            static const cf::TypeSys::VarsDocT  DocVars[];


            private:

            // Implement the btMotionState interface.
            void getWorldTransform(btTransform& worldTrans) const;
            void setWorldTransform(const btTransform& worldTrans);

            TypeSys::VarT<float> m_Mass;

            btCollisionShape*    m_CollisionShape;  ///< The collision shape for use with the rigid body.
            btRigidBody*         m_RigidBody;       ///< The rigid body for use in the physics world.
        };
    }
}

#endif
