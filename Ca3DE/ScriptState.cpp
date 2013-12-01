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

#include "ScriptState.hpp"
#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/Console_Lua.hpp"
#include "ConsoleCommands/ConsoleInterpreter.hpp"
#include "../Games/Game.hpp"
#include "../Games/GameInfo.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}


using namespace cf::GameSys;


ScriptStateT::ScriptStateT(cf::GameSys::GameInfoI* GameInfo, cf::GameSys::GameI* Game)
    : m_ScriptState()
{
    lua_State* LuaState = m_ScriptState.GetLuaState();

    // Load the console library. (Adds a global table with name "Console" to the LuaState with the functions of the ConsoleI interface.)
    cf::Console_RegisterLua(LuaState);

    // Load the "ci" (console interpreter) library. (Adds a global table with name "ci" to the LuaState with (some of) the functions of the ConsoleInterpreterI interface.)
    ConsoleInterpreterI::RegisterLua(LuaState);

    // Load and run the games "EntityClassDefs.lua" script.
    // This script was originally intended to only define the entity classes for the world editor CaWE,
    // but as it also contains default values of the properties of concrete entities
    // (including e.g. the "CppClass" key, which allows us to learn the C++ class name from the entity class name),
    // it makes very much sense to also load that script here!
    std::string FileName = "Games/" + GameInfo->GetName() + "/EntityClassDefs.lua";

    if (luaL_loadfile(LuaState, FileName.c_str())!=0 || lua_pcall(LuaState, 0, 0, 0)!=0)
    {
        Console->Warning("Lua script \"" + FileName + "\" could not be loaded\n");
        Console->Print(std::string("(")+lua_tostring(LuaState, -1)+").\n");
        lua_pop(LuaState, 1);
    }

    // For each (entity-)class that the TypeInfoMan knows about, add a (meta-)table to the registry of the LuaState.
    // The (meta-)table holds the Lua methods that the respective class implements in C++,
    // and is to be used as metatable for instances of this class.
    cf::ScriptBinderT Binder(LuaState);
    Binder.Init(Game->GetEntityTIM());

    // Make sure that everyone dealt properly with the Lua stack so far.
    assert(lua_gettop(LuaState)==0);
}


// This method returns the equivalent of the Lua expression "EntityClassDefs[EntClassName].CppClass".
std::string ScriptStateT::GetCppClassNameFromEntityClassName(const std::string& EntClassName)
{
    lua_State* LuaState = m_ScriptState.GetLuaState();

    assert(lua_gettop(LuaState)==0);

    lua_getglobal(LuaState, "EntityClassDefs");

    if (!lua_istable(LuaState, 1))
    {
        Console->Warning("Table EntityClassDefs not found in ScriptState!\n");
        lua_pop(LuaState, 1);
        return "";
    }

    lua_getfield(LuaState, 1, EntClassName.c_str());

    if (!lua_istable(LuaState, 2))
    {
        Console->Warning("Key \""+EntClassName+"\" not found in table EntityClassDefs!\n");
        lua_pop(LuaState, 2);
        return "";
    }

    lua_getfield(LuaState, 2, "CppClass");

    const char* CppClass =lua_tostring(LuaState, 3);
    std::string CppClass_=(CppClass!=NULL) ? CppClass : "";

    if (CppClass==NULL) Console->Warning("Key \"CppClass\" not found in table EntityClassDefs[\""+EntClassName+"\"]!\n");

    lua_pop(LuaState, 3);
    return CppClass_;
}
