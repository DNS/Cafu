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

/***************************/
/*** Human Player (Code) ***/
/***************************/

#include "HumanPlayer.hpp"
#include "Corpse.hpp"
#include "InfoPlayerStart.hpp"
#include "StaticDetailModel.hpp"
#include "cw.hpp"
#include "Constants_AmmoSlots.hpp"
#include "Constants_WeaponSlots.hpp"
#include "EntityCreateParams.hpp"
#include "GameImpl.hpp"
#include "PhysicsWorld.hpp"
#include "Libs/LookupTables.hpp"
#include "Libs/Physics.hpp"

#include "SoundSystem/SoundSys.hpp"
#include "../../GameWorld.hpp"
#include "ClipSys/ClipWorld.hpp"
#include "ClipSys/CollisionModelMan.hpp"
#include "ClipSys/TraceResult.hpp"
#include "Fonts/Font.hpp"
#include "GuiSys/GuiMan.hpp"
#include "GuiSys/Gui.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "Models/AnimPose.hpp"
#include "Models/Model_cmdl.hpp"
#include "OpenGL/OpenGLWindow.hpp"
#include "ParticleEngine/ParticleEngineMS.hpp"
#include "TypeSys.hpp"

#include <cstdio>
#include <string.h>

#ifndef _WIN32
#define _stricmp strcasecmp
#endif


// Constants for State.StateOfExistance.
const char StateOfExistance_Alive          =0;
const char StateOfExistance_Dead           =1;
const char StateOfExistance_FrozenSpectator=2;
const char StateOfExistance_FreeSpectator  =3;

// Constants for State.Flags.
const char Flags_OldWishJump=1;


const char EntHumanPlayerT::EventID_PrimaryFire  =0;
const char EntHumanPlayerT::EventID_SecondaryFire=1;


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

const cf::TypeSys::TypeInfoT EntHumanPlayerT::TypeInfo(GetBaseEntTIM(), "EntHumanPlayerT", "BaseEntityT", EntHumanPlayerT::CreateInstance, NULL /*MethodsList*/);


EntHumanPlayerT::EntHumanPlayerT(const EntityCreateParamsT& Params)
    : BaseEntityT(Params,
                  EntityStateT(Params.Origin,
                               VectorT(),
                               BoundingBox3T<double>(VectorT( 400.0,  400.0,   100.0),  // Roughly 32*32*72 inches, eye height at 68 inches.
                                            VectorT(-400.0, -400.0, -1728.8)),  // 68*25.4 == 1727.2
                               0,       // Heading (set/overridden by the BaseEntityT constructor by evaluating the PropertyPairs ("angles" property))
                               0,
                               0,
                               StateOfExistance_FrozenSpectator,
                               0,
                               0,       // ModelIndex
                               0,       // ModelSequNr
                               0.0,     // ModelFrameNr
                               100,     // Health
                               0,       // Armor
                               0,       // HaveItems
                               0,       // HaveWeapons
                               0,       // ActiveWeaponSlot
                               0,       // ActiveWeaponSequNr
                               0.0)),   // ActiveWeaponFrameNr
      m_CollisionShape(NULL),
      m_RigidBody(NULL),
      TimeForLightSource(0.0),
      GuiHUD(NULL)
{
    // We can only hope that 'Origin' is a nice place for a "Frozen Spectator"...

    // Because 'StateOfExistance==StateOfExistance_FrozenSpectator', we mis-use the 'Velocity' member variable a little
    // for slightly turning/swaying the head in this state. See the 'Think()' code below!
    State.Velocity.y=State.Heading;
    State.Velocity.z=State.Bank;


    // This would be the proper way to do it, but we don't know here whether this is the local human player of a client.
    // (Could also be the entity of another (remote) client, or a server-side entity.)
    // Note that if (is local human player) is true, cf::GuiSys::GuiMan!=NULL is implied to be true, too.
    // GuiHUD=(is local human player) ? cf::GuiSys::GuiMan->Find("Games/DeathMatch/GUIs/HUD.cgui", true) : NULL;


    assert(CollisionModel==NULL);
    // No "normal" collision model has been set for this entity.
    // Now simply setup a bounding box as the collision model.
    CollisionModel=cf::ClipSys::CollModelMan->GetCM(State.Dimensions, MaterialManager->GetMaterial("Textures/meta/collisionmodel"));
    ClipModel.SetCollisionModel(CollisionModel);
    ClipModel.SetOrigin(State.Origin);
 // ClipModel.Register();   // Do *not* register here! Registering/unregistering is done when there is a transition in State.StateOfExistance.


    // The /1000 is because our physics world is in meters.
    m_CollisionShape=new btBoxShape(conv((State.Dimensions.Max-State.Dimensions.Min)/2.0/1000.0));  // Should use a btCylinderShapeZ instead of btBoxShape?

    // Our rigid body is of Bullet type "kinematic". That is, we move it ourselves, not the world dynamics.
    m_RigidBody=new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(0, this /*btMotionState for this body*/, m_CollisionShape, btVector3()));

    m_RigidBody->setUserPointer(this);  // This entity is associated to the m_RigidBody.
    m_RigidBody->setCollisionFlags(m_RigidBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    m_RigidBody->setActivationState(DISABLE_DEACTIVATION);

    // PhysicsWorld->AddRigidBody(m_RigidBody);     // Don't add to the world here - adding/removing is done when State.StateOfExistance changes.
}


EntHumanPlayerT::~EntHumanPlayerT()
{
    if (m_RigidBody->isInWorld())
        PhysicsWorld->RemoveRigidBody(m_RigidBody);

    delete m_RigidBody;
    delete m_CollisionShape;

    cf::GuiSys::GuiMan->Free(GuiHUD);
}


void EntHumanPlayerT::TakeDamage(BaseEntityT* Entity, char Amount, const VectorT& ImpactDir)
{
    // Only human players that are still alive can take damage.
    if (State.StateOfExistance!=StateOfExistance_Alive) return;

    State.Velocity=State.Velocity+scale(VectorT(ImpactDir.x, ImpactDir.y, 0.0), 500.0*Amount);

    if (State.Health<=Amount)
    {
        unsigned short DeltaAngle=Entity->State.Heading-State.Heading;

        State.StateOfExistance=StateOfExistance_Dead;
        State.Health=0;

        ClipModel.Unregister();

             if (DeltaAngle>=57344 || DeltaAngle< 8192) State.ModelSequNr=21;   // 315° ...  45° - die forwards
        else if (DeltaAngle>=8192  && DeltaAngle<16384) State.ModelSequNr=22;   //  45° ...  90° - headshot
        else if (DeltaAngle>=16384 && DeltaAngle<24576) State.ModelSequNr=24;   //  90° ... 135° - gutshot
        else if (DeltaAngle>=24576 && DeltaAngle<32768) State.ModelSequNr=19;   // 135° ... 180° - die backwards1
        else if (DeltaAngle>=32768 && DeltaAngle<40960) State.ModelSequNr=20;   // 180° ... 225° - die backwards
        else if (DeltaAngle>=40960 && DeltaAngle<49152) State.ModelSequNr=18;   // 225° ... 270° - die simple
        else /* (DeltaAngle>=49152&&DeltaAngle<57344)*/ State.ModelSequNr=23;   // 270° ... 315° - die spin

        State.ModelFrameNr=0.0;

        // GameWorld->BroadcastText("%s was killed by %s", State.PlayerName, Entity->State.PlayerName);


        // Count the frag at the "creator" entity.
        BaseEntityT* FraggingEntity=Entity;

        while (FraggingEntity->ParentID!=0xFFFFFFFF)
        {
            BaseEntityT* ParentOfFE=GameWorld->GetBaseEntityByID(FraggingEntity->ParentID);

            if (ParentOfFE==NULL) break;
            FraggingEntity=ParentOfFE;
        }

        if (FraggingEntity->ID==ID)
        {
            // Uh! This was a self-kill!
            FraggingEntity->State.HaveAmmo[AMMO_SLOT_FRAGS]--;
        }
        else
        {
            FraggingEntity->State.HaveAmmo[AMMO_SLOT_FRAGS]++;
        }
    }
    else
    {
        State.Health-=Amount;
    }
}


void EntHumanPlayerT::ProcessConfigString(const void* ConfigData, const char* ConfigString)
{
         if (strcmp(ConfigString, "PlayerCommand")==0) PlayerCommands.PushBack(*((PlayerCommandT*)ConfigData));
    else if (strcmp(ConfigString, "IsAlive?"     )==0) *((bool*)ConfigData)=(State.StateOfExistance==StateOfExistance_Alive);
    else if (strcmp(ConfigString, "PlayerName"   )==0) strncpy(State.PlayerName, (const char*)ConfigData, sizeof(State.PlayerName));
    else if (strcmp(ConfigString, "ModelName"    )==0)
    {
        // This if course must match the ModelIndexToModel[] array above.
             if (_stricmp((const char*)ConfigData, "Alien"   )==0) State.ModelIndex=0;
        else if (_stricmp((const char*)ConfigData, "James"   )==0) State.ModelIndex=1;
        else if (_stricmp((const char*)ConfigData, "Punisher")==0) State.ModelIndex=2;
        else if (_stricmp((const char*)ConfigData, "Sentinel")==0) State.ModelIndex=3;
        else if (_stricmp((const char*)ConfigData, "Skeleton")==0) State.ModelIndex=4;
        else if (_stricmp((const char*)ConfigData, "T801"    )==0) State.ModelIndex=5;
        else                                                       State.ModelIndex=6;
    }

    // player name
    // model name
    // noclip
}


bool EntHumanPlayerT::CheckGUI(EntStaticDetailModelT* GuiEnt, Vector3fT& MousePos) const
{
    // 1. Has this entitiy an interactive GUI at all?
    cf::GuiSys::GuiI* Gui=GuiEnt->GetGUI();

    if (Gui==NULL) return false;
    if (!Gui->GetIsInteractive()) return false;


    // 2. Can we retrieve the plane of its screen panel?
    Vector3fT GuiOrigin;
    Vector3fT GuiAxisX;
    Vector3fT GuiAxisY;

    if (!GuiEnt->GetGuiPlane(0, GuiOrigin, GuiAxisX, GuiAxisY) && !GuiEnt->GetGuiPlane(GuiOrigin, GuiAxisX, GuiAxisY)) return false;


    // 3. Are we looking roughly into the screen normal?
    const Vector3fT GuiNormal=normalize(cross(GuiAxisY, GuiAxisX), 0.0f);
    const Plane3fT  GuiPlane =Plane3fT(GuiNormal, dot(GuiOrigin, GuiNormal));

    const float     ViewDirZ =-LookupTables::Angle16ToSin[State.Pitch];
    const float     ViewDirY = LookupTables::Angle16ToCos[State.Pitch];
    const Vector3fT ViewDir  =Vector3fT(ViewDirY*LookupTables::Angle16ToSin[State.Heading], ViewDirY*LookupTables::Angle16ToCos[State.Heading], ViewDirZ);

    if (-dot(ViewDir, GuiPlane.Normal)<0.001f) return false;


    // 4. Does our view ray hit the screen panel?
    // (I've obtained the equation for r by rolling the corresponding Plane3T<T>::GetIntersection() method out.)
    const Vector3fT OurOrigin=State.Origin.AsVectorOfFloat();
    const float     r        =(GuiPlane.Dist-dot(GuiPlane.Normal, OurOrigin))/dot(GuiPlane.Normal, ViewDir);

    if (r<0.0f || r>4000.0f) return false;

    const Vector3fT HitPos=OurOrigin+ViewDir*r;

    // Project HitPos into the GUI plane, in order to obtain the 2D coordinate in the GuiSys' virtual pixel space (640x480).
    const float px=dot(HitPos-GuiOrigin, GuiAxisX)/GuiAxisX.GetLengthSqr();
    const float py=dot(HitPos-GuiOrigin, GuiAxisY)/GuiAxisY.GetLengthSqr();

    if (px<0.0f || px>1.0f) return false;
    if (py<0.0f || py>1.0f) return false;



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

    const bool ThinkingOnServerSide=FrameTime_BAD_DONT_USE>=0.0;

    for (unsigned long PCNr=0; PCNr<PlayerCommands.Size(); PCNr++)
    {
        switch (State.StateOfExistance)
        {
            case StateOfExistance_Alive:
            {
                if (!m_RigidBody->isInWorld())
                    PhysicsWorld->AddRigidBody(m_RigidBody);

                // Update Heading
                State.Heading+=PlayerCommands[PCNr].DeltaHeading;

                if (PlayerCommands[PCNr].Keys & PCK_TurnLeft ) State.Heading-=(unsigned short)(21845.0*PlayerCommands[PCNr].FrameTime);
                if (PlayerCommands[PCNr].Keys & PCK_TurnRight) State.Heading+=(unsigned short)(21845.0*PlayerCommands[PCNr].FrameTime);

                // Update Pitch
                int OldPitch=State.Pitch;                     if (OldPitch>=32768) OldPitch-=65536;
                int DltPitch=PlayerCommands[PCNr].DeltaPitch; if (DltPitch>=32768) DltPitch-=65536;
                int NewPitch=OldPitch+DltPitch;

                if (PlayerCommands[PCNr].Keys & PCK_LookUp  ) NewPitch-=int(21845.0*PlayerCommands[PCNr].FrameTime);
                if (PlayerCommands[PCNr].Keys & PCK_LookDown) NewPitch+=int(21845.0*PlayerCommands[PCNr].FrameTime);

                if (NewPitch> 16384) NewPitch= 16384;
                if (NewPitch<-16384) NewPitch=-16384;

                if (NewPitch<0) NewPitch+=65536;
                State.Pitch=NewPitch;

                // Update Bank
                State.Bank+=PlayerCommands[PCNr].DeltaBank;


                VectorT             WishVelocity;
                bool                WishJump=false;
                const double        VelX    =6000.0*LookupTables::Angle16ToSin[State.Heading];     // 6000 == Client.MoveSpeed
                const double        VelY    =6000.0*LookupTables::Angle16ToCos[State.Heading];     // 6000 == Client.MoveSpeed
                const unsigned long Keys    =PlayerCommands[PCNr].Keys;

                if (Keys & PCK_MoveForward ) WishVelocity=             VectorT( VelX,  VelY, 0);
                if (Keys & PCK_MoveBackward) WishVelocity=WishVelocity+VectorT(-VelX, -VelY, 0);
                if (Keys & PCK_StrafeLeft  ) WishVelocity=WishVelocity+VectorT(-VelY,  VelX, 0);
                if (Keys & PCK_StrafeRight ) WishVelocity=WishVelocity+VectorT( VelY, -VelX, 0);

                if (Keys & PCK_CenterView  ) { State.Pitch=0; State.Bank=0; }
                if (Keys & PCK_Jump        ) WishJump=true;
             // if (Keys & PCK_Duck        ) ;
                if (Keys & PCK_Walk        ) WishVelocity=scale(WishVelocity, 0.5);

                VectorT       WishVelLadder;
                const double  ViewLadderZ=-LookupTables::Angle16ToSin[State.Pitch];
                const double  ViewLadderY= LookupTables::Angle16ToCos[State.Pitch];
                const VectorT ViewLadder =scale(VectorT(ViewLadderY*LookupTables::Angle16ToSin[State.Heading], ViewLadderY*LookupTables::Angle16ToCos[State.Heading], ViewLadderZ), 3800.0);

                // TODO: Also take LATERAL movement into account.
                // TODO: All this needs a HUGE clean-up! Can probably put a lot of this stuff into Physics::MoveHuman.
                if (Keys & PCK_MoveForward ) WishVelLadder=WishVelLadder+ViewLadder;
                if (Keys & PCK_MoveBackward) WishVelLadder=WishVelLadder-ViewLadder;
                if (Keys & PCK_Walk        ) WishVelLadder=scale(WishVelLadder, 0.5);

                /*if (Clients[ClientNr].move_noclip)
                {
                    // This code was simply changed and rewritten until it "worked".
                    // May still be buggy anyway.
                    double RadPitch=double(State.Pitch)/32768.0*3.141592654;
                    double Fak     =VectorDot(WishVelocity, VectorT(LookupTables::Angle16ToSin[State.Heading], LookupTables::Angle16ToCos[State.Heading], 0));

                    WishVelocity.x*=cos(RadPitch);
                    WishVelocity.y*=cos(RadPitch);
                    WishVelocity.z=-sin(RadPitch)*Fak;

                    State.Origin=State.Origin+scale(WishVelocity, PlayerCommands[PCNr].FrameTime);

                    // TODO: If not already done on state change (--> "noclip"), set the model sequence to "swim".  ;-)
                }
                else */
                {
                    VectorT XYVel      =State.Velocity; XYVel.z=0;
                    double  OldSpeed   =length(XYVel);
                    bool    OldWishJump=(State.Flags & Flags_OldWishJump) ? true : false;

                    Physics::MoveHuman(State, ClipModel, PlayerCommands[PCNr].FrameTime, WishVelocity, WishVelLadder, WishJump, OldWishJump, 470.0, GameWorld->GetClipWorld());

                    ClipModel.SetOrigin(State.Origin);
                    ClipModel.Register();
                    // The physics world will pick-up our new origin at the next opportunity from getWorldTransform().

                    if (OldWishJump) State.Flags|= Flags_OldWishJump;
                                else State.Flags&=~Flags_OldWishJump;

                    XYVel=State.Velocity;
                    XYVel.z=0;
                    double NewSpeed=length(XYVel);

                    if (OldSpeed<=1000.0 && NewSpeed>1000.0) State.ModelSequNr=3;
                    if (OldSpeed>=1000.0 && NewSpeed<1000.0) State.ModelSequNr=1;
                }

                // GameWorld->ModelAdvanceFrameTime() is called on client side in Draw().


                // Handle the state machine of the "_v" (view) model of the current weapon.
                if (State.HaveWeapons & (1 << State.ActiveWeaponSlot))
                {
                    const CarriedWeaponT* CarriedWeapon=cf::GameSys::GameImplT::GetInstance().GetCarriedWeapon(State.ActiveWeaponSlot);

                    // Advance the frame time of the weapon.
                    const CafuModelT* WeaponModel=CarriedWeapon->GetViewWeaponModel();

                    AnimPoseT* Pose=WeaponModel->GetSharedPose(State.ActiveWeaponSequNr, State.ActiveWeaponFrameNr);
                    Pose->Advance(PlayerCommands[PCNr].FrameTime, true);

                    const float NewFrameNr=Pose->GetFrameNr();
                    const bool  AnimSequenceWrap=NewFrameNr < State.ActiveWeaponFrameNr || NewFrameNr > WeaponModel->GetAnims()[State.ActiveWeaponSequNr].Frames.Size()-1;

                    State.ActiveWeaponFrameNr=NewFrameNr;

                    CarriedWeapon->ServerSide_Think(this, PlayerCommands[PCNr], ThinkingOnServerSide, ServerFrameNr, AnimSequenceWrap);
                }


                // Check if any key for changing the current weapon was pressed.
                ArrayT<char> SelectableWeapons;

                switch (Keys >> 28)
                {
                    case 0: break;  // No weapon slot was selected for changing the weapon.

                    case 1: if (State.HaveWeapons & (1 << WEAPON_SLOT_BATTLESCYTHE)) SelectableWeapons.PushBack(WEAPON_SLOT_BATTLESCYTHE);
                            if (State.HaveWeapons & (1 << WEAPON_SLOT_HORNETGUN   )) SelectableWeapons.PushBack(WEAPON_SLOT_HORNETGUN   );
                            break;

                    case 2: if ((State.HaveWeapons & (1 << WEAPON_SLOT_PISTOL)) && (State.HaveAmmoInWeapons[WEAPON_SLOT_PISTOL] || State.HaveAmmo[AMMO_SLOT_9MM])) SelectableWeapons.PushBack(WEAPON_SLOT_PISTOL);
                            if ((State.HaveWeapons & (1 << WEAPON_SLOT_357   )) && (State.HaveAmmoInWeapons[WEAPON_SLOT_357   ] || State.HaveAmmo[AMMO_SLOT_357])) SelectableWeapons.PushBack(WEAPON_SLOT_357   );
                            break;

                    case 3: if ((State.HaveWeapons & (1 << WEAPON_SLOT_SHOTGUN )) && (State.HaveAmmoInWeapons[WEAPON_SLOT_SHOTGUN ] || State.HaveAmmo[AMMO_SLOT_SHELLS]                                    )) SelectableWeapons.PushBack(WEAPON_SLOT_SHOTGUN );
                            if ((State.HaveWeapons & (1 << WEAPON_SLOT_9MMAR   )) && (State.HaveAmmoInWeapons[WEAPON_SLOT_9MMAR   ] || State.HaveAmmo[AMMO_SLOT_9MM   ] || State.HaveAmmo[AMMO_SLOT_ARGREN])) SelectableWeapons.PushBack(WEAPON_SLOT_9MMAR   );
                            if ((State.HaveWeapons & (1 << WEAPON_SLOT_CROSSBOW)) && (State.HaveAmmoInWeapons[WEAPON_SLOT_CROSSBOW] || State.HaveAmmo[AMMO_SLOT_ARROWS]                                    )) SelectableWeapons.PushBack(WEAPON_SLOT_CROSSBOW);
                            break;

                    case 4: if ((State.HaveWeapons & (1 << WEAPON_SLOT_RPG  )) && (State.HaveAmmoInWeapons[WEAPON_SLOT_RPG  ] || State.HaveAmmo[AMMO_SLOT_ROCKETS])) SelectableWeapons.PushBack(WEAPON_SLOT_RPG  );
                            if ((State.HaveWeapons & (1 << WEAPON_SLOT_GAUSS)) && (State.HaveAmmoInWeapons[WEAPON_SLOT_GAUSS] || State.HaveAmmo[AMMO_SLOT_CELLS  ])) SelectableWeapons.PushBack(WEAPON_SLOT_GAUSS);
                            if ((State.HaveWeapons & (1 << WEAPON_SLOT_EGON )) && (State.HaveAmmoInWeapons[WEAPON_SLOT_EGON ] || State.HaveAmmo[AMMO_SLOT_CELLS  ])) SelectableWeapons.PushBack(WEAPON_SLOT_EGON );
                            break;

                    case 5: if ((State.HaveWeapons & (1 << WEAPON_SLOT_GRENADE   )) && State.HaveAmmoInWeapons[WEAPON_SLOT_GRENADE   ]) SelectableWeapons.PushBack(WEAPON_SLOT_GRENADE   );
                            if ((State.HaveWeapons & (1 << WEAPON_SLOT_TRIPMINE  )) && State.HaveAmmoInWeapons[WEAPON_SLOT_TRIPMINE  ]) SelectableWeapons.PushBack(WEAPON_SLOT_TRIPMINE  );
                            if ((State.HaveWeapons & (1 << WEAPON_SLOT_FACEHUGGER)) && State.HaveAmmoInWeapons[WEAPON_SLOT_FACEHUGGER]) SelectableWeapons.PushBack(WEAPON_SLOT_FACEHUGGER);
                            break;

                    // case 6..15: break;
                }

                unsigned long SWNr;

                for (SWNr=0; SWNr<SelectableWeapons.Size(); SWNr++)
                    if (SelectableWeapons[SWNr]==State.ActiveWeaponSlot) break;

                if (SWNr>=SelectableWeapons.Size())
                {
                    // If the currently active weapon is NOT among the SelectableWeapons
                    // (that means another weapon category was chosen), choose SelectableWeapons[0] as the active weapon.
                    if (SelectableWeapons.Size()>0)
                    {
                        char DrawSequNr=0;

                        State.ActiveWeaponSlot=SelectableWeapons[0];

                        switch (State.ActiveWeaponSlot)
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

                        State.ActiveWeaponSequNr =DrawSequNr;
                        State.ActiveWeaponFrameNr=0.0;
                    }
                }
                else
                {
                    // Otherwise check if there are further selectable weapons (SelectableWeapons.Size()>1), and cycle to the next one.
                    if (SelectableWeapons.Size()>1)
                    {
                        char DrawSequNr=0;

                        State.ActiveWeaponSlot=SelectableWeapons[(SWNr+1) % SelectableWeapons.Size()];

                        switch (State.ActiveWeaponSlot)
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

                        State.ActiveWeaponSequNr =DrawSequNr;
                        State.ActiveWeaponFrameNr=0.0;
                    }
                }


                // Check if we touched another entity.
                const ArrayT<unsigned long>& AllEntityIDs=GameWorld->GetAllEntityIDs();

                for (unsigned long EntityIDNr=0; EntityIDNr<AllEntityIDs.Size(); EntityIDNr++)
                {
                    BaseEntityT* OtherEntity=GameWorld->GetBaseEntityByID(AllEntityIDs[EntityIDNr]);

                    if (OtherEntity    ==NULL) continue;
                    if (OtherEntity->ID==  ID) continue;    // We don't touch us ourselves.

                    // Test if maybe we're near a static detail model with an interactive GUI.
                    if (OtherEntity->GetType()==&EntStaticDetailModelT::TypeInfo)
                    {
                        // TODO 1: Also deal with the GUI when this is a REPREDICTION run???
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
                        EntStaticDetailModelT* GuiEnt=(EntStaticDetailModelT*)OtherEntity;

                        Vector3fT MousePos;
                        if (CheckGUI(GuiEnt, MousePos))
                        {
                            cf::GuiSys::GuiI* Gui=GuiEnt->GetGUI();
                            CaMouseEventT     ME;

                            Gui->SetMousePos(MousePos.x, MousePos.y);

                            ME.Type  =CaMouseEventT::CM_MOVE_X;
                            ME.Amount=0;

                            Gui->ProcessDeviceEvent(ME);

                            if ((Keys >> 28)==10 /*TODO 2(?): && ThinkingOnServerSide*/)
                            {
                                // TODO 3:   When a GUI script calls an entity script (or modifies entity variables),
                                // TODO 3:   we somehow must make sure that that only happens on the server-side,
                                // TODO 3:    but never on the client-side (i.e. during prediction or reprediction)!!
                                // TODO 3:   (E.g. the glue code that connects entity and gui scripts could do that.)
                                // Note: The TODOs "TODO 1", "TODO 2" and "TODO 3" belong together...!
                                ME.Type=CaMouseEventT::CM_BUTTON0;

                                ME.Amount=1;    // Button down.
                                Gui->ProcessDeviceEvent(ME);

                                ME.Amount=0;    // Button up.
                                Gui->ProcessDeviceEvent(ME);
                            }
                        }
                    }

                    // If the bounding boxes don't overlap, continue with the next entity (we did not touch this one).
                    BoundingBox3T<double> OtherEntityBB=OtherEntity->State.Dimensions;

                    OtherEntityBB.Min=OtherEntityBB.Min+OtherEntity->State.Origin-State.Origin;
                    OtherEntityBB.Max=OtherEntityBB.Max+OtherEntity->State.Origin-State.Origin;

                    if (!State.Dimensions.Intersects(OtherEntityBB)) continue;

                    // The bounding boxes overlap, so notify the 'OtherEntity' that we touched it.
                    OtherEntity->NotifyTouchedBy(this);
                }


                // See if we walked into the trigger volume of any entity.
                if (ThinkingOnServerSide)
                {
                    ArrayT<cf::ClipSys::ClipModelT*> ClipModels;
                    BoundingBox3dT                   AbsBB(State.Dimensions);

                    AbsBB.Min+=State.Origin;
                    AbsBB.Max+=State.Origin;

                    GameWorld->GetClipWorld().GetClipModelsFromBB(ClipModels, MaterialT::Clip_Trigger, AbsBB);
                    // printf("%lu clip models in AbsBB.\n", ClipModels.Size());

                    for (unsigned long ClipModelNr=0; ClipModelNr<ClipModels.Size(); ClipModelNr++)
                    {
                        const unsigned long Contents=ClipModels[ClipModelNr]->GetContents(State.Origin, 0, MaterialT::Clip_Trigger);
                        if ((Contents & MaterialT::Clip_Trigger)==0) continue;

                        BaseEntityT* TriggerEntity=static_cast<BaseEntityT*>(ClipModels[ClipModelNr]->GetUserData());
                        if (TriggerEntity==NULL) continue;

                        // TODO: if (another clip model already triggered TriggerEntity) continue;
                        //       *or* pass ClipModels[ClipModelNr] as another parameter to TriggerEntity->OnTrigger().

                        // Ok, we're inside this trigger.
                        // static unsigned long TriggCount=0;
                        // printf("You're inside the trigger. %lu, %i\n", TriggCount++, ThinkingOnServerSide);

                        // Don't call   ScriptState->CallEntityMethod(TriggerEntity, "OnTrigger", "E", this);
                        // directly, but rather leave that to   TriggerEntity->OnTrigger(this);   so that the C++
                        // code has a chance to override.
                        TriggerEntity->OnTrigger(this);
                    }
                }


                // Advance frame time of model sequence.
                const CafuModelT* PlayerModel=cf::GameSys::GameImplT::GetInstance().GetPlayerModel(State.ModelIndex);
                AnimPoseT*        Pose       =PlayerModel->GetSharedPose(State.ModelSequNr, State.ModelFrameNr);

                Pose->Advance(PlayerCommands[PCNr].FrameTime, true);
                State.ModelFrameNr=Pose->GetFrameNr();
                break;
            }

            case StateOfExistance_Dead:
            {
                bool         DummyOldWishJump=false;
                const double OldOriginZ      =State.Origin.z;
                const float  OldModelFrameNr =State.ModelFrameNr;

                if (m_RigidBody->isInWorld())
                    PhysicsWorld->RemoveRigidBody(m_RigidBody);

                Physics::MoveHuman(State, ClipModel, PlayerCommands[PCNr].FrameTime, VectorT(), VectorT(), false, DummyOldWishJump, 0.0, GameWorld->GetClipWorld());

                // We want to lower the view of the local client after it has been killed (in order to indicate the body collapse).
                // Unfortunately, the problem is much harder than just decreasing the 'State.Origin.z' in some way, because
                // a) other clients still need the original height for properly drawing the death sequence (from 3rd person view), and
                // b) the corpse that we create on leaving this StateOfExistance must have the proper height, too.
                // Therefore, we decrease the 'State.Origin.z', but "compensate" the 'State.Dimensions', such that the *absolute*
                // coordinates of our bounding box (obtained by "State.Origin plus State.Dimensions") remain constant.
                // This way, we can re-derive the proper height in both cases a) and b).
                const double Collapse=2000.0*PlayerCommands[PCNr].FrameTime;

                if (State.Dimensions.Min.z+Collapse<-100.0)
                {
                    State.Origin.z        -=Collapse;
                    State.Dimensions.Min.z+=Collapse;
                    State.Dimensions.Max.z+=Collapse;
                    State.Bank            +=(unsigned short)(PlayerCommands[PCNr].FrameTime*36000.0);
                }

                // Advance frame time of model sequence.
                const CafuModelT* PlayerModel=cf::GameSys::GameImplT::GetInstance().GetPlayerModel(State.ModelIndex);
                AnimPoseT*        Pose       =PlayerModel->GetSharedPose(State.ModelSequNr, State.ModelFrameNr);

                Pose->Advance(PlayerCommands[PCNr].FrameTime, false);
                State.ModelFrameNr=Pose->GetFrameNr();

                // We entered this state after we died.
                // Now leave it only after we have come to a complete halt, and the death sequence is over.
                if (OldOriginZ>=State.Origin.z && fabs(State.Velocity.x)<0.1 && fabs(State.Velocity.y)<0.1 && fabs(State.Velocity.z)<0.1 && OldModelFrameNr==State.ModelFrameNr)
                {
                    if (ThinkingOnServerSide)
                    {
                        std::map<std::string, std::string> Props; Props["classname"]="corpse";

                        // Create a new "corpse" entity in the place where we died, or else the model disappears.
                        unsigned long CorpseID=GameWorld->CreateNewEntity(Props, ServerFrameNr, VectorT());

                        if (CorpseID!=0xFFFFFFFF)
                        {
                            BaseEntityT* Corpse=GameWorld->GetBaseEntityByID(CorpseID);

                            Corpse->State=EntityStateT(State.Origin+VectorT(0.0, 0.0, State.Dimensions.Min.z+1728.8), VectorT(), BoundingBox3T<double>(Vector3dT()), State.Heading,
                                                       0, 0, 0, 0, State.ModelIndex, State.ModelSequNr, State.ModelFrameNr, 0, 0, 0, 0,
                                                       State.ActiveWeaponSlot, 0, 0.0);
                        }
                    }

                    State.Velocity.y=State.Heading;
                    State.Velocity.z=State.Bank;
                    State.Dimensions=BoundingBox3T<double>(VectorT(400.0, 400.0, 100.0), VectorT(-400.0, -400.0, -1728.8));
                    State.StateOfExistance=StateOfExistance_FrozenSpectator;
                }

                break;
            }

            case StateOfExistance_FrozenSpectator:
            {
                const float Pi          =3.14159265359f;
                const float SecPerSwing =15.0f;
                float       PC_FrameTime=PlayerCommands[PCNr].FrameTime;

                if (m_RigidBody->isInWorld())
                    PhysicsWorld->RemoveRigidBody(m_RigidBody);

                // In this 'StateOfExistance' is the 'State.Velocity' unused - thus mis-use it for other purposes!
                if (PC_FrameTime>0.05) PC_FrameTime=0.05f;  // Avoid jumpiness with very low FPS.
                State.Velocity.x+=PC_FrameTime*2.0*Pi/SecPerSwing;
                if (State.Velocity.x>6.3) State.Velocity.x-=2.0*Pi;

                const float SwingAngle=float(sin(State.Velocity.x)*200.0);

                State.Heading=(unsigned short)(State.Velocity.y+SwingAngle);
                State.Bank   =(unsigned short)(State.Velocity.z-SwingAngle);

                // TODO: We want the player to release the button between respawns in order to avoid permanent "respawn-flickering"
                //       that otherwise may occur if the player keeps the button continuously pressed down.
                //       These are the same technics that also apply to the "jump"-button.
                if ((PlayerCommands[PCNr].Keys & PCK_Fire1)==0) break;  // "Fire" button not pressed.

                const ArrayT<unsigned long>& AllEntityIDs=GameWorld->GetAllEntityIDs();
                BaseEntityT*                 IPSEntity   =NULL;
                VectorT                      OurNewOrigin;
                unsigned long                EntityIDNr;

                // The "Fire"-button was pressed. Now try to determine a free "InfoPlayerStart" entity for respawning there.
                for (EntityIDNr=0; EntityIDNr<AllEntityIDs.Size(); EntityIDNr++)
                {
                    IPSEntity=GameWorld->GetBaseEntityByID(AllEntityIDs[EntityIDNr]);
                    if (IPSEntity==NULL) continue;

                    if (IPSEntity->GetType()!=&EntInfoPlayerStartT::TypeInfo) continue;

                    // This is actually an "InfoPlayerStart" entity. Now try to put our own bounding box at the origin of 'IPSEntity',
                    // but try to correct/choose the height such that we are on ground (instead of hovering above it).
                    OurNewOrigin=IPSEntity->State.Origin;

                    // First, create a BB of dimensions (-300.0, -300.0, -100.0) - (300.0, 300.0, 100.0).
                    const BoundingBox3T<double> ClearingBB(VectorT(State.Dimensions.Min.x, State.Dimensions.Min.y, -State.Dimensions.Max.z), State.Dimensions.Max);

                    // Move ClearingBB up to a reasonable height (if possible!), such that the *full* BB (that is, State.Dimensions) is clear of (not stuck in) solid.
                    cf::ClipSys::TraceResultT Result(1.0);
                    GameWorld->GetClipWorld().TraceBoundingBox(ClearingBB, OurNewOrigin, VectorT(0.0, 0.0, 3000.0), MaterialT::Clip_Players, &ClipModel, Result);
                    const double AddHeight=3000.0*Result.Fraction;

                    // Move ClearingBB down as far as possible.
                    Result=cf::ClipSys::TraceResultT(1.0);
                    GameWorld->GetClipWorld().TraceBoundingBox(ClearingBB, OurNewOrigin+VectorT(0.0, 0.0, AddHeight), VectorT(0.0, 0.0, -999999.0), MaterialT::Clip_Players, &ClipModel, Result);
                    const double SubHeight=999999.0*Result.Fraction;

                    // Beachte: Hier für Epsilon 1.0 (statt z.B. 1.23456789) zu wählen hebt u.U. GENAU den (0 0 -1) Test in
                    // Physics::CategorizePosition() auf! Nicht schlimm, wenn aber auf Client-Seite übers Netz kleine Rundungsfehler
                    // vorhanden sind (es werden floats übertragen, nicht doubles!), kommt CategorizePosition() u.U. auf Client- und
                    // Server-Seite zu verschiedenen Ergebnissen! Der Effekt spielt sich zwar in einem Intervall der Größe 1.0 ab,
                    // kann mit OpenGL aber zu deutlichem Pixel-Flimmern führen!
                    OurNewOrigin.z=OurNewOrigin.z+AddHeight-SubHeight+(ClearingBB.Min.z-State.Dimensions.Min.z/*1628.8*/)+1.23456789/*Epsilon (sonst Ruckeln am Anfang!)*/;

                    // Old, deprecated code (can get us stuck in non-level ground).
                    // const double HeightAboveGround=GameWorld->MapClipLine(OurNewOrigin, VectorT(0, 0, -1.0), 0, 999999.9);
                    // OurNewOrigin.z=OurNewOrigin.z-HeightAboveGround-State.Dimensions.Min.z+1.23456789/*Epsilon (needed to avoid ruggy initial movement!)*/;


                    BoundingBox3T<double> OurBB(State.Dimensions);

                    OurBB.Min+=OurNewOrigin;
                    OurBB.Max+=OurNewOrigin;

                    ArrayT<cf::ClipSys::ClipModelT*> ClipModels;
                    GameWorld->GetClipWorld().GetClipModelsFromBB(ClipModels, MaterialT::Clip_Players, OurBB);

                    if (ClipModels.Size()==0) break;
                }

                if (EntityIDNr>=AllEntityIDs.Size()) break;     // No suitable "InfoPlayerStart" entity found!

                // Respawn!
                State.Origin             =OurNewOrigin;
                State.Velocity           =VectorT();
                State.Dimensions         =BoundingBox3T<double>(VectorT(400.0, 400.0, 100.0), VectorT(-400.0, -400.0, -1728.8));
                State.Heading            =IPSEntity->State.Heading;
                State.Pitch              =0;
                State.Bank               =0;
                State.StateOfExistance   =StateOfExistance_Alive;
                State.ModelSequNr        =0;
                State.ModelFrameNr       =0.0;
                State.Health             =100;
                State.Armor              =0;
                State.HaveItems          =0;
                State.HaveWeapons        =0;
                State.ActiveWeaponSlot   =0;
                State.ActiveWeaponSequNr =0;
                State.ActiveWeaponFrameNr=0.0;

                ClipModel.SetOrigin(State.Origin);
                ClipModel.Register();

                for (char Nr=0; Nr<15; Nr++) State.HaveAmmo         [Nr]=0;   // IMPORTANT: Do not clear the frags value in 'HaveAmmo[AMMO_SLOT_FRAGS]'!
                for (char Nr=0; Nr<32; Nr++) State.HaveAmmoInWeapons[Nr]=0;
                break;
            }

            case StateOfExistance_FreeSpectator:
                break;
        }
    }

    PlayerCommands.Clear();
}


void EntHumanPlayerT::ProcessEvent(char EventID)
{
    // GameWorld->PrintDebug("Entity %3u: ProcessEvent(%u)", TypeID, EventID);

    switch (EventID)
    {
        case EventID_PrimaryFire  : cf::GameSys::GameImplT::GetInstance().GetCarriedWeapon(State.ActiveWeaponSlot)->ClientSide_HandlePrimaryFireEvent  (this, LastSeenAmbientColor); break;
        case EventID_SecondaryFire: cf::GameSys::GameImplT::GetInstance().GetCarriedWeapon(State.ActiveWeaponSlot)->ClientSide_HandleSecondaryFireEvent(this, LastSeenAmbientColor); break;
    }
}


bool EntHumanPlayerT::GetLightSourceInfo(unsigned long& DiffuseColor, unsigned long& SpecularColor, VectorT& Position, float& Radius, bool& CastsShadows) const
{
#if 0
    int Pitch    =State.Pitch; if (Pitch>=32768) Pitch-=65536;
    int HalfPitch=Pitch/2;     if (HalfPitch<0) HalfPitch+=65536;

    const float LightDirZ=-LookupTables::Angle16ToSin[(unsigned short)HalfPitch];
    const float LightDirY= LookupTables::Angle16ToCos[(unsigned short)HalfPitch];

    const VectorT LightDir(LightDirY*LookupTables::Angle16ToSin[State.Heading], LightDirY*LookupTables::Angle16ToCos[State.Heading], LightDirZ);

    DiffuseColor =0x00FF3018;
    SpecularColor=0x00CCFF18;
    /* switch (ID & 0x3)
    {
        case  0: Color=0x00FF6000; break;
        case  1: Color=0x0060FF00; break;
        case  2: Color=0x000060FF; break;
        default: Color=0x00FF0060;
    } */

    const float f=120.0f*LookupTables::Angle16ToSin[(unsigned short)(TimeForLightSource/5.0f*65536.0f)];

    Position    =State.Origin+VectorT(0.0, 0.0, -150.0)+scale(LightDir, 390.0)+VectorT(LookupTables::Angle16ToCos[State.Heading]*f, LookupTables::Angle16ToSin[State.Heading]*f, 0.0);
    Radius      =20000.0;
    CastsShadows=true;

    return true;
#else
    return false;
#endif
}


void EntHumanPlayerT::Draw(bool FirstPersonView, float LodDist) const
{
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
        if (State.HaveWeapons & (1 << State.ActiveWeaponSlot))     // Only draw the active weapon if we actually "have" it
        {
            Vector3fT LgtPos(MatSys::Renderer->GetCurrentLightSourcePosition());
            Vector3fT EyePos(MatSys::Renderer->GetCurrentEyePosition());

            // The translation is not actually required, but gives the weapon a very nice 'shifting' effect when the player looks up/down.
            // If there ever is a problem with view model distortion, this may be a cause.
            LgtPos.z+=0.5f;
            EyePos.z+=0.5f;
            MatSys::Renderer->Translate(MatSys::RendererI::MODEL_TO_WORLD, 0.0f, 0.0f, -0.5f);

            const float DegPitch=float(State.Pitch)/8192.0f*45.0f;

            LgtPos=LgtPos.GetRotY(-DegPitch);
            EyePos=EyePos.GetRotY(-DegPitch);
            MatSys::Renderer->RotateY(MatSys::RendererI::MODEL_TO_WORLD, DegPitch);

            MatSys::Renderer->SetCurrentLightSourcePosition(LgtPos.x, LgtPos.y, LgtPos.z);
            MatSys::Renderer->SetCurrentEyePosition(EyePos.x, EyePos.y, EyePos.z);


            const CafuModelT* WeaponModel=cf::GameSys::GameImplT::GetInstance().GetCarriedWeapon(State.ActiveWeaponSlot)->GetViewWeaponModel();
            AnimPoseT*        Pose       =WeaponModel->GetSharedPose(State.ActiveWeaponSequNr, State.ActiveWeaponFrameNr);

            Pose->Draw(-1 /*default skin*/, LodDist);
        }
    }
    else
    {
        if (State.StateOfExistance!=StateOfExistance_Alive && State.StateOfExistance!=StateOfExistance_Dead) return;

        const float OffsetZ=(State.StateOfExistance!=StateOfExistance_Dead) ? -32.0f : -32.0f+float(State.Dimensions.Min.z+1728.8)/25.4f;

        MatSys::Renderer->GetCurrentLightSourcePosition()[2]-=OffsetZ;
        MatSys::Renderer->GetCurrentEyePosition        ()[2]-=OffsetZ;
        MatSys::Renderer->Translate(MatSys::RendererI::MODEL_TO_WORLD, 0.0f, 0.0f, OffsetZ);

        // Draw the own player body model and the "_p" (player) model of the active weapon as sub-model of the body.
        const CafuModelT* PlayerModel=cf::GameSys::GameImplT::GetInstance().GetPlayerModel(State.ModelIndex);
        AnimPoseT*        Pose       =PlayerModel->GetSharedPose(State.ModelSequNr, State.ModelFrameNr);

        Pose->Draw(-1 /*default skin*/, LodDist);

        if (State.HaveWeapons & (1 << State.ActiveWeaponSlot))
        {
            const CafuModelT* WeaponModel=cf::GameSys::GameImplT::GetInstance().GetCarriedWeapon(State.ActiveWeaponSlot)->GetPlayerWeaponModel();
            AnimPoseT*        WeaponPose =WeaponModel->GetSharedPose(0, 0.0f);

            WeaponPose->SetSuperPose(Pose);
            WeaponPose->Draw(-1 /*default skin*/, LodDist);
            WeaponPose->SetSuperPose(NULL);
        }
    }
}


void EntHumanPlayerT::PostDraw(float FrameTime, bool FirstPersonView)
{
    // Code for state driven effects.
    if (State.HaveWeapons & (1 << State.ActiveWeaponSlot))
        cf::GameSys::GameImplT::GetInstance().GetCarriedWeapon(State.ActiveWeaponSlot)->ClientSide_HandleStateDrivenEffects(this);


    if (FirstPersonView)
    {
        // This is a quite good place to deal with the ParticleEngine,
        // because we come here exactly once per frame, only after everything else has already been drawn,
        // and with the OpenGL modelview matrix set to world space.
        ParticleEngineMS::DrawParticles();
        ParticleEngineMS::MoveParticles(FrameTime);


        // Inside the next { ... } block is some leftover of the HUD code as it was *before* it was implemented by means of the GuiSys.
        // It would be easy to implement this with the below GuiHUD code, too, but I intentionally leave it in for now,
        // as it demonstrates both the old was well as an alternative (to the GuiSys) way of text output (or "custom rendering").
        {
            static FontT HUD_Font1("Fonts/FixedWidth");

            switch (State.StateOfExistance)
            {
                case StateOfExistance_Dead:
                    HUD_Font1.Print(50, 1024/2-4, 800.0f, 600.0f, 0x00FF0000, "You're dead.");
                    break;

                case StateOfExistance_FrozenSpectator:
                    HUD_Font1.Print(50, 1024/2+16, 800.0f, 600.0f, 0x00FF0000, "Press FIRE (left mouse button) to respawn!");
                    break;
            }

            // const int CharWidth=10;

            // HUD_Font1.Print(10+ 0*CharWidth, SingleOpenGLWindow->GetHeight()-16, 0x00FFFFFF, "Health %3u", State.Health);
            // HUD_Font1.Print(10+16*CharWidth, SingleOpenGLWindow->GetHeight()-16, 0x00FFFFFF, "Armor %3u", State.Armor);
            // HUD_Font1.Print(10+31*CharWidth, SingleOpenGLWindow->GetHeight()-16, 0x00FFFFFF, "Frags%3i", (signed short)(State.HaveAmmo[AMMO_SLOT_FRAGS]));
        }


        // This is a compromise for the not-so-great code in the constructor:
        // Obtain a pointer to our GUI if we haven't one already (so this is a one-time issue).
        if (GuiHUD==NULL) GuiHUD=cf::GuiSys::GuiMan->Find("Games/DeathMatch/GUIs/HUD_main.cgui", true);

        // Decide whether the GuiHUD should be drawn at all.
        const bool ActivateHUD=State.StateOfExistance==StateOfExistance_Alive || State.StateOfExistance==StateOfExistance_Dead;

        // GuiHUD could still be NULL, e.g. if the HUD.cgui file could not be found, or if there was a parse error.
        if (GuiHUD!=NULL) GuiHUD->Activate(ActivateHUD);


        // Now update the GuiHUD, but only if we successfully acquired it earlier (GuiHUD!=NULL) *and* it gets drawn (is active).
        // NOTE: It is *NOT* particularly great that we update the GuiHUD every frame (instead of only when the values change).
        // However, the general structure of the game code is currently not better suitable for the task, so that it's the best
        // for now to just do it in this place and in this way...
        if (GuiHUD!=NULL && ActivateHUD)
        {
            // Update the health, armor and frags indicators.
            GuiHUD->CallLuaFunc("UpdateHealthArmorFrags", "iii", State.Health, State.Armor, (signed short)(State.HaveAmmo[AMMO_SLOT_FRAGS]));


            // Let the HUD GUI know which material we wish to have for the crosshair, pass "" for none.
            if (State.StateOfExistance==StateOfExistance_Alive)
            {
                switch (State.ActiveWeaponSlot)
                {
                    case WEAPON_SLOT_HORNETGUN:
                    case WEAPON_SLOT_PISTOL:
                    case WEAPON_SLOT_CROSSBOW:
                    case WEAPON_SLOT_357:
                    case WEAPON_SLOT_9MMAR:
                        GuiHUD->CallLuaFunc("UpdateCrosshairMaterial", "s", "Gui/CrossHair1");
                        break;

                    case WEAPON_SLOT_SHOTGUN:
                    case WEAPON_SLOT_RPG:
                    case WEAPON_SLOT_GAUSS:
                    case WEAPON_SLOT_EGON:
                        GuiHUD->CallLuaFunc("UpdateCrosshairMaterial", "sb", "Gui/CrossHair2", true);
                        break;

                    default:
                        // Some weapons just don't have a crosshair.
                        GuiHUD->CallLuaFunc("UpdateCrosshairMaterial", "s", "");
                        break;
                }
            }
            else GuiHUD->CallLuaFunc("UpdateCrosshairMaterial", "s", "");


            // Update the HUDs ammo string.
            if (State.HaveWeapons & (1 << State.ActiveWeaponSlot))
            {
                char PrintBuffer[64];

                // Assignment table to determine which ammo is consumed by each weapon for primary fire (given a weapon slot, determine the ammo slot).
                // TODO: This is not optimal, ought to be static member function of each weapon???
                const char GetAmmoSlotForPrimaryFireByWeaponSlot[13] = {
                    AMMO_SLOT_NONE,
                    AMMO_SLOT_NONE,
                    AMMO_SLOT_9MM,
                    AMMO_SLOT_357,
                    AMMO_SLOT_SHELLS,
                    AMMO_SLOT_9MM,
                    AMMO_SLOT_ARROWS,
                    AMMO_SLOT_ROCKETS,
                    AMMO_SLOT_CELLS,
                    AMMO_SLOT_CELLS,
                    AMMO_SLOT_NONE,
                    AMMO_SLOT_NONE,
                    AMMO_SLOT_NONE };

                switch (State.ActiveWeaponSlot)
                {
                    case WEAPON_SLOT_BATTLESCYTHE:
                    case WEAPON_SLOT_HORNETGUN:
                        GuiHUD->CallLuaFunc("UpdateAmmoString", "s", "");
                        break;

                    case WEAPON_SLOT_9MMAR:
                        sprintf(PrintBuffer, " Ammo %2u (%2u) | %u Grenades", State.HaveAmmoInWeapons[WEAPON_SLOT_9MMAR],
                                State.HaveAmmo[GetAmmoSlotForPrimaryFireByWeaponSlot[WEAPON_SLOT_9MMAR]],
                                State.HaveAmmo[AMMO_SLOT_ARGREN]);
                        GuiHUD->CallLuaFunc("UpdateAmmoString", "s", PrintBuffer);
                        break;

                    case WEAPON_SLOT_FACEHUGGER:
                    case WEAPON_SLOT_GRENADE:
                    case WEAPON_SLOT_RPG:
                    case WEAPON_SLOT_TRIPMINE:
                        sprintf(PrintBuffer, " Ammo %2u", State.HaveAmmoInWeapons[State.ActiveWeaponSlot]);
                        GuiHUD->CallLuaFunc("UpdateAmmoString", "s", PrintBuffer);
                        break;

                    case WEAPON_SLOT_357:
                    case WEAPON_SLOT_CROSSBOW:
                    case WEAPON_SLOT_EGON:
                    case WEAPON_SLOT_GAUSS:
                    case WEAPON_SLOT_PISTOL:
                    case WEAPON_SLOT_SHOTGUN:
                        sprintf(PrintBuffer, " Ammo %2u (%2u)", State.HaveAmmoInWeapons[State.ActiveWeaponSlot], State.HaveAmmo[GetAmmoSlotForPrimaryFireByWeaponSlot[State.ActiveWeaponSlot]]);
                        GuiHUD->CallLuaFunc("UpdateAmmoString", "s", PrintBuffer);
                        break;
                }
            }
            else
            {
                // Let the HUD know that we have no weapon.
                GuiHUD->CallLuaFunc("UpdateAmmoString", "s", "");
            }
        }

        // Update listener here since this is the entity of the player.
        const float ViewX=sin(float(State.Heading)/32768.0f*3.1415926f);
        const float ViewY=cos(float(State.Heading)/32768.0f*3.1415926f);

        Vector3fT OrientationForward(ViewX, ViewY, 0.0f);
        Vector3fT OrientationUp     ( 0.0f,  0.0f, 1.0f);

        SoundSystem->UpdateListener(State.Origin, State.Velocity, OrientationForward, OrientationUp);
    }
    else
    {
        const CafuModelT* PlayerModel=cf::GameSys::GameImplT::GetInstance().GetPlayerModel(State.ModelIndex);
        AnimPoseT*        Pose       =PlayerModel->GetSharedPose(State.ModelSequNr, State.ModelFrameNr);

        // Implicit simple "mini-prediction". WARNING, this does not really work...!
        Pose->Advance(FrameTime, State.StateOfExistance!=StateOfExistance_Dead);
        State.ModelFrameNr=Pose->GetFrameNr();
    }

    TimeForLightSource+=FrameTime;
    if (TimeForLightSource>5.0) TimeForLightSource-=5.0;
}


void EntHumanPlayerT::getWorldTransform(btTransform& worldTrans) const
{
    Vector3dT Origin=State.Origin;

    // The box shape of our physics body is equally centered around the origin point,
    // whereas our State.Dimensions box is "non-uniformely displaced".
    // In order to compensate, compute how far the State.Dimensions center is away from the origin.
    Origin.z+=(State.Dimensions.Min.z+State.Dimensions.Max.z)/2.0;

    // Return the current transformation of our rigid body to the physics world.
    worldTrans.setIdentity();
    worldTrans.setOrigin(conv(Origin/1000.0));
}


void EntHumanPlayerT::setWorldTransform(const btTransform& /*worldTrans*/)
{
    // Never called for a kinematic rigid body.
    assert(false);
}
