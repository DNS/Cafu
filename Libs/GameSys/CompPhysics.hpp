/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
