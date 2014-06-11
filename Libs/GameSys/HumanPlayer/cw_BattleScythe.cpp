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

#include "cw_BattleScythe.hpp"
#include "../../../Games/PlayerCommand.hpp"      // TODO: This file must be moved (and/or its contents completely redesigned).
#include "Constants_WeaponSlots.hpp"
#include "Models/ModelManager.hpp"
#include "SoundSystem/SoundSys.hpp"
#include "SoundSystem/SoundShaderManager.hpp"
#include "SoundSystem/Sound.hpp"

using namespace cf::GameSys;


CarriedWeaponBattleScytheT::CarriedWeaponBattleScytheT(ModelManagerT& ModelMan)
    : CarriedWeaponT(ModelMan.GetModel("Games/DeathMatch/Models/Weapons/BattleScythe/BattleScythe_v.cmdl"),
                     ModelMan.GetModel("Games/DeathMatch/Models/Weapons/BattleScythe/BattleScythe_p.cmdl")),
      FireSound(SoundSystem ? SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/BattleScythe")) : NULL)
{
    // At this time, in CaWE, the map compile tools, and the server(?), we operate with SoundSystem == NULL.
}


CarriedWeaponBattleScytheT::~CarriedWeaponBattleScytheT()
{
    // Release Sound.
    if (FireSound) SoundSystem->DeleteSound(FireSound);
}


bool CarriedWeaponBattleScytheT::ServerSide_PickedUpByEntity(IntrusivePtrT<cf::GameSys::ComponentHumanPlayerT> HumanPlayer) const
{
    // If the touching entity already has a BattleScythe, ignore the touch.
    if (HumanPlayer->GetHaveWeapons() & (1 << WEAPON_SLOT_BATTLESCYTHE)) return false;

    // Otherwise, give the touching entity this weapon.
    HumanPlayer->SetHaveWeapons(HumanPlayer->GetHaveWeapons() | 1 << WEAPON_SLOT_BATTLESCYTHE);
    HumanPlayer->SetActiveWeaponSlot(WEAPON_SLOT_BATTLESCYTHE);
    HumanPlayer->SetActiveWeaponSequNr(1);    // Draw
    HumanPlayer->SetActiveWeaponFrameNr(0.0f);

    return true;
}


void CarriedWeaponBattleScytheT::ServerSide_Think(IntrusivePtrT<cf::GameSys::ComponentHumanPlayerT> HumanPlayer, const PlayerCommandT& PlayerCommand, bool /*ThinkingOnServerSide*/, bool AnimSequenceWrap) const
{
    switch (HumanPlayer->GetActiveWeaponSequNr())
    {
        case 1: // Draw
            if (AnimSequenceWrap)
            {
                // Back to idle.
                HumanPlayer->SetActiveWeaponSequNr(0);
                HumanPlayer->SetActiveWeaponFrameNr(0.0f);
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
                    // HumanPlayer->PostEvent(cf::GameSys::ComponentHumanPlayerT::EVENT_TYPE_PRIMARY_FIRE);
                }
                else
                {
                    HumanPlayer->SetActiveWeaponSequNr(0);
                 // HumanPlayer->SetActiveWeaponFrameNr(0.0f);
                }
            }
            break;

        case 5: // Attack 2 (miss)
            // TODO: Reconsider if it will be a hit or miss!
            if (AnimSequenceWrap)
            {
                if (PlayerCommand.Keys & PCK_Fire2)
                {
                    // HumanPlayer->PostEvent(cf::GameSys::ComponentHumanPlayerT::EVENT_TYPE_SECONDARY_FIRE);
                }
                else
                {
                    HumanPlayer->SetActiveWeaponSequNr(0);
                 // HumanPlayer->SetActiveWeaponFrameNr(0.0f);
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
                HumanPlayer->SetActiveWeaponSequNr(4);    // Attack 1 (miss)
                HumanPlayer->SetActiveWeaponFrameNr(0.0f);
                // HumanPlayer->PostEvent(cf::GameSys::ComponentHumanPlayerT::EVENT_TYPE_PRIMARY_FIRE);
                break;
            }

            if (PlayerCommand.Keys & PCK_Fire2)
            {
                // TODO: Determine if this attack was a hit or miss.
                HumanPlayer->SetActiveWeaponSequNr(5);    // Attack 2 (miss)
                HumanPlayer->SetActiveWeaponFrameNr(0.0f);
                // HumanPlayer->PostEvent(cf::GameSys::ComponentHumanPlayerT::EVENT_TYPE_SECONDARY_FIRE);
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


void CarriedWeaponBattleScytheT::ClientSide_HandlePrimaryFireEvent(IntrusivePtrT<const cf::GameSys::ComponentHumanPlayerT> HumanPlayer, const VectorT& /*LastSeenAmbientColor*/) const
{
    const Vector3dT ViewDir = HumanPlayer->GetCameraViewDirWS();

    if (FireSound)
    {
        // Update sound position and velocity.
        FireSound->SetPosition(HumanPlayer->GetCameraOriginWS() + scale(ViewDir, 12.0));
        FireSound->SetVelocity(HumanPlayer->GetPlayerVelocity());

        // Play the fire sound.
        // FireSound->Play();
    }
}


void CarriedWeaponBattleScytheT::ClientSide_HandleSecondaryFireEvent(IntrusivePtrT<const cf::GameSys::ComponentHumanPlayerT> HumanPlayer, const VectorT& /*LastSeenAmbientColor*/) const
{
    const Vector3dT ViewDir = HumanPlayer->GetCameraViewDirWS();

    if (FireSound)
    {
        // Update sound position and velocity.
        FireSound->SetPosition(HumanPlayer->GetCameraOriginWS() + scale(ViewDir, 12.0));
        FireSound->SetVelocity(HumanPlayer->GetPlayerVelocity());

        // Play the fire sound.
        // FireSound->Play();
    }
}
