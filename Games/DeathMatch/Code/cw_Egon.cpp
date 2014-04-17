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

#include "cw_Egon.hpp"
#include "../../PlayerCommand.hpp"
#include "HumanPlayer.hpp"
#include "Constants_AmmoSlots.hpp"
#include "Constants_WeaponSlots.hpp"
#include "Libs/LookupTables.hpp"
#include "Models/ModelManager.hpp"

using namespace GAME_NAME;


CarriedWeaponEgonT::CarriedWeaponEgonT(ModelManagerT& ModelMan)
    : CarriedWeaponT(ModelMan.GetModel("Games/DeathMatch/Models/Weapons/Egon/Egon_v.cmdl"),
                     ModelMan.GetModel("Games/DeathMatch/Models/Weapons/Egon/Egon_p.cmdl"))
{
}


bool CarriedWeaponEgonT::ServerSide_PickedUpByEntity(EntHumanPlayerT* Player, IntrusivePtrT<cf::GameSys::ComponentHumanPlayerT> HumanPlayer) const
{
    EntityStateT& State=Player->GetState();

    // Consider if the entity already has this weapon.
    if (HumanPlayer->GetHaveWeapons() & (1 << WEAPON_SLOT_EGON))
    {
        // If it also has the max. amount of ammo of this type, ignore the touch.
        if (State.HaveAmmo[AMMO_SLOT_CELLS]==200) return false;

        // Otherwise pick the weapon up and let it have the ammo.
        State.HaveAmmo[AMMO_SLOT_CELLS]+=40;
    }
    else
    {
        // This weapon is picked up for the first time.
        HumanPlayer->SetHaveWeapons(HumanPlayer->GetHaveWeapons() | 1 << WEAPON_SLOT_EGON);
        HumanPlayer->SetActiveWeaponSlot(WEAPON_SLOT_EGON);
        HumanPlayer->SetActiveWeaponSequNr(9);    // Draw
        HumanPlayer->SetActiveWeaponFrameNr(0.0f);

        State.HaveAmmoInWeapons[WEAPON_SLOT_EGON] =20;
        State.HaveAmmo         [AMMO_SLOT_CELLS ]+=20;
    }

    // Limit the amount of carryable ammo.
    if (State.HaveAmmo[AMMO_SLOT_CELLS]>200) State.HaveAmmo[AMMO_SLOT_CELLS]=200;

    return true;
}


void CarriedWeaponEgonT::ServerSide_Think(EntHumanPlayerT* Player, IntrusivePtrT<cf::GameSys::ComponentHumanPlayerT> HumanPlayer, const PlayerCommandT& PlayerCommand, bool /*ThinkingOnServerSide*/, unsigned long /*ServerFrameNr*/, bool AnimSequenceWrap) const
{
    EntityStateT& State=Player->GetState();

    enum SequenceNames
    {
        Idle,
        Fidget,
        AltFireOn,
        AltFireCycle,
        AltFireOff,
        Fire1,
        Fire2,
        Fire3,
        Fire4,
        Draw,
        Holster
    };

    switch (HumanPlayer->GetActiveWeaponSequNr())
    {
        case Draw:
            if (AnimSequenceWrap)
            {
                HumanPlayer->SetActiveWeaponSequNr(Idle);
                HumanPlayer->SetActiveWeaponFrameNr(0.0f);
            }
            break;

        case Idle:
        case Fidget:
            if (AnimSequenceWrap)
            {
                HumanPlayer->SetActiveWeaponSequNr(LookupTables::RandomUShort[PlayerCommand.Nr & 0xFFF] & 1);
                HumanPlayer->SetActiveWeaponFrameNr(0.0f);
            }
            break;
    }
}
