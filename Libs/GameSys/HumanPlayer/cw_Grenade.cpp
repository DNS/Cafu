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

#include "cw_Grenade.hpp"
#include "../../../Games/PlayerCommand.hpp"      // TODO: This file must be moved (and/or its contents completely redesigned).
#include "Constants_WeaponSlots.hpp"
#include "GameSys/CompLightPoint.hpp"
#include "GameSys/CompModel.hpp"
#include "GameSys/CompParticleSystemOld.hpp"
#include "GameSys/CompPlayerPhysics.hpp"
#include "GameSys/CompScript.hpp"
#include "GameSys/CompSound.hpp"
#include "GameSys/Entity.hpp"
#include "GameSys/EntityCreateParams.hpp"
#include "GameSys/World.hpp"
#include "Math3D/Angles.hpp"
#include "Models/ModelManager.hpp"
#include "SoundSystem/SoundSys.hpp"
#include "SoundSystem/SoundShaderManager.hpp"
#include "SoundSystem/Sound.hpp"

using namespace cf::GameSys;


CarriedWeaponGrenadeT::CarriedWeaponGrenadeT(ModelManagerT& ModelMan)
    : CarriedWeaponT(ModelMan.GetModel("Games/DeathMatch/Models/Weapons/Grenade/Grenade_v.cmdl"),
                     ModelMan.GetModel("Games/DeathMatch/Models/Weapons/Grenade/Grenade_p.cmdl")),
      FireSound(SoundSystem ? SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/FaceHugger_Throw")) : NULL)
{
    // At this time, in CaWE, the map compile tools, and the server(?), we operate with SoundSystem == NULL.
}


CarriedWeaponGrenadeT::~CarriedWeaponGrenadeT()
{
    // Release Sound.
    if (FireSound) SoundSystem->DeleteSound(FireSound);
}


bool CarriedWeaponGrenadeT::ServerSide_PickedUpByEntity(IntrusivePtrT<cf::GameSys::ComponentHumanPlayerT> HumanPlayer) const
{
    // Consider if the entity already has this weapon.
    if (HumanPlayer->GetHaveWeapons() & (1 << WEAPON_SLOT_GRENADE))
    {
        // If it also has the max. amount of ammo of this type, ignore the touch.
        if (HumanPlayer->GetHaveAmmoInWeapons()[WEAPON_SLOT_GRENADE]==7) return false;

        // Otherwise pick the weapon up and let it have the ammo.
        HumanPlayer->GetHaveAmmoInWeapons()[WEAPON_SLOT_GRENADE]+=1;
    }
    else
    {
        // This weapon is picked up for the first time.
        HumanPlayer->SetHaveWeapons(HumanPlayer->GetHaveWeapons() | 1 << WEAPON_SLOT_GRENADE);
        HumanPlayer->SetActiveWeaponSlot(WEAPON_SLOT_GRENADE);
        HumanPlayer->SetActiveWeaponSequNr(7);    // Draw
        HumanPlayer->SetActiveWeaponFrameNr(0.0f);

        HumanPlayer->GetHaveAmmoInWeapons()[WEAPON_SLOT_GRENADE]=1;
    }

    return true;
}


void CarriedWeaponGrenadeT::ServerSide_Think(IntrusivePtrT<cf::GameSys::ComponentHumanPlayerT> HumanPlayer, const PlayerCommandT& PlayerCommand, bool ThinkingOnServerSide, bool AnimSequenceWrap) const
{
    switch (HumanPlayer->GetActiveWeaponSequNr())
    {
        case 0: // Idle
        case 1: // Fidget
            if (PlayerCommand.Keys & (PCK_Fire1 | PCK_Fire2))
            {
                HumanPlayer->SetActiveWeaponSequNr(2);    // PinPull
                HumanPlayer->SetActiveWeaponFrameNr(0.0f);
                break;
            }

            if (AnimSequenceWrap)
            {
                if (HumanPlayer->GetActiveWeaponSequNr() == 0)
                {
                    HumanPlayer->SetActiveWeaponSequNr(rand() & 1);
                }
                else HumanPlayer->SetActiveWeaponSequNr(0);     // Don't play the "Fidget" sequence repeatedly.

                HumanPlayer->SetActiveWeaponFrameNr(0.0f);
            }
            break;

        case 2: // PinPull
            if (AnimSequenceWrap)
            {
                HumanPlayer->SetActiveWeaponSequNr(3);
                HumanPlayer->SetActiveWeaponFrameNr(0.0f);

                HumanPlayer->PostEvent(cf::GameSys::ComponentHumanPlayerT::EVENT_TYPE_PRIMARY_FIRE);

                // Important: ONLY create (throw) a new hand grenade IF we are on the server side!
                if (ThinkingOnServerSide)
                {
                    const Vector3dT ViewDir = HumanPlayer->GetViewDirWS();
                    // TODO: Clamp ViewDir.y to max. 1.0 (then renormalize) ? That is, clamp 'Pitch' values larger than 45° (==8192) to 45°.

                    // Note: There is a non-trivial relationship between heading, pitch, and the corresponding view vector.
                    // Especially does a heading and pitch of 45° NOT correspond to the view vector (1, 1, 1), and vice versa!
                    // Think carefully about this before changing the number 42.0 below (which actually is 2.0*(16.0+4.5) (+1.0 for "safety")).
                    const VectorT HandGrenadeOrigin(HumanPlayer->GetCameraOriginWS() + VectorT(0.0, 0.0, 10.0)+scale(ViewDir, 42.0)+scale(HumanPlayer->GetPlayerVelocity(), double(PlayerCommand.FrameTime)));

                    IntrusivePtrT<cf::GameSys::EntityT> Ent = new cf::GameSys::EntityT(cf::GameSys::EntityCreateParamsT(HumanPlayer->GetEntity()->GetWorld()));
                    HumanPlayer->GetEntity()->GetWorld().GetRootEntity()->AddChild(Ent);

                    Ent->GetBasics()->SetEntityName("HandGrenade");
                    Ent->GetTransform()->SetOriginWS(HandGrenadeOrigin.AsVectorOfFloat());
                    Ent->GetTransform()->SetQuatWS(HumanPlayer->GetEntity()->GetTransform()->GetQuatWS());

                    IntrusivePtrT<cf::GameSys::ComponentModelT> ModelComp = new cf::GameSys::ComponentModelT();
                    ModelComp->SetMember("Name", std::string("Games/DeathMatch/Models/Weapons/Grenade/Grenade_w.cmdl"));
                    Ent->AddComponent(ModelComp);

                    IntrusivePtrT<cf::GameSys::ComponentPlayerPhysicsT> PlayerPhysicsComp = new cf::GameSys::ComponentPlayerPhysicsT();
                    PlayerPhysicsComp->SetMember("Velocity", HumanPlayer->GetPlayerVelocity() + scale(ViewDir, 400.0));
                    PlayerPhysicsComp->SetMember("Dimensions", BoundingBox3dT(Vector3dT(3.0, 3.0, 6.0), Vector3dT(-3.0, -3.0, 0.0)));
                    Ent->AddComponent(PlayerPhysicsComp);

                    IntrusivePtrT<cf::GameSys::ComponentParticleSystemOldT> PaSysComp1 = new cf::GameSys::ComponentParticleSystemOldT();
                    PaSysComp1->SetMember("Type", std::string("HandGrenade_Expl_main"));
                    Ent->AddComponent(PaSysComp1);

                    IntrusivePtrT<cf::GameSys::ComponentParticleSystemOldT> PaSysComp2 = new cf::GameSys::ComponentParticleSystemOldT();
                    PaSysComp2->SetMember("Type", std::string("HandGrenade_Expl_sparkle"));
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
                    ScriptComp->SetMember("ScriptCode", std::string("local Grenade = ...\nGrenade.LightDuration = 0.5\n"));
                    Ent->AddComponent(ScriptComp);
                }
            }
            break;

        case 3: // Throw1
        case 4: // Throw2
        case 5: // Throw3
            if (AnimSequenceWrap)
            {
                HumanPlayer->SetActiveWeaponSequNr(7);
                HumanPlayer->SetActiveWeaponFrameNr(0.0f);
            }
            break;

        case 6: // Holster
            break;

        case 7: // Draw
            if (AnimSequenceWrap)
            {
                HumanPlayer->SetActiveWeaponSequNr(0);
                HumanPlayer->SetActiveWeaponFrameNr(0.0f);
            }
            break;
    }
}


void CarriedWeaponGrenadeT::ClientSide_HandlePrimaryFireEvent(IntrusivePtrT<const cf::GameSys::ComponentHumanPlayerT> HumanPlayer, const VectorT& /*LastSeenAmbientColor*/) const
{
    const Vector3dT ViewDir = HumanPlayer->GetViewDirWS();

    if (FireSound)
    {
        // Update sound position and velocity.
        FireSound->SetPosition(HumanPlayer->GetCameraOriginWS() + scale(ViewDir, 8.0));
        FireSound->SetVelocity(HumanPlayer->GetPlayerVelocity());

        // Play the fire sound.
        FireSound->Play();
    }
}
