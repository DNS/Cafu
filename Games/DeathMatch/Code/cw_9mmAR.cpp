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

#include "cw_9mmAR.hpp"
#include "_ResourceManager.hpp"
#include "HumanPlayer.hpp"
#include "Constants_AmmoSlots.hpp"
#include "Constants_WeaponSlots.hpp"
#include "PhysicsWorld.hpp"
#include "Libs/LookupTables.hpp"
#include "../../GameWorld.hpp"
#include "Models/ModelManager.hpp"
#include "ParticleEngine/ParticleEngineMS.hpp"
#include "SoundSystem/SoundSys.hpp"
#include "SoundSystem/SoundShaderManager.hpp"
#include "SoundSystem/Sound.hpp"


CarriedWeapon9mmART::CarriedWeapon9mmART(ModelManagerT& ModelMan)
    : CarriedWeaponT(ModelMan.GetModel("Games/DeathMatch/Models/Weapons/9mmAR/9mmAR_v.cmdl"),
                     ModelMan.GetModel("Games/DeathMatch/Models/Weapons/9mmAR/9mmAR_p.cmdl")),
      FireSound   (SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/9mmAR_Shot1"))),
      AltFireSound(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/9mmAR_GLauncher")))
{
}


CarriedWeapon9mmART::~CarriedWeapon9mmART()
{
    // Release Sound.
    SoundSystem->DeleteSound(FireSound);
    SoundSystem->DeleteSound(AltFireSound);
}


bool CarriedWeapon9mmART::ServerSide_PickedUpByEntity(BaseEntityT* Entity) const
{
    // Consider if the entity already has this weapon.
    if (Entity->State.HaveWeapons & (1 << WEAPON_SLOT_9MMAR))
    {
        // If it also has the max. amount of ammo of this type, ignore the touch.
        if (Entity->State.HaveAmmo[AMMO_SLOT_9MM]==250) return false;

        // Otherwise pick the weapon up and let it have the ammo.
        Entity->State.HaveAmmo[AMMO_SLOT_9MM   ]+=50;
        Entity->State.HaveAmmo[AMMO_SLOT_ARGREN]+= 2;               // Temp. solution until we have "ItemAmmoARGrenades" working.
    }
    else
    {
        // This weapon is picked up for the first time.
        Entity->State.HaveWeapons|=1 << WEAPON_SLOT_9MMAR;
        Entity->State.ActiveWeaponSlot   =WEAPON_SLOT_9MMAR;
        Entity->State.ActiveWeaponSequNr =4;    // Draw
        Entity->State.ActiveWeaponFrameNr=0.0;

        Entity->State.HaveAmmoInWeapons[WEAPON_SLOT_9MMAR] =25;
        Entity->State.HaveAmmo         [AMMO_SLOT_9MM    ]+=25;
        Entity->State.HaveAmmo         [AMMO_SLOT_ARGREN ]+= 2;     // Temp. solution until we have "ItemAmmoARGrenades" working.
    }

    // Limit the amount of carryable ammo.
    if (Entity->State.HaveAmmo[AMMO_SLOT_9MM   ]>250) Entity->State.HaveAmmo[AMMO_SLOT_9MM   ]=250;
    if (Entity->State.HaveAmmo[AMMO_SLOT_ARGREN]>  4) Entity->State.HaveAmmo[AMMO_SLOT_ARGREN]=  4;

    return true;
}


void CarriedWeapon9mmART::ServerSide_Think(EntHumanPlayerT* Player, const PlayerCommandT& PlayerCommand, bool ThinkingOnServerSide, unsigned long ServerFrameNr, bool AnimSequenceWrap) const
{
    EntityStateT& State=Player->State;

    switch (State.ActiveWeaponSequNr)
    {
        case 3: // Reload
            if (AnimSequenceWrap)
            {
                State.ActiveWeaponSequNr =0;
                State.ActiveWeaponFrameNr=0.0;

                const char Amount=State.HaveAmmo[AMMO_SLOT_9MM]>25 ? 25 : State.HaveAmmo[AMMO_SLOT_9MM];

                State.HaveAmmoInWeapons[WEAPON_SLOT_9MMAR]+=Amount;
                State.HaveAmmo         [AMMO_SLOT_9MM    ]-=Amount;
            }
            break;

        case 4: // Deploy (Draw)
            if (AnimSequenceWrap)
            {
                State.ActiveWeaponSequNr =0;
                State.ActiveWeaponFrameNr=0.0;
            }
            break;

        case 2: // Grenade
            if (!AnimSequenceWrap) break;

            State.ActiveWeaponSequNr =0;
            State.ActiveWeaponFrameNr=0.0;
            // Intentional fall-through.

        case 5: // Shoot 1
        case 6: // Shoot 2
        case 7: // Shoot 3
            if ((State.HaveAmmoInWeapons[WEAPON_SLOT_9MMAR] && (PlayerCommand.Keys & PCK_Fire1) && State.ActiveWeaponFrameNr>=1.0) || AnimSequenceWrap)
            {
                // At 10 FPS (sequences 5, 6, 7), 0.1 seconds correspond to exactly 1 frame.
                // End of this shoot sequence - go back to "idle" and recover from there (decide anew).
                State.ActiveWeaponSequNr =0;
                State.ActiveWeaponFrameNr=0.0;
                // Intentional fall-through!
            }
            else break;

        case 0: // Long Idle
        case 1: // Idle1
            // 1. First see if the magazine is empty and special action is required.
            if (!State.HaveAmmoInWeapons[WEAPON_SLOT_9MMAR])
            {
                if (State.HaveAmmo[AMMO_SLOT_9MM])
                {
                    State.ActiveWeaponSequNr =3;    // Reload
                    State.ActiveWeaponFrameNr=0.0;
                    break;
                }

                if (PlayerCommand.Keys & PCK_Fire1)
                {
                    // TODO: State.Events^=(1 << EventID_PrimaryFireEmpty);     // BUT LIMIT THE "FREQUENCY" OF THIS EVENT!
                    break;
                }
            }

            if (!State.HaveAmmo[AMMO_SLOT_ARGREN] && (PlayerCommand.Keys & PCK_Fire2))
            {
                // TODO: State.Events^=(1 << EventID_SecondaryFireEmpty);     // BUT LIMIT THE "FREQUENCY" OF THIS EVENT!
                break;
            }

            if (PlayerCommand.Keys & PCK_Fire1)
            {
                State.ActiveWeaponSequNr =5+(LookupTables::RandomUShort[PlayerCommand.Nr & 0xFFF] % 3);
                State.ActiveWeaponFrameNr=0.0;
                State.HaveAmmoInWeapons[WEAPON_SLOT_9MMAR]--;

                if (ThinkingOnServerSide)
                {
                    // If we are on server-side, fire the first single shot, and find out what or who we hit.
                    const unsigned short Pitch  =State.Pitch  +(rand() % 364)-182;  // ca. 2°
                    const unsigned short Heading=State.Heading+(rand() % 364)-182;  // ca. 2°

                    const float ViewDirZ=-LookupTables::Angle16ToSin[Pitch];
                    const float ViewDirY= LookupTables::Angle16ToCos[Pitch];

                    const VectorT ViewDir(ViewDirY*LookupTables::Angle16ToSin[Heading], ViewDirY*LookupTables::Angle16ToCos[Heading], ViewDirZ);

                    RayResultT RayResult(Player->GetRigidBody());
                    Player->PhysicsWorld->TraceRay(State.Origin/1000.0, scale(ViewDir, 9999999.0/1000.0), RayResult);

                    if (RayResult.hasHit() && RayResult.GetHitEntity()!=NULL)
                        RayResult.GetHitEntity()->TakeDamage(Player, 1, ViewDir);
                }
                break;
            }

            if (PlayerCommand.Keys & PCK_Fire2)
            {
                State.ActiveWeaponSequNr =2;    // Grenade
                State.ActiveWeaponFrameNr=0.0;
                State.HaveAmmo[AMMO_SLOT_ARGREN]--;
                State.Events^=(1 << EntHumanPlayerT::EventID_SecondaryFire);

                // Important: ONLY create (throw) a new AR grenade IF we are on the server side!
                if (ThinkingOnServerSide)
                {
                    // Clamp 'Pitch' values larger than 45° (==8192) to 45°.
                    const unsigned short Pitch=(State.Pitch>8192 && State.Pitch<=16384) ? 8192 : State.Pitch;

                    const float ViewDirZ=-LookupTables::Angle16ToSin[Pitch];
                    const float ViewDirY= LookupTables::Angle16ToCos[Pitch];

                    // Note: There is a non-trivial relationship between heading, pitch, and the corresponding view vector.
                    // Especially does a heading and pitch of 45° NOT correspond to the view vector (1, 1, 1), and vice versa!
                    // Think carefully about this before changing the number 1050.0 below (which actually is 2.0*(400.0+120.0) (+10.0 for "safety")).
                    const VectorT ViewDir(ViewDirY*LookupTables::Angle16ToSin[State.Heading], ViewDirY*LookupTables::Angle16ToCos[State.Heading], ViewDirZ);
                    const VectorT ARGrenadeOrigin(State.Origin-VectorT(0.0, 0.0, 250.0)+scale(ViewDir, 1050.0)+scale(State.Velocity, double(PlayerCommand.FrameTime)));
                    std::map<std::string, std::string> Props;

                    Props["classname"]="monster_argrenade";

                    unsigned long ARGrenadeID=Player->GameWorld->CreateNewEntity(Props, ServerFrameNr, ARGrenadeOrigin);

                    if (ARGrenadeID!=0xFFFFFFFF)
                    {
                        BaseEntityT* ARGrenade=Player->GameWorld->GetBaseEntityByID(ARGrenadeID);

                        ARGrenade->ParentID      =Player->ID;
                        ARGrenade->State.Heading =State.Heading;
                        ARGrenade->State.Velocity=State.Velocity+scale(ViewDir, 20000.0);
                    }
                }
                break;
            }

            if (AnimSequenceWrap)
            {
                if (State.ActiveWeaponSequNr==0)
                {
                    State.ActiveWeaponSequNr=LookupTables::RandomUShort[PlayerCommand.Nr & 0xFFF] & 1;
                }
                else State.ActiveWeaponSequNr=0;     // Don't play the "Idle1" sequence repeatedly.

                State.ActiveWeaponFrameNr=0.0;
            }
            break;
    }
}


static bool ParticleFunction_HitWall(ParticleMST* Particle, float Time)
{
    const float MaxAge=3.0;

    Particle->Age+=Time;
    if (Particle->Age>MaxAge) return false;

    const float p=1.0f-Particle->Age/MaxAge;     // % of remaining lifetime.

    Particle->Color[0]=char( 20.0f*p);
    Particle->Color[1]=char(180.0f*p);
    Particle->Color[2]=char(255.0f*p);

    return true;
}


static bool ParticleFunction_HitEntity(ParticleMST* Particle, float Time)
{
    const float MaxAge=1.0;

    Particle->Age+=Time;
    if (Particle->Age>MaxAge) return false;

    const float p=1.0f-Particle->Age/MaxAge;     // % of remaining lifetime.

    Particle->Color[0]=char(255.0f*p);
    Particle->Color[1]=0;
    Particle->Color[2]=0;

    return true;
}


void CarriedWeapon9mmART::ClientSide_HandleSecondaryFireEvent(const EntHumanPlayerT* Player, const VectorT& /*LastSeenAmbientColor*/) const
{
    const EntityStateT& State=Player->State;

    const float ViewDirZ=-LookupTables::Angle16ToSin[State.Pitch];
    const float ViewDirY= LookupTables::Angle16ToCos[State.Pitch];

    const VectorT ViewDir(ViewDirY*LookupTables::Angle16ToSin[State.Heading], ViewDirY*LookupTables::Angle16ToCos[State.Heading], ViewDirZ);

    // Update sound position and velocity.
    AltFireSound->SetPosition(State.Origin+scale(ViewDir, 400.0));
    AltFireSound->SetVelocity(State.Velocity);

    // Play the fire sound.
    AltFireSound->Play();
}


void CarriedWeapon9mmART::ClientSide_HandleStateDrivenEffects(const EntHumanPlayerT* Player) const
{
    const EntityStateT& State=Player->State;

    if (State.ActiveWeaponSequNr==5 || State.ActiveWeaponSequNr==6 || State.ActiveWeaponSequNr==7)
    {
        if (State.ActiveWeaponFrameNr==0.0)
        {
            const unsigned short Pitch  =State.Pitch  +(rand() % 400)-200;  // ca. 2°
            const unsigned short Heading=State.Heading+(rand() % 400)-200;  // ca. 2°

            const float ViewDirZ=-LookupTables::Angle16ToSin[Pitch];
            const float ViewDirY= LookupTables::Angle16ToCos[Pitch];

            const VectorT ViewDir(ViewDirY*LookupTables::Angle16ToSin[Heading], ViewDirY*LookupTables::Angle16ToCos[Heading], ViewDirZ);

            RayResultT RayResult(Player->GetRigidBody());
            Player->PhysicsWorld->TraceRay(State.Origin/1000.0, scale(ViewDir, 9999999.0/1000.0), RayResult);

            if (!RayResult.hasHit()) return;

            // Register a new particle at the hit point.
            ParticleMST NewParticle;

            NewParticle.Origin[0]=RayResult.m_hitPointWorld.x()*1000.0f;
            NewParticle.Origin[1]=RayResult.m_hitPointWorld.y()*1000.0f;
            NewParticle.Origin[2]=RayResult.m_hitPointWorld.z()*1000.0f;

            NewParticle.Velocity[0]=0;
            NewParticle.Velocity[1]=0;
            NewParticle.Velocity[2]=0;

            NewParticle.Age=0.0;
            NewParticle.Color[3]=0;

            NewParticle.Radius=300.0;
            NewParticle.StretchY=1.0;
            NewParticle.RenderMat=ResMan.RenderMats[ResMan.PARTICLE_GENERIC1];
            NewParticle.MoveFunction=RayResult.GetHitEntity()==NULL ? ParticleFunction_HitWall : ParticleFunction_HitEntity;

            ParticleEngineMS::RegisterNewParticle(NewParticle);

            // Update sound position and velocity.
            FireSound->SetPosition(State.Origin+scale(ViewDir, 400.0));
            FireSound->SetVelocity(State.Velocity);

            // Play the fire sound.
            FireSound->Play();
        }
    }
}
