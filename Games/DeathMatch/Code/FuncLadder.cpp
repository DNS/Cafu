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

#include "FuncLadder.hpp"
#include "EntityCreateParams.hpp"
#include "../../GameWorld.hpp"
#include "PhysicsWorld.hpp"
#include "Libs/LookupTables.hpp"
#include "ClipSys/CollisionModel_base.hpp"
#include "ClipSys/TraceSolid.hpp"
#include "TypeSys.hpp"

using namespace GAME_NAME;


// Implement the type info related code.
const cf::TypeSys::TypeInfoT* EntFuncLadderT::GetType() const
{
    return &TypeInfo;
 // return &EntFuncLadderT::TypeInfo;
}

void* EntFuncLadderT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new EntFuncLadderT(*static_cast<const EntityCreateParamsT*>(&Params));
}

const cf::TypeSys::TypeInfoT EntFuncLadderT::TypeInfo(GetBaseEntTIM(), "EntFuncLadderT", "BaseEntityT", EntFuncLadderT::CreateInstance, NULL /*MethodsList*/);


EntFuncLadderT::EntFuncLadderT(const EntityCreateParamsT& Params)
    : BaseEntityT(Params,
                  BoundingBox3dT(Vector3dT()),
                  0)
{
    assert(CollisionModel!=NULL);   // A ladder entity without collision model is useless.

    // Registering the clip model with the clip world is very important, so that we cannot run "into" ladder brushes.
    ClipModel.SetOrigin(m_Origin);
    ClipModel.Register();


    // Corresponding to this entities CollisionModelT, use the related btCollisionShape
    // for adding a btRigidBody (which "is a" btCollisionObject) to the PhysicsWorld.
    btCollisionShape* LadderShape=CollisionModel->GetBulletAdapter();

    m_RigidBody=new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(0, NULL /*MotionState*/, LadderShape, btVector3(0, 0, 0)));
    m_RigidBody->setUserPointer(this);  // This entity is associated to the m_RigidBody.

    GameWorld->GetPhysicsWorld().AddRigidBody(m_RigidBody);
}


EntFuncLadderT::~EntFuncLadderT()
{
    // Remove our ladder body from the physics world again and then delete it.
    GameWorld->GetPhysicsWorld().RemoveRigidBody(m_RigidBody);

    delete m_RigidBody;
}


void EntFuncLadderT::DoDeserialize(cf::Network::InStreamT& Stream)
{
    ClipModel.SetOrigin(m_Origin);
    ClipModel.Register();
}
