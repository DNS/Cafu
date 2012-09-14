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

#include "cw_Shotgun.hpp"
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


CarriedWeaponShotgunT::CarriedWeaponShotgunT(ModelManagerT& ModelMan)
    : CarriedWeaponT(ModelMan.GetModel("Games/DeathMatch/Models/Weapons/Shotgun/Shotgun_v.cmdl"),
                     ModelMan.GetModel("Games/DeathMatch/Models/Weapons/Shotgun/Shotgun_p.cmdl")),
      FireSound   (SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/Shotgun_sBarrel"))),
      AltFireSound(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/Shotgun_dBarrel")))
{
}


CarriedWeaponShotgunT::~CarriedWeaponShotgunT()
{
    // Release Sound.
    SoundSystem->DeleteSound(FireSound);
    SoundSystem->DeleteSound(AltFireSound);
}


bool CarriedWeaponShotgunT::ServerSide_PickedUpByEntity(EntHumanPlayerT* Player) const
{
    EntityStateT& State=Player->GetState();

    // Consider if the entity already has this weapon.
    if (State.HaveWeapons & (1 << WEAPON_SLOT_SHOTGUN))
    {
        // If it also has the max. amount of ammo of this type, ignore the touch.
        if (State.HaveAmmo[AMMO_SLOT_SHELLS]==125) return false;

        // Otherwise pick the weapon up and let it have the ammo.
        State.HaveAmmo[AMMO_SLOT_SHELLS]+=16;
    }
    else
    {
        // This weapon is picked up for the first time.
        State.HaveWeapons|=1 << WEAPON_SLOT_SHOTGUN;
        State.ActiveWeaponSlot   =WEAPON_SLOT_SHOTGUN;
        State.ActiveWeaponSequNr =6;    // Draw
        State.ActiveWeaponFrameNr=0.0;

        State.HaveAmmoInWeapons[WEAPON_SLOT_SHOTGUN] =8;
        State.HaveAmmo         [AMMO_SLOT_SHELLS   ]+=8;
    }

    // Limit the amount of carryable ammo.
    if (State.HaveAmmo[AMMO_SLOT_SHELLS]>125) State.HaveAmmo[AMMO_SLOT_SHELLS]=125;

    return true;
}


void CarriedWeaponShotgunT::ServerSide_Think(EntHumanPlayerT* Player, const PlayerCommandT& PlayerCommand, bool ThinkingOnServerSide, unsigned long /*ServerFrameNr*/, bool AnimSequenceWrap) const
{
    EntityStateT& State=Player->GetState();

    switch (State.ActiveWeaponSequNr)
    {
        case 3: // Reload / Insert shells
            if (AnimSequenceWrap)
            {
                State.HaveAmmoInWeapons[WEAPON_SLOT_SHOTGUN]+=1;
                State.HaveAmmo         [AMMO_SLOT_SHELLS   ]-=1;

                if (State.HaveAmmoInWeapons[WEAPON_SLOT_SHOTGUN]>=8 || State.HaveAmmo[AMMO_SLOT_SHELLS]==0)
                {
                    State.ActiveWeaponSequNr =4;
                    State.ActiveWeaponFrameNr=0.0;
                }
            }
            break;

        case 4: // Pump / After reload
            if (AnimSequenceWrap)
            {
                State.ActiveWeaponSequNr =0;
                State.ActiveWeaponFrameNr=0.0;
            }
            break;

        case 5: // Start reload
            if (AnimSequenceWrap)
            {
                State.ActiveWeaponSequNr =3;
                State.ActiveWeaponFrameNr=0.0;
            }
            break;

        case 6: // Draw
            if (AnimSequenceWrap)
            {
                // Back to idle.
                State.ActiveWeaponSequNr =0;
                State.ActiveWeaponFrameNr=0.0;
            }
            break;

        case 7: // Reholster
            break;

        case 1: // Shoot 1
            if (!AnimSequenceWrap) break;

            State.ActiveWeaponSequNr =0;
            State.ActiveWeaponFrameNr=0.0;
            // Intentional fall-through.

        case 2: // Shoot 2
            if (!AnimSequenceWrap) break;

            State.ActiveWeaponSequNr =0;
            State.ActiveWeaponFrameNr=0.0;
            // Intentional fall-through.

        case 0: // Idle
        case 8: // Idle4
        case 9: // Deep idle
        default:
            // 1. First see if the magazine is empty and special action is required.
            if (!State.HaveAmmoInWeapons[WEAPON_SLOT_SHOTGUN])
            {
                if (State.HaveAmmo[AMMO_SLOT_SHELLS])
                {
                    State.ActiveWeaponSequNr =5;    // StartReload
                    State.ActiveWeaponFrameNr=0.0;
                    break;
                }

                if (PlayerCommand.Keys & (PCK_Fire1 | PCK_Fire2))
                {
                    // TODO: Player->PostEvent(EntHumanPlayerT::EVENT_TYPE_PRIMARY_FIRE_EMPTY);    // BUT LIMIT THE "FREQUENCY" OF THIS EVENT!
                    break;
                }
            }

            if (State.HaveAmmoInWeapons[WEAPON_SLOT_SHOTGUN]==1 && (PlayerCommand.Keys & PCK_Fire2))
            {
                // TODO: Player->PostEvent(EntHumanPlayerT::EVENT_TYPE_SECONDARY_FIRE_EMPTY);      // BUT LIMIT THE "FREQUENCY" OF THIS EVENT!
                break;
            }

            // 2. Are we to fire single barreled (we have at least one shell)?
            if (PlayerCommand.Keys & PCK_Fire1)
            {
                State.ActiveWeaponSequNr =1;                // Shoot 1
                State.ActiveWeaponFrameNr=0.0;
                State.HaveAmmoInWeapons[WEAPON_SLOT_SHOTGUN]--;

                Player->PostEvent(EntHumanPlayerT::EVENT_TYPE_PRIMARY_FIRE);

                if (ThinkingOnServerSide)
                {
                    // If we are on the server-side, find out what or who we hit.
                    for (char i=0; i<8; i++)
                    {
                        const unsigned short Pitch  =Player->GetPitch()  +(rand() % 910)-455;  // ca. 5°
                        const unsigned short Heading=Player->GetHeading()+(rand() % 910)-455;  // ca. 5°

                        const float ViewDirZ=-LookupTables::Angle16ToSin[Pitch];
                        const float ViewDirY= LookupTables::Angle16ToCos[Pitch];

                        const VectorT ViewDir(ViewDirY*LookupTables::Angle16ToSin[Heading], ViewDirY*LookupTables::Angle16ToCos[Heading], ViewDirZ);

                        RayResultT RayResult(Player->GetRigidBody());
                        Player->GameWorld->GetPhysicsWorld().TraceRay(Player->GetOrigin()/1000.0, scale(ViewDir, 9999999.0/1000.0), RayResult);

                        if (RayResult.hasHit() && RayResult.GetHitEntity()!=NULL)
                            static_cast<BaseEntityT*>(RayResult.GetHitEntity())->TakeDamage(Player, 3, ViewDir);
                    }
                }
                break;
            }

            // 3. Are we to fire double barreled (we have at least two shells)?
            if (PlayerCommand.Keys & PCK_Fire2)
            {
                State.ActiveWeaponSequNr =2;            // Shoot 2
                State.ActiveWeaponFrameNr=0.0;
                State.HaveAmmoInWeapons[WEAPON_SLOT_SHOTGUN]-=2;

                Player->PostEvent(EntHumanPlayerT::EVENT_TYPE_SECONDARY_FIRE);

                if (ThinkingOnServerSide)
                {
                    // If we are on the server-side, find out what or who we hit.
                    for (char i=0; i<16; i++)
                    {
                        const unsigned short Pitch  =Player->GetPitch()  +(rand() % 910)-455;  // ca. 5°
                        const unsigned short Heading=Player->GetHeading()+(rand() % 910)-455;  // ca. 5°

                        const float ViewDirZ=-LookupTables::Angle16ToSin[Pitch];
                        const float ViewDirY= LookupTables::Angle16ToCos[Pitch];

                        const VectorT ViewDir(ViewDirY*LookupTables::Angle16ToSin[Heading], ViewDirY*LookupTables::Angle16ToCos[Heading], ViewDirZ);

                        RayResultT RayResult(Player->GetRigidBody());
                        Player->GameWorld->GetPhysicsWorld().TraceRay(Player->GetOrigin()/1000.0, scale(ViewDir, 9999999.0/1000.0), RayResult);

                        if (RayResult.hasHit() && RayResult.GetHitEntity()!=NULL)
                            static_cast<BaseEntityT*>(RayResult.GetHitEntity())->TakeDamage(Player, 3, ViewDir);
                    }
                }
                break;
            }

            // 4. If nothing else has happened, just choose another sequence number on sequence wrap.
            // if (AnimSequenceWrap)
            // {
            //     // Damn. No need to alternate randomly among 'Idle', 'Idle 4' and 'Deep Idle'.
            //     // They are all identical anyway (with the current Shotgun_v model).
            // }
            break;
    }
}


static bool ParticleFunction_ShotgunHitWall(ParticleMST* Particle, float Time)
{
    const float MaxAge=0.4f;

    Particle->Age+=Time;
    if (Particle->Age>MaxAge) return false;

    const float p=1.0f-Particle->Age/MaxAge;     // % of remaining lifetime

    Particle->Color[0]=char( 20.0f*p);
    Particle->Color[1]=char(255.0f*p);
    Particle->Color[2]=char(180.0f*p);

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


static bool ParticleFunction_ShotgunWhiteSmoke(ParticleMST* Particle, float Time)
{
    const float FPS   =20.0f;        // The default value is 20.0.
    const float MaxAge=32.0f/FPS;    // 32 frames at 20 FPS.

    Particle->Origin[0]+=Particle->Velocity[0]*Time;
    Particle->Origin[1]+=Particle->Velocity[1]*Time;
    Particle->Origin[2]+=Particle->Velocity[2]*Time;

    Particle->Radius+=Time*1000.0f;
    Particle->RenderMat=ResMan.RenderMats[ResMan.PARTICLE_WHITESMOKE_FRAME1+(unsigned long)(Particle->Age*FPS)];

    Particle->Age+=Time;
    if (Particle->Age>=MaxAge) return false;

    return true;
}


void CarriedWeaponShotgunT::ClientSide_HandlePrimaryFireEvent(const EntHumanPlayerT* Player, const VectorT& LastSeenAmbientColor) const
{
    const EntityStateT& State=Player->GetState();

    for (char i=0; i<8; i++)
    {
        const unsigned short Pitch  =Player->GetPitch()  +(rand() % 910)-455;  // ca. 5°
        const unsigned short Heading=Player->GetHeading()+(rand() % 910)-455;  // ca. 5°

        const float ViewDirZ=-LookupTables::Angle16ToSin[Pitch];
        const float ViewDirY= LookupTables::Angle16ToCos[Pitch];

        const VectorT ViewDir(ViewDirY*LookupTables::Angle16ToSin[Heading], ViewDirY*LookupTables::Angle16ToCos[Heading], ViewDirZ);

        RayResultT RayResult(Player->GetRigidBody());
        Player->GameWorld->GetPhysicsWorld().TraceRay(Player->GetOrigin()/1000.0, scale(ViewDir, 9999999.0/1000.0), RayResult);

        if (!RayResult.hasHit()) break;

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
        NewParticle.MoveFunction=RayResult.GetHitEntity()==NULL ? ParticleFunction_ShotgunHitWall : ParticleFunction_HitEntity;

        ParticleEngineMS::RegisterNewParticle(NewParticle);
    }

    const float   ViewDirZ=-LookupTables::Angle16ToSin[Player->GetPitch()];
    const float   ViewDirY= LookupTables::Angle16ToCos[Player->GetPitch()];
    const VectorT ViewDir(ViewDirY*LookupTables::Angle16ToSin[Player->GetHeading()], ViewDirY*LookupTables::Angle16ToCos[Player->GetHeading()], ViewDirZ);

    // Register a new particle as "muzzle flash".
    ParticleMST NewParticle;

    NewParticle.Origin[0]=float(Player->GetOrigin().x+ViewDir.x*400.0);
    NewParticle.Origin[1]=float(Player->GetOrigin().y+ViewDir.y*400.0);
    NewParticle.Origin[2]=float(Player->GetOrigin().z+ViewDir.z*400.0-100.0);

    NewParticle.Velocity[0]=float(ViewDir.x*1000.0);
    NewParticle.Velocity[1]=float(ViewDir.y*1000.0);
    NewParticle.Velocity[2]=float(ViewDir.z*1000.0);

    NewParticle.Age=0.0;
    NewParticle.Color[0]=char(LastSeenAmbientColor.x*255.0);
    NewParticle.Color[1]=char(LastSeenAmbientColor.y*255.0);
    NewParticle.Color[2]=char(LastSeenAmbientColor.z*255.0);
    NewParticle.Color[3]=100;

    NewParticle.Radius=80.0;
    NewParticle.Rotation=char(rand());
    NewParticle.StretchY=1.0;
    NewParticle.RenderMat=ResMan.RenderMats[ResMan.PARTICLE_WHITESMOKE_FRAME1];
    NewParticle.MoveFunction=ParticleFunction_ShotgunWhiteSmoke;

    ParticleEngineMS::RegisterNewParticle(NewParticle);

    // Update sound position and velocity.
    FireSound->SetPosition(Player->GetOrigin()+scale(ViewDir, 400.0));
    FireSound->SetVelocity(State.Velocity);

    // Play the fire sound.
    FireSound->Play();
}


void CarriedWeaponShotgunT::ClientSide_HandleSecondaryFireEvent(const EntHumanPlayerT* Player, const VectorT& LastSeenAmbientColor) const
{
    const EntityStateT& State=Player->GetState();

    for (char i=0; i<16; i++)
    {
        const unsigned short Pitch  =Player->GetPitch()  +(rand() % 910)-455;  // ca. 5°
        const unsigned short Heading=Player->GetHeading()+(rand() % 910)-455;  // ca. 5°

        const float ViewDirZ=-LookupTables::Angle16ToSin[Pitch];
        const float ViewDirY= LookupTables::Angle16ToCos[Pitch];

        const VectorT ViewDir(ViewDirY*LookupTables::Angle16ToSin[Heading], ViewDirY*LookupTables::Angle16ToCos[Heading], ViewDirZ);

        RayResultT RayResult(Player->GetRigidBody());
        Player->GameWorld->GetPhysicsWorld().TraceRay(Player->GetOrigin()/1000.0, scale(ViewDir, 9999999.0/1000.0), RayResult);

        if (!RayResult.hasHit()) break;

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
        NewParticle.MoveFunction=RayResult.GetHitEntity()==NULL ? ParticleFunction_ShotgunHitWall : ParticleFunction_HitEntity;

        ParticleEngineMS::RegisterNewParticle(NewParticle);
    }

    const float   ViewDirZ=-LookupTables::Angle16ToSin[Player->GetPitch()];
    const float   ViewDirY= LookupTables::Angle16ToCos[Player->GetPitch()];
    const VectorT ViewDir(ViewDirY*LookupTables::Angle16ToSin[Player->GetHeading()], ViewDirY*LookupTables::Angle16ToCos[Player->GetHeading()], ViewDirZ);

    // Register a new particle as "muzzle flash".
    ParticleMST NewParticle;

    NewParticle.Origin[0]=float(Player->GetOrigin().x+ViewDir.x*400.0);
    NewParticle.Origin[1]=float(Player->GetOrigin().y+ViewDir.y*400.0);
    NewParticle.Origin[2]=float(Player->GetOrigin().z+ViewDir.z*400.0-100.0);

    NewParticle.Velocity[0]=float(ViewDir.x*1500.0);
    NewParticle.Velocity[1]=float(ViewDir.y*1500.0);
    NewParticle.Velocity[2]=float(ViewDir.z*1500.0);

    NewParticle.Age=0.0;
    NewParticle.Color[0]=char(LastSeenAmbientColor.x*255.0);
    NewParticle.Color[1]=char(LastSeenAmbientColor.y*255.0);
    NewParticle.Color[2]=char(LastSeenAmbientColor.z*255.0);
    NewParticle.Color[3]=180;

    NewParticle.Radius=200.0;
    NewParticle.Rotation=char(rand());
    NewParticle.StretchY=1.0;
    NewParticle.RenderMat=ResMan.RenderMats[ResMan.PARTICLE_WHITESMOKE_FRAME1];
    NewParticle.MoveFunction=ParticleFunction_ShotgunWhiteSmoke;

    ParticleEngineMS::RegisterNewParticle(NewParticle);

    // Update sound position and velocity.
    AltFireSound->SetPosition(Player->GetOrigin()+scale(ViewDir, 400.0));
    AltFireSound->SetVelocity(State.Velocity);

    // Play the fire sound.
    AltFireSound->Play();
}
