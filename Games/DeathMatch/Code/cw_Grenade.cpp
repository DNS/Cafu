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

#include "cw_Grenade.hpp"
#include "HandGrenade.hpp"
#include "HumanPlayer.hpp"
#include "Constants_WeaponSlots.hpp"
#include "Libs/LookupTables.hpp"
#include "../../GameWorld.hpp"
#include "Models/ModelManager.hpp"
#include "SoundSystem/SoundSys.hpp"
#include "SoundSystem/SoundShaderManager.hpp"
#include "SoundSystem/Sound.hpp"


CarriedWeaponGrenadeT::CarriedWeaponGrenadeT(ModelManagerT& ModelMan)
    : CarriedWeaponT(ModelMan.GetModel("Games/DeathMatch/Models/Weapons/Grenade/Grenade_v.cmdl"),
                     ModelMan.GetModel("Games/DeathMatch/Models/Weapons/Grenade/Grenade_p.cmdl")),
      FireSound(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Weapon/FaceHugger_Throw")))
{
}


CarriedWeaponGrenadeT::~CarriedWeaponGrenadeT()
{
    // Release Sound.
    SoundSystem->DeleteSound(FireSound);
}


bool CarriedWeaponGrenadeT::ServerSide_PickedUpByEntity(EntHumanPlayerT* Player) const
{
    EntityStateT& State=Player->GetState();

    // Consider if the entity already has this weapon.
    if (State.HaveWeapons & (1 << WEAPON_SLOT_GRENADE))
    {
        // If it also has the max. amount of ammo of this type, ignore the touch.
        if (State.HaveAmmoInWeapons[WEAPON_SLOT_GRENADE]==7) return false;

        // Otherwise pick the weapon up and let it have the ammo.
        State.HaveAmmoInWeapons[WEAPON_SLOT_GRENADE]+=1;
    }
    else
    {
        // This weapon is picked up for the first time.
        State.HaveWeapons|=1 << WEAPON_SLOT_GRENADE;
        State.ActiveWeaponSlot   =WEAPON_SLOT_GRENADE;
        State.ActiveWeaponSequNr =7;    // Draw
        State.ActiveWeaponFrameNr=0.0;

        State.HaveAmmoInWeapons[WEAPON_SLOT_GRENADE]=1;
    }

    return true;
}


void CarriedWeaponGrenadeT::ServerSide_Think(EntHumanPlayerT* Player, const PlayerCommandT& PlayerCommand, bool ThinkingOnServerSide, unsigned long ServerFrameNr, bool AnimSequenceWrap) const
{
    EntityStateT& State=Player->GetState();

    switch (State.ActiveWeaponSequNr)
    {
        case 0: // Idle
        case 1: // Fidget
            if (PlayerCommand.Keys & (PCK_Fire1 | PCK_Fire2))
            {
                State.ActiveWeaponSequNr =2;    // PinPull
                State.ActiveWeaponFrameNr=0.0;
                break;
            }

            if (AnimSequenceWrap)
            {
                if (State.ActiveWeaponSequNr==0)
                {
                    State.ActiveWeaponSequNr=LookupTables::RandomUShort[PlayerCommand.Nr & 0xFFF] & 1;
                }
                else State.ActiveWeaponSequNr=0;     // Don't play the "Fidget" sequence repeatedly.

                State.ActiveWeaponFrameNr=0.0;
            }
            break;

        case 2: // PinPull
            if (AnimSequenceWrap)
            {
                State.ActiveWeaponSequNr =3;
                State.ActiveWeaponFrameNr=0.0;

                Player->PostEvent(EntHumanPlayerT::EVENT_TYPE_PRIMARY_FIRE);

                // Important: ONLY create (throw) a new hand grenade IF we are on the server side!
                if (ThinkingOnServerSide)
                {
                    // Clamp 'Pitch' values larger than 45° (==8192) to 45°.
                    const unsigned short Pitch=(Player->GetPitch()>8192 && Player->GetPitch()<=16384) ? 8192 : Player->GetPitch();

                    const float ViewDirZ=-LookupTables::Angle16ToSin[Pitch];
                    const float ViewDirY= LookupTables::Angle16ToCos[Pitch];

                    // Note: There is a non-trivial relationship between heading, pitch, and the corresponding view vector.
                    // Especially does a heading and pitch of 45° NOT correspond to the view vector (1, 1, 1), and vice versa!
                    // Think carefully about this before changing the number 1050.0 below (which actually is 2.0*(400.0+120.0) (+10.0 for "safety")).
                    const VectorT ViewDir(ViewDirY*LookupTables::Angle16ToSin[Player->GetHeading()], ViewDirY*LookupTables::Angle16ToCos[Player->GetHeading()], ViewDirZ);
                    const VectorT HandGrenadeOrigin(Player->GetOrigin()+VectorT(0.0, 0.0, 250.0)+scale(ViewDir, 1050.0)+scale(State.Velocity, double(PlayerCommand.FrameTime)));
                    std::map<std::string, std::string> Props;

                    Props["classname"]="monster_handgrenade";

                    unsigned long HandGrenadeID=Player->GameWorld->CreateNewEntity(Props, ServerFrameNr, HandGrenadeOrigin);

                    if (HandGrenadeID!=0xFFFFFFFF)
                    {
                        IntrusivePtrT<EntHandGrenadeT> HandGrenade=dynamic_pointer_cast<EntHandGrenadeT>(Player->GameWorld->GetGameEntityByID(HandGrenadeID));

                        HandGrenade->ParentID=Player->ID;
                        HandGrenade->SetHeading(Player->GetHeading());
                        HandGrenade->SetVelocity(State.Velocity+scale(ViewDir, 10000.0));
                    }
                }
            }
            break;

        case 3: // Throw1
        case 4: // Throw2
        case 5: // Throw3
            if (AnimSequenceWrap)
            {
                State.ActiveWeaponSequNr =7;
                State.ActiveWeaponFrameNr=0.0;
            }
            break;

        case 6: // Holster
            break;

        case 7: // Draw
            if (AnimSequenceWrap)
            {
                State.ActiveWeaponSequNr =0;
                State.ActiveWeaponFrameNr=0.0;
            }
            break;
    }
}


void CarriedWeaponGrenadeT::ClientSide_HandlePrimaryFireEvent(const EntHumanPlayerT* Player, const VectorT& /*LastSeenAmbientColor*/) const
{
    const EntityStateT& State=Player->GetState();

    const float ViewDirZ=-LookupTables::Angle16ToSin[Player->GetPitch()];
    const float ViewDirY= LookupTables::Angle16ToCos[Player->GetPitch()];

    const VectorT ViewDir(ViewDirY*LookupTables::Angle16ToSin[Player->GetHeading()], ViewDirY*LookupTables::Angle16ToCos[Player->GetHeading()], ViewDirZ);

    // Update sound position and velocity.
    FireSound->SetPosition(Player->GetOrigin()+scale(ViewDir, 200.0));
    FireSound->SetVelocity(State.Velocity);

    // Play the fire sound.
    FireSound->Play();
}
