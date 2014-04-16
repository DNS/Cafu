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

#include "VarVisitorsLua.hpp"

extern "C"
{
    #include <lua.h>
    #include <lauxlib.h>
}

#include <cctype>


using namespace cf::TypeSys;


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


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarT<uint16_t>& Var)
{
    lua_pushinteger(m_LuaState, Var.Get());
    m_NumResults++;
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarT<uint8_t>& Var)
{
    lua_pushinteger(m_LuaState, Var.Get());
    m_NumResults++;
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarT<bool>& Var)
{
    lua_pushboolean(m_LuaState, Var.Get());
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


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarT<Vector3dT>& Var)
{
    lua_pushnumber(m_LuaState, Var.Get().x); m_NumResults++;
    lua_pushnumber(m_LuaState, Var.Get().y); m_NumResults++;
    lua_pushnumber(m_LuaState, Var.Get().z); m_NumResults++;
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarT<BoundingBox3dT>& Var)
{
    lua_pushnumber(m_LuaState, Var.Get().Min.x); m_NumResults++;
    lua_pushnumber(m_LuaState, Var.Get().Min.y); m_NumResults++;
    lua_pushnumber(m_LuaState, Var.Get().Min.z); m_NumResults++;

    lua_pushnumber(m_LuaState, Var.Get().Max.x); m_NumResults++;
    lua_pushnumber(m_LuaState, Var.Get().Max.y); m_NumResults++;
    lua_pushnumber(m_LuaState, Var.Get().Max.z); m_NumResults++;
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarArrayT<uint32_t>& Var)
{
    lua_newtable(m_LuaState);
    m_NumResults++;

    for (unsigned int i = 0; i < Var.Size(); i++)
    {
        lua_pushinteger(m_LuaState, Var[i]);
        lua_rawseti(m_LuaState, -2, i + 1);   // Lua array numbering starts per convention at 1.
    }
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarArrayT<uint16_t>& Var)
{
    lua_newtable(m_LuaState);
    m_NumResults++;

    for (unsigned int i = 0; i < Var.Size(); i++)
    {
        lua_pushinteger(m_LuaState, Var[i]);
        lua_rawseti(m_LuaState, -2, i + 1);   // Lua array numbering starts per convention at 1.
    }
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarArrayT<uint8_t>& Var)
{
    lua_newtable(m_LuaState);
    m_NumResults++;

    for (unsigned int i = 0; i < Var.Size(); i++)
    {
        lua_pushinteger(m_LuaState, Var[i]);
        lua_rawseti(m_LuaState, -2, i + 1);   // Lua array numbering starts per convention at 1.
    }
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarArrayT<std::string>& Var)
{
    lua_newtable(m_LuaState);
    m_NumResults++;

    for (unsigned int i = 0; i < Var.Size(); i++)
    {
        lua_pushstring(m_LuaState, Var[i].c_str());
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


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<uint16_t>& Var)
{
    Var.Set(luaL_checkint(m_LuaState, -1));
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<uint8_t>& Var)
{
    Var.Set(luaL_checkint(m_LuaState, -1));
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<bool>& Var)
{
    // Also treat the number 0 as false, not just "false" and "nil".
    if (lua_isnumber(m_LuaState, -1))
        Var.Set(lua_tonumber(m_LuaState, -1) != 0.0);
    else
        Var.Set(lua_toboolean(m_LuaState, -1) != 0);
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<std::string>& Var)
{
    Var.Set(luaL_checkstring(m_LuaState, -1));
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<Vector2fT>& Var)
{
    Vector2fT v;

    if (lua_istable(m_LuaState, -1))
    {
        lua_rawgeti(m_LuaState, -1, 1); v.x = float(lua_tonumber(m_LuaState, -1)); lua_pop(m_LuaState, 1);
        lua_rawgeti(m_LuaState, -1, 2); v.y = float(lua_tonumber(m_LuaState, -1)); lua_pop(m_LuaState, 1);
    }
    else
    {
        v.x = float(luaL_checknumber(m_LuaState, -2));
        v.y = float(luaL_checknumber(m_LuaState, -1));
    }

    Var.Set(v);
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<Vector3fT>& Var)
{
    Vector3fT v;

    if (lua_istable(m_LuaState, -1))
    {
        lua_rawgeti(m_LuaState, -1, 1); v.x = float(lua_tonumber(m_LuaState, -1)); lua_pop(m_LuaState, 1);
        lua_rawgeti(m_LuaState, -1, 2); v.y = float(lua_tonumber(m_LuaState, -1)); lua_pop(m_LuaState, 1);
        lua_rawgeti(m_LuaState, -1, 3); v.z = float(lua_tonumber(m_LuaState, -1)); lua_pop(m_LuaState, 1);
    }
    else
    {
        v.x = float(luaL_checknumber(m_LuaState, -3));
        v.y = float(luaL_checknumber(m_LuaState, -2));
        v.z = float(luaL_checknumber(m_LuaState, -1));
    }

    Var.Set(v);
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<Vector3dT>& Var)
{
    Vector3dT v;

    if (lua_istable(m_LuaState, -1))
    {
        lua_rawgeti(m_LuaState, -1, 1); v.x = lua_tonumber(m_LuaState, -1); lua_pop(m_LuaState, 1);
        lua_rawgeti(m_LuaState, -1, 2); v.y = lua_tonumber(m_LuaState, -1); lua_pop(m_LuaState, 1);
        lua_rawgeti(m_LuaState, -1, 3); v.z = lua_tonumber(m_LuaState, -1); lua_pop(m_LuaState, 1);
    }
    else
    {
        v.x = luaL_checknumber(m_LuaState, -3);
        v.y = luaL_checknumber(m_LuaState, -2);
        v.z = luaL_checknumber(m_LuaState, -1);
    }

    Var.Set(v);
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<BoundingBox3dT>& Var)
{
    BoundingBox3dT BB;

    BB.Min.x = luaL_checknumber(m_LuaState, -6);
    BB.Min.y = luaL_checknumber(m_LuaState, -5);
    BB.Min.z = luaL_checknumber(m_LuaState, -4);

    BB.Max.x = luaL_checknumber(m_LuaState, -3);
    BB.Max.y = luaL_checknumber(m_LuaState, -2);
    BB.Max.z = luaL_checknumber(m_LuaState, -1);

    Var.Set(BB);
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarArrayT<uint32_t>& Var)
{
    Var.Overwrite();

    if (lua_istable(m_LuaState, -1))
    {
        const int Num = lua_rawlen(m_LuaState, -1);

        for (int i = 1; i <= Num; i++)
        {
            lua_rawgeti(m_LuaState, -1, i);
            Var.PushBack(lua_tointeger(m_LuaState, -1));
            lua_pop(m_LuaState, 1);
        }
    }
    else
    {
        // Stack index 1 has the "this" object,
        // stack index 2 has the variable name.
        for (int i = 3; i <= lua_gettop(m_LuaState); i++)
            Var.PushBack(lua_tointeger(m_LuaState, i));
    }
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarArrayT<uint16_t>& Var)
{
    Var.Overwrite();

    if (lua_istable(m_LuaState, -1))
    {
        const int Num = lua_rawlen(m_LuaState, -1);

        for (int i = 1; i <= Num; i++)
        {
            lua_rawgeti(m_LuaState, -1, i);
            Var.PushBack(lua_tointeger(m_LuaState, -1));
            lua_pop(m_LuaState, 1);
        }
    }
    else
    {
        // Stack index 1 has the "this" object,
        // stack index 2 has the variable name.
        for (int i = 3; i <= lua_gettop(m_LuaState); i++)
            Var.PushBack(lua_tointeger(m_LuaState, i));
    }
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarArrayT<uint8_t>& Var)
{
    Var.Overwrite();

    if (lua_istable(m_LuaState, -1))
    {
        const int Num = lua_rawlen(m_LuaState, -1);

        for (int i = 1; i <= Num; i++)
        {
            lua_rawgeti(m_LuaState, -1, i);
            Var.PushBack(lua_tointeger(m_LuaState, -1));
            lua_pop(m_LuaState, 1);
        }
    }
    else
    {
        // Stack index 1 has the "this" object,
        // stack index 2 has the variable name.
        for (int i = 3; i <= lua_gettop(m_LuaState); i++)
            Var.PushBack(lua_tointeger(m_LuaState, i));
    }
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarArrayT<std::string>& Var)
{
    Var.Overwrite();

    if (lua_istable(m_LuaState, -1))
    {
        const int Num = lua_rawlen(m_LuaState, -1);

        for (int i = 1; i <= Num; i++)
        {
            lua_rawgeti(m_LuaState, -1, i);
            const char* s = lua_tostring(m_LuaState, -1);
            Var.PushBack(s ? s : "NULL");
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
            Var.PushBack(s ? s : "NULL");
        }
    }
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
void VarVisitorSetFloatT::visit(cf::TypeSys::VarT<uint16_t>& Var) { }
void VarVisitorSetFloatT::visit(cf::TypeSys::VarT<uint8_t>& Var) { }
void VarVisitorSetFloatT::visit(cf::TypeSys::VarT<bool>& Var) { }
void VarVisitorSetFloatT::visit(cf::TypeSys::VarT<std::string>& Var) { }
void VarVisitorSetFloatT::visit(cf::TypeSys::VarT<Vector3dT>& Var) { }
void VarVisitorSetFloatT::visit(cf::TypeSys::VarT<BoundingBox3dT>& Var) { }
void VarVisitorSetFloatT::visit(cf::TypeSys::VarArrayT<uint32_t>& Var) { }
void VarVisitorSetFloatT::visit(cf::TypeSys::VarArrayT<uint16_t>& Var) { }
void VarVisitorSetFloatT::visit(cf::TypeSys::VarArrayT<uint8_t>& Var) { }
void VarVisitorSetFloatT::visit(cf::TypeSys::VarArrayT<std::string>& Var) { }


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


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarT<uint16_t>& Var)
{
    m_Out << Var.Get();
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarT<uint8_t>& Var)
{
    m_Out << uint16_t(Var.Get());   // Write numbers, not characters.
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarT<bool>& Var)
{
    m_Out << (Var.Get() ? "true" : "false");
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


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarT<Vector3dT>& Var)
{
    m_Out << Var.Get().x << ", " << Var.Get().y << ", " << Var.Get().z;
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarT<BoundingBox3dT>& Var)
{
    m_Out << Var.Get().Min.x << ", " << Var.Get().Min.y << ", " << Var.Get().Min.z << ", ";
    m_Out << Var.Get().Max.x << ", " << Var.Get().Max.y << ", " << Var.Get().Max.z;
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarArrayT<uint32_t>& Var)
{
    m_Out << "{ ";

    for (unsigned int i = 0; i < Var.Size(); i++)
    {
        m_Out << Var[i];
        if (i+1 < Var.Size()) m_Out << ", ";
    }

    m_Out << " }";
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarArrayT<uint16_t>& Var)
{
    m_Out << "{ ";

    for (unsigned int i = 0; i < Var.Size(); i++)
    {
        m_Out << Var[i];
        if (i+1 < Var.Size()) m_Out << ", ";
    }

    m_Out << " }";
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarArrayT<uint8_t>& Var)
{
    m_Out << "{ ";

    for (unsigned int i = 0; i < Var.Size(); i++)
    {
        m_Out << uint16_t(Var[i]);    // Write numbers, not characters.
        if (i+1 < Var.Size()) m_Out << ", ";
    }

    m_Out << " }";
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarArrayT<std::string>& Var)
{
    m_Out << "{ ";

    for (unsigned int i = 0; i < Var.Size(); i++)
    {
        WriteString(Var[i]);
        if (i+1 < Var.Size()) m_Out << ", ";
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

            // Why do we write an extra newline following the opening long bracket?
            // The answer is given in the Lua reference manual:
            //
            // > For convenience, when the opening long bracket is immediately
            // > followed by a newline, the newline is not included in the string.
            //
            // That is, if s begins with a character that is *not* a newline, prepending the extra newline
            // doesn't make a difference. But if the first character in s happened to be a newline, it would
            // get lost if the extra newline was not written.
            m_Out << "[" << Equals << "[\n";
            m_Out << s;
            m_Out << "]" << Equals << "]";
            return;
        }

    m_Out << "\"" << s << "\"";
}
