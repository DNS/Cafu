/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CONSOLE_LUA_HPP_INCLUDED
#define CAFU_CONSOLE_LUA_HPP_INCLUDED

struct lua_State;


namespace cf
{
    void Console_RegisterLua(lua_State* LuaState);
}

#endif
