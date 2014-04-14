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

#include "cw_FaceHugger.hpp"
#include "../../PlayerCommand.hpp"
#include "../../GameWorld.hpp"
#include "HumanPlayer.hpp"
#include "Constants_WeaponSlots.hpp"
#include "Libs/LookupTables.hpp"
#include "GameSys/CompModel.hpp"
#include "GameSys/CompParticleSystemOld.hpp"
#include "GameSys/CompPlayerPhysics.hpp"
#include "GameSys/CompScript.hpp"
#include "Math3D/Angles.hpp"
#include "Models/ModelManager.hpp"
#include "SoundSystem/SoundSys.hpp"
#include "SoundSystem/SoundShaderManager.hpp"
#include "SoundSystem/Sound.hpp"

using namespace GAME_NAME;


CarriedWeaponFaceHuggerT::CarriedWeaponFaceHuggerT(ModelManagerT& ModelMan)
    : CarriedWeaponT(ModelMan.GetModel("Games/DeathMatch/Models/Weapons/FaceHugger/FaceHugger_v.cmdl"),
                     ModelMan.GetModel("Games/DeathMatch/Models/Weapons/FaceHugger/FaceHugger_p.cmdl")),
      FireSound(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/FaceHugger_Throw")))
{
}


CarriedWeaponFaceHuggerT::~CarriedWeaponFaceHuggerT()
{
    // Release Sound.
    SoundSystem->DeleteSound(FireSound);
}


bool CarriedWeaponFaceHuggerT::ServerSide_PickedUpByEntity(EntHumanPlayerT* Player, IntrusivePtrT<cf::GameSys::ComponentHumanPlayerT> HumanPlayer) const
{
    EntityStateT& State=Player->GetState();

    // Consider if the entity already has this weapon.
    if (State.HaveWeapons & (1 << WEAPON_SLOT_FACEHUGGER))
    {
        // If it also has the max. amount of ammo of this type, ignore the touch.
        if (State.HaveAmmoInWeapons[WEAPON_SLOT_FACEHUGGER]>=35) return false;

        // Otherwise pick the weapon up and let it have the ammo.
        State.HaveAmmoInWeapons[WEAPON_SLOT_FACEHUGGER]+=5;
    }
    else
    {
        // This weapon is picked up for the first time.
        State.HaveWeapons|=1 << WEAPON_SLOT_FACEHUGGER;
        State.ActiveWeaponSlot   =WEAPON_SLOT_FACEHUGGER;
        State.ActiveWeaponSequNr =4;    // Draw
        State.ActiveWeaponFrameNr=0.0;

        State.HaveAmmoInWeapons[WEAPON_SLOT_FACEHUGGER]=5;
    }

    return true;
}


void CarriedWeaponFaceHuggerT::ServerSide_Think(EntHumanPlayerT* Player, IntrusivePtrT<cf::GameSys::ComponentHumanPlayerT> HumanPlayer, const PlayerCommandT& PlayerCommand, bool ThinkingOnServerSide, unsigned long ServerFrameNr, bool AnimSequenceWrap) const
{
    EntityStateT& State=Player->GetState();

    switch (State.ActiveWeaponSequNr)
    {
        case 0: // Idle1
        case 1: // Idle2 (fidget fit)
        case 2: // Idle3 (fidget nip)
            if (PlayerCommand.Keys & (PCK_Fire1 | PCK_Fire2))
            {
                State.ActiveWeaponSequNr =5;
                State.ActiveWeaponFrameNr=0.0;

                Player->PostEvent(EntHumanPlayerT::EVENT_TYPE_PRIMARY_FIRE);

                // Important: ONLY create (throw) a new face-hugger IF we are on the server side!
                if (ThinkingOnServerSide)
                {
                    const Vector3dT ViewDir = HumanPlayer->GetViewDirWS();
                    // TODO: Clamp ViewDir.y to max. 1.0 (then renormalize) ? That is, clamp 'Pitch' values larger than 45° (==8192) to 45°.

                    // Note: There is a non-trivial relationship between heading, pitch, and the corresponding view vector.
                    // Especially does a heading and pitch of 45° NOT correspond to the view vector (1, 1, 1), and vice versa!
                    // Think carefully about this before changing the number 41.0 below (which actually is 2.0*(16.0+4.0) (+1.0 for "safety")).
                    const VectorT FaceHuggerOrigin(HumanPlayer->GetOriginWS() + scale(ViewDir, 41.0)+scale(HumanPlayer->GetPlayerVelocity(), double(PlayerCommand.FrameTime)));
                    std::map<std::string, std::string> Props;

                    Props["classname"]="monster_facehugger";

                    unsigned long FaceHuggerID=Player->GameWorld->CreateNewEntity(Props, ServerFrameNr, FaceHuggerOrigin);

                    if (FaceHuggerID!=0xFFFFFFFF)
                    {
                        IntrusivePtrT<BaseEntityT> FaceHugger = dynamic_pointer_cast<BaseEntityT>(Player->GameWorld->GetGameEntityByID(FaceHuggerID));
                        IntrusivePtrT<cf::GameSys::EntityT> Ent = FaceHugger->m_Entity;

                        Ent->GetTransform()->SetOriginWS(FaceHuggerOrigin.AsVectorOfFloat());
                        Ent->GetTransform()->SetQuatWS(Player->m_Entity->GetTransform()->GetQuatWS());

                        IntrusivePtrT<cf::GameSys::ComponentModelT> ModelComp = new cf::GameSys::ComponentModelT();
                        ModelComp->SetMember("Name", std::string("Games/DeathMatch/Models/LifeForms/FaceHugger/FaceHugger.cmdl"));
                        Ent->AddComponent(ModelComp);

                        IntrusivePtrT<cf::GameSys::ComponentPlayerPhysicsT> PlayerPhysicsComp = new cf::GameSys::ComponentPlayerPhysicsT();
                        PlayerPhysicsComp->SetMember("Velocity", HumanPlayer->GetPlayerVelocity() + scale(ViewDir, 280.0));
                        PlayerPhysicsComp->SetMember("Dimensions", BoundingBox3dT(Vector3dT( 4.0,  4.0, 4.0), Vector3dT(-4.0, -4.0, 0.0)));
                        Ent->AddComponent(PlayerPhysicsComp);

                        IntrusivePtrT<cf::GameSys::ComponentParticleSystemOldT> PaSysComp = new cf::GameSys::ComponentParticleSystemOldT();
                        PaSysComp->SetMember("Type", std::string("FaceHugger"));
                        Ent->AddComponent(PaSysComp);

                        IntrusivePtrT<cf::GameSys::ComponentScriptT> ScriptComp = new cf::GameSys::ComponentScriptT();
                        ScriptComp->SetMember("Name", std::string("Games/DeathMatch/Scripts/FaceHugger.lua"));
                        Ent->AddComponent(ScriptComp);

                        // As we're inserting a new entity into a live map, post-load stuff must be run here.
                        ScriptComp->OnPostLoad(false);
                        ScriptComp->CallLuaMethod("OnInit", 0);
                    }
                }
                break;
            }

            if (AnimSequenceWrap)
            {
                if (State.ActiveWeaponSequNr==0)
                {
                    const char RandomNumber=char(LookupTables::RandomUShort[PlayerCommand.Nr & 0xFFF]);

                         if (RandomNumber<32) State.ActiveWeaponSequNr=1;
                    else if (RandomNumber<64) State.ActiveWeaponSequNr=2;
                }
                else State.ActiveWeaponSequNr=0;    // Always play "Idle1" after "Idle2" or "Idle3"!

                State.ActiveWeaponFrameNr=0.0;
            }
            break;

        case 3: // Down (Holster)
            break;

        case 4: // Up (Draw)
            if (AnimSequenceWrap)
            {
                State.ActiveWeaponSequNr =0;
                State.ActiveWeaponFrameNr=0.0;
            }
            break;

        case 5: // Throw
            if (AnimSequenceWrap)
            {
                if (PlayerCommand.Keys & (PCK_Fire1 | PCK_Fire2))
                {
                    Player->PostEvent(EntHumanPlayerT::EVENT_TYPE_PRIMARY_FIRE);
                }
                else
                {
                    State.ActiveWeaponSequNr =0;
                    State.ActiveWeaponFrameNr=0.0;
                }
            }
            break;
    }
}


void CarriedWeaponFaceHuggerT::ClientSide_HandlePrimaryFireEvent(const EntHumanPlayerT* Player, IntrusivePtrT<const cf::GameSys::ComponentHumanPlayerT> HumanPlayer, const VectorT& /*LastSeenAmbientColor*/) const
{
    const Vector3dT ViewDir = HumanPlayer->GetViewDirWS();

    // Update sound position and velocity.
    IntrusivePtrT<const cf::GameSys::ComponentPlayerPhysicsT> CompPlayerPhysics = dynamic_pointer_cast<cf::GameSys::ComponentPlayerPhysicsT>(Player->m_Entity->GetComponent("PlayerPhysics"));

    FireSound->SetPosition(HumanPlayer->GetOriginWS() + scale(ViewDir, 8.0));
    FireSound->SetVelocity(CompPlayerPhysics->GetVelocity());

    // Play the fire sound.
    FireSound->Play();
}
