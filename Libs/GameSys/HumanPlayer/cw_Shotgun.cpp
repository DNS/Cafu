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

#include "cw_Shotgun.hpp"
#include "../../../Games/PlayerCommand.hpp"      // TODO: This file must be moved (and/or its contents completely redesigned).
#include "Constants_AmmoSlots.hpp"
#include "Constants_WeaponSlots.hpp"
#include "GameSys/CompPhysics.hpp"
#include "Models/ModelManager.hpp"
#include "ParticleEngine/ParticleEngineMS.hpp"
#include "SoundSystem/SoundSys.hpp"
#include "SoundSystem/SoundShaderManager.hpp"
#include "SoundSystem/Sound.hpp"

using namespace cf::GameSys;


CarriedWeaponShotgunT::CarriedWeaponShotgunT(ModelManagerT& ModelMan)
    : CarriedWeaponT(ModelMan.GetModel("Games/DeathMatch/Models/Weapons/Shotgun/Shotgun_v.cmdl"),
                     ModelMan.GetModel("Games/DeathMatch/Models/Weapons/Shotgun/Shotgun_p.cmdl")),
      FireSound   (SoundSystem ? SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/Shotgun_sBarrel")) : NULL),
      AltFireSound(SoundSystem ? SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/Shotgun_dBarrel")) : NULL),
      m_GenericMatSet(NULL),
      m_WhiteSmokeMatSet(NULL)
{
    // At this time, in CaWE, the map compile tools, and the server(?), we operate with SoundSystem == NULL.

    // TODO: Should rather store ParticleMaterialSetTs where their lifetime cannot be shorter than that of all particles...
    m_GenericMatSet = new ParticleMaterialSetT("generic", "Sprites/Generic1");
    m_WhiteSmokeMatSet = new ParticleMaterialSetT("white-smoke", "Sprites/smoke1/smoke_%02u");
}


CarriedWeaponShotgunT::~CarriedWeaponShotgunT()
{
    delete m_WhiteSmokeMatSet;
    m_WhiteSmokeMatSet = NULL;

    delete m_GenericMatSet;
    m_GenericMatSet = NULL;

    // Release Sound.
    if (FireSound) SoundSystem->DeleteSound(FireSound);
    if (AltFireSound) SoundSystem->DeleteSound(AltFireSound);
}


bool CarriedWeaponShotgunT::ServerSide_PickedUpByEntity(IntrusivePtrT<cf::GameSys::ComponentHumanPlayerT> HumanPlayer) const
{
    // Consider if the entity already has this weapon.
    if (HumanPlayer->GetHaveWeapons() & (1 << WEAPON_SLOT_SHOTGUN))
    {
        // If it also has the max. amount of ammo of this type, ignore the touch.
        if (HumanPlayer->GetHaveAmmo()[AMMO_SLOT_SHELLS]==125) return false;

        // Otherwise pick the weapon up and let it have the ammo.
        HumanPlayer->GetHaveAmmo()[AMMO_SLOT_SHELLS]+=16;
    }
    else
    {
        // This weapon is picked up for the first time.
        HumanPlayer->SetHaveWeapons(HumanPlayer->GetHaveWeapons() | 1 << WEAPON_SLOT_SHOTGUN);
        HumanPlayer->SetActiveWeaponSlot(WEAPON_SLOT_SHOTGUN);
        HumanPlayer->SetActiveWeaponSequNr(6);    // Draw
        HumanPlayer->SetActiveWeaponFrameNr(0.0f);

        HumanPlayer->GetHaveAmmoInWeapons()[WEAPON_SLOT_SHOTGUN] =8;
        HumanPlayer->GetHaveAmmo()         [AMMO_SLOT_SHELLS   ]+=8;
    }

    // Limit the amount of carryable ammo.
    if (HumanPlayer->GetHaveAmmo()[AMMO_SLOT_SHELLS]>125) HumanPlayer->GetHaveAmmo()[AMMO_SLOT_SHELLS]=125;

    return true;
}


void CarriedWeaponShotgunT::ServerSide_Think(IntrusivePtrT<cf::GameSys::ComponentHumanPlayerT> HumanPlayer, const PlayerCommandT& PlayerCommand, bool ThinkingOnServerSide, bool AnimSequenceWrap) const
{
    switch (HumanPlayer->GetActiveWeaponSequNr())
    {
        case 3: // Reload / Insert shells
            if (AnimSequenceWrap)
            {
                HumanPlayer->GetHaveAmmoInWeapons()[WEAPON_SLOT_SHOTGUN]+=1;
                HumanPlayer->GetHaveAmmo()         [AMMO_SLOT_SHELLS   ]-=1;

                if (HumanPlayer->GetHaveAmmoInWeapons()[WEAPON_SLOT_SHOTGUN]>=8 || HumanPlayer->GetHaveAmmo()[AMMO_SLOT_SHELLS]==0)
                {
                    HumanPlayer->SetActiveWeaponSequNr(4);
                    HumanPlayer->SetActiveWeaponFrameNr(0.0f);
                }
            }
            break;

        case 4: // Pump / After reload
            if (AnimSequenceWrap)
            {
                HumanPlayer->SetActiveWeaponSequNr(0);
                HumanPlayer->SetActiveWeaponFrameNr(0.0f);
            }
            break;

        case 5: // Start reload
            if (AnimSequenceWrap)
            {
                HumanPlayer->SetActiveWeaponSequNr(3);
                HumanPlayer->SetActiveWeaponFrameNr(0.0f);
            }
            break;

        case 6: // Draw
            if (AnimSequenceWrap)
            {
                // Back to idle.
                HumanPlayer->SetActiveWeaponSequNr(0);
                HumanPlayer->SetActiveWeaponFrameNr(0.0f);
            }
            break;

        case 7: // Reholster
            break;

        case 1: // Shoot 1
            if (!AnimSequenceWrap) break;

            HumanPlayer->SetActiveWeaponSequNr(0);
            HumanPlayer->SetActiveWeaponFrameNr(0.0f);
            // Intentional fall-through.

        case 2: // Shoot 2
            if (!AnimSequenceWrap) break;

            HumanPlayer->SetActiveWeaponSequNr(0);
            HumanPlayer->SetActiveWeaponFrameNr(0.0f);
            // Intentional fall-through.

        case 0: // Idle
        case 8: // Idle4
        case 9: // Deep idle
        default:
            // 1. First see if the magazine is empty and special action is required.
            if (!HumanPlayer->GetHaveAmmoInWeapons()[WEAPON_SLOT_SHOTGUN])
            {
                if (HumanPlayer->GetHaveAmmo()[AMMO_SLOT_SHELLS])
                {
                    HumanPlayer->SetActiveWeaponSequNr(5);    // StartReload
                    HumanPlayer->SetActiveWeaponFrameNr(0.0f);
                    break;
                }

                if (PlayerCommand.Keys & (PCK_Fire1 | PCK_Fire2))
                {
                    // TODO: Player->PostEvent(EntHumanPlayerT::EVENT_TYPE_PRIMARY_FIRE_EMPTY);    // BUT LIMIT THE "FREQUENCY" OF THIS EVENT!
                    break;
                }
            }

            if (HumanPlayer->GetHaveAmmoInWeapons()[WEAPON_SLOT_SHOTGUN]==1 && (PlayerCommand.Keys & PCK_Fire2))
            {
                // TODO: Player->PostEvent(EntHumanPlayerT::EVENT_TYPE_SECONDARY_FIRE_EMPTY);      // BUT LIMIT THE "FREQUENCY" OF THIS EVENT!
                break;
            }

            // 2. Are we to fire single barreled (we have at least one shell)?
            if (PlayerCommand.Keys & PCK_Fire1)
            {
                HumanPlayer->SetActiveWeaponSequNr(1);                // Shoot 1
                HumanPlayer->SetActiveWeaponFrameNr(0.0f);
                HumanPlayer->GetHaveAmmoInWeapons()[WEAPON_SLOT_SHOTGUN]--;

                HumanPlayer->PostEvent(cf::GameSys::ComponentHumanPlayerT::EVENT_TYPE_PRIMARY_FIRE);

                if (ThinkingOnServerSide)
                {
                    // If we are on the server-side, find out what or who we hit.
                    for (char i=0; i<8; i++)
                    {
                        const Vector3dT  ViewDir = HumanPlayer->GetCameraViewDirWS(0.08748866);   // ca. 5°
                        const RayResultT RayResult(HumanPlayer->TraceCameraRay(ViewDir));

                        if (RayResult.hasHit() && RayResult.GetHitPhysicsComp())
                            HumanPlayer->InflictDamage(RayResult.GetHitPhysicsComp()->GetEntity(), 3.0f, ViewDir);
                    }
                }
                break;
            }

            // 3. Are we to fire double barreled (we have at least two shells)?
            if (PlayerCommand.Keys & PCK_Fire2)
            {
                HumanPlayer->SetActiveWeaponSequNr(2);            // Shoot 2
                HumanPlayer->SetActiveWeaponFrameNr(0.0f);
                HumanPlayer->GetHaveAmmoInWeapons()[WEAPON_SLOT_SHOTGUN]-=2;

                HumanPlayer->PostEvent(cf::GameSys::ComponentHumanPlayerT::EVENT_TYPE_SECONDARY_FIRE);

                if (ThinkingOnServerSide)
                {
                    // If we are on the server-side, find out what or who we hit.
                    for (char i=0; i<16; i++)
                    {
                        const Vector3dT  ViewDir = HumanPlayer->GetCameraViewDirWS(0.08748866);   // ca. 5°
                        const RayResultT RayResult(HumanPlayer->TraceCameraRay(ViewDir));

                        if (RayResult.hasHit() && RayResult.GetHitPhysicsComp()!=NULL)
                            HumanPlayer->InflictDamage(RayResult.GetHitPhysicsComp()->GetEntity(), 3.0f, ViewDir);
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

    Particle->Radius+=Time*40.0f;

    const unsigned long MatNr = (unsigned long)(Particle->Age * FPS);
    assert(MatNr < Particle->AllRMs->Size());
    Particle->RenderMat = (*Particle->AllRMs)[MatNr];

    Particle->Age+=Time;
    if (Particle->Age>=MaxAge) return false;

    return true;
}


void CarriedWeaponShotgunT::ClientSide_HandlePrimaryFireEvent(IntrusivePtrT<const cf::GameSys::ComponentHumanPlayerT> HumanPlayer, const VectorT& LastSeenAmbientColor) const
{
    for (char i=0; i<8; i++)
    {
        const Vector3dT  ViewDir = HumanPlayer->GetCameraViewDirWS(0.08748866);   // ca. 5°
        const RayResultT RayResult(HumanPlayer->TraceCameraRay(ViewDir));

        if (!RayResult.hasHit()) break;

        // Register a new particle at the 'Hit' point.
        ParticleMST NewParticle;

        NewParticle.Origin[0]=PhysToUnits(RayResult.m_hitPointWorld.x());
        NewParticle.Origin[1]=PhysToUnits(RayResult.m_hitPointWorld.y());
        NewParticle.Origin[2]=PhysToUnits(RayResult.m_hitPointWorld.z());

        NewParticle.Velocity[0]=0;
        NewParticle.Velocity[1]=0;
        NewParticle.Velocity[2]=0;

        NewParticle.Age=0.0;
        NewParticle.Color[3]=0;

        NewParticle.Radius=12.0;
        NewParticle.StretchY=1.0;
        NewParticle.AllRMs = NULL;
        NewParticle.RenderMat = m_GenericMatSet->GetRenderMats()[0];
        NewParticle.MoveFunction=RayResult.GetHitPhysicsComp()==NULL ? ParticleFunction_ShotgunHitWall : ParticleFunction_HitEntity;

        ParticleEngineMS::RegisterNewParticle(NewParticle);
    }

    const Vector3dT ViewDir = HumanPlayer->GetCameraViewDirWS();

    // Register a new particle as "muzzle flash".
    ParticleMST NewParticle;

    NewParticle.Origin[0]=float(HumanPlayer->GetCameraOriginWS().x + ViewDir.x*16.0);
    NewParticle.Origin[1]=float(HumanPlayer->GetCameraOriginWS().y + ViewDir.y*16.0);
    NewParticle.Origin[2]=float(HumanPlayer->GetCameraOriginWS().z + ViewDir.z*16.0-4.0);

    NewParticle.Velocity[0]=float(ViewDir.x*40.0);
    NewParticle.Velocity[1]=float(ViewDir.y*40.0);
    NewParticle.Velocity[2]=float(ViewDir.z*40.0);

    NewParticle.Age=0.0;
    NewParticle.Color[0]=char(LastSeenAmbientColor.x*255.0);
    NewParticle.Color[1]=char(LastSeenAmbientColor.y*255.0);
    NewParticle.Color[2]=char(LastSeenAmbientColor.z*255.0);
    NewParticle.Color[3]=100;

    NewParticle.Radius=3.2f;
    NewParticle.Rotation=char(rand());
    NewParticle.StretchY=1.0;
    NewParticle.AllRMs = &m_WhiteSmokeMatSet->GetRenderMats();
    NewParticle.RenderMat = m_WhiteSmokeMatSet->GetRenderMats()[0];
    NewParticle.MoveFunction=ParticleFunction_ShotgunWhiteSmoke;

    ParticleEngineMS::RegisterNewParticle(NewParticle);

    if (FireSound)
    {
        // Update sound position and velocity.
        FireSound->SetPosition(HumanPlayer->GetCameraOriginWS() + scale(ViewDir, 16.0));
        FireSound->SetVelocity(HumanPlayer->GetPlayerVelocity());

        // Play the fire sound.
        FireSound->Play();
    }
}


void CarriedWeaponShotgunT::ClientSide_HandleSecondaryFireEvent(IntrusivePtrT<const cf::GameSys::ComponentHumanPlayerT> HumanPlayer, const VectorT& LastSeenAmbientColor) const
{
    for (char i=0; i<16; i++)
    {
        const Vector3dT  ViewDir = HumanPlayer->GetCameraViewDirWS(0.08748866);   // ca. 5°
        const RayResultT RayResult(HumanPlayer->TraceCameraRay(ViewDir));

        if (!RayResult.hasHit()) break;

        // Register a new particle at the 'Hit' point.
        ParticleMST NewParticle;

        NewParticle.Origin[0]=PhysToUnits(RayResult.m_hitPointWorld.x());
        NewParticle.Origin[1]=PhysToUnits(RayResult.m_hitPointWorld.y());
        NewParticle.Origin[2]=PhysToUnits(RayResult.m_hitPointWorld.z());

        NewParticle.Velocity[0]=0;
        NewParticle.Velocity[1]=0;
        NewParticle.Velocity[2]=0;

        NewParticle.Age=0.0;
        NewParticle.Color[3]=0;

        NewParticle.Radius=12.0;
        NewParticle.StretchY=1.0;
        NewParticle.AllRMs = NULL;
        NewParticle.RenderMat = m_GenericMatSet->GetRenderMats()[0];
        NewParticle.MoveFunction=RayResult.GetHitPhysicsComp()==NULL ? ParticleFunction_ShotgunHitWall : ParticleFunction_HitEntity;

        ParticleEngineMS::RegisterNewParticle(NewParticle);
    }

    const Vector3dT ViewDir = HumanPlayer->GetCameraViewDirWS();

    // Register a new particle as "muzzle flash".
    ParticleMST NewParticle;

    NewParticle.Origin[0]=float(HumanPlayer->GetCameraOriginWS().x + ViewDir.x*16.0);
    NewParticle.Origin[1]=float(HumanPlayer->GetCameraOriginWS().y + ViewDir.y*16.0);
    NewParticle.Origin[2]=float(HumanPlayer->GetCameraOriginWS().z + ViewDir.z*16.0-4.0);

    NewParticle.Velocity[0]=float(ViewDir.x*60.0);
    NewParticle.Velocity[1]=float(ViewDir.y*60.0);
    NewParticle.Velocity[2]=float(ViewDir.z*60.0);

    NewParticle.Age=0.0;
    NewParticle.Color[0]=char(LastSeenAmbientColor.x*255.0);
    NewParticle.Color[1]=char(LastSeenAmbientColor.y*255.0);
    NewParticle.Color[2]=char(LastSeenAmbientColor.z*255.0);
    NewParticle.Color[3]=180;

    NewParticle.Radius=8.0;
    NewParticle.Rotation=char(rand());
    NewParticle.StretchY=1.0;
    NewParticle.AllRMs = &m_WhiteSmokeMatSet->GetRenderMats();
    NewParticle.RenderMat = m_WhiteSmokeMatSet->GetRenderMats()[0];
    NewParticle.MoveFunction=ParticleFunction_ShotgunWhiteSmoke;

    ParticleEngineMS::RegisterNewParticle(NewParticle);

    if (AltFireSound)
    {
        // Update sound position and velocity.
        AltFireSound->SetPosition(HumanPlayer->GetCameraOriginWS() + scale(ViewDir, 16.0));
        AltFireSound->SetVelocity(HumanPlayer->GetPlayerVelocity());

        // Play the fire sound.
        AltFireSound->Play();
    }
}