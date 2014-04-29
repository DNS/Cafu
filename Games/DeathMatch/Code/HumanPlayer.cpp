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


void EntHumanPlayerT::Think(float FrameTime_BAD_DONT_USE, unsigned long ServerFrameNr)
{
    // **********************************************************************************************************************************************
    // IMPORTANT NOTE:
    //
    // Although the Think() functions of all entity classes (children of BaseEntityT) are only ever called on the SERVER-SIDE,
    // and you should always think of them as ONLY EVER BEING CALLED ON THE SERVER-SIDE, the truth is that there is an exception:
    // For EntHumanPlayerT entities (but no other entity classes!), this function is also called on the client-side for prediction.
    // You should still think of this function as ONLY EVER BEING CALLED ON THE SERVER-SIDE,
    // but the above mentioned exception implies that this function must always be deterministic (that is, reproducible).
    // That especially means:
    // 1. Calls to rand()-like functions are NOT allowed! Rather, you have to use something like
    //        LookupTables::RandomUShort[PlayerCommands[PCNr].Nr & 0xFFF]
    //    in order to obtain pseudo-random numbers from PlayerCommands[PCNr].Nr. Related examples are found in the code below.
    // 2. This function must NOT have a state besides the member variable 'EntHumanPlayerT::State'.
    //    That is, you can NOT introduce and use additional private or protected member variables, because the
    //    client-side prediction will only take the State variable (and no additional private variables) into account!
    //    If you did anyway, this function would not be reproducible by the State variable alone,
    //    and thus violate the most vital requirement for prediction.
    //    As a solution, do only use member variables of State for all your state management!
    //    This seems sometimes problematic and like working-around-the-corner, but you will find that very often you can exploit
    //    one or more of the existing variables in State for your purposes! Examples are found in the code below,
    //    where the next weapon animation sequence is simply determined by the previous weapon animation sequence!
    //    If that in rare cases does not work, use for example one of the unused HaveAmmo or HaveAmmoInWeapon variables.
    //    It should also be noted that having *constant* state is perfectly acceptable.
    //    For example, if you load something from disk and/or precalculate something in the constructor (thus both on client and server side),
    //    it is perfectly fine if you refer to this data later from Think() - just don't modify it!
    //    I make heavy use of this feature in the MOD for the USAF, where complicated spatial paths are precalculated and stored as "constant state".
    //
    // Another special case (not related to prediction) is the fact that the Think()ing of EntHumanPlayerT entities
    // is driven by PlayerCommands, and NOT by the usual FrameTime and ServerFrameNr parameters of this function.
    // It is the task of this function to process all the PlayerCommands that are available
    // (fed by the server by calling the EntHumanPlayerT::ProcessConfigString() function appropriately),
    // and thus the regular parameters 'FrameTime' and 'ServerFrameNr' MUST NOT BE USED!
    // (Moreover, the client-side prediction has no idea what to pass-in here anyway, and thus always passes 'FrameTime=-1.0' and 'ServerFrameNr=0'.)
    //
    // Unfortunately, there is an exception to the exception:
    // Sometimes, we want to or have to do things that should only occur on server-side (and thus be exempted from prediction), like
    // a) modifying other (usually non-predicted) entities (like calling another entities TakeDamage() function), or
    // b) creating new entities (e.g. a thrown face-hugger, a fired missile, ...) (not possible on client-side).
    // Against my initial statement above, this requires a concious consideration if this is a regular server-side thinking call,
    // or a client-side prediction call.
    // Whether we are on server-side or not is determined by the ThinkingOnServerSide variable as shown below.
    // The code below also has examples (partially hidden in the subfunctions it calls) for both cases a) and b).
    //
    // Note that all this applies ONLY to HumanPlayerT::Think()! All OTHER entities are NOT affected!
    // **********************************************************************************************************************************************

    IntrusivePtrT<cf::GameSys::ComponentHumanPlayerT> CompHP = dynamic_pointer_cast<cf::GameSys::ComponentHumanPlayerT>(m_Entity->GetComponent("HumanPlayer"));
    IntrusivePtrT<cf::GameSys::ComponentPlayerPhysicsT> CompPlayerPhysics = dynamic_pointer_cast<cf::GameSys::ComponentPlayerPhysicsT>(m_Entity->GetComponent("PlayerPhysics"));
    IntrusivePtrT<cf::GameSys::ComponentModelT> Model3rdPerson = dynamic_pointer_cast<cf::GameSys::ComponentModelT>(m_Entity->GetComponent("Model"));

    if (CompHP == NULL) return;     // The presence of CompHP is mandatory...
    if (CompPlayerPhysics == NULL) return;     // The presence of CompPlayerPhysics is mandatory...
    if (Model3rdPerson == NULL) return;     // The presence of CompPlayerPhysics is mandatory...

    ArrayT<PlayerCommandT>& PlayerCommands = CompHP->GetPlayerCommands();

    const bool ThinkingOnServerSide=FrameTime_BAD_DONT_USE>=0.0;

    for (unsigned long PCNr=0; PCNr<PlayerCommands.Size(); PCNr++)
    {
        CompHP->Think(PlayerCommands[PCNr], ThinkingOnServerSide);
    }

    PlayerCommands.Overwrite();
}


void EntHumanPlayerT::Draw(bool FirstPersonView, float LodDist) const
{
    IntrusivePtrT<cf::GameSys::ComponentHumanPlayerT> CompHP = dynamic_pointer_cast<cf::GameSys::ComponentHumanPlayerT>(m_Entity->GetComponent("HumanPlayer"));

    if (MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::AMBIENT)
    {
        const float* ALC=MatSys::Renderer->GetCurrentAmbientLightColor();

        LastSeenAmbientColor=VectorT(ALC[0], ALC[1], ALC[2]);
    }


    // Keep in mind that when 'FirstPersonView==true', this is usually the human player entity
    // of the local client, which gets *predicted*. Thus, there is no point in modifying the 'State' member variable.
    // Otherwise however, when 'FirstPersonView==false', this is usually a human player entity
    // of another client, which is *NOT* predicted. Thus, we may modify the 'State' member variable up to a certain extend.
    if (FirstPersonView)
    {
        // Draw "view" model of the weapon
        if (CompHP->GetHaveWeapons() & (1 << CompHP->GetActiveWeaponSlot()))     // Only draw the active weapon if we actually "have" it
        {
#if 0     // TODO!
            Vector3fT LgtPos(MatSys::Renderer->GetCurrentLightSourcePosition());
            Vector3fT EyePos(MatSys::Renderer->GetCurrentEyePosition());

            // The translation is not actually required, but gives the weapon a very nice 'shifting' effect when the player looks up/down.
            // If there ever is a problem with view model distortion, this may be a cause.
            LgtPos.z+=0.5f;
            EyePos.z+=0.5f;
            MatSys::Renderer->Translate(MatSys::RendererI::MODEL_TO_WORLD, 0.0f, 0.0f, -0.5f);

            const float DegPitch=float(m_Pitch)/8192.0f*45.0f;

            LgtPos=LgtPos.GetRotY(-DegPitch);
            EyePos=EyePos.GetRotY(-DegPitch);
            MatSys::Renderer->RotateY(MatSys::RendererI::MODEL_TO_WORLD, DegPitch);

            MatSys::Renderer->SetCurrentLightSourcePosition(LgtPos.x, LgtPos.y, LgtPos.z);
            MatSys::Renderer->SetCurrentEyePosition(EyePos.x, EyePos.y, EyePos.z);
#endif


            const CafuModelT* WeaponModel=CompHP->GetCarriedWeapon(CompHP->GetActiveWeaponSlot())->GetViewWeaponModel();
            AnimPoseT*        Pose       =WeaponModel->GetSharedPose(WeaponModel->GetAnimExprPool().GetStandard(CompHP->GetActiveWeaponSequNr(), CompHP->GetActiveWeaponFrameNr()));

            Pose->Draw(-1 /*default skin*/, LodDist);
        }
    }
    else
    {
        if (CompHP->GetStateOfExistence() != StateOfExistence_Alive && CompHP->GetStateOfExistence() != StateOfExistence_Dead) return;

        // TODO / FIXME: Are these four lines still needed?
        const float OffsetZ = -32.0f;
        MatSys::Renderer->GetCurrentLightSourcePosition()[2]-=OffsetZ;
        MatSys::Renderer->GetCurrentEyePosition        ()[2]-=OffsetZ;
        MatSys::Renderer->Translate(MatSys::RendererI::MODEL_TO_WORLD, 0.0f, 0.0f, OffsetZ);


        // The own player body model is drawn autonomously by the respective Model component.
        // Here, also draw the "_p" (player) model of the active weapon as sub-model of the body.
        if (CompHP->GetHaveWeapons() & (1 << CompHP->GetActiveWeaponSlot()))
        {
            IntrusivePtrT<cf::GameSys::ComponentModelT> Model3rdPerson = dynamic_pointer_cast<cf::GameSys::ComponentModelT>(m_Entity->GetComponent("Model"));
            const CafuModelT* WeaponModel = CompHP->GetCarriedWeapon(CompHP->GetActiveWeaponSlot())->GetPlayerWeaponModel();
            AnimPoseT*        WeaponPose  = WeaponModel->GetSharedPose(WeaponModel->GetAnimExprPool().GetStandard(0, 0.0f));

            if (Model3rdPerson != NULL)
            {
                WeaponPose->SetSuperPose(Model3rdPerson->GetPose());
                WeaponPose->Draw(-1 /*default skin*/, LodDist);
                WeaponPose->SetSuperPose(NULL);
            }
        }
    }
}


void EntHumanPlayerT::PostDraw(float FrameTime, bool FirstPersonView)
{
    IntrusivePtrT<cf::GameSys::ComponentHumanPlayerT> CompHP = dynamic_pointer_cast<cf::GameSys::ComponentHumanPlayerT>(m_Entity->GetComponent("HumanPlayer"));

    // Code for state driven effects.
    if (CompHP->GetHaveWeapons() & (1 << CompHP->GetActiveWeaponSlot()))
    {
        IntrusivePtrT<cf::GameSys::ComponentHumanPlayerT> CompHP = dynamic_pointer_cast<cf::GameSys::ComponentHumanPlayerT>(m_Entity->GetComponent("HumanPlayer"));

        CompHP->GetCarriedWeapon(CompHP->GetActiveWeaponSlot())->ClientSide_HandleStateDrivenEffects(CompHP);
    }
}
