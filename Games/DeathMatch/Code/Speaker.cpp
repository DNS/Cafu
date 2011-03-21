/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#include "Speaker.hpp"

#include "EntityCreateParams.hpp"
#include "TypeSys.hpp"
#include "ScriptState.hpp"
#include "Libs/LookupTables.hpp"

#include "SoundSystem/SoundSys.hpp"
#include "SoundSystem/Sound.hpp"
#include "SoundSystem/SoundShaderManager.hpp"

#include <iostream>

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}


// Implement the type info related code.
const cf::TypeSys::TypeInfoT* EntSpeakerT::GetType() const
{
    return &TypeInfo;
}

void* EntSpeakerT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new EntSpeakerT(*static_cast<const EntityCreateParamsT*>(&Params));
}

// Create method list for scripting.
const luaL_Reg EntSpeakerT::MethodsList[]=
{
    { "Play",  EntSpeakerT::Play },
    { "Stop",  EntSpeakerT::Stop },
    { NULL, NULL }
};

const cf::TypeSys::TypeInfoT EntSpeakerT::TypeInfo(GetBaseEntTIM(), "EntSpeakerT", "BaseEntityT", EntSpeakerT::CreateInstance, MethodsList);


EntSpeakerT::EntSpeakerT(const EntityCreateParamsT& Params)
    : BaseEntityT(Params,
                  EntityStateT(Params.Origin,
                               VectorT(),
                               BoundingBox3T<double>(VectorT(0.0, 0.0, 0.0),
                                                     VectorT(0.0, 0.0, 0.0)),
                               0,                           // Heading
                               0,                           // Pitch
                               0,                           // Bank
                               0,                           // StateOfExistance
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
                               0.0)),                       // ActiveWeaponFrameNr)
      m_Interval(GetProp("interval", 0.0f)),
      m_TimeUntilNextSound(m_Interval),
      m_Sound(SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader(GetProp("soundshader", ""))))
{
    State.Flags=GetProp("autoplay", 0); // Set initial playback state.

    m_Sound->SetInnerVolume   (GetProp("innerVolume",   0.5f));
    m_Sound->SetOuterVolume   (GetProp("outerVolume",   0.0f));
    m_Sound->SetInnerConeAngle(GetProp("innerCone",   360.0f));
    m_Sound->SetOuterConeAngle(GetProp("outerCone",   360.0f));
}


EntSpeakerT::~EntSpeakerT()
{
    SoundSystem->DeleteSound(m_Sound);
}


void EntSpeakerT::PostDraw(float FrameTime, bool FirstPersonView)
{
    m_Sound->SetPosition(State.Origin);

    const float ViewDirZ=-LookupTables::Angle16ToSin[State.Pitch];
    const float ViewDirY= LookupTables::Angle16ToCos[State.Pitch];

    const Vector3dT Direction(ViewDirY*LookupTables::Angle16ToSin[State.Heading], ViewDirY*LookupTables::Angle16ToCos[State.Heading], ViewDirZ);

    m_Sound->SetDirection(Direction);

    if (!State.Flags || m_Interval==0.0f)
    {
        m_TimeUntilNextSound=m_Interval;
        return;
    }

    m_TimeUntilNextSound-=FrameTime;
    if (m_TimeUntilNextSound<0.0f)
    {
        m_TimeUntilNextSound=m_Interval;
        if (m_Interval<=0.0f) State.Flags=0; // Only play this sound once per trigger if interval is 0.0f.
        m_Sound->Play();
    }
}


void EntSpeakerT::ProcessEvent(char EventID)
{
    if (EventID==EventID_Play)
        m_Sound->Play();

    if (EventID==EventID_Stop)
        m_Sound->Stop();
}


int EntSpeakerT::Play(lua_State* LuaState)
{
    EntSpeakerT* Ent=(EntSpeakerT*)cf::GameSys::ScriptStateT::GetCheckedObjectParam(LuaState, 1, TypeInfo);

    Ent->State.Flags=1;
    Ent->State.Events^=(1 << EventID_Play);

    return 0;
}


int EntSpeakerT::Stop(lua_State* LuaState)
{
    EntSpeakerT* Ent=(EntSpeakerT*)cf::GameSys::ScriptStateT::GetCheckedObjectParam(LuaState, 1, TypeInfo);

    Ent->State.Flags=0;
    Ent->State.Events^=(1 << EventID_Stop);

    return 0;
}
