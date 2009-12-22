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

/***************/
/*** Console ***/
/***************/

#ifndef _CF_CONSOLE_HPP_
#define _CF_CONSOLE_HPP_

#include <string>

struct lua_State;


namespace cf
{
    /// This class is an interface to the application console.
    /// Google search for "console input non blocking" yields interesting insights for non-blocking console input under Linux.
    /// The unix_main.c file from Q3 has something, too.
    class ConsoleI
    {
        public:

        /// Virtual dtor so that derived classes properly destroy.
        virtual ~ConsoleI() { }

        // Methods for console output.
        // TODO: These methods could also have a signature like (const char* s="", ...) directly, see GuiSys/Gui.hpp for an example!!
        //       Much better of course would be to implement operator <<, as with std::cout.
        virtual void Print(const std::string& s)=0;      ///< Print message to console.
        virtual void DevPrint(const std::string& s)=0;   ///< Print dev message to console.
        virtual void Warning(const std::string& s)=0;    ///< Print warning to console.
        virtual void DevWarning(const std::string& s)=0; ///< Print dev warning to console.


        /// Registers the methods of this interface with LuaState as a Lua module as described in the PiL2 book, chapter 26.2.
        /// The key idea is that all methods are called via the global Console variable defined below,
        /// and therefore we may consider them as a collection of C-style functions (no OO involved),
        /// so that putting them in a Lua table as described in chapter 26 of the PiL2 book is straightforward.
        static void RegisterLua(lua_State* LuaState);
    };


    /// Builds a std::string from a printf-like format string and the variable argument list.
    std::string va(const char* FormatString, ...);
}


/// Each module (the exe and each dll) needs a pointer to the application-wide global ConsoleI implementation.
/// For the exe, which hosts the implementation of the ConsoleI, the pointer is defined and set by linking in e.g. the ConsoleStdout.cpp file.
/// Each dll that uses the console has to provide a definition by itself, and initialize it to point to the exe's console instance.
extern cf::ConsoleI* Console;

#endif
