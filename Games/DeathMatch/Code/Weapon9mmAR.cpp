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
/*** Weapon 9mm AR (Code) ***/
/****************************/

#include "Weapon9mmAR.hpp"
#include "cw.hpp"
#include "Constants_AmmoSlots.hpp"
#include "Constants_WeaponSlots.hpp"
#include "EntityCreateParams.hpp"
#include "GameImpl.hpp"
#include "HumanPlayer.hpp"
#include "TypeSys.hpp"


// Implement the type info related code.
const cf::TypeSys::TypeInfoT* EntWeapon9mmART::GetType() const
{
    return &TypeInfo;
 // return &EntWeapon9mmART::TypeInfo;
}

void* EntWeapon9mmART::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new EntWeapon9mmART(*static_cast<const EntityCreateParamsT*>(&Params));
}

const cf::TypeSys::TypeInfoT EntWeapon9mmART::TypeInfo(GetBaseEntTIM(), "EntWeapon9mmART", "EntWeaponT", EntWeapon9mmART::CreateInstance, NULL /*MethodsList*/);


EntWeapon9mmART::EntWeapon9mmART(const EntityCreateParamsT& Params)
    : EntWeaponT(Params, "Games/DeathMatch/Models/Weapons/9mmAR/9mmAR_w.cmdl")
{
}


void EntWeapon9mmART::NotifyTouchedBy(BaseEntityT* Entity)
{
    // If we are touched by anything else than a human player, ignore the touch.
    // Would be interesting to also allow touchs by bots, though.
    if (Entity->GetType()!=&EntHumanPlayerT::TypeInfo) return;

    // If we are touched when not being "active", ignore the touch.
    if (State.StateOfExistance!=StateOfExistance_Active) return;

    // Give this weapon to the entity.
    if (!cf::GameSys::GameImplT::GetInstance().GetCarriedWeapon(WEAPON_SLOT_9MMAR)->ServerSide_PickedUpByEntity(Entity)) return;

    // And finally retire for a while.
    State.StateOfExistance=StateOfExistance_NotActive;
    State.Events^=(1 << EventID_PickedUp);
    m_TimeLeftNotActive=5.0;
}
