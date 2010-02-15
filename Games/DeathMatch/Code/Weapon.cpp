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

/*********************/
/*** Weapon (Code) ***/
/*********************/

#include "Weapon.hpp"
#include "EntityCreateParams.hpp"
#include "PhysicsWorld.hpp"
#include "SoundSystem/SoundSys.hpp"
#include "SoundSystem/Sound.hpp"
#include "SoundSystem/SoundShaderManager.hpp"
#include "../../GameWorld.hpp"
#include "ConsoleCommands/Console.hpp"
#include "Models/Model_proxy.hpp"
#include "TypeSys.hpp"


// Implement the type info related code.
const cf::TypeSys::TypeInfoT* EntWeaponT::GetType() const
{
    return &TypeInfo;
 // return &EntWeaponT::TypeInfo;
}

void* EntWeaponT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    Console->Warning("Cannot instantiate abstract class!\n");
    assert(false);
    return NULL;
}

const cf::TypeSys::TypeInfoT EntWeaponT::TypeInfo(GetBaseEntTIM(), "EntWeaponT", "BaseEntityT", EntWeaponT::CreateInstance, NULL /*MethodsList*/);


const char EntWeaponT::StateOfExistance_Active   =0;
const char EntWeaponT::StateOfExistance_NotActive=1;


EntWeaponT::EntWeaponT(const EntityCreateParamsT& Params, const std::string& ModelName)
    : BaseEntityT(Params,
                  EntityStateT(Params.Origin,
                               VectorT(),
                               BoundingBox3T<double>(VectorT( 200.0,  200.0,  400.0),
                                                     VectorT(-200.0, -200.0, -100.0)),
                               0,                           // Heading
                               0,                           // Pitch
                               0,                           // Bank
                               StateOfExistance_Active,     // StateOfExistance
                               0,                           // Flags
                               0,                           // ModelIndex
                               0,                           // ModelSequNr
                               0.0,                         // ModelFrameNr
                               0,                           // Health
                               0,                           // Armor
                               0,                           // HaveItems
                               0,                           // HaveWeapons
                               0,                           // ActiveWeaponSlot
                               0,                           // ActiveWeaponSequNr
                               0.0)),                       // ActiveWeaponFrameNr
      WeaponModel(ModelName),
      TimeLeftNotActive(2.0),
      PickUp(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Item/PickUp"))),
      Respawn(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader("Item/Respawn")))
{
    // Drop weapons on ground. It doesn't look good when they hover in the air.
    // TODO: Do this only on the server side, it doesn't make sense for clients (where State.Origin==(0, 0, 0) here).
    RayResultT RayResult(NULL /*No object to ignore*/);
    PhysicsWorld->TraceRay((State.Origin+Vector3dT(0, 0, 200.0))/1000.0, VectorT(0.0, 0.0, -999999.0/1000.0), RayResult);

    if (RayResult.hasHit())
        State.Origin=convd(RayResult.m_hitPointWorld)*1000.0;
}


EntWeaponT::~EntWeaponT()
{
    // Release sounds.
    SoundSystem->DeleteSound(PickUp);
    SoundSystem->DeleteSound(Respawn);
}


void EntWeaponT::Think(float FrameTime, unsigned long /*ServerFrameNr*/)
{
    switch (State.StateOfExistance)
    {
        case StateOfExistance_NotActive:
            TimeLeftNotActive-=FrameTime;
            if (TimeLeftNotActive<=0.0)
            {
                State.StateOfExistance=StateOfExistance_Active;
                State.Events^=(1 << EventID_Respawn);
            }
            break;
    }
}


void EntWeaponT::ProcessEvent(char EventID) const
{
    switch (EventID)
    {
        case EventID_PickedUp:
            PickUp->Play();
            break;

        case EventID_Respawn:
            Respawn->Play();
            break;
    }
}


void EntWeaponT::Draw(bool /*FirstPersonView*/, float LodDist) const
{
    if (State.StateOfExistance==StateOfExistance_NotActive) return;

    WeaponModel.Draw(0, 0.0, LodDist);

    // RotAngle  +=  234.0*FrameTime; if (RotAngle>360.0) RotAngle-=360.0;
    // SwingAngle+=54321.0*FrameTime; // Wraps automatically!
}


void EntWeaponT::PostDraw(float FrameTime, bool FirstPersonView)
{
    // Set sound position to entity origin.
    PickUp ->SetPosition(State.Origin);
    Respawn->SetPosition(State.Origin);
}