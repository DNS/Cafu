/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#ifndef CAFU_RIGID_BODY_HPP_INCLUDED
#define CAFU_RIGID_BODY_HPP_INCLUDED

#include "../../BaseEntity.hpp"
#include "btBulletDynamicsCommon.h"


class EntityCreateParamsT;
struct luaL_Reg;
namespace cf { namespace SceneGraph { class GenericNodeT; } }


class EntRigidBodyT : public BaseEntityT, public btMotionState
{
    public:

    EntRigidBodyT(const EntityCreateParamsT& Params);
    ~EntRigidBodyT();

    // Implement the BaseEntityT interface.
    void TakeDamage(BaseEntityT* Entity, char Amount, const VectorT& ImpactDir);
    void Think(float FrameTime, unsigned long ServerFrameNr);
    void Draw(bool FirstPersonView, float LodDist) const;

    // Implement the btMotionState interface.
    void getWorldTransform(btTransform& worldTrans) const;
    void setWorldTransform(const btTransform& worldTrans);


    const cf::TypeSys::TypeInfoT* GetType() const;
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

    // Override the base class methods.
    void DoSerialize(cf::Network::OutStreamT& Stream) const;
    void DoDeserialize(cf::Network::InStreamT& Stream);

    const cf::SceneGraph::GenericNodeT* m_RootNode;         ///< The root node of the scene graph of the model (brushwork) of this entity.
    btCollisionShape*                   m_CollisionShape;   ///< The collision shape for use with the rigid body.
    btRigidBody*                        m_RigidBody;        ///< The rigid body for use in the physics world.
    const Vector3dT                     m_OrigOffset;       ///< The offset from the entity origin to the physics model origin.
    const Vector3dT                     m_HalfExtents;      ///< Half of the extents (the "radius") of the bounding-box of this model.
    btQuaternion                        m_Rotation;         ///< Complementing the origin, this is the spatial "orientation" of the model (the "better" alternative to heading, pitch and bank).


    // Script methods (to be called from the map/entity Lua scripts).
    static int ApplyImpulse(lua_State* LuaState);
    static int SetGravity(lua_State* LuaState);

    static const luaL_Reg MethodsList[];
};

#endif
