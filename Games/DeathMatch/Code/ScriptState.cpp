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
#include "../../BaseEntity.hpp"        // Only required so that we can call EntCppInstance->GetType().

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
static void CreateLuaDoxygenHeader(lua_State* LuaState)
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

    for (unsigned long RootNr=0; RootNr<GetBaseEntTIM().GetTypeInfoRoots().Size(); RootNr++)
    {
        for (const cf::TypeSys::TypeInfoT* TI=GetBaseEntTIM().GetTypeInfoRoots()[RootNr]; TI!=NULL; TI=TI->GetNext())
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


ScriptStateT::ScriptStateT()
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
    if (luaL_loadfile(LuaState, "Games/DeathMatch/EntityClassDefs.lua")!=0 || lua_pcall(LuaState, 0, 0, 0)!=0)
    {
        Console->Warning("Lua script \"Games/DeathMatch/EntityClassDefs.lua\" could not be loaded\n");
        Console->Print(std::string("(")+lua_tostring(LuaState, -1)+").\n");
        lua_pop(LuaState, 1);
    }

    // For each (entity-)class that the TypeInfoMan knows about, add a (meta-)table to the registry of the LuaState.
    // The (meta-)table holds the Lua methods that the respective class implements in C++,
    // and is to be used as metatable for instances of this class.
    m_ScriptState.Init(GetBaseEntTIM());

    // Make sure that everyone dealt properly with the Lua stack so far.
    assert(lua_gettop(LuaState)==0);

    CreateLuaDoxygenHeader(LuaState);
}


// This method returns the equivalent of the Lua expression "EntityClassDefs[EntClassName].CppClass".
std::string ScriptStateT::GetCppClassNameFromEntityClassName(const std::string& EntClassName) const
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


bool ScriptStateT::AddEntityInstance(BaseEntityT* EntCppInstance)
{
    lua_State* LuaState = m_ScriptState.GetLuaState();

    if (EntCppInstance->Name=="")
    {
        Console->Warning("Cannot create entity script instance with empty (\"\") name.\n");
        return false;
    }


    // See if a global variable with name EntCppInstance->Name already exists (it shouldn't).
    lua_getglobal(LuaState, EntCppInstance->Name.c_str());

    if (!lua_isnil(LuaState, -1))
    {
        lua_pop(LuaState, 1);
        Console->Warning("Global variable with name \""+EntCppInstance->Name+"\" already exists (with non-nil value).\n");
        return false;
    }

    lua_pop(LuaState, 1);


    // Now do the actual work: add the new table that represents the entity.
    assert(lua_gettop(LuaState)==0);

    // Stack indices of the table and userdata that we create in this loop.
    const int USERDATA_INDEX=2;
    const int TABLE_INDEX   =1;

    // Create a new table T, which is pushed on the stack and thus at stack index TABLE_INDEX.
    lua_newtable(LuaState);

    // Create a new user datum UD, which is pushed on the stack and thus at stack index USERDATA_INDEX.
    BaseEntityT** UserData=(BaseEntityT**)lua_newuserdata(LuaState, sizeof(BaseEntityT*));

    // Initialize the memory allocated by the lua_newuserdata() function.
    *UserData=EntCppInstance;

    // T["__userdata_cf"] = UD
    lua_pushvalue(LuaState, USERDATA_INDEX);    // Duplicate the userdata on top of the stack (as the argument for lua_setfield()).
    lua_setfield(LuaState, TABLE_INDEX, "__userdata_cf");

    // Get the table with name (key) EntCppInstance->GetType()->ClassName from the registry,
    // and set it as metatable of the newly created table.
    // This is the crucial step that establishes the main functionality of our new table.
    luaL_getmetatable(LuaState, EntCppInstance->GetType()->ClassName);
    lua_setmetatable(LuaState, TABLE_INDEX);

    // Get the table with name (key) EntCppInstance->GetType()->ClassName from the registry,
    // and set it as metatable of the newly created userdata item.
    // This is important for userdata type safety (see PiL2, chapter 28.2) and to have automatic garbage collection work
    // (contrary to the text in the "Game Programming Gems 6" book, chapter 4.2, a __gc method in the metatable
    //  is only called for full userdata, see my email to the Lua mailing list on 2008-Apr-01 for more details).
    luaL_getmetatable(LuaState, EntCppInstance->GetType()->ClassName);
    lua_setmetatable(LuaState, USERDATA_INDEX);

    // Remove UD from the stack, so that only the new table T is left on top of the stack.
    // Then add it as a global variable whose name is EntCppInstance->Name.
    // As lua_setglobal() pops the table from the stack, the stack is left empty.
    lua_pop(LuaState, 1);
    lua_setglobal(LuaState, EntCppInstance->Name.c_str());

    // Register the entity among the "known" entities.
    assert(KnownEntities[EntCppInstance]=="");
    KnownEntities[EntCppInstance]=EntCppInstance->Name;


    // Done. Make sure that everyone dealt properly with the Lua stack so far.
    assert(lua_gettop(LuaState)==0);
    return true;
}


void ScriptStateT::RemoveEntityInstance(BaseEntityT* EntCppInstance)
{
    lua_State* LuaState = m_ScriptState.GetLuaState();

    std::map<BaseEntityT*, std::string>::iterator It=KnownEntities.find(EntCppInstance);

    // If EntCppInstance is not known, just do nothing.
    if (It==KnownEntities.end()) return;

    lua_pushnil(LuaState);
    lua_setglobal(LuaState, It->second.c_str());

    KnownEntities.erase(It);
}


bool ScriptStateT::CallEntityMethod(BaseEntityT* Entity, const std::string& MethodName, const char* Signature, ...)
{
    va_list vl;

    va_start(vl, Signature);
    const bool Result=CallEntityMethod(Entity, MethodName, Signature, vl);
    va_end(vl);

    return Result;
}


bool ScriptStateT::CallEntityMethod(BaseEntityT* Entity, const std::string& MethodName, const char* Signature, va_list vl)
{
    lua_State* LuaState = m_ScriptState.GetLuaState();

    assert(Entity!=NULL);

    // Put the Lua table that represents the Entity onto the stack.
    lua_getglobal(LuaState, Entity->Name.c_str());

    // assert(UniScriptStateT::GetCheckedObjectParam(LuaState, -1, *Entity->GetType())==Entity);

    // For release builds, checking only lua_istable() is much cheaper than calling ScriptStateT::GetCheckedObjectParam().
    // It's also safe in the sense that it prevents crashes and working on totally false assumptions.
    // It's not "perfect" though because somebody could substitute the entity table with a custom table of the same name and a matching function entry,
    // which however should never happen and even if it does, that should only be a toy problem rather than something serious.
    if (!lua_istable(LuaState, -1))
    {
        // This should never happen, because a call to AddEntityInstance(Entity) should have created the entity table.
        Console->Warning(std::string("Lua table for entity \"")+Entity->Name+"\" does not exist.\n");
        lua_pop(LuaState, 1);   // Pop whatever is not a table.
        return false;
    }


    // Put the desired method (from the entity table) onto the stack of LuaState.
#if 1
    lua_getfield(LuaState, -1, MethodName.c_str());
#else
    // lua_getfield(LuaState, -1, MethodName.c_str()); or lua_gettable() instead of lua_rawget() just doesn't work,
    // it results in a "PANIC: unprotected error in call to Lua API (loop in gettable)" abortion.
    // I don't know exactly why this is so.
    lua_pushstring(LuaState, MethodName.c_str());
    lua_rawget(LuaState, -2);
#endif

    if (!lua_isfunction(LuaState, -1))
    {
        // If we get here, this usually means that the value at -1 is just nil, i.e. the
        // function that we would like to call was just not defined in the Lua script.
        Console->Warning(Entity->Name+"."+MethodName+" is not a function.\n");
        lua_pop(LuaState, 2);   // Pop whatever is not a function, and the entity table.
        return false;
    }

    // Swap the entity table and the function.
    // ***************************************

    // The current stack contents of LuaState is
    //      2  function to be called
    //      1  entity table
    // Now just swap the two, because the entity table is not needed any more but for the first argument to the function
    // (the "self" or "this" value for the object-oriented method call), and having the function at index 1 means that
    // after the call to lua_resume(), the stack is populated only with results (no remains from our code here).
    lua_insert(LuaState, -2);   // Inserting the function at index -2 shifts the entity table to index -1.

    return m_ScriptState.StartNewCoroutine(1, Signature, vl, Entity->Name + ":" + MethodName + "()");
}


static ArrayT<std::string> MapCmds;


void ScriptStateT::RunMapCmdsFromConsole()
{
    // Run all the map command strings in the context of the LuaState.
    for (unsigned long MapCmdNr=0; MapCmdNr<MapCmds.Size(); MapCmdNr++)
        m_ScriptState.DoString(MapCmds[MapCmdNr].c_str());

    MapCmds.Overwrite();
}


#include "GameImpl.hpp"


// This console function is called at any time (e.g. when we're NOT thinking)...
/*static*/ int ScriptStateT::ConFunc_runMapCmd_Callback(lua_State* LuaState)
{
    // *** WARNING ***
    // The LuaState is maintained in the main executable, not in this DLL, so its probably *NOT* safe to modify it here
    // with the CRT being *statically* linked. Inspecting the stack is probably harmless, but returning values probably
    // implies DLL-local memory allocation and thus a corrputed heap!

    const cf::GameSys::GameImplT&    GameImpl   =cf::GameSys::GameImplT::GetInstance();
    const cf::GameSys::ScriptStateT* ScriptState=GameImpl.GetScriptState();

    // Check if we're running client-only, because map script commands will only be processed when a server is running.
    if (ScriptState==NULL) return luaL_error(LuaState, "No local (server) map/entity script is currently active.\n");

    // Obtain the map command string from the function parameter state LuaState, and keep it until the server thinks next.
    // Note that it is unlikely but not impossible that this console function is called multiply between two server Think() calls,
    // but in fact, the user only has to enter it twice in the same line, semicolon separated, in order to achieve the effect.
    // (That means that we actually have to keep the command strings in an *array*, a single string is not enough.)
    MapCmds.PushBack(luaL_checkstring(LuaState, 1));

    // Console->Print(cf::va("%lu ", MapCmds.Size()-1)+MapCmds[MapCmds.Size()-1]+"\n");
    return 0;
}

static ConFuncT ConFunc_runMapCmd("runMapCmd", ScriptStateT::ConFunc_runMapCmd_Callback, ConFuncT::FLAG_GAMEDLL,
    "Keeps the given command string until the server \"thinks\" next, then runs it in the context of the current map/entity script.");
