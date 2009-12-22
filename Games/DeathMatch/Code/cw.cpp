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

/**********************/
/*** Carried Weapon ***/
/**********************/

#include "cw.hpp"
#include "cw_357.hpp"
#include "cw_9mmAR.hpp"
#include "cw_BattleScythe.hpp"
#include "cw_CrossBow.hpp"
#include "cw_Egon.hpp"
#include "cw_FaceHugger.hpp"
#include "cw_Gauss.hpp"
#include "cw_Grenade.hpp"
#include "cw_Pistol.hpp"
#include "cw_RPG.hpp"
#include "cw_Shotgun.hpp"


const CarriedWeaponT* CarriedWeaponT::GetCarriedWeapon(char ActiveWeaponSlot)
{
    static CarriedWeapon357T          CarriedWeapon_NONE_DUMMY;
    static CarriedWeapon357T          CarriedWeapon357;
    static CarriedWeapon9mmART        CarriedWeapon9mmAR;
    static CarriedWeaponBattleScytheT CarriedWeaponBattleScythe;
    static CarriedWeaponCrossBowT     CarriedWeaponCrossBow;
    static CarriedWeaponEgonT         CarriedWeaponEgon;
    static CarriedWeaponFaceHuggerT   CarriedWeaponFaceHugger;
    static CarriedWeaponGaussT        CarriedWeaponGauss;
    static CarriedWeaponGrenadeT      CarriedWeaponGrenade;
    static CarriedWeaponPistolT       CarriedWeaponPistol;
    static CarriedWeaponRPGT          CarriedWeaponRPG;
    static CarriedWeaponShotgunT      CarriedWeaponShotgun;

    static CarriedWeaponT* CarriedWeaponPtrs[13]={ &CarriedWeaponBattleScythe,
                                                   &CarriedWeapon_NONE_DUMMY,
                                                   &CarriedWeaponPistol,
                                                   &CarriedWeapon357,
                                                   &CarriedWeaponShotgun,
                                                   &CarriedWeapon9mmAR,
                                                   &CarriedWeaponCrossBow,
                                                   &CarriedWeaponRPG,
                                                   &CarriedWeaponGauss,
                                                   &CarriedWeaponEgon,
                                                   &CarriedWeaponGrenade,
                                                   &CarriedWeapon_NONE_DUMMY,
                                                   &CarriedWeaponFaceHugger };

    return ActiveWeaponSlot<13 ? CarriedWeaponPtrs[ActiveWeaponSlot] : &CarriedWeapon_NONE_DUMMY;
}


bool CarriedWeaponT::ServerSide_PickedUpByEntity(BaseEntityT* /*Entity*/) const { return false; }
void CarriedWeaponT::ServerSide_Think(EntHumanPlayerT* /*Player*/, const PlayerCommandT& /*PlayerCommand*/, bool /*ThinkingOnServerSide*/, unsigned long /*ServerFrameNr*/, bool /*AnimSequenceWrap*/) const { }
void CarriedWeaponT::ClientSide_HandlePrimaryFireEvent(const EntHumanPlayerT* /*Player*/, const VectorT& /*LastSeenAmbientColor*/) const { }
void CarriedWeaponT::ClientSide_HandleSecondaryFireEvent(const EntHumanPlayerT* /*Player*/, const VectorT& /*LastSeenAmbientColor*/) const { }
void CarriedWeaponT::ClientSide_HandleStateDrivenEffects(const EntHumanPlayerT* /*Player*/) const { }
