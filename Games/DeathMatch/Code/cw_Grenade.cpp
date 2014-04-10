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
#include "../../PlayerCommand.hpp"
#include "../../GameWorld.hpp"
#include "HumanPlayer.hpp"
#include "Constants_WeaponSlots.hpp"
#include "Libs/LookupTables.hpp"
#include "GameSys/CompLightPoint.hpp"
#include "GameSys/CompModel.hpp"
#include "GameSys/CompParticleSystemOld.hpp"
#include "GameSys/CompPlayerPhysics.hpp"
#include "GameSys/CompScript.hpp"
#include "GameSys/CompSound.hpp"
#include "Math3D/Angles.hpp"
#include "Models/ModelManager.hpp"
#include "SoundSystem/SoundSys.hpp"
#include "SoundSystem/SoundShaderManager.hpp"
#include "SoundSystem/Sound.hpp"

using namespace GAME_NAME;


CarriedWeaponGrenadeT::CarriedWeaponGrenadeT(ModelManagerT& ModelMan)
    : CarriedWeaponT(ModelMan.GetModel("Games/DeathMatch/Models/Weapons/Grenade/Grenade_v.cmdl"),
                     ModelMan.GetModel("Games/DeathMatch/Models/Weapons/Grenade/Grenade_p.cmdl")),
      FireSound(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/FaceHugger_Throw")))
{
}


CarriedWeaponGrenadeT::~CarriedWeaponGrenadeT()
{
    // Release Sound.
    SoundSystem->DeleteSound(FireSound);
}


bool CarriedWeaponGrenadeT::ServerSide_PickedUpByEntity(EntHumanPlayerT* Player) const
{
    EntityStateT& State=Player->GetState();

    // Consider if the entity already has this weapon.
    if (State.HaveWeapons & (1 << WEAPON_SLOT_GRENADE))
    {
        // If it also has the max. amount of ammo of this type, ignore the touch.
        if (State.HaveAmmoInWeapons[WEAPON_SLOT_GRENADE]==7) return false;

        // Otherwise pick the weapon up and let it have the ammo.
        State.HaveAmmoInWeapons[WEAPON_SLOT_GRENADE]+=1;
    }
    else
    {
        // This weapon is picked up for the first time.
        State.HaveWeapons|=1 << WEAPON_SLOT_GRENADE;
        State.ActiveWeaponSlot   =WEAPON_SLOT_GRENADE;
        State.ActiveWeaponSequNr =7;    // Draw
        State.ActiveWeaponFrameNr=0.0;

        State.HaveAmmoInWeapons[WEAPON_SLOT_GRENADE]=1;
    }

    return true;
}


void CarriedWeaponGrenadeT::ServerSide_Think(EntHumanPlayerT* Player, const PlayerCommandT& PlayerCommand, bool ThinkingOnServerSide, unsigned long ServerFrameNr, bool AnimSequenceWrap) const
{
    EntityStateT& State=Player->GetState();

    switch (State.ActiveWeaponSequNr)
    {
        case 0: // Idle
        case 1: // Fidget
            if (PlayerCommand.Keys & (PCK_Fire1 | PCK_Fire2))
            {
                State.ActiveWeaponSequNr =2;    // PinPull
                State.ActiveWeaponFrameNr=0.0;
                break;
            }

            if (AnimSequenceWrap)
            {
                if (State.ActiveWeaponSequNr==0)
                {
                    State.ActiveWeaponSequNr=LookupTables::RandomUShort[PlayerCommand.Nr & 0xFFF] & 1;
                }
                else State.ActiveWeaponSequNr=0;     // Don't play the "Fidget" sequence repeatedly.

                State.ActiveWeaponFrameNr=0.0;
            }
            break;

        case 2: // PinPull
            if (AnimSequenceWrap)
            {
                State.ActiveWeaponSequNr =3;
                State.ActiveWeaponFrameNr=0.0;

                Player->PostEvent(EntHumanPlayerT::EVENT_TYPE_PRIMARY_FIRE);

                // Important: ONLY create (throw) a new hand grenade IF we are on the server side!
                if (ThinkingOnServerSide)
                {
                    IntrusivePtrT<const cf::GameSys::ComponentPlayerPhysicsT> CompPlayerPhysics = dynamic_pointer_cast<cf::GameSys::ComponentPlayerPhysicsT>(Player->m_Entity->GetComponent("PlayerPhysics"));
                    const Vector3dT ViewDir = Player->GetViewDir();
                    // TODO: Clamp ViewDir.y to max. 1.0 (then renormalize) ? That is, clamp 'Pitch' values larger than 45° (==8192) to 45°.

                    // Note: There is a non-trivial relationship between heading, pitch, and the corresponding view vector.
                    // Especially does a heading and pitch of 45° NOT correspond to the view vector (1, 1, 1), and vice versa!
                    // Think carefully about this before changing the number 42.0 below (which actually is 2.0*(16.0+4.5) (+1.0 for "safety")).
                    const VectorT HandGrenadeOrigin(Player->GetOrigin()+VectorT(0.0, 0.0, 10.0)+scale(ViewDir, 42.0)+scale(CompPlayerPhysics->GetVelocity(), double(PlayerCommand.FrameTime)));
                    std::map<std::string, std::string> Props;

                    Props["classname"]="monster_handgrenade";

                    unsigned long HandGrenadeID=Player->GameWorld->CreateNewEntity(Props, ServerFrameNr, HandGrenadeOrigin);

                    if (HandGrenadeID!=0xFFFFFFFF)
                    {
                        IntrusivePtrT<BaseEntityT> HandGrenade = dynamic_pointer_cast<BaseEntityT>(Player->GameWorld->GetGameEntityByID(HandGrenadeID));
                        IntrusivePtrT<cf::GameSys::EntityT> Ent = HandGrenade->m_Entity;

                        Ent->GetBasics()->SetEntityName("HandGrenade");
                        Ent->GetTransform()->SetOriginWS(HandGrenadeOrigin.AsVectorOfFloat());
                        Ent->GetTransform()->SetQuatWS(Player->m_Entity->GetTransform()->GetQuatWS());

                        IntrusivePtrT<cf::GameSys::ComponentModelT> ModelComp = new cf::GameSys::ComponentModelT();
                        ModelComp->SetMember("Name", std::string("Games/DeathMatch/Models/Weapons/Grenade/Grenade_w.cmdl"));
                        Ent->AddComponent(ModelComp);

                        IntrusivePtrT<cf::GameSys::ComponentPlayerPhysicsT> PlayerPhysicsComp = new cf::GameSys::ComponentPlayerPhysicsT();
                        PlayerPhysicsComp->SetMember("Velocity", CompPlayerPhysics->GetVelocity() + scale(ViewDir, 400.0));
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

                        IntrusivePtrT<cf::GameSys::ComponentScriptT> ScriptComp = new cf::GameSys::ComponentScriptT();
                        ScriptComp->SetMember("Name", std::string("Games/DeathMatch/Scripts/Grenade.lua"));
                        ScriptComp->SetMember("ScriptCode", std::string("local Grenade = ...\nGrenade.LightDuration = 0.5\n"));
                        Ent->AddComponent(ScriptComp);

                        // As we're inserting a new entity into a live map, post-load stuff must be run here.
                        ScriptComp->OnPostLoad(false);
                        ScriptComp->CallLuaMethod("OnInit", 0);
                    }
                }
            }
            break;

        case 3: // Throw1
        case 4: // Throw2
        case 5: // Throw3
            if (AnimSequenceWrap)
            {
                State.ActiveWeaponSequNr =7;
                State.ActiveWeaponFrameNr=0.0;
            }
            break;

        case 6: // Holster
            break;

        case 7: // Draw
            if (AnimSequenceWrap)
            {
                State.ActiveWeaponSequNr =0;
                State.ActiveWeaponFrameNr=0.0;
            }
            break;
    }
}


void CarriedWeaponGrenadeT::ClientSide_HandlePrimaryFireEvent(const EntHumanPlayerT* Player, const VectorT& /*LastSeenAmbientColor*/) const
{
    const EntityStateT& State   = Player->GetState();
    const Vector3dT     ViewDir = Player->GetViewDir();

    // Update sound position and velocity.
    IntrusivePtrT<const cf::GameSys::ComponentPlayerPhysicsT> CompPlayerPhysics = dynamic_pointer_cast<cf::GameSys::ComponentPlayerPhysicsT>(Player->m_Entity->GetComponent("PlayerPhysics"));

    FireSound->SetPosition(Player->GetOrigin()+scale(ViewDir, 8.0));
    FireSound->SetVelocity(CompPlayerPhysics->GetVelocity());

    // Play the fire sound.
    FireSound->Play();
}
