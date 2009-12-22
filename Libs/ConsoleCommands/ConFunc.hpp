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

#ifndef _CF_CONSOLE_FUNCTIONS_HPP_
#define _CF_CONSOLE_FUNCTIONS_HPP_

extern "C"
{
    #include <lua.h>
}

#include <string>


class ConFuncT
{
    public:

    enum FlagT
    {
        FLAG_ALL      =     -1,

        FLAG_MAIN_EXE = 0x0001,
        FLAG_MATSYS   = 0x0002,
        FLAG_SOUNDSYS = 0x0004,
        FLAG_GAMEDLL  = 0x0008,

        FLAG_CHEAT    = 0x0080  ///< Use of this function is considered a cheat.
    };


    /// The constructor.
    ConFuncT(const std::string& Name_, lua_CFunction ExecCallback_, const unsigned long Flags_, const std::string& Description_);

    /// The destructor.
    ~ConFuncT();

    // Get methods.
    const std::string& GetName() const { return Name; }
    const std::string& GetDescription() const { return Description; }
    unsigned long      GetFlags() const { return Flags; }


    /// Registers all convars in the StaticList with the ConsoleInterpreter and invalidates the StaticList.
    /// This has to be called after the ConsoleInterpreter pointer is set when the dll is first initialized!
    /// The ctors of all convars that are instantiated after this call (those that are declared as local (static) variables)
    /// then automatically register themselves with the ConsoleInterpreter rather than with the StaticList.
    static void RegisterStaticList();


    private:

    /// Give the implementation of the console interpreter full access.
    friend class ConsoleInterpreterImplT;

    const std::string Name;             ///< The name of this console variable.
    const std::string Description;      ///< The description (i.e. user help text) for this console variable.
    const int         Flags;            ///< The flags for this convar.
    lua_CFunction     LuaCFunction;     ///< The actual C function that is registered with (made available to) Lua.
};

#endif
