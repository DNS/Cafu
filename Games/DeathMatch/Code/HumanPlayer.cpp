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
#include "Constants_AmmoSlots.hpp"
#include "Constants_WeaponSlots.hpp"
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


bool EntHumanPlayerT::CheckGUI(IntrusivePtrT<cf::GameSys::ComponentModelT> CompModel, Vector3fT& MousePos) const
{
    // 1. Has this entitiy an interactive GUI at all?
    IntrusivePtrT<cf::GuiSys::GuiImplT> Gui = CompModel->GetGui();

    if (Gui.IsNull()) return false;
    if (!Gui->GetIsInteractive()) return false;


    // 2. Can we retrieve the plane of its screen panel?
    Vector3fT GuiOrigin;
    Vector3fT GuiAxisX;
    Vector3fT GuiAxisY;

    AnimPoseT* Pose = CompModel->GetPose();

    if (!Pose) return false;
    if (!Pose->GetGuiPlane(0, GuiOrigin, GuiAxisX, GuiAxisY)) return false;

    const MatrixT M2W = CompModel->GetEntity()->GetTransform()->GetEntityToWorld();

    GuiOrigin = M2W.Mul1(GuiOrigin);
    GuiAxisX  = M2W.Mul0(GuiAxisX);
    GuiAxisY  = M2W.Mul0(GuiAxisY);


    // 3. Are we looking roughly into the screen normal?
    IntrusivePtrT<cf::GameSys::ComponentHumanPlayerT> CompHP = dynamic_pointer_cast<cf::GameSys::ComponentHumanPlayerT>(m_Entity->GetComponent("HumanPlayer"));

    const Vector3fT GuiNormal = normalize(cross(GuiAxisY, GuiAxisX), 0.0f);
    const Plane3fT  GuiPlane  = Plane3fT(GuiNormal, dot(GuiOrigin, GuiNormal));
    const Vector3fT ViewDir   = CompHP->GetViewDirWS().AsVectorOfFloat();

    if (-dot(ViewDir, GuiPlane.Normal)<0.001f) return false;


    // 4. Does our view ray hit the screen panel?
    // (I've obtained the equation for r by rolling the corresponding Plane3T<T>::GetIntersection() method out.)
    const Vector3fT OurOrigin = m_Entity->GetTransform()->GetOriginWS();
    const float     r         = (GuiPlane.Dist-dot(GuiPlane.Normal, OurOrigin))/dot(GuiPlane.Normal, ViewDir);

    if (r<0.0f || r>160.0f) return false;

    const Vector3fT HitPos=OurOrigin+ViewDir*r;

    // Project HitPos into the GUI plane, in order to obtain the 2D coordinate in the GuiSys' virtual pixel space (640x480).
    float px = dot(HitPos - GuiOrigin, GuiAxisX) / GuiAxisX.GetLengthSqr();
    float py = dot(HitPos - GuiOrigin, GuiAxisY) / GuiAxisY.GetLengthSqr();

    if (px < -0.5f || px > 1.5f) return false;
    if (py < -0.5f || py > 1.5f) return false;

    if (px < 0.0f) px = 0.0f; if (px > 0.99f) px = 0.99f;
    if (py < 0.0f) py = 0.0f; if (py > 0.98f) py = 0.98f;


    // TODO: Trace gegen walls!
    // TODO: Benutzt schon jemand anderes dieses GUI?  Sehr wichtig, um zu vermeiden, daß andauernd auf dem Server der Mauscursor umherspringt
    //       (und damit z.B. in jedem Frame Interpolation-Einträge anlegt, die beliebig anwachsen würden).


    MousePos.x=px*640.0f;
    MousePos.y=py*480.0f;

    return true;
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
        switch (CompHP->GetStateOfExistence())
        {
            case StateOfExistence_Alive:
            {
                // Update Heading
                {
                    cf::math::AnglesfT Angles(m_Entity->GetTransform()->GetQuatPS());       // We actually rotate the entity.

                    Angles.yaw() -= PlayerCommands[PCNr].DeltaHeading / 8192.0f * 45.0f;

                    if (PlayerCommands[PCNr].Keys & PCK_TurnLeft ) Angles.yaw() += 120.0f * PlayerCommands[PCNr].FrameTime;
                    if (PlayerCommands[PCNr].Keys & PCK_TurnRight) Angles.yaw() -= 120.0f * PlayerCommands[PCNr].FrameTime;

                    Angles.pitch() = 0.0f;
                    Angles.roll()  = 0.0f;

                    m_Entity->GetTransform()->SetQuatPS(cf::math::QuaternionfT(Angles));
                }

                // Update Pitch and Bank
                {
                    cf::math::AnglesfT Angles(m_Entity->GetChildren()[0]->GetTransform()->GetQuatPS());     // We update the camera, not the entity.

                    const int dp = PlayerCommands[PCNr].DeltaPitch;
                    Angles.pitch() += (dp < 32768 ? dp : dp - 65536) / 8192.0f * 45.0f;

                    if (PlayerCommands[PCNr].Keys & PCK_LookUp  ) Angles.pitch() -= 120.0f * PlayerCommands[PCNr].FrameTime;
                    if (PlayerCommands[PCNr].Keys & PCK_LookDown) Angles.pitch() += 120.0f * PlayerCommands[PCNr].FrameTime;

                    if (Angles.pitch() >  90.0f) Angles.pitch() =  90.0f;
                    if (Angles.pitch() < -90.0f) Angles.pitch() = -90.0f;

                    const int db = PlayerCommands[PCNr].DeltaBank;
                    Angles.roll() += (db < 32768 ? db : db - 65536) / 8192.0f * 45.0f;

                    if (PlayerCommands[PCNr].Keys & PCK_CenterView)
                    {
                        Angles.yaw()   = 0.0f;
                        Angles.pitch() = 0.0f;
                        Angles.roll()  = 0.0f;
                    }

                    m_Entity->GetChildren()[0]->GetTransform()->SetQuatPS(cf::math::QuaternionfT(Angles));
                }


                VectorT             WishVelocity;
                bool                WishJump=false;
                const Vector3dT     Vel     = cf::math::Matrix3x3fT(m_Entity->GetTransform()->GetQuatWS()).GetAxis(0).AsVectorOfDouble() * 240.0;
                const unsigned long Keys    =PlayerCommands[PCNr].Keys;

                if (Keys & PCK_MoveForward ) WishVelocity=             VectorT( Vel.x,  Vel.y, 0);
                if (Keys & PCK_MoveBackward) WishVelocity=WishVelocity+VectorT(-Vel.x, -Vel.y, 0);
                if (Keys & PCK_StrafeLeft  ) WishVelocity=WishVelocity+VectorT(-Vel.y,  Vel.x, 0);
                if (Keys & PCK_StrafeRight ) WishVelocity=WishVelocity+VectorT( Vel.y, -Vel.x, 0);

                if (Keys & PCK_Jump        ) WishJump=true;
             // if (Keys & PCK_Duck        ) ;
                if (Keys & PCK_Walk        ) WishVelocity=scale(WishVelocity, 0.5);

                VectorT       WishVelLadder;
                const VectorT ViewLadder = CompHP->GetViewDirWS() * 150.0;

                // TODO: Also take LATERAL movement into account.
                // TODO: All this needs a HUGE clean-up! Can probably put a lot of this stuff into Physics::MoveHuman.
                if (Keys & PCK_MoveForward ) WishVelLadder=WishVelLadder+ViewLadder;
                if (Keys & PCK_MoveBackward) WishVelLadder=WishVelLadder-ViewLadder;
                if (Keys & PCK_Walk        ) WishVelLadder=scale(WishVelLadder, 0.5);

                /*if (Clients[ClientNr].move_noclip)
                {
                    // This code was simply changed and rewritten until it "worked".
                    // May still be buggy anyway.
                    double RadPitch=double(m_Pitch)/32768.0*3.141592654;
                    double Fak     =VectorDot(WishVelocity, VectorT(LookupTables::Angle16ToSin[m_Heading], LookupTables::Angle16ToCos[m_Heading], 0));

                    WishVelocity.x*=cos(RadPitch);
                    WishVelocity.y*=cos(RadPitch);
                    WishVelocity.z=-sin(RadPitch)*Fak;

                    m_Origin=m_Origin+scale(WishVelocity, PlayerCommands[PCNr].FrameTime);

                    // TODO: If not already done on state change (--> "noclip"), set the model sequence to "swim".  ;-)
                }
                else */
                {
                    VectorT XYVel    = CompPlayerPhysics->GetVelocity(); XYVel.z = 0;
                    double  OldSpeed = length(XYVel);

                    CompPlayerPhysics->SetMember("StepHeight", 18.5);
                    CompPlayerPhysics->MoveHuman(PlayerCommands[PCNr].FrameTime, WishVelocity.AsVectorOfFloat(), WishVelLadder.AsVectorOfFloat(), WishJump);

                    XYVel = CompPlayerPhysics->GetVelocity(); XYVel.z = 0;
                    double NewSpeed = length(XYVel);

                    if (OldSpeed <= 40.0 && NewSpeed > 40.0) Model3rdPerson->SetMember("Animation", 3);
                    if (OldSpeed >= 40.0 && NewSpeed < 40.0) Model3rdPerson->SetMember("Animation", 1);
                }

                // GameWorld->ModelAdvanceFrameTime() is called on client side in Draw().


                // Handle the state machine of the "_v" (view) model of the current weapon.
                if (CompHP->GetHaveWeapons() & (1 << CompHP->GetActiveWeaponSlot()))
                {
                    const cf::GameSys::CarriedWeaponT* CarriedWeapon=CompHP->GetCarriedWeapon(CompHP->GetActiveWeaponSlot());

                    // Advance the frame time of the weapon.
                    const CafuModelT* WeaponModel=CarriedWeapon->GetViewWeaponModel();

                    IntrusivePtrT<AnimExprStandardT> StdAE = WeaponModel->GetAnimExprPool().GetStandard(CompHP->GetActiveWeaponSequNr(), CompHP->GetActiveWeaponFrameNr());

                    StdAE->SetForceLoop(true);
                    StdAE->AdvanceTime(PlayerCommands[PCNr].FrameTime);

                    const float NewFrameNr = StdAE->GetFrameNr();
                    const bool  AnimSequenceWrap = NewFrameNr < CompHP->GetActiveWeaponFrameNr() || NewFrameNr > WeaponModel->GetAnims()[CompHP->GetActiveWeaponSequNr()].Frames.Size()-1;

                    CompHP->SetActiveWeaponFrameNr(NewFrameNr);

                    CarriedWeapon->ServerSide_Think(CompHP, PlayerCommands[PCNr], ThinkingOnServerSide, AnimSequenceWrap);
                }


                // Check if any key for changing the current weapon was pressed.
                ArrayT<char> SelectableWeapons;

                switch (Keys >> 28)
                {
                    case 0: break;  // No weapon slot was selected for changing the weapon.

                    case 1: if (CompHP->GetHaveWeapons() & (1 << WEAPON_SLOT_BATTLESCYTHE)) SelectableWeapons.PushBack(WEAPON_SLOT_BATTLESCYTHE);
                            if (CompHP->GetHaveWeapons() & (1 << WEAPON_SLOT_HORNETGUN   )) SelectableWeapons.PushBack(WEAPON_SLOT_HORNETGUN   );
                            break;

                    case 2: if ((CompHP->GetHaveWeapons() & (1 << WEAPON_SLOT_PISTOL)) && (CompHP->GetHaveAmmoInWeapons()[WEAPON_SLOT_PISTOL] || CompHP->GetHaveAmmo()[AMMO_SLOT_9MM])) SelectableWeapons.PushBack(WEAPON_SLOT_PISTOL);
                            if ((CompHP->GetHaveWeapons() & (1 << WEAPON_SLOT_357   )) && (CompHP->GetHaveAmmoInWeapons()[WEAPON_SLOT_357   ] || CompHP->GetHaveAmmo()[AMMO_SLOT_357])) SelectableWeapons.PushBack(WEAPON_SLOT_357   );
                            break;

                    case 3: if ((CompHP->GetHaveWeapons() & (1 << WEAPON_SLOT_SHOTGUN )) && (CompHP->GetHaveAmmoInWeapons()[WEAPON_SLOT_SHOTGUN ] || CompHP->GetHaveAmmo()[AMMO_SLOT_SHELLS]                                    )) SelectableWeapons.PushBack(WEAPON_SLOT_SHOTGUN );
                            if ((CompHP->GetHaveWeapons() & (1 << WEAPON_SLOT_9MMAR   )) && (CompHP->GetHaveAmmoInWeapons()[WEAPON_SLOT_9MMAR   ] || CompHP->GetHaveAmmo()[AMMO_SLOT_9MM   ] || CompHP->GetHaveAmmo()[AMMO_SLOT_ARGREN])) SelectableWeapons.PushBack(WEAPON_SLOT_9MMAR   );
                            if ((CompHP->GetHaveWeapons() & (1 << WEAPON_SLOT_CROSSBOW)) && (CompHP->GetHaveAmmoInWeapons()[WEAPON_SLOT_CROSSBOW] || CompHP->GetHaveAmmo()[AMMO_SLOT_ARROWS]                                    )) SelectableWeapons.PushBack(WEAPON_SLOT_CROSSBOW);
                            break;

                    case 4: if ((CompHP->GetHaveWeapons() & (1 << WEAPON_SLOT_RPG  )) && (CompHP->GetHaveAmmoInWeapons()[WEAPON_SLOT_RPG  ] || CompHP->GetHaveAmmo()[AMMO_SLOT_ROCKETS])) SelectableWeapons.PushBack(WEAPON_SLOT_RPG  );
                            if ((CompHP->GetHaveWeapons() & (1 << WEAPON_SLOT_GAUSS)) && (CompHP->GetHaveAmmoInWeapons()[WEAPON_SLOT_GAUSS] || CompHP->GetHaveAmmo()[AMMO_SLOT_CELLS  ])) SelectableWeapons.PushBack(WEAPON_SLOT_GAUSS);
                            if ((CompHP->GetHaveWeapons() & (1 << WEAPON_SLOT_EGON )) && (CompHP->GetHaveAmmoInWeapons()[WEAPON_SLOT_EGON ] || CompHP->GetHaveAmmo()[AMMO_SLOT_CELLS  ])) SelectableWeapons.PushBack(WEAPON_SLOT_EGON );
                            break;

                    case 5: if ((CompHP->GetHaveWeapons() & (1 << WEAPON_SLOT_GRENADE   )) && CompHP->GetHaveAmmoInWeapons()[WEAPON_SLOT_GRENADE   ]) SelectableWeapons.PushBack(WEAPON_SLOT_GRENADE   );
                            if ((CompHP->GetHaveWeapons() & (1 << WEAPON_SLOT_TRIPMINE  )) && CompHP->GetHaveAmmoInWeapons()[WEAPON_SLOT_TRIPMINE  ]) SelectableWeapons.PushBack(WEAPON_SLOT_TRIPMINE  );
                            if ((CompHP->GetHaveWeapons() & (1 << WEAPON_SLOT_FACEHUGGER)) && CompHP->GetHaveAmmoInWeapons()[WEAPON_SLOT_FACEHUGGER]) SelectableWeapons.PushBack(WEAPON_SLOT_FACEHUGGER);
                            break;

                    // case 6..15: break;
                }

                unsigned long SWNr;

                for (SWNr=0; SWNr<SelectableWeapons.Size(); SWNr++)
                    if (SelectableWeapons[SWNr] == CompHP->GetActiveWeaponSlot()) break;

                if (SWNr>=SelectableWeapons.Size())
                {
                    // If the currently active weapon is NOT among the SelectableWeapons
                    // (that means another weapon category was chosen), choose SelectableWeapons[0] as the active weapon.
                    if (SelectableWeapons.Size()>0)
                    {
                        char DrawSequNr=0;

                        CompHP->SetActiveWeaponSlot(SelectableWeapons[0]);

                        switch (CompHP->GetActiveWeaponSlot())
                        {
                            case WEAPON_SLOT_BATTLESCYTHE: DrawSequNr=1; break;
                            case WEAPON_SLOT_HORNETGUN   : DrawSequNr=0; break;
                            case WEAPON_SLOT_PISTOL      : DrawSequNr=7; break;
                            case WEAPON_SLOT_357         : DrawSequNr=5; break;
                            case WEAPON_SLOT_SHOTGUN     : DrawSequNr=6; break;
                            case WEAPON_SLOT_9MMAR       : DrawSequNr=4; break;
                            case WEAPON_SLOT_CROSSBOW    : DrawSequNr=5; break;
                            case WEAPON_SLOT_RPG         : DrawSequNr=5; break;
                            case WEAPON_SLOT_GAUSS       : DrawSequNr=8; break;
                            case WEAPON_SLOT_EGON        : DrawSequNr=9; break;
                            case WEAPON_SLOT_GRENADE     : DrawSequNr=7; break;
                            case WEAPON_SLOT_TRIPMINE    : DrawSequNr=0; break;
                            case WEAPON_SLOT_FACEHUGGER  : DrawSequNr=4; break;
                        }

                        CompHP->SetActiveWeaponSequNr(DrawSequNr);
                        CompHP->SetActiveWeaponFrameNr(0.0f);
                    }
                }
                else
                {
                    // Otherwise check if there are further selectable weapons (SelectableWeapons.Size()>1), and cycle to the next one.
                    if (SelectableWeapons.Size()>1)
                    {
                        char DrawSequNr=0;

                        CompHP->SetActiveWeaponSlot(SelectableWeapons[(SWNr+1) % SelectableWeapons.Size()]);

                        switch (CompHP->GetActiveWeaponSlot())
                        {
                            case WEAPON_SLOT_BATTLESCYTHE: DrawSequNr=1; break;
                            case WEAPON_SLOT_HORNETGUN   : DrawSequNr=0; break;
                            case WEAPON_SLOT_PISTOL      : DrawSequNr=7; break;
                            case WEAPON_SLOT_357         : DrawSequNr=5; break;
                            case WEAPON_SLOT_SHOTGUN     : DrawSequNr=6; break;
                            case WEAPON_SLOT_9MMAR       : DrawSequNr=4; break;
                            case WEAPON_SLOT_CROSSBOW    : DrawSequNr=5; break;
                            case WEAPON_SLOT_RPG         : DrawSequNr=5; break;
                            case WEAPON_SLOT_GAUSS       : DrawSequNr=8; break;
                            case WEAPON_SLOT_EGON        : DrawSequNr=9; break;
                            case WEAPON_SLOT_GRENADE     : DrawSequNr=7; break;
                            case WEAPON_SLOT_TRIPMINE    : DrawSequNr=0; break;
                            case WEAPON_SLOT_FACEHUGGER  : DrawSequNr=4; break;
                        }

                        CompHP->SetActiveWeaponSequNr(DrawSequNr);
                        CompHP->SetActiveWeaponFrameNr(0.0f);
                    }
                }


                // Check if we touched another entity.
                ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > AllEnts;

                m_Entity->GetWorld().GetRootEntity()->GetAll(AllEnts);

                for (unsigned int EntNr = 0; EntNr < AllEnts.Size(); EntNr++)
                {
                    if (AllEnts[EntNr] == m_Entity) continue;   // We don't touch us ourselves.

                    // Test if maybe we're near a static detail model with an interactive GUI.
                    const ArrayT< IntrusivePtrT<cf::GameSys::ComponentBaseT> >& Components = AllEnts[EntNr]->GetComponents();

                    // TODO: We iterate over each component of each entity here... can this somehow be reduced?
                    for (unsigned int CompNr = 0; CompNr < Components.Size(); CompNr++)
                    {
                        if (Components[CompNr]->GetType() != &cf::GameSys::ComponentModelT::TypeInfo) continue;

                        IntrusivePtrT<cf::GameSys::ComponentModelT> CompModel = static_pointer_cast<cf::GameSys::ComponentModelT>(Components[CompNr]);

                        // TODO: Also deal with the GUI when this is a REPREDICTION run???
                        //
                        // Answer: Normally not, because what is done during prediction is only for eye candy,
                        // and repeating it in REprediction would e.g. trigger/schedule interpolations *twice* (or in fact even more often).
                        //
                        // On the other hand, compare this to what happens when the player e.g. enters his name into a text field.
                        // The string with the name would be part of the "relevant GUI state" (state that is sync'ed over the network).
                        // As such, the string would ONLY be handled correctly when REprediction runs are applies to GUIs as they are applied
                        // to HumanPlayerTs (assuming the string is also handled in normal initial prediction).
                        // Example: The player enters "abc" on the client and prediction updates the string, but the server then sends a message
                        // that the player was force-moved by an explosion and the "abc" string was actually typed into the wall next to the GUI.
                        //
                        // ==> Either we have to run prediction AND REpredection with the GUIs,
                        //     OR we treat them like any other entity and ONLY update them on the server-side.
                        //
                        // ==> Conflict of interests: Only if the GUIs interpolation timers were a part of the "GUI state" would they work properly,
                        //     (which doesn't make much sense), or if we ran GUIs in prediction (but not REprediction) only (no good in the above example).
                        //
                        // ==> Solution: Update the relevant GUI state only ever on the server-side, and run GUI updates in prediction only
                        //               (but never in REprediction).
                        //
                        // ==> How do we separate the two???
                        //     ...
                        Vector3fT MousePos;

                        if (CheckGUI(CompModel, MousePos))
                        {
                            IntrusivePtrT<cf::GuiSys::GuiImplT> Gui = CompModel->GetGui();
                            CaMouseEventT ME;

                            Gui->SetMousePos(MousePos.x, MousePos.y);

                            ME.Type  =CaMouseEventT::CM_MOVE_X;
                            ME.Amount=0;

                            Gui->ProcessDeviceEvent(ME);

                            // Process mouse button events only on the server side.
                            // Note that this is a somewhat *arbitrary* compromise to the question "Where do we stop / how far do we go in"
                            // client prediction.
                            // Drawing the line here means that GUIs should be programmed in a way such that mouse movements do *not* affect
                            // world state -- only mouse clicks can do that. (In fact, we should probably also keep mouse movement events from
                            // the GUI when this is a *reprediction* run, as detailed in the (much older) comment above.)
                            if (ThinkingOnServerSide)
                            {
                                if ((Keys >> 28) == 10)
                                {
                                    ME.Type = CaMouseEventT::CM_BUTTON0;

                                    ME.Amount = 1;  // Button down.
                                    Gui->ProcessDeviceEvent(ME);

                                    ME.Amount = 0;  // Button up.
                                    Gui->ProcessDeviceEvent(ME);
                                }
                            }
                        }
                    }


#if 0
                    // Touching a GUI is processed both on the client and the server.
                    // Other items, e.g. a weapon that can be picked up are notified only on the server, though, not in prediction.
                    if (!ThinkingOnServerSide) continue;

                    // ...
                    // This is no longer relevant, as picking up weapons and items it now handled generically via trigger volumes below!
#endif
                }

                break;
            }

            case StateOfExistence_Dead:
            {
                const Vector3fT OldOrigin = m_Entity->GetTransform()->GetOriginWS();

                CompPlayerPhysics->SetMember("StepHeight", 4.0);
                CompPlayerPhysics->MoveHuman(PlayerCommands[PCNr].FrameTime, Vector3fT(), Vector3fT(), false);

                // We want to lower the view of the local client after it has been killed (in order to indicate the body collapse).
                // Unfortunately, the problem is much harder than just decreasing the 'm_Origin.z' in some way, because
                // a) other clients still need the original height for properly drawing the death sequence (from 3rd person view), and
                // b) the corpse that we create on leaving this StateOfExistence must have the proper height, too.
                // Therefore, we decrease the 'm_Origin.z', but "compensate" the 'm_Dimensions', such that the *absolute*
                // coordinates of our bounding box (obtained by "m_Origin plus m_Dimensions") remain constant.
                // This way, we can re-derive the proper height in both cases a) and b).
                const double Collapse=80.0*PlayerCommands[PCNr].FrameTime;

                if (m_Dimensions.Min.z + Collapse < -100.0)
                {
                    const Vector3dT O = m_Entity->GetTransform()->GetOriginWS().AsVectorOfDouble();

                    m_Entity->GetTransform()->SetOriginWS(Vector3dT(O.x, O.y, O.z - Collapse).AsVectorOfFloat());

                    m_Dimensions.Min.z+=Collapse;
                    m_Dimensions.Max.z+=Collapse;

                    cf::math::AnglesfT Angles(m_Entity->GetChildren()[0]->GetTransform()->GetQuatPS());     // We update the camera, not the entity.
                    Angles.roll() += PlayerCommands[PCNr].FrameTime * 200.0f;
                    m_Entity->GetChildren()[0]->GetTransform()->SetQuatPS(cf::math::QuaternionfT(Angles));
                }

                // We entered this state after we died.
                // Now leave it only after we have come to a complete halt, and the death sequence is over.
                if (OldOrigin.z >= m_Entity->GetTransform()->GetOriginWS().z && length(CompPlayerPhysics->GetVelocity()) < 0.1 /* && TODO: Is death anim sequence over?? */)
                {
                    if (ThinkingOnServerSide)
                    {
                        const Vector3fT Origin = m_Entity->GetTransform()->GetOriginWS();
                        IntrusivePtrT<cf::GameSys::ComponentModelT> PlayerModelComp = dynamic_pointer_cast<cf::GameSys::ComponentModelT>(m_Entity->GetComponent("Model"));

                        // Create a new "corpse" entity in the place where we died, or else the model disappears.
                        if (PlayerModelComp != NULL)
                        {
                            IntrusivePtrT<cf::GameSys::EntityT> Ent = new cf::GameSys::EntityT(cf::GameSys::EntityCreateParamsT(m_Entity->GetWorld()));
                            m_Entity->GetWorld().GetRootEntity()->AddChild(Ent);

                            Ent->GetTransform()->SetOriginWS(Origin);
                            Ent->GetTransform()->SetQuatWS(m_Entity->GetTransform()->GetQuatWS());

                            IntrusivePtrT<cf::GameSys::ComponentModelT> ModelComp = new cf::GameSys::ComponentModelT(*PlayerModelComp);
                            Ent->AddComponent(ModelComp);

                            // TODO: Disappear when some condition is met (timeout, not in anyones PVS, alpha fade-out, too many corpses, ...)
                            // TODO: Decompose to gibs when hit by a rocket.
                        }
                    }

                    // TODO: HEAD SWAY: State.Velocity.y=m_Heading;
                    // TODO: HEAD SWAY: State.Velocity.z=m_Bank;
                    m_Dimensions=BoundingBox3dT(Vector3dT(16.0, 16.0, 4.0), Vector3dT(-16.0, -16.0, -68.0));
                    CompHP->SetStateOfExistence(StateOfExistence_FrozenSpectator);
                }

                break;
            }

            case StateOfExistence_FrozenSpectator:
            {
#if 0   // TODO: HEAD SWAY
                const float Pi          =3.14159265359f;
                const float SecPerSwing =15.0f;
                float       PC_FrameTime=PlayerCommands[PCNr].FrameTime;

                // In this 'StateOfExistence' is the 'State.Velocity' unused - thus mis-use it for other purposes!
                if (PC_FrameTime>0.05) PC_FrameTime=0.05f;  // Avoid jumpiness with very low FPS.
                State.Velocity.x+=PC_FrameTime*2.0*Pi/SecPerSwing;
                if (State.Velocity.x>6.3) State.Velocity.x-=2.0*Pi;

                const float SwingAngle=float(sin(State.Velocity.x)*200.0);

                m_Heading=(unsigned short)(State.Velocity.y+SwingAngle);
                m_Bank   =(unsigned short)(State.Velocity.z-SwingAngle);
#endif

                // TODO: We want the player to release the button between respawns in order to avoid permanent "respawn-flickering"
                //       that otherwise may occur if the player keeps the button continuously pressed down.
                //       These are the same technics that also apply to the "jump"-button.
                if ((PlayerCommands[PCNr].Keys & PCK_Fire1)==0) break;  // "Fire" button not pressed.

                ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > AllEnts;
                m_Entity->GetWorld().GetRootEntity()->GetAll(AllEnts);

                // The "Fire"-button was pressed. Now try to determine a free "InfoPlayerStart" entity for respawning there.
                for (unsigned int EntNr = 0; EntNr < AllEnts.Size(); EntNr++)
                {
                    IntrusivePtrT<cf::GameSys::EntityT> IPSEntity = AllEnts[EntNr];

                    if (IPSEntity->GetComponent("PlayerStart") == NULL) continue;

                    // This is actually an "InfoPlayerStart" entity. Now try to put our own bounding box at the origin of 'IPSEntity',
                    // but try to correct/choose the height such that we are on ground (instead of hovering above it).
                    Vector3dT OurNewOrigin = IPSEntity->GetTransform()->GetOriginWS().AsVectorOfDouble();

                    // First, create a BB of dimensions (-300.0, -300.0, -100.0) - (300.0, 300.0, 100.0).
                    const BoundingBox3T<double> ClearingBB(VectorT(m_Dimensions.Min.x, m_Dimensions.Min.y, -m_Dimensions.Max.z), m_Dimensions.Max);

                    cf::ClipSys::ClipModelT* IgnorePlayerClipModel = NULL;  // TODO!

                    // Move ClearingBB up to a reasonable height (if possible!), such that the *full* BB (that is, m_Dimensions) is clear of (not stuck in) solid.
                    cf::ClipSys::TraceResultT Result(1.0);
                    m_Entity->GetWorld().GetClipWorld()->TraceBoundingBox(ClearingBB, OurNewOrigin, VectorT(0.0, 0.0, 120.0), MaterialT::Clip_Players, IgnorePlayerClipModel, Result);
                    const double AddHeight=120.0*Result.Fraction;

                    // Move ClearingBB down as far as possible.
                    Result=cf::ClipSys::TraceResultT(1.0);
                    m_Entity->GetWorld().GetClipWorld()->TraceBoundingBox(ClearingBB, OurNewOrigin+VectorT(0.0, 0.0, AddHeight), VectorT(0.0, 0.0, -1000.0), MaterialT::Clip_Players, IgnorePlayerClipModel, Result);
                    const double SubHeight=1000.0*Result.Fraction;

                    // Beachte: Hier für Epsilon 1.0 (statt z.B. 1.23456789) zu wählen hebt u.U. GENAU den (0 0 -1) Test in
                    // Physics::CategorizePosition() auf! Nicht schlimm, wenn aber auf Client-Seite übers Netz kleine Rundungsfehler
                    // vorhanden sind (es werden floats übertragen, nicht doubles!), kommt CategorizePosition() u.U. auf Client- und
                    // Server-Seite zu verschiedenen Ergebnissen! Der Effekt spielt sich zwar in einem Intervall der Größe 1.0 ab,
                    // kann mit OpenGL aber zu deutlichem Pixel-Flimmern führen!
                    OurNewOrigin.z=OurNewOrigin.z+AddHeight-SubHeight+(ClearingBB.Min.z-m_Dimensions.Min.z/*1628.8*/)+0.123456789/*Epsilon (sonst Ruckeln am Anfang!)*/;

                    // Old, deprecated code (can get us stuck in non-level ground).
                    // const double HeightAboveGround=GameWorld->MapClipLine(OurNewOrigin, VectorT(0, 0, -1.0), 0, 999999.9);
                    // OurNewOrigin.z=OurNewOrigin.z-HeightAboveGround-m_Dimensions.Min.z+1.23456789/*Epsilon (needed to avoid ruggy initial movement!)*/;


                    BoundingBox3T<double> OurBB(m_Dimensions);

                    OurBB.Min+=OurNewOrigin;
                    OurBB.Max+=OurNewOrigin;

                    ArrayT<cf::ClipSys::ClipModelT*> ClipModels;
                    m_Entity->GetWorld().GetClipWorld()->GetClipModelsFromBB(ClipModels, MaterialT::Clip_Players, OurBB);

                    if (ClipModels.Size() == 0)
                    {
                        // A suitable "InfoPlayerStart" entity was found -- respawn!
                        m_Entity->GetTransform()->SetOriginWS(OurNewOrigin.AsVectorOfFloat());
                        m_Entity->GetTransform()->SetQuatWS(IPSEntity->GetTransform()->GetQuatWS());  // TODO: Can we make sure that the z-axis points straight up, i.e. bank and pitch are 0?
                        m_Entity->GetChildren()[0]->GetTransform()->SetQuatPS(cf::math::QuaternionfT());
                        m_Dimensions             =BoundingBox3dT(Vector3dT(16.0, 16.0, 4.0), Vector3dT(-16.0, -16.0, -68.0));
                        CompHP->SetStateOfExistence(StateOfExistence_Alive);
                        Model3rdPerson->SetMember("Animation", 0);
                        CompHP->SetHealth(100);
                        CompHP->SetArmor(0);
                        CompHP->SetHaveItems(0);
                        CompHP->SetHaveWeapons(0);
                        CompHP->SetActiveWeaponSlot(0);
                        CompHP->SetActiveWeaponSequNr(0);
                        CompHP->SetActiveWeaponFrameNr(0.0f);

                        IntrusivePtrT<cf::GameSys::ComponentCollisionModelT> CompCollMdl = dynamic_pointer_cast<cf::GameSys::ComponentCollisionModelT>(m_Entity->GetComponent("CollisionModel"));

                        if (CompCollMdl != NULL)
                            CompCollMdl->SetBoundingBox(m_Dimensions, "Textures/meta/collisionmodel");

                        for (char Nr=0; Nr<15; Nr++) CompHP->GetHaveAmmo()[Nr]=0;   // IMPORTANT: Do not clear the frags value in 'HaveAmmo[AMMO_SLOT_FRAGS]'!
                        for (char Nr=0; Nr<32; Nr++) CompHP->GetHaveAmmoInWeapons()[Nr]=0;

                        break;
                    }
                }

                break;
            }

            case StateOfExistence_FreeSpectator:
                break;
        }
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

        const float OffsetZ = (CompHP->GetStateOfExistence() != StateOfExistence_Dead) ? -32.0f : -32.0f+float(m_Dimensions.Min.z+68.0);

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
