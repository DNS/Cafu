/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2013 Carsten Fuchs Software.

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

#include "CompSound.hpp"
#include "AllComponents.hpp"
#include "Entity.hpp"

#include "SoundSystem/SoundSys.hpp"
#include "SoundSystem/Sound.hpp"
#include "SoundSystem/SoundShaderManager.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GameSys;


namespace
{
    const char* FlagsIsFileName[] = { "IsGenericFileName", NULL };
}


const char* ComponentSoundT::DocClass =
    "This component adds 3D sound output to its entity.";


const cf::TypeSys::VarsDocT ComponentSoundT::DocVars[] =
{
    { "Name",     "The name of the sound shader or sound file to play." },
    { "AutoPlay", "Whether the sound is played automatically in interval-spaced loops.\nIf `false`, playbacks of the sound must be triggered by explicit calls to the Play() method." },
    { "Interval", "If `m_AutoPlay` is `true`, this is the time in seconds between successive playbacks of the sound." },
    { NULL, NULL }
};


ComponentSoundT::ComponentSoundT()
    : ComponentBaseT(),
      m_Name("Name", "", FlagsIsFileName),
      m_AutoPlay("AutoPlay", false),
      m_Interval("Interval", 0.0f),
      m_PrevName(""),
      m_Sound(NULL),
      m_PauseLeft(0.0)
{
    GetMemberVars().Add(&m_Name);
    GetMemberVars().Add(&m_AutoPlay);
    GetMemberVars().Add(&m_Interval);
}


ComponentSoundT::ComponentSoundT(const ComponentSoundT& Comp)
    : ComponentBaseT(Comp),
      m_Name(Comp.m_Name),
      m_AutoPlay(Comp.m_AutoPlay),
      m_Interval(Comp.m_Interval),
      m_PrevName(""),
      m_Sound(NULL),
      m_PauseLeft(0.0)
{
    GetMemberVars().Add(&m_Name);
    GetMemberVars().Add(&m_AutoPlay);
    GetMemberVars().Add(&m_Interval);
}


ComponentSoundT::~ComponentSoundT()
{
    if (m_Sound)
    {
        // In CaWE and the map compile tools (and the server?), we may not have
        // a SoundSystem, but if we have an m_Sound, there must be one.
        assert(SoundSystem);

        SoundSystem->DeleteSound(m_Sound);
        m_Sound = NULL;
    }
}


ComponentSoundT* ComponentSoundT::Clone() const
{
    return new ComponentSoundT(*this);
}


void ComponentSoundT::DoClientFrame(float t)
{
    if (m_AutoPlay.Get())
    {
        SoundI*  S = GetSound();
        EntityT* E = GetEntity();

        if (S && E)
        {
            S->SetPosition(E->GetTransform()->GetOrigin().AsVectorOfDouble());
         // S->SetVelocity(...);
        }

        if (m_PauseLeft <= 0.0f)
        {
            if (S) S->Play();

            // TODO: Can we somehow make sure that m_PauseLeft is at least as long as the sound?
            m_PauseLeft = std::max(1.0f, m_Interval.Get());
        }
        else
        {
            m_PauseLeft -= t;
        }
    }
    else
    {
        m_PauseLeft = 0.0f;
    }
}


SoundI* ComponentSoundT::GetSound()
{
    if (m_Name.Get() == m_PrevName)
        return m_Sound;

    // At this time, in CaWE, we operate with SoundSystem == NULL.
    // Additionally, if an assert() triggers here (e.g. in an "on paint" event handler),
    // it seems like wxMSW cannot handle the situation gracefully: no assert dialog appears.
    // assert(SoundSystem);
    if (!SoundSystem)
    {
        assert(!m_Sound);

        m_PrevName = m_Name.Get();
        return m_Sound;
    }

    SoundSystem->DeleteSound(m_Sound);
    m_Sound = NULL;

    if (m_Name.Get() != "")
        m_Sound = SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader(m_Name.Get()));

    m_PrevName = m_Name.Get();
    return m_Sound;
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__toString",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int ComponentSoundT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "sound component");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentSoundT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentSoundT();
}

const luaL_reg ComponentSoundT::MethodsList[] =
{
    { "__tostring", ComponentSoundT::toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentSoundT::DocMethods[] =
{
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentSoundT::TypeInfo(GetComponentTIM(), "ComponentSoundT", "ComponentBaseT", ComponentSoundT::CreateInstance, MethodsList, DocClass, DocMethods, DocVars);
