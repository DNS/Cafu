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

#include "cw_BattleScythe.hpp"
#include "HumanPlayer.hpp"
#include "Constants_WeaponSlots.hpp"
#include "Libs/LookupTables.hpp"
#include "Models/ModelManager.hpp"
#include "SoundSystem/SoundSys.hpp"
#include "SoundSystem/SoundShaderManager.hpp"
#include "SoundSystem/Sound.hpp"


CarriedWeaponBattleScytheT::CarriedWeaponBattleScytheT(ModelManagerT& ModelMan)
    : CarriedWeaponT(ModelMan.GetModel("Games/DeathMatch/Models/Weapons/BattleScythe/BattleScythe_v.cmdl"),
                     ModelMan.GetModel("Games/DeathMatch/Models/Weapons/BattleScythe/BattleScythe_p.cmdl")),
      FireSound(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/BattleScythe")))
{
}


CarriedWeaponBattleScytheT::~CarriedWeaponBattleScytheT()
{
    // Release Sound.
    SoundSystem->DeleteSound(FireSound);
}


bool CarriedWeaponBattleScytheT::ServerSide_PickedUpByEntity(EntHumanPlayerT* Player) const
{
    EntityStateT& State=Player->GetState();

    // If the touching entity already has a BattleScythe, ignore the touch.
    if (State.HaveWeapons & (1 << WEAPON_SLOT_BATTLESCYTHE)) return false;

    // Otherwise, give the touching entity this weapon.
    State.HaveWeapons|=1 << WEAPON_SLOT_BATTLESCYTHE;
    State.ActiveWeaponSlot   =WEAPON_SLOT_BATTLESCYTHE;
    State.ActiveWeaponSequNr =1;    // Draw
    State.ActiveWeaponFrameNr=0.0;

    return true;
}


void CarriedWeaponBattleScytheT::ServerSide_Think(EntHumanPlayerT* Player, const PlayerCommandT& PlayerCommand, bool /*ThinkingOnServerSide*/, unsigned long /*ServerFrameNr*/, bool AnimSequenceWrap) const
{
    EntityStateT& State=Player->GetState();

    switch (State.ActiveWeaponSequNr)
    {
        case 1: // Draw
            if (AnimSequenceWrap)
            {
                // Back to idle.
                State.ActiveWeaponSequNr =0;
                State.ActiveWeaponFrameNr=0.0;
            }
            break;

        case 2: // Holster
            break;

        case 3: // Attack 1 (hit)
            break;

        case 4: // Attack 1 (miss)
            // TODO: Reconsider if it will be a hit or miss!
            if (AnimSequenceWrap)
            {
                if (PlayerCommand.Keys & PCK_Fire1)
                {
                    Player->PostEvent(EntHumanPlayerT::EVENT_TYPE_PRIMARY_FIRE);
                }
                else
                {
                    State.ActiveWeaponSequNr =0;
                 // State.ActiveWeaponFrameNr=0.0;
                }
            }
            break;

        case 5: // Attack 2 (miss)
            // TODO: Reconsider if it will be a hit or miss!
            if (AnimSequenceWrap)
            {
                if (PlayerCommand.Keys & PCK_Fire2)
                {
                    Player->PostEvent(EntHumanPlayerT::EVENT_TYPE_SECONDARY_FIRE);
                }
                else
                {
                    State.ActiveWeaponSequNr =0;
                 // State.ActiveWeaponFrameNr=0.0;
                }
            }
            break;

        case 6: // Attack 2 (hit)
            break;

        case 7: // Attack 3 (miss)
            break;

        case 8: // Attack 3 (hit)
            break;

        case  0: // Idle
        case  9: // Idle 2
        case 10: // Idle 3
        default:
            if (PlayerCommand.Keys & PCK_Fire1)
            {
                // TODO: Determine if this attack was a hit or miss.
                // TODO: Alternate randomly with Attack 3.
                State.ActiveWeaponSequNr =4;    // Attack 1 (miss)
                State.ActiveWeaponFrameNr=0.0;
                Player->PostEvent(EntHumanPlayerT::EVENT_TYPE_PRIMARY_FIRE);
                break;
            }

            if (PlayerCommand.Keys & PCK_Fire2)
            {
                // TODO: Determine if this attack was a hit or miss.
                State.ActiveWeaponSequNr =5;    // Attack 2 (miss)
                State.ActiveWeaponFrameNr=0.0;
                Player->PostEvent(EntHumanPlayerT::EVENT_TYPE_SECONDARY_FIRE);
                break;
            }

            // if (AnimSequenceWrap)
            // {
            //     // No need to alternate randomly among 'Idle', 'Idle 2' and 'Idle 3'.
            //     // They are all identical anyway.
            // }
            break;
    }
}


void CarriedWeaponBattleScytheT::ClientSide_HandlePrimaryFireEvent(const EntHumanPlayerT* Player, const VectorT& /*LastSeenAmbientColor*/) const
{
    const EntityStateT& State=Player->GetState();

    const float ViewDirZ=-LookupTables::Angle16ToSin[Player->GetPitch()];
    const float ViewDirY= LookupTables::Angle16ToCos[Player->GetPitch()];

    const VectorT ViewDir(ViewDirY*LookupTables::Angle16ToSin[Player->GetHeading()], ViewDirY*LookupTables::Angle16ToCos[Player->GetHeading()], ViewDirZ);

    // Update sound position and velocity.
    FireSound->SetPosition(Player->GetOrigin()+scale(ViewDir, 300.0));
    FireSound->SetVelocity(State.Velocity);

    // Play the fire sound.
    FireSound->Play();
}


void CarriedWeaponBattleScytheT::ClientSide_HandleSecondaryFireEvent(const EntHumanPlayerT* Player, const VectorT& /*LastSeenAmbientColor*/) const
{
    const EntityStateT& State=Player->GetState();

    const float ViewDirZ=-LookupTables::Angle16ToSin[Player->GetPitch()];
    const float ViewDirY= LookupTables::Angle16ToCos[Player->GetPitch()];

    const VectorT ViewDir(ViewDirY*LookupTables::Angle16ToSin[Player->GetHeading()], ViewDirY*LookupTables::Angle16ToCos[Player->GetHeading()], ViewDirZ);

    // Update sound position and velocity.
    FireSound->SetPosition(Player->GetOrigin()+scale(ViewDir, 300.0));
    FireSound->SetVelocity(State.Velocity);

    // Play the fire sound.
    FireSound->Play();
}
