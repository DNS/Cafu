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

#include "CompBase.hpp"
#include "AllComponents.hpp"
#include "GuiImpl.hpp"
#include "VarVisitorsLua.hpp"
#include "Window.hpp"
#include "UniScriptState.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GuiSys;


void* ComponentBaseT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentBaseT();
}

const luaL_reg ComponentBaseT::MethodsList[] =
{
    { "get",             ComponentBaseT::Get },
    { "set",             ComponentBaseT::Set },
    { "GetExtraMessage", ComponentBaseT::GetExtraMessage },
    { "__tostring",      ComponentBaseT::toString },
    { NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentBaseT::TypeInfo(GetComponentTIM(), "ComponentBaseT", NULL /*No base class.*/, ComponentBaseT::CreateInstance, MethodsList);


ComponentBaseT::ComponentBaseT()
    : m_Window(NULL),
      m_MemberVars()
{
}


ComponentBaseT::ComponentBaseT(const ComponentBaseT& Comp)
    : m_Window(NULL),
      m_MemberVars()
{
}


ComponentBaseT* ComponentBaseT::Clone() const
{
    return new ComponentBaseT(*this);
}


bool ComponentBaseT::CallLuaMethod(const char* MethodName, const char* Signature, ...)
{
    va_list vl;

    va_start(vl, Signature);
    const bool Result = m_Window && m_Window->GetGui().GetScriptState().CallMethod(IntrusivePtrT<ComponentBaseT>(this), MethodName, Signature, vl);
    va_end(vl);

    return Result;
}


void ComponentBaseT::UpdateDependencies(WindowT* Window)
{
    m_Window = Window;
}


int ComponentBaseT::Get(lua_State* LuaState)
{
    ScriptBinderT                 Binder(LuaState);
    VarVisitorGetToLuaT           GetToLua(LuaState);
    IntrusivePtrT<ComponentBaseT> Comp    = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);
    const char*                   VarName = luaL_checkstring(LuaState, 2);
    const cf::TypeSys::VarBaseT*  Var     = Comp->m_MemberVars.Find(VarName);

    if (Var)
        Var->accept(GetToLua);

    return GetToLua.GetNumResults();
}


int ComponentBaseT::Set(lua_State* LuaState)
{
    ScriptBinderT                 Binder(LuaState);
    VarVisitorSetFromLuaT         SetFromLua(LuaState);
    IntrusivePtrT<ComponentBaseT> Comp    = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);
    const char*                   VarName = luaL_checkstring(LuaState, 2);
    cf::TypeSys::VarBaseT*        Var     = Comp->m_MemberVars.Find(VarName);

    if (Var)
        Var->accept(SetFromLua);

    return 0;
}


int ComponentBaseT::GetExtraMessage(lua_State* LuaState)
{
    ScriptBinderT                 Binder(LuaState);
    IntrusivePtrT<ComponentBaseT> Comp    = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);
    const char*                   VarName = luaL_checkstring(LuaState, 2);
    cf::TypeSys::VarBaseT*        Var     = Comp->m_MemberVars.Find(VarName);

    if (Var)
    {
        lua_pushstring(LuaState, Var->GetExtraMessage().c_str());
        return 1;
    }

    return 0;
}


int ComponentBaseT::toString(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "base component");
    return 1;
}
