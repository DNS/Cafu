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

#include "cw_9mmAR.hpp"
#include "../../PlayerCommand.hpp"
#include "_ResourceManager.hpp"
#include "Constants_AmmoSlots.hpp"
#include "Constants_WeaponSlots.hpp"
#include "PhysicsWorld.hpp"
#include "GameSys/CompLightPoint.hpp"
#include "GameSys/CompModel.hpp"
#include "GameSys/CompParticleSystemOld.hpp"
#include "GameSys/CompPhysics.hpp"
#include "GameSys/CompPlayerPhysics.hpp"
#include "GameSys/CompScript.hpp"
#include "GameSys/CompSound.hpp"
#include "GameSys/Entity.hpp"
#include "GameSys/EntityCreateParams.hpp"
#include "GameSys/World.hpp"
#include "Math3D/Angles.hpp"
#include "Models/ModelManager.hpp"
#include "ParticleEngine/ParticleEngineMS.hpp"
#include "SoundSystem/SoundSys.hpp"
#include "SoundSystem/SoundShaderManager.hpp"
#include "SoundSystem/Sound.hpp"

using namespace GAME_NAME;


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


bool CarriedWeapon9mmART::ServerSide_PickedUpByEntity(IntrusivePtrT<cf::GameSys::ComponentHumanPlayerT> HumanPlayer) const
{
    // Consider if the entity already has this weapon.
    if (HumanPlayer->GetHaveWeapons() & (1 << WEAPON_SLOT_9MMAR))
    {
        // If it also has the max. amount of ammo of this type, ignore the touch.
        if (HumanPlayer->GetHaveAmmo()[AMMO_SLOT_9MM]==250) return false;

        // Otherwise pick the weapon up and let it have the ammo.
        HumanPlayer->GetHaveAmmo()[AMMO_SLOT_9MM   ]+=50;
        HumanPlayer->GetHaveAmmo()[AMMO_SLOT_ARGREN]+= 2;               // Temp. solution until we have "ItemAmmoARGrenades" working.
    }
    else
    {
        // This weapon is picked up for the first time.
        HumanPlayer->SetHaveWeapons(HumanPlayer->GetHaveWeapons() | 1 << WEAPON_SLOT_9MMAR);
        HumanPlayer->SetActiveWeaponSlot(WEAPON_SLOT_9MMAR);
        HumanPlayer->SetActiveWeaponSequNr(4);    // Draw
        HumanPlayer->SetActiveWeaponFrameNr(0.0f);

        HumanPlayer->GetHaveAmmoInWeapons()[WEAPON_SLOT_9MMAR] =25;
        HumanPlayer->GetHaveAmmo()         [AMMO_SLOT_9MM    ]+=25;
        HumanPlayer->GetHaveAmmo()         [AMMO_SLOT_ARGREN ]+= 2;     // Temp. solution until we have "ItemAmmoARGrenades" working.
    }

    // Limit the amount of carryable ammo.
    if (HumanPlayer->GetHaveAmmo()[AMMO_SLOT_9MM   ]>250) HumanPlayer->GetHaveAmmo()[AMMO_SLOT_9MM   ]=250;
    if (HumanPlayer->GetHaveAmmo()[AMMO_SLOT_ARGREN]>  4) HumanPlayer->GetHaveAmmo()[AMMO_SLOT_ARGREN]=  4;

    return true;
}


void CarriedWeapon9mmART::ServerSide_Think(IntrusivePtrT<cf::GameSys::ComponentHumanPlayerT> HumanPlayer, const PlayerCommandT& PlayerCommand, bool ThinkingOnServerSide, unsigned long ServerFrameNr, bool AnimSequenceWrap) const
{
    switch (HumanPlayer->GetActiveWeaponSequNr())
    {
        case 3: // Reload
            if (AnimSequenceWrap)
            {
                HumanPlayer->SetActiveWeaponSequNr(0);
                HumanPlayer->SetActiveWeaponFrameNr(0.0f);

                const char Amount=HumanPlayer->GetHaveAmmo()[AMMO_SLOT_9MM]>25 ? 25 : HumanPlayer->GetHaveAmmo()[AMMO_SLOT_9MM];

                HumanPlayer->GetHaveAmmoInWeapons()[WEAPON_SLOT_9MMAR]+=Amount;
                HumanPlayer->GetHaveAmmo()         [AMMO_SLOT_9MM    ]-=Amount;
            }
            break;

        case 4: // Deploy (Draw)
            if (AnimSequenceWrap)
            {
                HumanPlayer->SetActiveWeaponSequNr(0);
                HumanPlayer->SetActiveWeaponFrameNr(0.0f);
            }
            break;

        case 2: // Grenade
            if (!AnimSequenceWrap) break;

            HumanPlayer->SetActiveWeaponSequNr(0);
            HumanPlayer->SetActiveWeaponFrameNr(0.0f);
            // Intentional fall-through.

        case 5: // Shoot 1
        case 6: // Shoot 2
        case 7: // Shoot 3
            if ((HumanPlayer->GetHaveAmmoInWeapons()[WEAPON_SLOT_9MMAR] && (PlayerCommand.Keys & PCK_Fire1) && HumanPlayer->GetActiveWeaponFrameNr() >= 1.0) || AnimSequenceWrap)
            {
                // At 10 FPS (sequences 5, 6, 7), 0.1 seconds correspond to exactly 1 frame.
                // End of this shoot sequence - go back to "idle" and recover from there (decide anew).
                HumanPlayer->SetActiveWeaponSequNr(0);
                HumanPlayer->SetActiveWeaponFrameNr(0.0f);
                // Intentional fall-through!
            }
            else break;

        case 0: // Long Idle
        case 1: // Idle1
            // 1. First see if the magazine is empty and special action is required.
            if (!HumanPlayer->GetHaveAmmoInWeapons()[WEAPON_SLOT_9MMAR])
            {
                if (HumanPlayer->GetHaveAmmo()[AMMO_SLOT_9MM])
                {
                    HumanPlayer->SetActiveWeaponSequNr(3);    // Reload
                    HumanPlayer->SetActiveWeaponFrameNr(0.0f);
                    break;
                }

                if (PlayerCommand.Keys & PCK_Fire1)
                {
                    // TODO: Player->PostEvent(EVENT_TYPE_PRIMARY_FIRE_EMPTY);     // BUT LIMIT THE "FREQUENCY" OF THIS EVENT!
                    break;
                }
            }

            if (!HumanPlayer->GetHaveAmmo()[AMMO_SLOT_ARGREN] && (PlayerCommand.Keys & PCK_Fire2))
            {
                // TODO: Player->PostEvent(EVENT_TYPE_SECONDARY_FIRE_EMPTY);     // BUT LIMIT THE "FREQUENCY" OF THIS EVENT!
                break;
            }

            if (PlayerCommand.Keys & PCK_Fire1)
            {
                HumanPlayer->SetActiveWeaponSequNr(5 + (rand() % 3));
                HumanPlayer->SetActiveWeaponFrameNr(0.0f);
                HumanPlayer->GetHaveAmmoInWeapons()[WEAPON_SLOT_9MMAR]--;

                if (ThinkingOnServerSide)
                {
                    // If we are on server-side, fire the first single shot, and find out what or who we hit.
                    const Vector3dT  ViewDir = HumanPlayer->GetViewDirWS(0.03492);  // ca. 2°
                    const RayResultT RayResult(HumanPlayer->TracePlayerRay(ViewDir));

                    if (RayResult.hasHit() && RayResult.GetHitPhysicsComp())
                        HumanPlayer->InflictDamage(RayResult.GetHitPhysicsComp()->GetEntity(), 1.0f, ViewDir);
                }
                break;
            }

            if (PlayerCommand.Keys & PCK_Fire2)
            {
                HumanPlayer->SetActiveWeaponSequNr(2);    // Grenade
                HumanPlayer->SetActiveWeaponFrameNr(0.0f);
                HumanPlayer->GetHaveAmmo()[AMMO_SLOT_ARGREN]--;

                HumanPlayer->PostEvent(cf::GameSys::ComponentHumanPlayerT::EVENT_TYPE_SECONDARY_FIRE);

                // Important: ONLY create (throw) a new AR grenade IF we are on the server side!
                if (ThinkingOnServerSide)
                {
                    const Vector3dT ViewDir = HumanPlayer->GetViewDirWS();
                    // TODO: Clamp ViewDir.y to max. 1.0 (then renormalize) ? That is, clamp 'Pitch' values larger than 45° (==8192) to 45°.

                    // Note: There is a non-trivial relationship between heading, pitch, and the corresponding view vector.
                    // Especially does a heading and pitch of 45° NOT correspond to the view vector (1, 1, 1), and vice versa!
                    // Think carefully about this before changing the number 42.0 below (which actually is 2.0*(16.0+4.5) (+1.0 for "safety")).
                    const VectorT ARGrenadeOrigin(HumanPlayer->GetOriginWS() - VectorT(0.0, 0.0, 10.0) + scale(ViewDir, 42.0) + scale(HumanPlayer->GetPlayerVelocity(), double(PlayerCommand.FrameTime)));

                    IntrusivePtrT<cf::GameSys::EntityT> Ent = new cf::GameSys::EntityT(cf::GameSys::EntityCreateParamsT(HumanPlayer->GetEntity()->GetWorld()));
                    HumanPlayer->GetEntity()->GetWorld().GetRootEntity()->AddChild(Ent);

                    Ent->GetBasics()->SetEntityName("ARGrenade");
                    Ent->GetTransform()->SetOriginWS(ARGrenadeOrigin.AsVectorOfFloat());
                    Ent->GetTransform()->SetQuatWS(HumanPlayer->GetEntity()->GetTransform()->GetQuatWS());

                    IntrusivePtrT<cf::GameSys::ComponentModelT> ModelComp = new cf::GameSys::ComponentModelT();
                    ModelComp->SetMember("Name", std::string("Games/DeathMatch/Models/Weapons/Grenade/Grenade_w.cmdl"));
                    Ent->AddComponent(ModelComp);

                    IntrusivePtrT<cf::GameSys::ComponentPlayerPhysicsT> PlayerPhysicsComp = new cf::GameSys::ComponentPlayerPhysicsT();
                    PlayerPhysicsComp->SetMember("Velocity", HumanPlayer->GetPlayerVelocity() + scale(ViewDir, 800.0));
                    PlayerPhysicsComp->SetMember("Dimensions", BoundingBox3dT(Vector3dT(3.0, 3.0, 6.0), Vector3dT(-3.0, -3.0, 0.0)));
                    Ent->AddComponent(PlayerPhysicsComp);

                    IntrusivePtrT<cf::GameSys::ComponentParticleSystemOldT> PaSysComp1 = new cf::GameSys::ComponentParticleSystemOldT();
                    PaSysComp1->SetMember("Type", std::string("ARGrenade_Expl_main"));
                    Ent->AddComponent(PaSysComp1);

                    IntrusivePtrT<cf::GameSys::ComponentParticleSystemOldT> PaSysComp2 = new cf::GameSys::ComponentParticleSystemOldT();
                    PaSysComp2->SetMember("Type", std::string("ARGrenade_Expl_sparkle"));
                    Ent->AddComponent(PaSysComp2);

                    IntrusivePtrT<cf::GameSys::ComponentPointLightT> LightComp = new cf::GameSys::ComponentPointLightT();
                    LightComp->SetMember("On", false);
                    LightComp->SetMember("Color", Vector3fT(1.0f, 1.0f, 1.0f));
                    LightComp->SetMember("Radius", 400.0f);
                    LightComp->SetMember("ShadowType", int(cf::GameSys::ComponentPointLightT::VarShadowTypeT::STENCIL));
                    Ent->AddComponent(LightComp);

                    IntrusivePtrT<cf::GameSys::ComponentSoundT> SoundComp = new cf::GameSys::ComponentSoundT();
                    SoundComp->SetMember("Name", std::string("Weapon/Shotgun_dBarrel"));
                    SoundComp->SetMember("AutoPlay", false);
                    Ent->AddComponent(SoundComp);

                    // Note that any post-load stuff is automatically run by the `CaServerWorldT` implementation.
                    IntrusivePtrT<cf::GameSys::ComponentScriptT> ScriptComp = new cf::GameSys::ComponentScriptT();
                    ScriptComp->SetMember("Name", std::string("Games/DeathMatch/Scripts/Grenade.lua"));
                    ScriptComp->SetMember("ScriptCode", std::string("local Grenade = ...\nGrenade.LightDuration = 0.8\n"));
                    Ent->AddComponent(ScriptComp);
                }
                break;
            }

            if (AnimSequenceWrap)
            {
                if (HumanPlayer->GetActiveWeaponSequNr() == 0)
                {
                    HumanPlayer->SetActiveWeaponSequNr(rand() & 1);
                }
                else HumanPlayer->SetActiveWeaponSequNr(0);     // Don't play the "Idle1" sequence repeatedly.

                HumanPlayer->SetActiveWeaponFrameNr(0.0f);
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


void CarriedWeapon9mmART::ClientSide_HandleSecondaryFireEvent(IntrusivePtrT<const cf::GameSys::ComponentHumanPlayerT> HumanPlayer, const VectorT& /*LastSeenAmbientColor*/) const
{
    const Vector3dT ViewDir = HumanPlayer->GetViewDirWS();

    // Update sound position and velocity.
    AltFireSound->SetPosition(HumanPlayer->GetOriginWS() + scale(ViewDir, 16.0));
    AltFireSound->SetVelocity(HumanPlayer->GetPlayerVelocity());

    // Play the fire sound.
    AltFireSound->Play();
}


void CarriedWeapon9mmART::ClientSide_HandleStateDrivenEffects(IntrusivePtrT<const cf::GameSys::ComponentHumanPlayerT> HumanPlayer) const
{
    if (HumanPlayer->GetActiveWeaponSequNr() == 5 || HumanPlayer->GetActiveWeaponSequNr() == 6 || HumanPlayer->GetActiveWeaponSequNr() == 7)
    {
        if (HumanPlayer->GetActiveWeaponFrameNr() == 0.0f)
        {
            const Vector3dT  ViewDir = HumanPlayer->GetViewDirWS(0.03492);  // ca. 2°
            const RayResultT RayResult(HumanPlayer->TracePlayerRay(ViewDir));

            if (!RayResult.hasHit()) return;

            // Register a new particle at the hit point.
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
            NewParticle.RenderMat=ResMan.RenderMats[ResMan.PARTICLE_GENERIC1];
            NewParticle.MoveFunction=RayResult.GetHitPhysicsComp()==NULL ? ParticleFunction_HitWall : ParticleFunction_HitEntity;

            ParticleEngineMS::RegisterNewParticle(NewParticle);

            // Update sound position and velocity.
            FireSound->SetPosition(HumanPlayer->GetOriginWS() + scale(ViewDir, 16.0));
            FireSound->SetVelocity(HumanPlayer->GetPlayerVelocity());

            // Play the fire sound.
            FireSound->Play();
        }
    }
}
