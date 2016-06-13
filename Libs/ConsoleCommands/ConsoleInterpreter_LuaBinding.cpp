/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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

    lua_newtable(LuaState);
    luaL_setfuncs(LuaState, ciFunctions, 0);
    lua_setglobal(LuaState, "ci");  // Also pops the table from the stack.
}
