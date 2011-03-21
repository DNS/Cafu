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

#include "ConsoleInterpreter.hpp"
#include "ConVar.hpp"

extern "C"
{
    #include <lua.h>
    #include <lauxlib.h>
}


static int LuaCI_GetValue(lua_State* LuaState)
{
    ConVarT* cv=ConsoleInterpreter->FindVar(luaL_checkstring(LuaState, 1));
    if (cv==NULL) return 0;     // As above, nil will be the implied return value if the convar with name s doesn't exist.

    switch (cv->GetType())
    {
        case ConVarT::String:  lua_pushstring (LuaState, cv->GetValueString().c_str()); break;
        case ConVarT::Integer: lua_pushinteger(LuaState, cv->GetValueInt());            break;
        case ConVarT::Bool:    lua_pushboolean(LuaState, cv->GetValueBool());           break;
        case ConVarT::Double:  lua_pushnumber (LuaState, cv->GetValueDouble());         break;
    }

    return 1;
}


static int LuaCI_SetValue(lua_State* LuaState)
{
    ConVarT* cv=ConsoleInterpreter->FindVar(luaL_checkstring(LuaState, 1));
    if (cv==NULL) return 0;     // As above, nil will be the implied return value if the convar with name s doesn't exist.

    switch (cv->GetType())
    {
        case ConVarT::String:  cv->SetValue(lua_tostring(LuaState, 2)); break;
        case ConVarT::Integer: cv->SetValue(lua_tointeger(LuaState, 2)); break;
        case ConVarT::Bool:    cv->SetValue(lua_isboolean(LuaState, 2) ? lua_toboolean(LuaState, 2)!=0 : lua_tointeger(LuaState, 2)!=0); break;
        case ConVarT::Double:  cv->SetValue(lua_tonumber(LuaState, 2)); break;
    }

    return 0;
}


static int LuaCI_LineCompletion(lua_State* LuaState)
{
    ArrayT<std::string> Completions;
    std::string         CommonPrefix=ConsoleInterpreter->LineCompletion(luaL_checkstring(LuaState, 1), Completions);

    lua_pushstring(LuaState, CommonPrefix.c_str());
    lua_createtable(LuaState, Completions.Size(), 0);

    for (unsigned long CompletionNr=0; CompletionNr<Completions.Size(); CompletionNr++)
    {
        lua_pushstring(LuaState, Completions[CompletionNr].c_str());
        lua_rawseti(LuaState, -2, CompletionNr+1);
    }

    return 2;
}


static int LuaCI_RunCommand(lua_State* LuaState)
{
    const bool Result=ConsoleInterpreter->RunCommand(luaL_checkstring(LuaState, 1));

    lua_pushboolean(LuaState, Result);
    return 1;
}


void ConsoleInterpreterI::RegisterLua(lua_State* LuaState)
{
    static const luaL_Reg ciFunctions[]=
    {
        { "GetValue",       LuaCI_GetValue },
        { "SetValue",       LuaCI_SetValue },
        { "LineCompletion", LuaCI_LineCompletion },
        { "RunCommand",     LuaCI_RunCommand },
        { NULL, NULL }
    };

    luaL_register(LuaState, "ci", ciFunctions);
    lua_pop(LuaState, 1);   // Remove the ci table from the stack (it was left there by the luaL_register() function).
}
