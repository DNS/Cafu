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

#include "cw_FaceHugger.hpp"
#include "FaceHugger.hpp"
#include "HumanPlayer.hpp"
#include "Constants_WeaponSlots.hpp"
#include "Libs/LookupTables.hpp"
#include "../../GameWorld.hpp"
#include "Models/ModelManager.hpp"
#include "ParticleEngine/ParticleEngineMS.hpp"
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


bool CarriedWeaponFaceHuggerT::ServerSide_PickedUpByEntity(EntHumanPlayerT* Player) const
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


void CarriedWeaponFaceHuggerT::ServerSide_Think(EntHumanPlayerT* Player, const PlayerCommandT& PlayerCommand, bool ThinkingOnServerSide, unsigned long ServerFrameNr, bool AnimSequenceWrap) const
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
                    // Clamp 'Pitch' values larger than 45° (==8192) to 45°.
                    const unsigned short Pitch=(Player->GetPitch()>8192 && Player->GetPitch()<=16384) ? 8192 : Player->GetPitch();

                    const float ViewDirZ=-LookupTables::Angle16ToSin[Pitch];
                    const float ViewDirY= LookupTables::Angle16ToCos[Pitch];

                    // Note: There is a non-trivial relationship between heading, pitch, and the corresponding view vector.
                    // Especially does a heading and pitch of 45° NOT correspond to the view vector (1, 1, 1), and vice versa!
                    // Think carefully about this before changing the number 1010.0 below (which actually is 2.0*(400.0+100.0) (+10.0 for "safety")).
                    const VectorT ViewDir(ViewDirY*LookupTables::Angle16ToSin[Player->GetHeading()], ViewDirY*LookupTables::Angle16ToCos[Player->GetHeading()], ViewDirZ);
                    const VectorT FaceHuggerOrigin(Player->GetOrigin()+scale(ViewDir, 1010.0)+scale(State.Velocity, double(PlayerCommand.FrameTime)));
                    std::map<std::string, std::string> Props;

                    Props["classname"]="monster_facehugger";

                    unsigned long FaceHuggerID=Player->GameWorld->CreateNewEntity(Props, ServerFrameNr, FaceHuggerOrigin);

                    if (FaceHuggerID!=0xFFFFFFFF)
                    {
                        IntrusivePtrT<EntFaceHuggerT> FaceHugger=dynamic_pointer_cast<EntFaceHuggerT>(Player->GameWorld->GetGameEntityByID(FaceHuggerID));

                        FaceHugger->ParentID=Player->ID;
                        FaceHugger->SetHeading(Player->GetHeading());
                        FaceHugger->SetVelocity(State.Velocity+scale(ViewDir, 7000.0));
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


void CarriedWeaponFaceHuggerT::ClientSide_HandlePrimaryFireEvent(const EntHumanPlayerT* Player, const VectorT& /*LastSeenAmbientColor*/) const
{
    const EntityStateT& State=Player->GetState();

    const float ViewDirZ=-LookupTables::Angle16ToSin[Player->GetPitch()];
    const float ViewDirY= LookupTables::Angle16ToCos[Player->GetPitch()];

    const VectorT ViewDir(ViewDirY*LookupTables::Angle16ToSin[Player->GetHeading()], ViewDirY*LookupTables::Angle16ToCos[Player->GetHeading()], ViewDirZ);

    // Update sound position and velocity.
    FireSound->SetPosition(Player->GetOrigin()+scale(ViewDir, 200.0));
    FireSound->SetVelocity(State.Velocity);

    // Play the fire sound.
    FireSound->Play();
}
