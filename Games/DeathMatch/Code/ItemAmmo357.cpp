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

/****************************/
/*** Item Ammo 357 (Code) ***/
/****************************/

#include "ItemAmmo357.hpp"
#include "Constants_AmmoSlots.hpp"
#include "EntityCreateParams.hpp"
#include "HumanPlayer.hpp"
#include "TypeSys.hpp"

using namespace GAME_NAME;


// Implement the type info related code.
const cf::TypeSys::TypeInfoT* EntItemAmmo357T::GetType() const
{
    return &TypeInfo;
 // return &EntItemAmmo357T::TypeInfo;
}

void* EntItemAmmo357T::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new EntItemAmmo357T(*static_cast<const EntityCreateParamsT*>(&Params));
}

const cf::TypeSys::TypeInfoT EntItemAmmo357T::TypeInfo(GetBaseEntTIM(), "EntItemAmmo357T", "EntItemT", EntItemAmmo357T::CreateInstance, NULL /*MethodsList*/);


EntItemAmmo357T::EntItemAmmo357T(const EntityCreateParamsT& Params)
    : EntItemT(Params, "Games/DeathMatch/Models/Items/Ammo_DesertEagle/Ammo_DesertEagle.cmdl")
{
}


void EntItemAmmo357T::NotifyTouchedBy(BaseEntityT* Entity)
{
    // If we are touched by anything else than a human player, ignore the touch.
    // Would be interesting to also allow touchs by bots, though.
    if (Entity->GetType()!=&EntHumanPlayerT::TypeInfo) return;

    // If we are touched when not being "active", ignore the touch.
    if (!IsActive()) return;

    EntityStateT& PlayerState=dynamic_cast<EntHumanPlayerT*>(Entity)->GetState();

    // If we already have the max. amount of ammo of this type, ignore the touch.
    if (PlayerState.HaveAmmo[AMMO_SLOT_357]==36) return;

    // Otherwise pick the item up and collect the ammo.
    PlayerState.HaveAmmo[AMMO_SLOT_357]+=6;

    // Limit the amount of carryable ammo.
    if (PlayerState.HaveAmmo[AMMO_SLOT_357]>36) PlayerState.HaveAmmo[AMMO_SLOT_357]=36;

    // And finally retire for a while.
    PostEvent(EVENT_TYPE_PICKED_UP);
    Deactivate(5.0f);
}
