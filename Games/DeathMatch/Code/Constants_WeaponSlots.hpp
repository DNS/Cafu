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

/********************/
/*** Weapon Slots ***/
/********************/

#ifndef CAFU_WEAPONSLOTS_HPP_INCLUDED
#define CAFU_WEAPONSLOTS_HPP_INCLUDED


namespace GAME_NAME
{
    // Bit numbers in State.HaveWeapons, indices into State.HaveAmmoInWeapons[], and for State.ActiveWeaponSlot.
    const char WEAPON_SLOT_BATTLESCYTHE= 0;
    const char WEAPON_SLOT_HORNETGUN   = 1;
    const char WEAPON_SLOT_PISTOL      = 2;
    const char WEAPON_SLOT_357         = 3;
    const char WEAPON_SLOT_SHOTGUN     = 4;
    const char WEAPON_SLOT_9MMAR       = 5;
    const char WEAPON_SLOT_CROSSBOW    = 6;
    const char WEAPON_SLOT_RPG         = 7;
    const char WEAPON_SLOT_GAUSS       = 8;
    const char WEAPON_SLOT_EGON        = 9;
    const char WEAPON_SLOT_GRENADE     =10;
    const char WEAPON_SLOT_TRIPMINE    =11;
    const char WEAPON_SLOT_FACEHUGGER  =12;
}

#endif
