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

#include "cw_357.hpp"
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


CarriedWeapon357T::CarriedWeapon357T(ModelManagerT& ModelMan)
    : CarriedWeaponT(ModelMan.GetModel("Games/DeathMatch/Models/Weapons/DesertEagle/DesertEagle_v.cmdl"),
                     ModelMan.GetModel("Games/DeathMatch/Models/Weapons/DesertEagle/DesertEagle_p.cmdl")),
      FireSound(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/DesertEagle_Shot1"))),
      m_GenericMatSet(NULL)
{
    // TODO: Should rather store ParticleMaterialSetTs where their lifetime cannot be shorter than that of all particles...
    m_GenericMatSet = new ParticleMaterialSetT("generic", "Sprites/Generic1");
}


CarriedWeapon357T::~CarriedWeapon357T()
{
    delete m_GenericMatSet;
    m_GenericMatSet = NULL;

    // Release Sound.
    SoundSystem->DeleteSound(FireSound);
}


bool CarriedWeapon357T::ServerSide_PickedUpByEntity(IntrusivePtrT<cf::GameSys::ComponentHumanPlayerT> HumanPlayer) const
{
    // Consider if the entity already has this weapon.
    if (HumanPlayer->GetHaveWeapons() & (1 << WEAPON_SLOT_357))
    {
        // If it also has the max. amount of ammo of this type, ignore the touch.
        if (HumanPlayer->GetHaveAmmo()[AMMO_SLOT_357]==36) return false;

        // Otherwise pick the weapon up and let it have the ammo.
        HumanPlayer->GetHaveAmmo()[AMMO_SLOT_357]+=12;
    }
    else
    {
        // This weapon is picked up for the first time.
        HumanPlayer->SetHaveWeapons(HumanPlayer->GetHaveWeapons() | 1 << WEAPON_SLOT_357);
        HumanPlayer->SetActiveWeaponSlot(WEAPON_SLOT_357);
        HumanPlayer->SetActiveWeaponSequNr(5);    // Draw
        HumanPlayer->SetActiveWeaponFrameNr(0.0f);

        HumanPlayer->GetHaveAmmoInWeapons()[WEAPON_SLOT_357] =6;
        HumanPlayer->GetHaveAmmo()         [AMMO_SLOT_357  ]+=6;
    }

    // Limit the amount of carryable ammo.
    if (HumanPlayer->GetHaveAmmo()[AMMO_SLOT_357]>36) HumanPlayer->GetHaveAmmo()[AMMO_SLOT_357]=36;

    return true;
}


void CarriedWeapon357T::ServerSide_Think(IntrusivePtrT<cf::GameSys::ComponentHumanPlayerT> HumanPlayer, const PlayerCommandT& PlayerCommand, bool ThinkingOnServerSide, unsigned long /*ServerFrameNr*/, bool AnimSequenceWrap) const
{
    switch (HumanPlayer->GetActiveWeaponSequNr())
    {
        case 3: // Reload
            if (AnimSequenceWrap)
            {
                HumanPlayer->SetActiveWeaponSequNr(0);
                HumanPlayer->SetActiveWeaponFrameNr(0.0f);

                const char Amount=HumanPlayer->GetHaveAmmo()[AMMO_SLOT_357]>6 ? 6 : HumanPlayer->GetHaveAmmo()[AMMO_SLOT_357];

                HumanPlayer->GetHaveAmmoInWeapons()[WEAPON_SLOT_357]+=Amount;
                HumanPlayer->GetHaveAmmo()         [  AMMO_SLOT_357]-=Amount;
            }
            break;

        case 4: // Holster
            break;

        case 5: // Draw
            if (AnimSequenceWrap)
            {
                // Back to idle.
                HumanPlayer->SetActiveWeaponSequNr(0);
                HumanPlayer->SetActiveWeaponFrameNr(0.0f);
            }
            break;

        case 2: // Fire
            if (!AnimSequenceWrap) break;

            // Back to idle.
            HumanPlayer->SetActiveWeaponSequNr(0);
            HumanPlayer->SetActiveWeaponFrameNr(0.0f);
            // Intentional fall-through.

        case 0: // Idle 1
        case 6: // Idle 2
        case 7: // Idle 3
        case 1: // Fidget 1
        {
            // 1. First see if the magazine is empty and special action is required.
            if (!HumanPlayer->GetHaveAmmoInWeapons()[WEAPON_SLOT_357])
            {
                if (HumanPlayer->GetHaveAmmo()[AMMO_SLOT_357])
                {
                    HumanPlayer->SetActiveWeaponSequNr(3);    // Reload
                    HumanPlayer->SetActiveWeaponFrameNr(0.0f);
                    break;
                }

                if (PlayerCommand.Keys & PCK_Fire1)
                {
                    // TODO: State.Events^=(1 << EVENT_TYPE_PRIMARY_FIRE_EMPTY);     // BUT LIMIT THE "FREQUENCY" OF THIS EVENT!
                    break;
                }
            }

            // 2. Are we to fire a bullet (we have at least one)?
            if (PlayerCommand.Keys & PCK_Fire1)
            {
                HumanPlayer->SetActiveWeaponSequNr(2);                // Fire
                HumanPlayer->SetActiveWeaponFrameNr(0.0f);
                HumanPlayer->GetHaveAmmoInWeapons()[WEAPON_SLOT_357]--;

                HumanPlayer->PostEvent(cf::GameSys::ComponentHumanPlayerT::EVENT_TYPE_PRIMARY_FIRE);

                if (ThinkingOnServerSide)
                {
                    // If we are on the server-side, find out what or who we hit.
                    const Vector3dT  ViewDir = HumanPlayer->GetViewDirWS();
                    const RayResultT RayResult(HumanPlayer->TracePlayerRay(ViewDir));

                    if (RayResult.hasHit() && RayResult.GetHitPhysicsComp())
                        HumanPlayer->InflictDamage(RayResult.GetHitPhysicsComp()->GetEntity(), 7.0f, ViewDir);
                }
                break;
            }

            // 3. If nothing else has happened, just choose another sequence number on sequence wrap.
            if (AnimSequenceWrap)
            {
                const char RandomNumber = rand();

                     if (RandomNumber< 96) HumanPlayer->SetActiveWeaponSequNr(0);  // Idle 1
                else if (RandomNumber<192) HumanPlayer->SetActiveWeaponSequNr(6);  // Idle 2
                else if (RandomNumber<224) HumanPlayer->SetActiveWeaponSequNr(7);  // Idle 3
                else                       HumanPlayer->SetActiveWeaponSequNr(1);  // Fidget 1

                HumanPlayer->SetActiveWeaponFrameNr(0.0f);
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


void CarriedWeapon357T::ClientSide_HandlePrimaryFireEvent(IntrusivePtrT<const cf::GameSys::ComponentHumanPlayerT> HumanPlayer, const VectorT& /*LastSeenAmbientColor*/) const
{
    const Vector3dT  ViewDir = HumanPlayer->GetViewDirWS();
    const RayResultT RayResult(HumanPlayer->TracePlayerRay(ViewDir));

    if (!RayResult.hasHit()) return;


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
    NewParticle.MoveFunction=RayResult.GetHitPhysicsComp()==NULL ? ParticleFunction_HitWall : ParticleFunction_HitEntity;

    ParticleEngineMS::RegisterNewParticle(NewParticle);

    // Update sound position and velocity.
    FireSound->SetPosition(HumanPlayer->GetOriginWS() + scale(ViewDir, 16.0));
    FireSound->SetVelocity(HumanPlayer->GetPlayerVelocity());

    // Play the fire sound.
    FireSound->Play();
}
