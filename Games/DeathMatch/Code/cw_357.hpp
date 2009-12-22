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

/****************************/
/*** Carried Weapon - 357 ***/
/****************************/

#ifndef _CW_357_HPP_
#define _CW_357_HPP_

#include "cw.hpp"
#include "SoundSystem/SoundSys.hpp"
#include "SoundSystem/SoundShaderManager.hpp"
#include "SoundSystem/Sound.hpp"


struct CarriedWeapon357T : public CarriedWeaponT
{
    CarriedWeapon357T()
        : FireSound(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/DesertEagle_Shot1")))
    {
    };

    ~CarriedWeapon357T()
    {
        // Release Sound.
        SoundSystem->DeleteSound(FireSound);
    }

    ModelProxyT& GetViewWeaponModel  () const;
    ModelProxyT& GetPlayerWeaponModel() const;

    bool ServerSide_PickedUpByEntity(BaseEntityT* Entity) const;
    void ServerSide_Think(EntHumanPlayerT* Player, const PlayerCommandT& PlayerCommand, bool ThinkingOnServerSide, unsigned long ServerFrameNr, bool AnimSequenceWrap) const;

    void ClientSide_HandlePrimaryFireEvent(const EntHumanPlayerT* Player, const VectorT& LastSeenAmbientColor) const;


    private:

    SoundI* FireSound;
};

#endif
