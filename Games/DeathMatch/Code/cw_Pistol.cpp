/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

/*******************************/
/*** Carried Weapon - Pistol ***/
/*******************************/

#include "cw_Pistol.hpp"
#include "HumanPlayer.hpp"
#include "Constants_AmmoSlots.hpp"
#include "Constants_WeaponSlots.hpp"
#include "Models/Model_proxy.hpp"


ModelProxyT& CarriedWeaponPistolT::GetViewWeaponModel  () const { static ModelProxyT M("Games/DeathMatch/Models/Weapons/Beretta_v.mdl"); return M; }
ModelProxyT& CarriedWeaponPistolT::GetPlayerWeaponModel() const { static ModelProxyT M("Games/DeathMatch/Models/Weapons/Beretta_p.mdl"); return M; }


bool CarriedWeaponPistolT::ServerSide_PickedUpByEntity(BaseEntityT* Entity) const
{
    // Consider if the entity already has this weapon.
    if (Entity->State.HaveWeapons & (1 << WEAPON_SLOT_PISTOL))
    {
        // If it also has the max. amount of ammo of this type, ignore the touch.
        if (Entity->State.HaveAmmo[AMMO_SLOT_9MM]==250) return false;

        // Otherwise pick the weapon up and let it have the ammo.
        Entity->State.HaveAmmo[AMMO_SLOT_9MM]+=34;
    }
    else
    {
        // This weapon is picked up for the first time.
        Entity->State.HaveWeapons|=1 << WEAPON_SLOT_PISTOL;
        Entity->State.ActiveWeaponSlot   =WEAPON_SLOT_PISTOL;
        Entity->State.ActiveWeaponSequNr =7;    // Draw
        Entity->State.ActiveWeaponFrameNr=0.0;

        Entity->State.HaveAmmoInWeapons[WEAPON_SLOT_PISTOL] =17;
        Entity->State.HaveAmmo         [AMMO_SLOT_9MM     ]+=17;
    }

    // Limit the amount of carryable ammo.
    if (Entity->State.HaveAmmo[AMMO_SLOT_9MM]>250) Entity->State.HaveAmmo[AMMO_SLOT_9MM]=250;

    return true;
}
