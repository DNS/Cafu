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

#include "VarVisitorsLua.hpp"

extern "C"
{
    #include <lua.h>
    #include <lauxlib.h>
}


using namespace cf::GuiSys;


/***************************/
/*** VarVisitorGetToLuaT ***/
/***************************/

VarVisitorGetToLuaT::VarVisitorGetToLuaT(lua_State* LuaState)
    : m_LuaState(LuaState),
      m_NumResults(0)
{
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarT<float>& Var)
{
    lua_pushnumber(m_LuaState, Var.Get());
    m_NumResults++;
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarT<double>& Var)
{
    lua_pushnumber(m_LuaState, Var.Get());
    m_NumResults++;
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarT<int>& Var)
{
    lua_pushinteger(m_LuaState, Var.Get());
    m_NumResults++;
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarT<std::string>& Var)
{
    lua_pushstring(m_LuaState, Var.Get().c_str());
    m_NumResults++;
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarT<Vector3fT>& Var)
{
    lua_pushnumber(m_LuaState, Var.Get().x); m_NumResults++;
    lua_pushnumber(m_LuaState, Var.Get().y); m_NumResults++;
    lua_pushnumber(m_LuaState, Var.Get().z); m_NumResults++;
}


/*****************************/
/*** VarVisitorSetFromLuaT ***/
/*****************************/

VarVisitorSetFromLuaT::VarVisitorSetFromLuaT(lua_State* LuaState)
    : m_LuaState(LuaState)
{
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<float>& Var)
{
    Var.Set(float(luaL_checknumber(m_LuaState, -1)));
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<double>& Var)
{
    Var.Set(luaL_checknumber(m_LuaState, -1));
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<int>& Var)
{
    Var.Set(luaL_checkint(m_LuaState, -1));
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<std::string>& Var)
{
    Var.Set(luaL_checkstring(m_LuaState, -1));
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<Vector3fT>& Var)
{
    Vector3fT v;

    v.x = float(luaL_checknumber(m_LuaState, -3));
    v.y = float(luaL_checknumber(m_LuaState, -2));
    v.z = float(luaL_checknumber(m_LuaState, -1));

    Var.Set(v);
}
