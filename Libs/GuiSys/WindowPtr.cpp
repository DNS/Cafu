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

#include "WindowPtr.hpp"
#include "GuiImpl.hpp"
#include "Window.hpp"

extern "C"
{
    #include <lua.h>
}


using namespace cf::GuiSys;


WindowPtrT::WindowPtrT(WindowT* Win_)
    : Win(Win_)
{
    if (Win)
    {
        Win->CppReferencesCount++;

        if (Win->CppReferencesCount==1)
        {
            Anchor();
        }
    }
}


WindowPtrT::WindowPtrT(const WindowPtrT& Other)
    : Win(Other.Win)
{
    if (Win)
    {
        // There must have been at least one reference before, namely that of Other.
        assert(Win->CppReferencesCount>0);

        Win->CppReferencesCount++;
    }
}


WindowPtrT::~WindowPtrT()
{
    if (Win)
    {
        assert(Win->CppReferencesCount>0);

        Win->CppReferencesCount--;

        if (Win->CppReferencesCount==0)
        {
            Unanchor();
        }
    }
}


WindowPtrT& WindowPtrT::operator = (const WindowPtrT& Other)
{
    // Note that this implementation handles self-assignment implicitly right!
    if (Other.Win)
    {
        assert(Other.Win->CppReferencesCount>0);

        Other.Win->CppReferencesCount++;
    }

    if (Win)
    {
        assert(Win->CppReferencesCount>0);

        Win->CppReferencesCount--;

        if (Win->CppReferencesCount==0)
        {
            Unanchor();
        }
    }

    Win=Other.Win;
    return *this;
}


static const char* __have_refs_in_cpp_cf="__have_refs_in_cpp_cf";


void WindowPtrT::Anchor()
{
    assert(Win!=NULL);

    // First find the Lua instance of Win (it's in REGISTRY.__windows_list_cf[Win]) and push it on the stack.
    Win->PushAlterEgo();

    // Next, we do: REGISTRY.__have_refs_in_cpp_cf[Win]=AlterEgo;
    //
    // In summary, this means that this method just implements the equivalent of
    //     REGISTRY.__have_refs_in_cpp_cf[Win] = REGISTRY.__windows_list_cf[Win];
    // which doesn't seem to be very worthwhile by itself. However, note that the
    // __windows_list_cf table has weak values, whereas __have_refs_in_cpp_cf is
    // normal, and that the purposes of both tables are different by definition.
    lua_State* LuaState=Win->Gui.LuaState;

    lua_getfield(LuaState, LUA_REGISTRYINDEX, __have_refs_in_cpp_cf);

    if (!lua_istable(LuaState, -1))
    {
        // Create new table with name __have_refs_in_cpp_cf in the registry, and leave it at the stack top.
        lua_pop(LuaState, 1);       // Remove whatever was not a table.
        lua_newtable(LuaState);     // Push a new table instead.
        lua_pushvalue(LuaState, -1);
        lua_setfield(LuaState, LUA_REGISTRYINDEX, __have_refs_in_cpp_cf);
    }

    lua_pushlightuserdata(LuaState, Win);
    lua_pushvalue(LuaState, -3);
    lua_rawset(LuaState, -3);       // __have_refs_in_cpp_cf[Win]=AlterEgo;

    // Remove __have_refs_in_cpp_cf and our AlterEgo from the stack top again.
    lua_pop(LuaState, 2);
}


void WindowPtrT::Unanchor()
{
    assert(Win!=NULL);

    // Analogous to Anchor(), we do: REGISTRY.__have_refs_in_cpp_cf[Win]=nil;
    lua_State* LuaState=Win->Gui.LuaState;

    lua_getfield(LuaState, LUA_REGISTRYINDEX, __have_refs_in_cpp_cf);

    if (!lua_istable(LuaState, -1))
    {
        // Should never get here:
        // Anchor() should have correctly created the __have_refs_in_cpp_cf table!
        assert(false);
        lua_pop(LuaState, 1);
        return;
    }

    // After this, the object can be garbage-collected again
    // (as far as the C++ code is concerned: it keeps no references to the object any more).
    lua_pushlightuserdata(LuaState, Win);
    lua_pushnil(LuaState);
    lua_rawset(LuaState, -3);   // __have_refs_in_cpp_cf[Win] = nil;

    // Remove __have_refs_in_cpp_cf from the stack top again.
    lua_pop(LuaState, 1);
}
