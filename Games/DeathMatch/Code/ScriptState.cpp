/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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
#include "ConsoleCommands/ConsoleInterpreter.hpp"
#include "ConsoleCommands/ConFunc.hpp"
#include "../../BaseEntity.hpp"        // Only required so that we can call EntCppInstance->GetType().

#include <cassert>
#include <cstring>

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}


using namespace cf::GameSys;


// The constructor does four things to initialize the Script State:
//   1. Open (create, init) a new LuaState.
//   2. Open (load, init) the Lua standard libraries.
//   3. Open (add) some "Cafu standard libaries" (e.g. the Console).
//   4. Add a metatable for each possible entity type to the registry, taking class inheritance into account.
ScriptStateT::ScriptStateT()
    : LuaState(NULL),
      CoroutinesCount(0)
{
    // Initialize Lua.
    LuaState=lua_open();

    lua_pushcfunction(LuaState, luaopen_base);    lua_pushstring(LuaState, "");              lua_call(LuaState, 1, 0);  // Opens the basic library.
    lua_pushcfunction(LuaState, luaopen_package); lua_pushstring(LuaState, LUA_LOADLIBNAME); lua_call(LuaState, 1, 0);  // Opens the package library.
    lua_pushcfunction(LuaState, luaopen_table);   lua_pushstring(LuaState, LUA_TABLIBNAME);  lua_call(LuaState, 1, 0);  // Opens the table library.
    lua_pushcfunction(LuaState, luaopen_io);      lua_pushstring(LuaState, LUA_IOLIBNAME);   lua_call(LuaState, 1, 0);  // Opens the I/O library.
    lua_pushcfunction(LuaState, luaopen_os);      lua_pushstring(LuaState, LUA_OSLIBNAME);   lua_call(LuaState, 1, 0);  // Opens the OS library.
    lua_pushcfunction(LuaState, luaopen_string);  lua_pushstring(LuaState, LUA_STRLIBNAME);  lua_call(LuaState, 1, 0);  // Opens the string lib.
    lua_pushcfunction(LuaState, luaopen_math);    lua_pushstring(LuaState, LUA_MATHLIBNAME); lua_call(LuaState, 1, 0);  // Opens the math lib.

    // Load the console library. (Adds a global table with name "Console" to the LuaState with the functions of the ConsoleI interface.)
    cf::ConsoleI::RegisterLua(LuaState);

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

    // Add an additional function "thread" that we provide for the map script.
    lua_pushcfunction(LuaState, RegisterThread);
    lua_setglobal(LuaState, "thread");


    // For each (entity-)class that the TypeInfoMan knows about, add a (meta-)table to the registry of the LuaState.
    // The (meta-)table holds the Lua methods that the respective class implements in C++ and is to be used as metatable for instances of this class.
    cf::TypeSys::TypeInfoManT& TIM=GetBaseEntTIM();

    for (unsigned long RootNr=0; RootNr<TIM.GetTypeInfoRoots().Size(); RootNr++)
    {
        for (const cf::TypeSys::TypeInfoT* TI=TIM.GetTypeInfoRoots()[RootNr]; TI!=NULL; TI=TI->GetNext())
        {
            assert(lua_gettop(LuaState)==0);

            // Create a new table T and add it into the registry table with TI->ClassName (e.g. "cf::GameSys::EntMoverT") as the key and T as the value.
            // This also leaves T on top of the stack. See PiL2 chapter 28.2 for more details.
            luaL_newmetatable(LuaState, TI->ClassName);

            // See PiL2 chapter 28.3 for a great explanation on what is going on here.
            // Essentially, we set T.__index = T (the luaL_newmetatable() function left T on the top of the stack).
            lua_pushvalue(LuaState, -1);                // Pushes/duplicates the new table T on the stack.
            lua_setfield(LuaState, -2, "__index");      // T.__index = T;

            // Now insert the functions listed in TI->MethodsList into T (the table on top of the stack).
            if (TI->MethodsList!=NULL)
                luaL_register(LuaState, NULL, TI->MethodsList);

            // If TI has a base class, model that relationship for T, too, by setting the metatable of the base class as the metatable for T.
            // Note that this works because the for-loop (over TI) enumerates the base classes always before their child classes!
            if (TI->Base)
            {
                assert(strcmp(TI->BaseClassName, TI->Base->ClassName)==0);

                // Get the metatable M with name (key) TI->Base->ClassName (e.g. "cf::GameSys::BaseEntityT")
                // from the registry, and set it as metatable of T.
                luaL_getmetatable(LuaState, TI->Base->ClassName);
                lua_setmetatable(LuaState, -2);
            }

            // Clear the stack.
            assert(lua_gettop(LuaState)==1);
            lua_pop(LuaState, 1);
        }
    }


    // Add a table with name "__pending_coroutines_cf" to the registry.
    // This table will be used to keep track of the pending coroutines, making sure that Lua doesn't garbage collect them early.
    lua_newtable(LuaState);
    lua_setfield(LuaState, LUA_REGISTRYINDEX, "__pending_coroutines_cf");


    // Run the equivalent to "wait=coroutine.yield;" and "waitFrame=coroutine.yield;", that is,
    // provide aliases for coroutine.yield as known from Doom3 map scripting.
    lua_getglobal(LuaState, "coroutine");
    lua_getfield(LuaState, -1, "yield");
    lua_setglobal(LuaState, "wait");
    lua_getfield(LuaState, -1, "yield");
    lua_setglobal(LuaState, "waitFrame");
    lua_pop(LuaState, 1);


    // Make sure that everyone dealt properly with the Lua stack so far.
    assert(lua_gettop(LuaState)==0);
}


ScriptStateT::~ScriptStateT()
{
    // Close Lua.
    if (LuaState!=NULL) lua_close(LuaState);
}


// This method returns the equivalent of the Lua expression "EntityClassDefs[EntClassName].CppClass".
std::string ScriptStateT::GetCppClassNameFromEntityClassName(const std::string& EntClassName) const
{
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
    std::map<BaseEntityT*, std::string>::iterator It=KnownEntities.find(EntCppInstance);

    // If EntCppInstance is not known, just do nothing.
    if (It==KnownEntities.end()) return;

    lua_pushnil(LuaState);
    lua_setglobal(LuaState, It->second.c_str());

    KnownEntities.erase(It);
}


void ScriptStateT::LoadMapScript(const std::string& FileName)
{
    // Load the mapname.lua script!
    if (luaL_loadfile(LuaState, FileName.c_str())!=0 || lua_pcall(LuaState, 0, 0, 0)!=0)
    {
        Console->Warning(std::string("Lua script \"")+FileName+"\" could not be loaded\n");
        Console->Print(std::string("(")+lua_tostring(LuaState, -1)+").\n");
        lua_pop(LuaState, 1);
    }

    // Make sure that everyone dealt properly with the Lua stack so far.
    assert(lua_gettop(LuaState)==0);
}


void ScriptStateT::PrintGlobalVars() const
{
    // I'm too lazy (and in a hurry) to implement this via Luas C API right now,
    // see http://lua-users.org/lists/lua-l/2007-01/threads.html#00325 for some information
    // (we don't have to recurse into tables, of course).

    // The code below was simply copied from ConsoleInterpreterImpl.cpp, then slightly modified...
    if (luaL_loadstring(LuaState, "for n in pairs(_G) do Console.Print(\"in _G: \" .. n .. \"\\n\"); end")!=0 || lua_pcall(LuaState, 0, 0, 0)!=0)
    {
        // Note that we need an extra newline here, because (all?) the Lua error messages don't have one.
        Console->Print(std::string(lua_tostring(LuaState, -1))+"\n");
        lua_pop(LuaState, 1);
    }
}


bool ScriptStateT::HasEntityInstances() const
{
    return !KnownEntities.empty();
}


static void CountHookFunction(lua_State* CrtState, lua_Debug* ar)
{
    assert(ar->event==LUA_HOOKCOUNT);

    luaL_error(CrtState, "Instruction count exceeds predefined limit (infinite loop error).");
}


void ScriptStateT::RunCmd(const char* Cmd)
{
    // Create a new coroutine for this function call (or else they cannot call coroutine.yield()).
    // The new coroutine is pushed onto the stack of LuaState as a value of type "thread".
    lua_State* NewThread=lua_newthread(LuaState);

    assert(lua_gettop(LuaState )==1);
    assert(lua_gettop(NewThread)==0);

    // Load Cmd as a function onto the stack of NewThread.
    if (luaL_loadstring(NewThread, Cmd)!=0)
    {
        // Note that we need an extra newline here, because (all?) the Lua error messages don't have one.
        Console->Print(std::string(lua_tostring(NewThread, -1))+"\n");

        lua_pop(NewThread, 1);  // Pop the error message.
        assert(lua_gettop(NewThread)==0);

        lua_pop(LuaState, 1);   // Pop the thread.
        assert(lua_gettop(LuaState)==0);
        return;
    }

    // Set the hook function for the "count" event, so that we can detect and prevent infinite loops.
    lua_sethook(NewThread, CountHookFunction, LUA_MASKCOUNT, 10000);    // Should have a ConVar for the number of instruction counts!?

    // Start the new coroutine.
    switch (lua_resume(NewThread, 0))
    {
        case 0:
            // The coroutine returned normally.
            break;

        case LUA_YIELD:
            // The argument to the coroutine.yield() call is the wait time in seconds until the coroutine is supposed to be resumed.
            // If the argument is not given (or not a number), a wait time of 0 is assumed.
            // In any case, the earliest when the coroutine will be resumed is in the next (subsequent) server Think() frame.

            // Check the argument(s).
            if (lua_gettop(NewThread)==0) lua_pushnumber(NewThread, 0);
            if (!lua_isnumber(NewThread, -1)) lua_pushnumber(NewThread, 0);

            CoroutineT Crt;

            Crt.ID          =CoroutinesCount++;
            Crt.State       =NewThread;
            Crt.NumParams   =0;
            Crt.WaitTimeLeft=float(lua_tonumber(NewThread, -1));

            PendingCoroutines.PushBack(Crt);

            // REGISTRY["__pending_coroutines_cf"][Crt.ID]=Crt.State;
            lua_getfield(LuaState, LUA_REGISTRYINDEX, "__pending_coroutines_cf");   // Put REGISTRY["__pending_coroutines_cf"] onto the stack (index 2).
            assert(lua_istable(LuaState, 2));   // Make sure that REGISTRY["__pending_coroutines_cf"] really is a table.
            lua_pushvalue(LuaState, 1);         // Duplicate the "thread" (==Ctr.State) value from index 1 to the top of the stack (index 3).
            lua_rawseti(LuaState, 2, Crt.ID);   // table[Crt.ID]=Crt.State;    -- Pops the value from the stack.
            lua_pop(LuaState, 1);               // Pop the table again.
            break;

        default:
            // An error occurred when running the coroutine.
            // Note that we need an extra newline here, because (all?) the Lua error messages don't have one.
            Console->Print(std::string(lua_tostring(NewThread, -1))+"\n");
            break;
    }

    // Make sure that everyone dealt properly with the LuaState stack so far,
    // then remove the value of type "thread" from the stack, so that it is garbage collected (unless kept in the above __pending_coroutines_cf table).
    assert(lua_gettop(LuaState)==1);
    lua_pop(LuaState, 1);
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
    assert(Entity!=NULL);

    // Create a new coroutine for this function call (or else they cannot call coroutine.yield()).
    // The new coroutine is pushed onto the stack of LuaState as a value of type "thread".
    lua_State* NewThread=lua_newthread(LuaState);

    assert(lua_gettop(LuaState )==1);
    assert(lua_gettop(NewThread)==0);


    // Put the Lua table that represents the Entity onto the stack of NewThread.
    lua_getglobal(NewThread, Entity->Name.c_str());

    assert(ScriptStateT::GetCheckedObjectParam(NewThread, 1, *Entity->GetType())==Entity);
    assert(lua_gettop(NewThread)==1);

    // For release builds, checking only lua_istable() is much cheaper than calling ScriptStateT::GetCheckedObjectParam().
    // It's also safe in the sense that it prevents crashes and working on totally false assumptions.
    // It's not "perfect" though because somebody could substitute the entity table with a custom table of the same name and a machting function entry,
    // which however should never happen and even if it does, that should only be a toy problem rather than something serious.
    if (!lua_istable(NewThread, -1))
    {
        // This should never happen, because a call to AddEntityInstance(Entity) should have created the entity table.
        Console->Warning(std::string("Lua table for entity \"")+Entity->Name+"\" does not exist.\n");
        lua_pop(NewThread, 1);  // Pop whatever is not a table.
        lua_pop(LuaState,  1);  // Pop the thread (it will then be garbage collected).
        return false;
    }


    // Put the desired method (from the entity table) onto the stack of NewThread.
#if 1
    lua_getfield(NewThread, -1, MethodName.c_str());
#else
    // lua_getfield(NewThread, -1, MethodName.c_str()); or lua_gettable() instead of lua_rawget() just doesn't work,
    // it results in a "PANIC: unprotected error in call to Lua API (loop in gettable)" abortion.
    // I don't know exactly why this is so.
    lua_pushstring(NewThread, MethodName.c_str());
    lua_rawget(NewThread, -2);
#endif

    if (!lua_isfunction(NewThread, -1))
    {
        // If we get here, this usually means that the function that we would like to call was just not defined in the .lua script.
        Console->Warning(Entity->Name+"."+MethodName+" is not a function.\n");
        lua_pop(NewThread, 2);  // Pop whatever is not a function, and the entity table.
        lua_pop(LuaState,  1);  // Pop the thread (it will then be garbage collected).
        return false;
    }


    // Swap the entity table and the function.
    // ***************************************

    // The current stack contents of NewThread is
    //      2  function to be called
    //      1  entity table
    // Now just swap the two, because the entity table is not needed any more but for the first argument to the function
    // (the "self" or "this" value for the object-oriented method call), and having the function at index 1 means that
    // after the call to lua_resume(), the stack is populated only with results (no remains from our code here).
    lua_insert(NewThread, 1);   // Insert the function at index 1, shifting the entity table to index 2.


    // Put all other arguments for the function onto the stack of NewThread.
    // *********************************************************************

    const char* Results="";

    for (const char* c=Signature; *c; c++)
    {
        if (*c=='>')
        {
            Results=c+1;
            break;
        }

        switch (*c)
        {
            // According to the g++ compiler, bool is promoted to int, and float is promoted to double when passed through '...',
            // and therefore we should pass int and double to va_arg() instead of bool and float.
            case 'b': lua_pushboolean(NewThread, va_arg(vl, /*bool*/int    )); break;
            case 'i': lua_pushinteger(NewThread, va_arg(vl, int            )); break;
            case 'f': lua_pushnumber (NewThread, va_arg(vl, /*float*/double)); break;
            case 'd': lua_pushnumber (NewThread, va_arg(vl, double         )); break;
            case 's': lua_pushstring (NewThread, va_arg(vl, char*          )); break;
            case 'E': lua_getglobal  (NewThread, va_arg(vl, BaseEntityT*)->Name.c_str()); break;

            default:
                Console->Warning(std::string("Invalid signature \"")+Signature+"\" in call to "+Entity->Name+":"+MethodName+"().\n");
                lua_settop(NewThread, 0);   // Clear the stack of NewThread (the function and its arguments).
                lua_pop(LuaState,  1);      // Pop the thread (it will then be garbage collected).
                return false;
        }

        // WARNING: Do NOT issue a call like   lua_tostring(NewThread, -1)   here!
        // This is because "If the value is a number, then lua_tolstring also changes the actual value in the stack to a string.",
        // as described at http://www.lua.org/manual/5.1/manual.html#lua_tolstring
    }

    const int ResCount=int(strlen(Results));


    // Do the actual function call.
    // ****************************

    // Set the hook function for the "count" event, so that we can detect and prevent infinite loops.
    lua_sethook(NewThread, CountHookFunction, LUA_MASKCOUNT, 10000);    // Should have a ConVar for the number of instruction counts!?

    // Start the new coroutine.
    const int ThreadResult=lua_resume(NewThread, lua_gettop(NewThread)-1);


    // Deal with the results.
    // **********************

    if (ThreadResult==0)
    {
        // The coroutine returned normally, now return the results to the caller.
        int StackIndex=1;

        // If we expect more arguments back than we got, push a single nil that will help to fill-up any number of missing arguments.
        if (ResCount>lua_gettop(NewThread)) lua_pushnil(NewThread);

        for (const char* c=Results; *c; c++)
        {
            switch (*c)
            {
                case 'b': *va_arg(vl, bool*  )=lua_toboolean(NewThread, StackIndex)!=0; break;
                case 'i': *va_arg(vl, int*   )=lua_tointeger(NewThread, StackIndex); break;
                case 'f': *va_arg(vl, float* )=float(lua_tonumber(NewThread, StackIndex)); break;
                case 'd': *va_arg(vl, double*)=lua_tonumber(NewThread, StackIndex); break;
                case 'E': *va_arg(vl, BaseEntityT**)=(BaseEntityT*)ScriptStateT::GetCheckedObjectParam(NewThread, StackIndex, BaseEntityT::TypeInfo); break;

                case 's':
                {
                    const char*        s=lua_tostring(NewThread, StackIndex);
                    static const char* e="";

                    *va_arg(vl, const char**)=(s!=NULL) ? s : e;
                    break;
                }

                case 'S':
                {
                    const char* s=lua_tostring(NewThread, StackIndex);

                    *va_arg(vl, std::string*)=(s!=NULL) ? s : "";
                    break;
                }

                default:
                    Console->Warning(std::string("Invalid results signature \"")+Signature+"\" in call to "+Entity->Name+":"+MethodName+"().\n");
                    break;
            }

            if (StackIndex<lua_gettop(NewThread)) StackIndex++;
        }

        lua_settop(NewThread, 0);           // Pop everything (the results) from the NewThread stack.
        assert(lua_gettop(LuaState)==1);    // Make sure that everyone dealt properly with the LuaState stack so far.
        lua_pop(LuaState, 1);               // Pop the thread (it will then be garbage collected).
        return true;
    }

    if (ThreadResult==LUA_YIELD)
    {
        // The argument to the coroutine.yield() call is the wait time in seconds until the coroutine is supposed to be resumed.
        // If the argument is not given (or not a number), a wait time of 0 is assumed.
        // In any case, the earliest when the coroutine will be resumed is in the next (subsequent) server Think() frame.

        // Check the argument(s).
        if (lua_gettop(NewThread)==0) lua_pushnumber(NewThread, 0);
        if (!lua_isnumber(NewThread, -1)) lua_pushnumber(NewThread, 0);

        CoroutineT Crt;

        Crt.ID          =CoroutinesCount++;
        Crt.State       =NewThread;
        Crt.NumParams   =0;
        Crt.WaitTimeLeft=float(lua_tonumber(NewThread, -1));

        PendingCoroutines.PushBack(Crt);

        // REGISTRY["__pending_coroutines_cf"][Crt.ID]=Crt.State;
        lua_getfield(LuaState, LUA_REGISTRYINDEX, "__pending_coroutines_cf");   // Put REGISTRY["__pending_coroutines_cf"] onto the stack (index 2).
        assert(lua_istable(LuaState, 2));   // Make sure that REGISTRY["__pending_coroutines_cf"] really is a table.
        lua_pushvalue(LuaState, 1);         // Duplicate the "thread" (==Ctr.State) value from index 1 to the top of the stack (index 3).
        lua_rawseti(LuaState, 2, Crt.ID);   // table[Crt.ID]=Crt.State;    -- Pops the value from the stack.
        lua_pop(LuaState, 1);               // Pop the table again.

        if (ResCount>0)
            Console->Warning("The call to "+Entity->Name+":"+MethodName+"() yielded while return values were expected ("+Signature+").\n");

        lua_settop(NewThread, 0);           // Pop everything (the parameters to coroutine.yield()) from the NewThread stack.
        assert(lua_gettop(LuaState)==1);    // Make sure that everyone dealt properly with the LuaState stack so far.
        lua_pop(LuaState, 1);               // Pop the thread (it's kept inside the __pending_coroutines_cf table now).
        return ResCount==0;
    }

    // ThreadResult is not 0 and not LUA_YIELD, so an error occurred when running the coroutine.
    // Note that we need an extra newline here, because (all?) the Lua error messages don't have one.
    Console->Warning("Lua error in call to "+Entity->Name+":"+MethodName+"():\n");
    Console->Print(std::string(lua_tostring(NewThread, -1))+"\n");

    // Note that the stack of NewThread was not unwound after the error (but we currently have no use for it).
    lua_settop(NewThread, 0);           // Just pop everything from the NewThread stack.
    assert(lua_gettop(LuaState)==1);    // Make sure that everyone dealt properly with the LuaState stack so far.
    lua_pop(LuaState, 1);               // Pop the thread (it will then be garbage collected).
    return false;
}


void ScriptStateT::RunPendingCoroutines(float FrameTime)
{
    // Iterate over all elements in the REGISTRY["__pending_coroutines_cf"] table, which has all the pending coroutines.
    const int PENDING_COROUTINES_TABLE_IDX=1;
    lua_getfield(LuaState, LUA_REGISTRYINDEX, "__pending_coroutines_cf");   // Put REGISTRY["__pending_coroutines_cf"] onto the stack at index 1.
    assert(lua_gettop(LuaState)==1);
    assert(lua_istable(LuaState, PENDING_COROUTINES_TABLE_IDX));

    // This variable is used to handle the occurrence of re-entrancy.
    // Re-entrancy occurs when a script that is resumed via the call to lua_resume(Crt.State, 0) below
    // manages to add another entry into the PendingCoroutines array, e.g. by calling the game.startNewThread() function.
    unsigned long PendingCoroutines_Size=PendingCoroutines.Size();

    for (unsigned long PendingCrtNr=0; PendingCrtNr<PendingCoroutines_Size; PendingCrtNr++)
    {
        CoroutineT& Crt=PendingCoroutines[PendingCrtNr];

        Crt.WaitTimeLeft-=FrameTime;
        if (Crt.WaitTimeLeft>0) continue;

        // Set the hook function for the "count" event, so that we can detect and prevent infinite loops.
        // Must do this before the next call to lua_resume, or else the instruction count is *not* reset to zero.
        lua_sethook(Crt.State, CountHookFunction, LUA_MASKCOUNT, 10000);    // Should have a ConVar for the number of instruction counts!?

        // Wait time is over, resume the coroutine.
        const int Result=lua_resume(Crt.State, Crt.NumParams);

        if (Result==LUA_YIELD)
        {
            // The argument to the coroutine.yield() call is the wait time in seconds until the coroutine is supposed to be resumed.
            // If the argument is not given (or not a number), a wait time of 0 is assumed.
            // In any case, the earliest when the coroutine will be resumed is in the next (subsequent) server Think() frame.

            // Check the argument(s).
            if (lua_gettop(Crt.State)==0) lua_pushnumber(Crt.State, 0);
            if (!lua_isnumber(Crt.State, -1)) lua_pushnumber(Crt.State, 0);

            // Re-new the wait time.
            Crt.WaitTimeLeft=float(lua_tonumber(Crt.State, -1));
            Crt.NumParams   =0;
        }
        else
        {
            // The coroutine either completed normally (the body function returned), or an error occurred.
            // In both cases, we remove it from the list of pending coroutines, as it makes no sense to try to resume it once more.
            if (Result!=0)
            {
                // An error occurred when running the coroutine.
                // Note that we need an extra newline here, because (all?) the Lua error messages don't have one.
                Console->Print(std::string(lua_tostring(Crt.State, -1))+"\n");
            }

            // Remove the Crt.State from the __pending_coroutines_cf table, so that Lua will eventually garbage collect it.
            // __pending_coroutines_cf[Crt.ID]=nil;
            lua_pushnil(LuaState);
            lua_rawseti(LuaState, PENDING_COROUTINES_TABLE_IDX, Crt.ID);

            PendingCoroutines.RemoveAtAndKeepOrder(PendingCrtNr);
            PendingCrtNr--;
            PendingCoroutines_Size--;
        }
    }

    // Make sure that everyone dealt properly with the LuaState stack so far,
    // then remove the remaining __pending_coroutines_cf table.
    assert(lua_gettop(LuaState)==1);
    lua_pop(LuaState, 1);
}


static ArrayT<std::string> MapCmds;


void ScriptStateT::RunMapCmdsFromConsole()
{
    // Run all the map command strings in the context of the LuaState.
    for (unsigned long MapCmdNr=0; MapCmdNr<MapCmds.Size(); MapCmdNr++)
        RunCmd(MapCmds[MapCmdNr].c_str());

    MapCmds.Overwrite();
}


// There is no compelling reason to keep this static function inside this .cpp file or as a member of the ScriptStateT class,
// except for the fact that it relies on knowledge that is only found here: The way how metatables are metatables of each
// other when inheritance occurs, the private "__userdata_cf" string, etc.
/*static*/ void* ScriptStateT::GetCheckedObjectParam(lua_State* LuaState, int StackIndex, const cf::TypeSys::TypeInfoT& TypeInfo)
{
    // First make sure that the table that represents the entity itself is at StackIndex.
    luaL_argcheck(LuaState, lua_istable(LuaState, StackIndex), StackIndex, "Expected a table that represents an entity." /*of type TypeInfo.ClassName*/);

    // Put the contents of the "__userdata_cf" field on top of the stack (other values may be between it and the table at position StackIndex).
    lua_getfield(LuaState, StackIndex, "__userdata_cf");

#if 1
    // This approach takes inheritance properly into account by "manually traversing up the inheriance hierarchy".
    // See the "Game Programming Gems 6" book, page 353 for the inspiration for this code.

    // Put the metatable of the desired type on top of the stack.
    luaL_getmetatable(LuaState, TypeInfo.ClassName);

    // Put the metatable for the given userdata on top of the stack (it may belong to a derived class).
    if (!lua_getmetatable(LuaState, -2)) lua_pushnil(LuaState);     // Don't have it push nothing in case of failure.

    while (lua_istable(LuaState, -1))
    {
        if (lua_rawequal(LuaState, -1, -2))
        {
            void** UserData=(void**)lua_touserdata(LuaState, -3); if (UserData==NULL) luaL_error(LuaState, "NULL userdata in entity table.");
            void*  Entity  =(*UserData);

            // Pop the two matching metatables and the userdata.
            lua_pop(LuaState, 3);
            return Entity;
        }

        // Replace the metatable on top of the stack with its metatable (i.e. "the metatable of the metatable").
        if (!lua_getmetatable(LuaState, -1)) lua_pushnil(LuaState);     // Don't have it push nothing in case of failure.
        lua_replace(LuaState, -2);
    }

    luaL_typerror(LuaState, StackIndex, TypeInfo.ClassName);
    return NULL;
#else
    // This approach is too simplistic and thus doesn't work when inheritance is used.
    void** UserData=(void**)luaL_checkudata(LuaState, -1, TypeInfo.ClassName); if (UserData==NULL) luaL_error(LuaState, "NULL userdata in entity table.");
    void*  Entity  =(*UserData);

    // Pop the userdata from the stack again. Not necessary though as it doesn't hurt there.
    // lua_pop(LuaState, 1);
    return Entity;
#endif
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


// Registers the given Lua script function as a new thread.
/*static*/ int ScriptStateT::RegisterThread(lua_State* LuaState)
{
    const cf::GameSys::GameImplT& GameImpl   =cf::GameSys::GameImplT::GetInstance();
    cf::GameSys::ScriptStateT*    ScriptState=GameImpl.GetScriptState();

    // Our stack (parameter list) comes with the function to be registered as a new thread, and the parameters for its initial call.
    luaL_argcheck(LuaState, lua_isfunction(LuaState, 1), 1, "function expected");
    const unsigned long StackSize=lua_gettop(LuaState);

    // Put the __pending_coroutines_cf table on top of the stack,
    // where we will insert ("anchor") the new thread below.
    lua_getfield(LuaState, LUA_REGISTRYINDEX, "__pending_coroutines_cf");   // Put REGISTRY["__pending_coroutines_cf"] onto the stack at index 1.
    assert(lua_istable(LuaState, -1));

    CoroutineT Crt;

    Crt.ID          =ScriptState->CoroutinesCount++;
    Crt.State       =lua_newthread(LuaState);   // Creates a new coroutine and puts it onto the stack of LuaState.
    Crt.NumParams   =StackSize-1;               // The number of function parameters in the stack of Crt.State for the upcoming call of lua_resume().
    Crt.WaitTimeLeft=0;                         // Run at next opportunity, i.e. at next server Think() frame.

    ScriptState->PendingCoroutines.PushBack(Crt);

    // The thread value is at the top (-1) of the stack, the __pending_coroutines_cf table directly below it at -2.
    // Now anchor the new thread at index Crt.ID in the __pending_coroutines_cf table.
    lua_rawseti(LuaState, -2, Crt.ID);          // __pending_coroutines_cf[Crt.ID]=Crt.State;    -- Pops the value from the stack.

    // Remove the __pending_coroutines_cf table from the stack again, restoring the stack to its original state.
    lua_pop(LuaState, 1);
    assert(lua_gettop(LuaState)==int(StackSize));

    // Preparing for the first lua_resume() call for Crt.State,
    // move the body ("main") function plus its paramters on the stack of Crt.State.
    lua_xmove(LuaState, Crt.State, StackSize);
    return 0;
}
