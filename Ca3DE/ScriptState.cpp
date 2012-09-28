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

#include "ScriptState.hpp"
#include "TypeSys.hpp"
#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/Console_Lua.hpp"
#include "ConsoleCommands/ConsoleInterpreter.hpp"
#include "ConsoleCommands/ConFunc.hpp"
#include "../Games/Game.hpp"
#include "../Games/GameInfo.hpp"

#include <cassert>
#include <cstring>
#include <fstream>

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}


using namespace cf::GameSys;


#ifndef NDEBUG
namespace
{
    // Contains the information from the EntityClassDefs.lua file.
    struct EntDefInfoT
    {
        std::string MapName;
        std::string CppName;
        std::string Description;
    };
}
#endif


// This function is a variant of TypeInfoManT::CreateLuaDoxygenHeader(),
// customized for the additional information that comes with map entities.
static void CreateLuaDoxygenHeader(lua_State* LuaState, cf::GameSys::GameI* Game)
{
    assert(lua_gettop(LuaState)==0);

#ifndef NDEBUG
    static bool Done=false;

    if (Done) return;
    Done=true;

    std::map<std::string, EntDefInfoT> CppToInfo;

    lua_getglobal(LuaState, "EntityClassDefs");

    if (!lua_istable(LuaState, 1))
    {
        Console->Warning("Table EntityClassDefs not found in ScriptState!\n");
        lua_pop(LuaState, 1);
        return;
    }

    lua_pushnil(LuaState);  // The initial key for the traversal.

    while (lua_next(LuaState, -2)!=0)
    {
        // The key is now at stack index -2, the value is at index -1.
        // Note that in general, the warning from the Lua reference documentation applies:
        // "While traversing a table, do not call lua_tolstring() directly on a key, unless you know that the key is actually a string."
        if (lua_type(LuaState, -2)==LUA_TSTRING && lua_type(LuaState, -1)==LUA_TTABLE)
        {
            EntDefInfoT Info;
            const char* s=NULL;

            s=lua_tostring(LuaState, -2);
            if (s) Info.MapName=s;

            lua_getfield(LuaState, -1, "CppClass");
            s=lua_tostring(LuaState, -1);
            if (s) Info.CppName=s;
            lua_pop(LuaState, 1);

            lua_getfield(LuaState, -1, "description");
            s=lua_tostring(LuaState, -1);
            if (s) Info.Description=s;
            lua_pop(LuaState, 1);

            if (Info.CppName!="" && Info.MapName!="")
                CppToInfo[Info.CppName]=Info;
        }

        // Make sure that the code above left the stack behind properly.
        assert(lua_gettop(LuaState)==3);

        // Remove the value, keep the key for the next iteration.
        lua_pop(LuaState, 1);
    }

    assert(lua_gettop(LuaState)==1);
    lua_pop(LuaState, 1);


    std::ofstream Out("Doxygen/scripting/MapEntitiesDM.tmpl");

    if (!Out.is_open()) return;

    for (unsigned long RootNr=0; RootNr<Game->GetEntityTIM().GetTypeInfoRoots().Size(); RootNr++)
    {
        for (const cf::TypeSys::TypeInfoT* TI=Game->GetEntityTIM().GetTypeInfoRoots()[RootNr]; TI!=NULL; TI=TI->GetNext())
        {
            const std::map<std::string, EntDefInfoT>::const_iterator It=CppToInfo.find(TI->ClassName);

            Out << "\n\n";
            if (It!=CppToInfo.end())
            {
                std::string Desc=It->second.Description;

                size_t Start=0;
                for (size_t Pos=Desc.find("\n", Start); Pos!=std::string::npos; Pos=Desc.find("\n", Start))
                {
                    Desc.replace(Pos, 1, "\n/// ");
                    Start=Pos+1;
                }

                Out << "/// " << Desc << "\n";
                Out << "///\n";
                Out << "/// @mapName{" << It->second.MapName << "}\n";
            }
            Out << "/// @cppName{" << TI->ClassName << "}\n";
            Out << "class " << TI->ClassName;
            if (TI->Base) Out << " : public " << TI->BaseClassName;
            Out << "\n";
            Out << "{\n";
            Out << "    public:\n";
            Out << "\n";

            if (TI->MethodsList)
            {
                for (unsigned int MethodNr=0; TI->MethodsList[MethodNr].name; MethodNr++)
                {
                    if (strncmp(TI->MethodsList[MethodNr].name, "__", 2)==0) continue;

                    Out << "    " << /*"void " <<*/ TI->MethodsList[MethodNr].name << "();\n";
                }
            }

            Out << "};\n";
        }
    }
#endif
}


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

    CreateLuaDoxygenHeader(LuaState, Game);
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
