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

#ifndef CAFU_UNI_SCRIPT_STATE_HPP_INCLUDED
#define CAFU_UNI_SCRIPT_STATE_HPP_INCLUDED

#include "Templates/Array.hpp"
#include <string>


namespace cf { namespace TypeSys { class TypeInfoManT; } }
struct lua_State;


namespace cf
{
    /// This class represents the state of a script:
    /// the underlying Lua state, pending coroutines, metatables for C++ class hierarchies, etc.
    ///
    /// Its main features are:
    ///   - easy calling of Lua functions and object methods from C++,
    ///   - easy to use support for coroutines/threads: Lua code can call "thread()" and "wait()",
    ///   - easy creation of Lua instances for C++ objects and binding of C++-implemented methods.
    class UniScriptStateT
    {
        public:

        /// The constructor.
        UniScriptStateT();

        /// The destructor.
        ~UniScriptStateT();

        /// This method registers all C++ classes known to the TIM with this script state.
        ///
        /// Subsequently created Lua instances of these classes will use the related information
        /// so that script code can call the C++-implemented methods, e.g. "obj:myCppFunc()".
        /// The method also takes inheritance into account: Lua instances of derived classes can
        /// access the attributes and call the methods of their base classes.
        void Init(const cf::TypeSys::TypeInfoManT& TIM);

        /// Returns the Lua state that implements this script state.
        lua_State* GetLuaState() const { return m_LuaState; }   // TODO: Remove the "const"


        private:

        UniScriptStateT(const UniScriptStateT&);    ///< Use of the Copy Constructor    is not allowed.
        void operator = (const UniScriptStateT&);   ///< Use of the Assignment Operator is not allowed.

        lua_State*         m_LuaState;          ///< State of the Lua instance. This is what "really" represents the script.
    };
}

#endif
