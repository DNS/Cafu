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

#include <cctype>


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


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarT<unsigned int>& Var)
{
    lua_pushinteger(m_LuaState, Var.Get());
    m_NumResults++;
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarT<std::string>& Var)
{
    lua_pushstring(m_LuaState, Var.Get().c_str());
    m_NumResults++;
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarT<Vector2fT>& Var)
{
    lua_pushnumber(m_LuaState, Var.Get().x); m_NumResults++;
    lua_pushnumber(m_LuaState, Var.Get().y); m_NumResults++;
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarT<Vector3fT>& Var)
{
    lua_pushnumber(m_LuaState, Var.Get().x); m_NumResults++;
    lua_pushnumber(m_LuaState, Var.Get().y); m_NumResults++;
    lua_pushnumber(m_LuaState, Var.Get().z); m_NumResults++;
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarT< ArrayT<std::string> >& Var)
{
    lua_newtable(m_LuaState);
    m_NumResults++;

    for (unsigned int i = 0; i < Var.Get().Size(); i++)
    {
        lua_pushstring(m_LuaState, Var.Get()[i].c_str());
        lua_rawseti(m_LuaState, -2, i + 1);   // Lua array numbering starts per convention at 1.
    }
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


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<unsigned int>& Var)
{
    Var.Set(luaL_checkint(m_LuaState, -1));
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<std::string>& Var)
{
    Var.Set(luaL_checkstring(m_LuaState, -1));
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<Vector2fT>& Var)
{
    Vector2fT v;

    v.x = float(luaL_checknumber(m_LuaState, -2));
    v.y = float(luaL_checknumber(m_LuaState, -1));

    Var.Set(v);
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<Vector3fT>& Var)
{
    Vector3fT v;

    v.x = float(luaL_checknumber(m_LuaState, -3));
    v.y = float(luaL_checknumber(m_LuaState, -2));
    v.z = float(luaL_checknumber(m_LuaState, -1));

    Var.Set(v);
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT< ArrayT<std::string> >& Var)
{
    ArrayT<std::string> A;

    if (lua_istable(m_LuaState, -1))
    {
        const int Num = lua_objlen(m_LuaState, -1);

        for (int i = 1; i <= Num; i++)
        {
            lua_rawgeti(m_LuaState, -1, i);
            const char* s = lua_tostring(m_LuaState, -1);
            A.PushBack(s ? s : "NULL");
            lua_pop(m_LuaState, 1);
        }
    }
    else
    {
        // Stack index 1 has the "this" object,
        // stack index 2 has the variable name.
        for (int i = 3; i <= lua_gettop(m_LuaState); i++)
        {
            const char* s = lua_tostring(m_LuaState, i);
            A.PushBack(s ? s : "NULL");
        }
    }

    Var.Set(A);
}


/***************************/
/*** VarVisitorSetFloatT ***/
/***************************/

VarVisitorSetFloatT::VarVisitorSetFloatT(unsigned int Suffix, float Value)
    : m_Suffix(Suffix),
      m_Value(Value)
{
}


void VarVisitorSetFloatT::visit(cf::TypeSys::VarT<float>& Var)
{
    Var.Set(m_Value);
}


void VarVisitorSetFloatT::visit(cf::TypeSys::VarT<Vector2fT>& Var)
{
    Vector2fT v = Var.Get();

    v[m_Suffix % 2] = m_Value;

    Var.Set(v);
}


void VarVisitorSetFloatT::visit(cf::TypeSys::VarT<Vector3fT>& Var)
{
    Vector3fT v = Var.Get();

    v[m_Suffix % 3] = m_Value;

    Var.Set(v);
}


void VarVisitorSetFloatT::visit(cf::TypeSys::VarT<double>& Var) { }
void VarVisitorSetFloatT::visit(cf::TypeSys::VarT<int>& Var) { }
void VarVisitorSetFloatT::visit(cf::TypeSys::VarT<unsigned int>& Var) { }
void VarVisitorSetFloatT::visit(cf::TypeSys::VarT<std::string>& Var) { }
void VarVisitorSetFloatT::visit(cf::TypeSys::VarT< ArrayT<std::string> >& Var) { }


/****************************/
/*** VarVisitorToLuaCodeT ***/
/****************************/

VarVisitorToLuaCodeT::VarVisitorToLuaCodeT(std::ostream& Out)
    : m_Out(Out)
{
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarT<float>& Var)
{
    m_Out << Var.Get();
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarT<double>& Var)
{
    m_Out << Var.Get();
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarT<int>& Var)
{
    m_Out << Var.Get();
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarT<unsigned int>& Var)
{
    m_Out << Var.Get();
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarT<std::string>& Var)
{
    WriteString(Var.Get());
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarT<Vector2fT>& Var)
{
    m_Out << Var.Get().x << ", " << Var.Get().y;
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarT<Vector3fT>& Var)
{
    m_Out << Var.Get().x << ", " << Var.Get().y << ", " << Var.Get().z;
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarT< ArrayT<std::string> >& Var)
{
    m_Out << "{ ";

    for (unsigned int i = 0; i < Var.Get().Size(); i++)
    {
        WriteString(Var.Get()[i]);
        if (i+1 < Var.Get().Size()) m_Out << ", ";
    }

    m_Out << " }";
}


void VarVisitorToLuaCodeT::WriteString(const std::string& s) const
{
    for (size_t i = 0; i < s.size(); i++)
        if (iscntrl(s[i]) || s[i] == '"' || s[i] == '\\')
        {
            std::string Equals = "";

            while (s.find("[" + Equals + "[") != std::string::npos ||
                   s.find("]" + Equals + "]") != std::string::npos)
                Equals += "=";

            m_Out << "[" << Equals << "[";
            m_Out << s;
            m_Out << "]" << Equals << "]";
            return;
        }

    m_Out << "\"" << s << "\"";
}
