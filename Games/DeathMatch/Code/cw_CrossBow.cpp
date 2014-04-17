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

#include "cw_CrossBow.hpp"
#include "../../PlayerCommand.hpp"
#include "../../GameWorld.hpp"
#include "HumanPlayer.hpp"
#include "Constants_AmmoSlots.hpp"
#include "Constants_WeaponSlots.hpp"
#include "PhysicsWorld.hpp"
#include "Libs/LookupTables.hpp"
#include "GameSys/CompPhysics.hpp"
#include "Models/ModelManager.hpp"

using namespace GAME_NAME;


CarriedWeaponCrossBowT::CarriedWeaponCrossBowT(ModelManagerT& ModelMan)
    : CarriedWeaponT(ModelMan.GetModel("Games/DeathMatch/Models/Weapons/DartGun/DartGun_v.cmdl"),
                     ModelMan.GetModel("Games/DeathMatch/Models/Weapons/DartGun/DartGun_p.cmdl"))
{
}


bool CarriedWeaponCrossBowT::ServerSide_PickedUpByEntity(EntHumanPlayerT* Player, IntrusivePtrT<cf::GameSys::ComponentHumanPlayerT> HumanPlayer) const
{
    // Consider if the entity already has this weapon.
    if (HumanPlayer->GetHaveWeapons() & (1 << WEAPON_SLOT_CROSSBOW))
    {
        // If it also has the max. amount of ammo of this type, ignore the touch.
        if (HumanPlayer->GetHaveAmmo()[AMMO_SLOT_ARROWS]==30) return false;

        // Otherwise pick the weapon up and let it have the ammo.
        HumanPlayer->GetHaveAmmo()[AMMO_SLOT_ARROWS]+=10;
    }
    else
    {
        // This weapon is picked up for the first time.
        HumanPlayer->SetHaveWeapons(HumanPlayer->GetHaveWeapons() | 1 << WEAPON_SLOT_CROSSBOW);
        HumanPlayer->SetActiveWeaponSlot(WEAPON_SLOT_CROSSBOW);
        HumanPlayer->SetActiveWeaponSequNr(5);    // Draw
        HumanPlayer->SetActiveWeaponFrameNr(0.0f);

        HumanPlayer->GetHaveAmmoInWeapons()[WEAPON_SLOT_CROSSBOW] =5;
        HumanPlayer->GetHaveAmmo()         [AMMO_SLOT_ARROWS    ]+=5;
    }

    // Limit the amount of carryable ammo.
    if (HumanPlayer->GetHaveAmmo()[AMMO_SLOT_ARROWS]>30) HumanPlayer->GetHaveAmmo()[AMMO_SLOT_ARROWS]=30;

    return true;
}


void CarriedWeaponCrossBowT::ServerSide_Think(EntHumanPlayerT* Player, IntrusivePtrT<cf::GameSys::ComponentHumanPlayerT> HumanPlayer, const PlayerCommandT& PlayerCommand, bool ThinkingOnServerSide, unsigned long /*ServerFrameNr*/, bool AnimSequenceWrap) const
{
    switch (HumanPlayer->GetActiveWeaponSequNr())
    {
        case 0: // Idle1
        case 1: // Idle2
        case 2: // Idle3
            if (PlayerCommand.Keys & PCK_Fire1)
            {
                HumanPlayer->SetActiveWeaponSequNr(3);    // Fire
                HumanPlayer->SetActiveWeaponFrameNr(0.0f);

                if (ThinkingOnServerSide)
                {
                    // If we are on the server-side, find out what or who we hit.
                    const Vector3dT  ViewDir = HumanPlayer->GetViewDirWS();
                    const RayResultT RayResult(HumanPlayer->TracePlayerRay(ViewDir));

                    if (RayResult.hasHit() && RayResult.GetHitPhysicsComp())
                        HumanPlayer->InflictDamage(RayResult.GetHitPhysicsComp()->GetEntity(), 20.0f, ViewDir);
                }
                break;
            }

            if (AnimSequenceWrap)
            {
                if (HumanPlayer->GetActiveWeaponSequNr() == 2)
                {
                    HumanPlayer->SetActiveWeaponSequNr(LookupTables::RandomUShort[PlayerCommand.Nr & 0xFFF] % 3);
                }
                else HumanPlayer->SetActiveWeaponSequNr(2);    // Idle3 is the "best-looking" sequence.

                HumanPlayer->SetActiveWeaponFrameNr(0.0f);
            }
            break;

        case 3: // Fire
            if (AnimSequenceWrap)
            {
                HumanPlayer->SetActiveWeaponSequNr(4);
                HumanPlayer->SetActiveWeaponFrameNr(0.0f);
            }
            break;

        case 4: // Reload
            if (AnimSequenceWrap)
            {
                HumanPlayer->SetActiveWeaponSequNr(2);
                HumanPlayer->SetActiveWeaponFrameNr(0.0f);
            }
            break;

        case 5: // Draw (TakeOut)
            if (AnimSequenceWrap)
            {
                HumanPlayer->SetActiveWeaponSequNr(0);
                HumanPlayer->SetActiveWeaponFrameNr(0.0f);
            }
            break;

        case 6: // Holster (PutAway)
            break;
    }
}
