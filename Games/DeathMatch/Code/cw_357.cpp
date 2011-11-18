/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#include "cw_357.hpp"
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


CarriedWeapon357T::CarriedWeapon357T(ModelManagerT& ModelMan)
    : CarriedWeaponT(ModelMan.GetModel("Games/DeathMatch/Models/Weapons/DesertEagle_v.mdl"),
                     ModelMan.GetModel("Games/DeathMatch/Models/Weapons/DesertEagle_p.mdl")),
      FireSound(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/DesertEagle_Shot1")))
{
}


CarriedWeapon357T::~CarriedWeapon357T()
{
    // Release Sound.
    SoundSystem->DeleteSound(FireSound);
}


bool CarriedWeapon357T::ServerSide_PickedUpByEntity(BaseEntityT* Entity) const
{
    // Consider if the entity already has this weapon.
    if (Entity->State.HaveWeapons & (1 << WEAPON_SLOT_357))
    {
        // If it also has the max. amount of ammo of this type, ignore the touch.
        if (Entity->State.HaveAmmo[AMMO_SLOT_357]==36) return false;

        // Otherwise pick the weapon up and let it have the ammo.
        Entity->State.HaveAmmo[AMMO_SLOT_357]+=12;
    }
    else
    {
        // This weapon is picked up for the first time.
        Entity->State.HaveWeapons|=1 << WEAPON_SLOT_357;
        Entity->State.ActiveWeaponSlot   =WEAPON_SLOT_357;
        Entity->State.ActiveWeaponSequNr =5;    // Draw
        Entity->State.ActiveWeaponFrameNr=0.0;

        Entity->State.HaveAmmoInWeapons[WEAPON_SLOT_357] =6;
        Entity->State.HaveAmmo         [AMMO_SLOT_357  ]+=6;
    }

    // Limit the amount of carryable ammo.
    if (Entity->State.HaveAmmo[AMMO_SLOT_357]>36) Entity->State.HaveAmmo[AMMO_SLOT_357]=36;

    return true;
}


void CarriedWeapon357T::ServerSide_Think(EntHumanPlayerT* Player, const PlayerCommandT& PlayerCommand, bool ThinkingOnServerSide, unsigned long /*ServerFrameNr*/, bool AnimSequenceWrap) const
{
    EntityStateT& State=Player->State;

    switch (State.ActiveWeaponSequNr)
    {
        case 3: // Reload
            if (AnimSequenceWrap)
            {
                State.ActiveWeaponSequNr =0;
                State.ActiveWeaponFrameNr=0.0;

                const char Amount=State.HaveAmmo[AMMO_SLOT_357]>6 ? 6 : State.HaveAmmo[AMMO_SLOT_357];

                State.HaveAmmoInWeapons[WEAPON_SLOT_357]+=Amount;
                State.HaveAmmo         [  AMMO_SLOT_357]-=Amount;
            }
            break;

        case 4: // Holster
            break;

        case 5: // Draw
            if (AnimSequenceWrap)
            {
                // Back to idle.
                State.ActiveWeaponSequNr =0;
                State.ActiveWeaponFrameNr=0.0;
            }
            break;

        case 2: // Fire
            if (!AnimSequenceWrap) break;

            // Back to idle.
            State.ActiveWeaponSequNr =0;
            State.ActiveWeaponFrameNr=0.0;
            // Intentional fall-through.

        case 0: // Idle 1
        case 6: // Idle 2
        case 7: // Idle 3
        case 1: // Fidget 1
        {
            // 1. First see if the magazine is empty and special action is required.
            if (!State.HaveAmmoInWeapons[WEAPON_SLOT_357])
            {
                if (State.HaveAmmo[AMMO_SLOT_357])
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

            // 2. Are we to fire a bullet (we have at least one)?
            if (PlayerCommand.Keys & PCK_Fire1)
            {
                State.ActiveWeaponSequNr =2;                // Fire
                State.ActiveWeaponFrameNr=0.0;
                State.HaveAmmoInWeapons[WEAPON_SLOT_357]--;
                State.Events^=(1 << EntHumanPlayerT::EventID_PrimaryFire);   // Flip event flag.

                if (ThinkingOnServerSide)
                {
                    // If we are on the server-side, find out what or who we hit.
                    const float ViewDirZ=-LookupTables::Angle16ToSin[State.Pitch];
                    const float ViewDirY= LookupTables::Angle16ToCos[State.Pitch];

                    const VectorT ViewDir(ViewDirY*LookupTables::Angle16ToSin[State.Heading], ViewDirY*LookupTables::Angle16ToCos[State.Heading], ViewDirZ);

                    RayResultT RayResult(Player->GetRigidBody());
                    Player->PhysicsWorld->TraceRay(State.Origin/1000.0, scale(ViewDir, 9999999.0/1000.0), RayResult);

                    if (RayResult.hasHit() && RayResult.GetHitEntity()!=NULL)
                        RayResult.GetHitEntity()->TakeDamage(Player, 7, ViewDir);
                }
                break;
            }

            // 3. If nothing else has happened, just choose another sequence number on sequence wrap.
            if (AnimSequenceWrap)
            {
                const char RandomNumber=char(LookupTables::RandomUShort[PlayerCommand.Nr & 0xFFF]);

                     if (RandomNumber< 96) State.ActiveWeaponSequNr=0;  // Idle 1
                else if (RandomNumber<192) State.ActiveWeaponSequNr=6;  // Idle 2
                else if (RandomNumber<224) State.ActiveWeaponSequNr=7;  // Idle 3
                else                       State.ActiveWeaponSequNr=1;  // Fidget 1
            }
            break;
        }
    }
}


static bool ParticleFunction_HitWall(ParticleMST* Particle, float Time)
{
    const float MaxAge=3.0f;

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
    const float MaxAge=1.0f;

    Particle->Age+=Time;
    if (Particle->Age>MaxAge) return false;

    const float p=1.0f-Particle->Age/MaxAge;     // % of remaining lifetime.

    Particle->Color[0]=char(255.0*p);
    Particle->Color[1]=0;
    Particle->Color[2]=0;

    return true;
}


void CarriedWeapon357T::ClientSide_HandlePrimaryFireEvent(const EntHumanPlayerT* Player, const VectorT& /*LastSeenAmbientColor*/) const
{
    const EntityStateT& State=Player->State;

    const float ViewDirZ=-LookupTables::Angle16ToSin[State.Pitch];
    const float ViewDirY= LookupTables::Angle16ToCos[State.Pitch];

    const VectorT ViewDir(ViewDirY*LookupTables::Angle16ToSin[State.Heading], ViewDirY*LookupTables::Angle16ToCos[State.Heading], ViewDirZ);

    RayResultT RayResult(Player->GetRigidBody());
    Player->PhysicsWorld->TraceRay(State.Origin/1000.0, scale(ViewDir, 9999999.0/1000.0), RayResult);

    if (!RayResult.hasHit()) return;


    // Register a new particle at the 'Hit' point.
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
