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

#include "UniScriptState.hpp"
#include "TypeSys.hpp"
#include "ConsoleCommands/Console.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

#include <cassert>


using namespace cf;


UniScriptStateT::UniScriptStateT()
    : m_LuaState(NULL)
{
    // Open (create, init) a new Lua state.
    m_LuaState=lua_open();

    // Open (load, init) the Lua standard libraries.
    lua_pushcfunction(m_LuaState, luaopen_base);    lua_pushstring(m_LuaState, "");              lua_call(m_LuaState, 1, 0);  // Opens the basic library.
    lua_pushcfunction(m_LuaState, luaopen_package); lua_pushstring(m_LuaState, LUA_LOADLIBNAME); lua_call(m_LuaState, 1, 0);  // Opens the package library.
    lua_pushcfunction(m_LuaState, luaopen_table);   lua_pushstring(m_LuaState, LUA_TABLIBNAME);  lua_call(m_LuaState, 1, 0);  // Opens the table library.
    lua_pushcfunction(m_LuaState, luaopen_io);      lua_pushstring(m_LuaState, LUA_IOLIBNAME);   lua_call(m_LuaState, 1, 0);  // Opens the I/O library.
    lua_pushcfunction(m_LuaState, luaopen_os);      lua_pushstring(m_LuaState, LUA_OSLIBNAME);   lua_call(m_LuaState, 1, 0);  // Opens the OS library.
    lua_pushcfunction(m_LuaState, luaopen_string);  lua_pushstring(m_LuaState, LUA_STRLIBNAME);  lua_call(m_LuaState, 1, 0);  // Opens the string lib.
    lua_pushcfunction(m_LuaState, luaopen_math);    lua_pushstring(m_LuaState, LUA_MATHLIBNAME); lua_call(m_LuaState, 1, 0);  // Opens the math lib.

    // Record a pointer to this UniScriptStateT C++ instance in the Lua state,
    // so that our C++-implemented global methods (like \c thread below) can get back to it.
    lua_pushlightuserdata(m_LuaState, this);
    lua_setfield(m_LuaState, LUA_REGISTRYINDEX, "cafu_script_state");

    // Add a table with name "__pending_coroutines_cf" to the registry.
    // This table will be used to keep track of the pending coroutines, making sure that Lua doesn't garbage collect them early.
    lua_newtable(m_LuaState);
    lua_setfield(m_LuaState, LUA_REGISTRYINDEX, "__pending_coroutines_cf");

    // Run the equivalent to "wait=coroutine.yield;" and "waitFrame=coroutine.yield;", that is,
    // provide aliases for coroutine.yield as known from Doom3 map scripting.
    lua_getglobal(m_LuaState, "coroutine");
    lua_getfield(m_LuaState, -1, "yield");
    lua_setglobal(m_LuaState, "wait");
    lua_getfield(m_LuaState, -1, "yield");
    lua_setglobal(m_LuaState, "waitFrame");
    lua_pop(m_LuaState, 1);

    // Did everyone deal properly with the Lua stack so far?
    assert(lua_gettop(m_LuaState)==0);
}


UniScriptStateT::~UniScriptStateT()
{
    lua_close(m_LuaState);
}


void UniScriptStateT::Init(const cf::TypeSys::TypeInfoManT& TIM)
{
    // For each class that the TIM knows about, add a (meta-)table to the registry of the Lua state.
    // The (meta-)table holds the Lua methods that the respective class implements in C++,
    // and is to be used as metatable for instances of this class.
    for (unsigned long RootNr=0; RootNr<TIM.GetTypeInfoRoots().Size(); RootNr++)
    {
        for (const cf::TypeSys::TypeInfoT* TI=TIM.GetTypeInfoRoots()[RootNr]; TI!=NULL; TI=TI->GetNext())
        {
            assert(lua_gettop(m_LuaState)==0);

            // Create a new table T and add it into the registry table with TI->ClassName (e.g. "cf::GuiSys::WindowT" or "cf::GameSys::EntMoverT") as the key and T as the value.
            // This also leaves T on top of the stack. See PiL2 chapter 28.2 for more details.
            luaL_newmetatable(m_LuaState, TI->ClassName);

            // See PiL2 chapter 28.3 for a great explanation on what is going on here.
            // Essentially, we set T.__index = T (the luaL_newmetatable() function left T on the top of the stack).
            lua_pushvalue(m_LuaState, -1);              // Pushes/duplicates the new table T on the stack.
            lua_setfield(m_LuaState, -2, "__index");    // T.__index = T;

            // Now insert the functions listed in TI->MethodsList into T (the table on top of the stack).
            if (TI->MethodsList!=NULL)
                luaL_register(m_LuaState, NULL, TI->MethodsList);

            // If TI has a base class, model that relationship for T, too, by setting the metatable of the base class as the metatable for T.
            // Note that this works because the for-loop (over TI) enumerates the base classes always before their child classes!
            if (TI->Base)
            {
                assert(strcmp(TI->BaseClassName, TI->Base->ClassName)==0);

                // Get the metatable M with name (key) TI->Base->ClassName (e.g. "cf::GameSys::BaseEntityT")
                // from the registry, and set it as metatable of T.
                luaL_getmetatable(m_LuaState, TI->Base->ClassName);
                lua_setmetatable(m_LuaState, -2);
            }

            // Clear the stack.
            assert(lua_gettop(m_LuaState)==1);
            lua_pop(m_LuaState, 1);
        }
    }
}
