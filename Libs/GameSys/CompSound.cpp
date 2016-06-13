/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "CompSound.hpp"
#include "AllComponents.hpp"
#include "Entity.hpp"

#include "SoundSystem/SoundSys.hpp"
#include "SoundSystem/Sound.hpp"
#include "SoundSystem/SoundShaderManager.hpp"
#include "UniScriptState.hpp"

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
    SoundI*  S = GetSound();
    EntityT* E = GetEntity();

    if (S && E)
    {
        S->SetPosition(E->GetTransform()->GetOriginWS().AsVectorOfDouble());
     // S->SetVelocity(...);
    }

    if (m_AutoPlay.Get())
    {
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
    {
        m_Sound = SoundSystem->CreateSound3D(SoundShaderManager->GetSoundShader(m_Name.Get()));

        if (GetEntity())
        {
            m_Sound->SetPosition(GetEntity()->GetTransform()->GetOriginWS().AsVectorOfDouble());
         // m_Sound->SetVelocity(...);
        }
    }

    m_PrevName = m_Name.Get();
    return m_Sound;
}


static const cf::TypeSys::MethsDocT META_Play =
{
    "Play",
    "This method plays the sound once.",
    "", "()"
};

int ComponentSoundT::Play(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentSoundT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentSoundT> >(1);
    SoundI* Sound = Comp->GetSound();

    if (Sound) Sound->Play();
    return 0;
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
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

const luaL_Reg ComponentSoundT::MethodsList[] =
{
    { "Play",       Play },
    { "__tostring", toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentSoundT::DocMethods[] =
{
    META_Play,
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentSoundT::TypeInfo(GetComponentTIM(), "GameSys::ComponentSoundT", "GameSys::ComponentBaseT", ComponentSoundT::CreateInstance, MethodsList, DocClass, DocMethods, NULL, DocVars);
