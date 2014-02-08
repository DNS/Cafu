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


ComponentScriptT* ComponentScriptT::Clone() const
{
    return new ComponentScriptT(*this);
}


void ComponentScriptT::OnPostLoad(bool InEditor)
{
    if (InEditor) return;
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
    for (unsigned int i = 0; i < m_EventsCount.Size(); i++)
        Stream << m_EventsCount[i];
}


void ComponentScriptT::DoDeserialize(cf::Network::InStreamT& Stream, bool IsIniting)
{
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

    if (Comp->m_EventsCount.Size() > 0)
        luaL_argerror(LuaState, 2, "The event counters have already been set (SetEventTypes() can only be called once).");

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

        const Vector3fT Impact = OtherEnt->GetTransform()->GetOrigin() - This->GetTransform()->GetOrigin();
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

const cf::TypeSys::TypeInfoT ComponentScriptT::TypeInfo(GetComponentTIM(), "GameSys::ComponentScriptT", "GameSys::ComponentBaseT", ComponentScriptT::CreateInstance, MethodsList, DocClass, DocMethods, DocVars);
