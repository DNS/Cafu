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
#include "UniScriptState.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GameSys;


const char* ComponentScriptT::DocClass =
    "This component adds a Lua script to its entity, implementing the entity's behaviour.";


const cf::TypeSys::VarsDocT ComponentScriptT::DocVars[] =
{
    { "Name", "The file name of the script." },
    { NULL, NULL }
};


namespace
{
    const char* FlagsIsLuaFileName[] = { "IsLuaFileName", NULL };
}


ComponentScriptT::ComponentScriptT()
    : ComponentBaseT(),
      m_FileName("Name", "", FlagsIsLuaFileName)
{
    GetMemberVars().Add(&m_FileName);
}


ComponentScriptT::ComponentScriptT(const ComponentScriptT& Comp)
    : ComponentBaseT(Comp),
      m_FileName(Comp.m_FileName)
{
    GetMemberVars().Add(&m_FileName);
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

    ScriptState.DoFile(m_FileName.Get().c_str(), "G", INT_GLOBAL);

    lua_pushnil(LuaState);
    lua_setglobal(LuaState, INT_GLOBAL);

    // CallLuaMethod("Init");
}


void ComponentScriptT::DoServerFrame(float t)
{
    CallLuaMethod("Think", "f", t);
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__toString",
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

const luaL_reg ComponentScriptT::MethodsList[] =
{
    { "__tostring", ComponentScriptT::toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentScriptT::DocMethods[] =
{
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentScriptT::TypeInfo(GetComponentTIM(), "ComponentScriptT", "ComponentBaseT", ComponentScriptT::CreateInstance, MethodsList, DocClass, DocMethods, DocVars);
