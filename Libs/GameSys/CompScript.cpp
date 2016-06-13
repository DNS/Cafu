/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "CompScript.hpp"
#include "AllComponents.hpp"
#include "Entity.hpp"
#include "World.hpp"

#include "Network/State.hpp"
#include "UniScriptState.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GameSys;


const char* ComponentScriptT::DocClass =
    "This component runs custom Lua script code, implementing the behaviour of the entity in the game world.\n"
    "The script code can be loaded from a separate file, or it can be entered and kept directly in the component.\n"
    "\n"
    "Keeping the script code in a separate file is useful when it is re-used with several entity instances\n"
    "or in several maps.\n"
    "Keeping the script code directly in the component is useful for short scripts that are unique to a single\n"
    "map and entity instance.\n"
    "Note that both options can also be combined: The script code from a file is loaded first, and immediate\n"
    "code can be used to augment it (for example to \"configure\" it).\n";


const cf::TypeSys::VarsDocT ComponentScriptT::DocVars[] =
{
    { "Name",       "The file to load the Lua script code from." },
    { "ScriptCode", "Immediate Lua script code to use with this entity." },
    { NULL, NULL }
};


namespace
{
    const char* FlagsIsLuaFileName[] = { "IsLuaFileName", NULL };
    const char* FlagsIsLongString[]  = { "IsLongString", NULL };
}


ComponentScriptT::ComponentScriptT()
    : ComponentBaseT(),
      m_FileName("Name", "", FlagsIsLuaFileName),
      m_ScriptCode("ScriptCode", "", FlagsIsLongString),
      m_EventsCount(),
      m_EventsRef()
{
    GetMemberVars().Add(&m_FileName);
    GetMemberVars().Add(&m_ScriptCode);
}


ComponentScriptT::ComponentScriptT(const ComponentScriptT& Comp)
    : ComponentBaseT(Comp),
      m_FileName(Comp.m_FileName),
      m_ScriptCode(Comp.m_ScriptCode),
      m_EventsCount(),
      m_EventsRef()
{
    GetMemberVars().Add(&m_FileName);
    GetMemberVars().Add(&m_ScriptCode);
}


void ComponentScriptT::PostEvent(unsigned int EventType)
{
    // It is assumed that in the script (e.g. "HumanPlayer.lua"),
    // script method InitEventTypes() has been called.
    // See `PostEvent(lua_State* LuaState)` below for further details.
    m_EventsCount[EventType - 1]++;
}


ComponentScriptT* ComponentScriptT::Clone() const
{
    return new ComponentScriptT(*this);
}


void ComponentScriptT::OnPostLoad(bool OnlyStatic)
{
    if (OnlyStatic) return;
    if (!GetEntity()) return;

    cf::UniScriptStateT& ScriptState = GetEntity()->GetWorld().GetScriptState();
    lua_State*           LuaState    = ScriptState.GetLuaState();
    const char*          INT_GLOBAL  = "__CAFU_INTERNAL__";
    const StackCheckerT  StackChecker(LuaState);
    cf::ScriptBinderT    Binder(LuaState);

    // Using the INT_GLOBAL here is a trick to overcome the limitations of the parameter-passing to DoFile().
    //
    // Ideally, we would write
    //     ScriptState.DoFile(m_FileName.Get().c_str(), "O", IntrusivePtrT<ComponentScriptT>(this));
    // but implementing this properly (type safe) requires variadic templates.
    //
    // As an alternative, it would be possible to augment the interface of DoFile() to take a number of
    // "extra" arguments, very much like `UniScriptStateT::StartNewCoroutine(int NumExtraArgs, [...]` does
    // anyway.
    Binder.Push(IntrusivePtrT<ComponentScriptT>(this));
    lua_setglobal(LuaState, INT_GLOBAL);

    if (m_FileName.Get() != "")
        ScriptState.DoFile(m_FileName.Get().c_str(), "G", INT_GLOBAL);

    if (m_ScriptCode.Get() != "")
        ScriptState.DoString(m_ScriptCode.Get().c_str(), "G", INT_GLOBAL);

    lua_pushnil(LuaState);
    lua_setglobal(LuaState, INT_GLOBAL);

    // CallLuaMethod("Init", 0, "b", ClientOrServer);
}


void ComponentScriptT::DoSerialize(cf::Network::OutStreamT& Stream) const
{
    Stream << uint8_t(m_EventsCount.Size());

    for (unsigned int i = 0; i < m_EventsCount.Size(); i++)
        Stream << m_EventsCount[i];
}


void ComponentScriptT::DoDeserialize(cf::Network::InStreamT& Stream, bool IsIniting)
{
    uint8_t NUM_EVENT_TYPES = 0;
    Stream >> NUM_EVENT_TYPES;

    if (m_EventsCount.Size() != NUM_EVENT_TYPES)
    {
        // If this component was created in EntityT::Deserialize(), there is a chicken-and-egg situation:
        // Only the deserialization (in ComponentBaseT::Deserialize()) can set our m_FileName and m_ScriptCode
        // members so that we know which script code to load and run, but then we immediately get here before
        // that can happen. Thus, InitEventTypes() has not yet been called and thus the m_EventsCount and
        // m_EventsRef members not yet initialized.
        // As we still have to cope with the situation, we now have to initialize these members ourselves.
        m_EventsCount.Overwrite();
        m_EventsRef  .Overwrite();

        m_EventsCount.PushBackEmptyExact(NUM_EVENT_TYPES);
        m_EventsRef  .PushBackEmptyExact(NUM_EVENT_TYPES);

        for (unsigned int i = 0; i < NUM_EVENT_TYPES; i++)
        {
            m_EventsCount[i] = 0;
            m_EventsRef  [i] = 0;
        }
    }

    // Process events.
    // Note that events, as implemented here, are fully predictable:
    // they work well even in the presence of client prediction.
    for (unsigned int i = 0; i < m_EventsCount.Size(); i++)
    {
        Stream >> m_EventsCount[i];

        // Don't process the events if we got here as part of the
        // construction / first-time initialization of the entity.
        if (!IsIniting && m_EventsCount[i] > m_EventsRef[i])
        {
            const unsigned int EventType = i + 1;   // As per Lua convention.

            CallLuaMethod("ProcessEvent", 0, "ii", EventType, m_EventsCount[i] - m_EventsRef[i]);
        }

        m_EventsRef[i] = m_EventsCount[i];
    }
}


void ComponentScriptT::DoServerFrame(float t)
{
    CallLuaMethod("Think", 0, "f", t);
}


static const cf::TypeSys::MethsDocT META_InitEventTypes =
{
    "InitEventTypes",
    "This functions sets the number of event types that can be used with PostEvent().",
    "", "(number n)"
};

int ComponentScriptT::InitEventTypes(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentScriptT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentScriptT> >(1);

    const unsigned int NUM_EVENT_TYPES = unsigned(luaL_checkint(LuaState, 2));

    if (NUM_EVENT_TYPES < 1 || NUM_EVENT_TYPES > 8)    // If this is ever too less, simply increase it.
        luaL_argerror(LuaState, 2, "The number of event types must be an integer in the range from 1 to 8.");

    if (Comp->m_EventsCount.Size() == NUM_EVENT_TYPES)
        // On the client, DoDeserialize() may have had to initialize the event types in the same deserialization
        // step that set the m_FileName and m_ScriptCode, because it had no chance to load and run the script
        // code at this time, but still had to read the event data.
        // Therefore, when we get here because the script code is eventually load and run, and "retroactively"
        // attempts to do the same, just ignore the occurrence.
        return 0;

    if (Comp->m_EventsCount.Size() > 0)
        luaL_argerror(LuaState, 2, "The event counters have already been set (InitEventTypes() can only be called once).");

    Comp->m_EventsCount.PushBackEmptyExact(NUM_EVENT_TYPES);
    Comp->m_EventsRef  .PushBackEmptyExact(NUM_EVENT_TYPES);

    for (unsigned int i = 0; i < NUM_EVENT_TYPES; i++)
    {
        Comp->m_EventsCount[i] = 0;
        Comp->m_EventsRef  [i] = 0;
    }

    return 0;
}


static const cf::TypeSys::MethsDocT META_PostEvent =
{
    "PostEvent",
    "This function is used for posting an event of the given type.\n"
    "The event is automatically sent from the entity instance on the server to the entity instances\n"
    "on the clients, and causes a matching call to the ProcessEvent() callback there.\n"
    "The meaning of the event type is up to the script code that implements ProcessEvent().\n"
    "Note that events are fully predictable: they work well even in the presence of client prediction.",
    "", "(number EventType)"
};

int ComponentScriptT::PostEvent(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentScriptT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentScriptT> >(1);

    const unsigned int EventType = unsigned(luaL_checkint(LuaState, 2));

    if (EventType == 0)
        luaL_argerror(LuaState, 2, "Use event type numbers starting from 1 (not from 0).");

    if (EventType > Comp->m_EventsCount.Size())
        luaL_argerror(LuaState, 2, "Unknown event type. Did you use InitEventTypes()?");

    Comp->m_EventsCount[EventType - 1]++;
    return 0;
}


static const cf::TypeSys::MethsDocT META_DamageAll =
{
    "DamageAll",
    "Inflicts damage to nearby entities.\n"
    "\n"
    "This function finds all entities that are close to this (within a distance of `OuterRadius`,\n"
    "excluding this entity itself) and have a ComponentScriptT component.\n"
    "It then calls that script component's TakeDamage() method in order to apply the damage accordingly.\n"
    "\n"
    "@param Damage        The maximum damage to apply to nearby entities.\n"
    "@param InnerRadius   Entities that are closer than `InnerRaduis` are damaged by the full amount of `Damage`.\n"
    "@param OuterRadius   Entities that are farther than `OuterRadius` are not damaged at all.\n"
    "\n"
    "The damage that is inflicted to entities that are between `InnerRaduis` and `OuterRadius`\n"
    "is linearly scaled from `Damage` to 0. Entities must implement the TakeDamage() script callback\n"
    "method in order to actually process the inflicted damage.",
    "", "(number Damage, number InnerRadius, number OuterRadius)"
};

int ComponentScriptT::DamageAll(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentScriptT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentScriptT> >(1);

    const float DamageAmount = float(luaL_checknumber(LuaState, 2));
    const float InnerRadius  = float(luaL_checknumber(LuaState, 3));
    const float OuterRadius  = float(luaL_checknumber(LuaState, 4));

    if (!Comp->GetEntity())
        luaL_error(LuaState, "The component must be added to an entity before this function can be called.");

    IntrusivePtrT<EntityT> This = Comp->GetEntity();
    ArrayT< IntrusivePtrT<EntityT> > Entities;

    This->GetWorld().GetRootEntity()->GetAll(Entities);

    for (unsigned int EntNr = 0; EntNr < Entities.Size(); EntNr++)
    {
        IntrusivePtrT<EntityT>          OtherEnt    = Entities[EntNr];
        IntrusivePtrT<ComponentScriptT> OtherScript = dynamic_pointer_cast<ComponentScriptT>(OtherEnt->GetComponent("Script"));

        if (OtherEnt == This) continue;   // We don't damage us ourselves.
        if (OtherScript.IsNull()) continue;

        const Vector3fT Impact = OtherEnt->GetTransform()->GetOriginWS() - This->GetTransform()->GetOriginWS();
        const float     Dist   = length(Impact);

        if (Dist < 0.1f) continue;            // Should never happen.
        if (Dist >= OuterRadius) continue;    // Too far away.

        // TODO: In order to avoid damaging entities through thin walls, we should also perform a simple "visibility test".
        ScriptBinderT   OtherBinder(OtherEnt->GetWorld().GetScriptState().GetLuaState());
        const Vector3fT ImpDir = scale(Impact, 1.0f / Dist);

        OtherBinder.Push(This);
        OtherScript->CallLuaMethod("TakeDamage", 1, "ffff",
            DamageAmount * (1.0f - (std::max(InnerRadius, Dist) - InnerRadius)/(OuterRadius - InnerRadius)),
            ImpDir.x, ImpDir.y, ImpDir.z);
    }

    return 0;
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int ComponentScriptT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "script component");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentScriptT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentScriptT();
}

const luaL_Reg ComponentScriptT::MethodsList[] =
{
    { "InitEventTypes", InitEventTypes },
    { "PostEvent",      PostEvent },
    { "DamageAll",      DamageAll },
    { "__tostring",     toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentScriptT::DocMethods[] =
{
    META_InitEventTypes,
    META_PostEvent,
    META_DamageAll,
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentScriptT::DocCallbacks[] =
{
    { "OnActivate",
      "This method is called when another entity wants to prompt us to become active.\n"
      "Note that this method is usually not called directly from Cafu's C++ code, but rather\n"
      "from other script code, e.g. from GUIs whose button has been pressed.",
      "", "(EntityT Other)" },
    { "OnTrigger",
      "This method is called when another entity moves into this entity's trigger volume.",
      "", "(EntityT Other)" },
    { "ProcessEvent",
      "This method is called on the client in order to process and react to events.",
      "", "(int EventType, int EventCount)" },
    { "Think",
      "The server calls this method on each server clock tick, in order to advance the world\n"
      "to the next server frame.",
      "", "(number FrameTime)" },
    { "GetMove",
      "This method is called when there also is a ComponentMoverT component in the entity.\n"
      "The mover calls this method in order to learn which of its part to move where over\n"
      "the given frame time.",
      "tuple", "(int PartNr, number FrameTime)" },
    { "ChangeWeapon",
      "This method is called when the player has pressed a button to change the weapon.\n"
      "@param GroupNr   The number of the weapon group from which the next weapon is to be drawn.",
      "", "(int GroupNr)" },
    { "TakeDamage",
      "This method is called when another entity caused damage to this entity.",
      "", "(EntityT Other, number Amount, number DirX, number DirY, number DirZ)" },
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentScriptT::TypeInfo(
    GetComponentTIM(),
    "GameSys::ComponentScriptT",
    "GameSys::ComponentBaseT",
    ComponentScriptT::CreateInstance,
    MethodsList,
    DocClass,
    DocMethods,
    DocCallbacks,
    DocVars);
