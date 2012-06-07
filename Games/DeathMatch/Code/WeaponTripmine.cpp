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

/******************************/
/*** Weapon Tripmine (Code) ***/
/******************************/

#include "WeaponTripmine.hpp"
#include "cw.hpp"
#include "Constants_AmmoSlots.hpp"
#include "Constants_WeaponSlots.hpp"
#include "EntityCreateParams.hpp"
#include "GameImpl.hpp"
#include "HumanPlayer.hpp"
#include "TypeSys.hpp"


// Implement the type info related code.
const cf::TypeSys::TypeInfoT* EntWeaponTripmineT::GetType() const
{
    return &TypeInfo;
 // return &EntWeaponTripmineT::TypeInfo;
}

void* EntWeaponTripmineT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new EntWeaponTripmineT(*static_cast<const EntityCreateParamsT*>(&Params));
}

const cf::TypeSys::TypeInfoT EntWeaponTripmineT::TypeInfo(GetBaseEntTIM(), "EntWeaponTripmineT", "EntWeaponT", EntWeaponTripmineT::CreateInstance, NULL /*MethodsList*/);


EntWeaponTripmineT::EntWeaponTripmineT(const EntityCreateParamsT& Params)
    : EntWeaponT(Params, "Games/DeathMatch/Models/Weapons/Tripmine/Tripmine_w.cmdl")
{
}


void EntWeaponTripmineT::NotifyTouchedBy(BaseEntityT* Entity)
{
    // If we are touched by anything else than a human player, ignore the touch.
    // Would be interesting to also allow touchs by bots, though.
    if (Entity->GetType()!=&EntHumanPlayerT::TypeInfo) return;

    // If we are touched when not being "active", ignore the touch.
    if (State.StateOfExistance!=StateOfExistance_Active) return;

    // Consider if the entity already has this weapon.
    if (Entity->State.HaveWeapons & (1 << WEAPON_SLOT_TRIPMINE))
    {
        // If it also has the max. amount of ammo of this type, ignore the touch.
        if (Entity->State.HaveAmmoInWeapons[WEAPON_SLOT_TRIPMINE]==4) return;

        // Otherwise pick the weapon up and let it have the ammo.
        Entity->State.HaveAmmoInWeapons[WEAPON_SLOT_TRIPMINE]+=1;
    }
    else
    {
        // This weapon is picked up for the first time.
        Entity->State.HaveWeapons|=1 << WEAPON_SLOT_TRIPMINE;
        Entity->State.ActiveWeaponSlot   =WEAPON_SLOT_TRIPMINE;
        Entity->State.ActiveWeaponSequNr =0;    // Draw
        Entity->State.ActiveWeaponFrameNr=0.0;

        Entity->State.HaveAmmoInWeapons[WEAPON_SLOT_TRIPMINE]=1;
    }

    // And finally retire for a while.
    State.StateOfExistance=StateOfExistance_NotActive;
    PostEvent(EVENT_TYPE_PICKED_UP);
    m_TimeLeftNotActive=5.0;
}
