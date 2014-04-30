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

#include "HumanPlayer.hpp"
#include "EntityCreateParams.hpp"
#include "GameImpl.hpp"
#include "Interpolator.hpp"

#include "../../PlayerCommand.hpp"
#include "ClipSys/ClipWorld.hpp"
#include "ClipSys/CollisionModelMan.hpp"
#include "ClipSys/TraceResult.hpp"
#include "GameSys/HumanPlayer/cw.hpp"
#include "GameSys/CompCollisionModel.hpp"
#include "GameSys/CompHumanPlayer.hpp"
#include "GameSys/CompModel.hpp"
#include "GameSys/CompPlayerPhysics.hpp"
#include "GameSys/CompScript.hpp"
#include "GameSys/EntityCreateParams.hpp"
#include "GameSys/World.hpp"
#include "GuiSys/GuiImpl.hpp"
#include "GuiSys/GuiManImpl.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "Math3D/Angles.hpp"
#include "Models/AnimPose.hpp"
#include "Models/Model_cmdl.hpp"
#include "Network/State.hpp"
#include "OpenGL/OpenGLWindow.hpp"
#include "TypeSys.hpp"
#include "UniScriptState.hpp"

#include <cstdio>
#include <string.h>

#ifndef _WIN32
#define _stricmp strcasecmp
#endif

#undef min    // See http://stackoverflow.com/questions/5004858/stdmin-gives-error
#undef max

using namespace GAME_NAME;


// Constants for State.StateOfExistence.
const char StateOfExistence_Alive          =0;
const char StateOfExistence_Dead           =1;
const char StateOfExistence_FrozenSpectator=2;
const char StateOfExistence_FreeSpectator  =3;


// Implement the type info related code.
const cf::TypeSys::TypeInfoT* EntHumanPlayerT::GetType() const
{
    return &TypeInfo;
 // return &EntHumanPlayerT::TypeInfo;
}

void* EntHumanPlayerT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new EntHumanPlayerT(*static_cast<const EntityCreateParamsT*>(&Params));
}

const cf::TypeSys::TypeInfoT EntHumanPlayerT::TypeInfo(GetBaseEntTIM(), "EntHumanPlayerT", "BaseEntityT", EntHumanPlayerT::CreateInstance, NULL);


EntHumanPlayerT::EntHumanPlayerT(const EntityCreateParamsT& Params)
    : BaseEntityT(Params,
                  BoundingBox3dT(Vector3dT( 16.0,  16.0,   4.0),    // A total of 32*32*72 inches, eye height at 68 inches.
                                 Vector3dT(-16.0, -16.0, -68.0)),
                  0)
{
    // TODO: This must be reconsidered when finally switching to the Component System!
    // // Interpolation is only required for client entities that are not "our" local entity (which is predicted).
    // // Therefore, the engine core automatically exempts "our" client from interpolation, but applies it to others.
    // Register(new InterpolatorT<Vector3dT>(m_Origin));

    // We can only hope that 'Origin' is a nice place for a "Frozen Spectator"...

    // Because 'StateOfExistence==StateOfExistence_FrozenSpectator', we mis-use the 'Velocity' member variable a little
    // for slightly turning/swaying the head in this state. See the 'Think()' code below!
    // TODO: HEAD SWAY: State.Velocity.y=m_Heading;
    // TODO: HEAD SWAY: State.Velocity.z=m_Bank;


    // This would be the proper way to do it, but we don't know here whether this is the local human player of a client.
    // (Could also be the entity of another (remote) client, or a server-side entity.)
    // Note that if (is local human player) is true, cf::GuiSys::GuiMan!=NULL is implied to be true, too.
    // GuiHUD=(is local human player) ? cf::GuiSys::GuiMan->Find("Games/DeathMatch/GUIs/HUD.cgui", true) : NULL;


#if 0   // TODO: Re-add a physics body to the human player when ported to the Component System!
    // btCollisionShape* m_CollisionShape;         ///< The collision shape that is used to approximate and represent this player in the physics world.
    // btRigidBody*      m_RigidBody;              ///< The rigid body (of "kinematic" type) for use in the physics world.

    // The /1000 is because our physics world is in meters.
    m_CollisionShape=new btBoxShape(conv(UnitsToPhys(m_Dimensions.Max-m_Dimensions.Min)/2.0));  // Should use a btCylinderShapeZ instead of btBoxShape?

    // Our rigid body is of Bullet type "kinematic". That is, we move it ourselves, not the world dynamics.
    m_RigidBody=new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(0, this /*btMotionState for this body*/, m_CollisionShape, btVector3()));

    m_RigidBody->setUserPointer(this);  // This entity is associated to the m_RigidBody.
    m_RigidBody->setCollisionFlags(m_RigidBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    m_RigidBody->setActivationState(DISABLE_DEACTIVATION);

    // GameWorld->GetPhysicsWorld().AddRigidBody(m_RigidBody);     // Don't add to the world here - adding/removing is done when State.StateOfExistence changes.
#endif
}


void EntHumanPlayerT::Draw(bool FirstPersonView, float LodDist) const
{
    IntrusivePtrT<cf::GameSys::ComponentHumanPlayerT> CompHP = dynamic_pointer_cast<cf::GameSys::ComponentHumanPlayerT>(m_Entity->GetComponent("HumanPlayer"));

    if (MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::AMBIENT)
    {
        const float* ALC=MatSys::Renderer->GetCurrentAmbientLightColor();

        LastSeenAmbientColor=VectorT(ALC[0], ALC[1], ALC[2]);
    }
}
