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

#ifndef CAFU_GAME_SCRIPTSTATE_HPP_INCLUDED
#define CAFU_GAME_SCRIPTSTATE_HPP_INCLUDED

#include "UniScriptState.hpp"

#include <cstdarg>
#include <map>


class BaseEntityT;


namespace cf
{
    namespace GameSys
    {
        /// This class represents the state of the map/entity script of the map.
        class ScriptStateT
        {
            public:

            /// The constructor.
            /// This constructor *requires* that the global interface pointers are already initialized!
            ScriptStateT();

            /// This method returns the value of the Lua expression "EntityClassDefs[EntClassName].CppClass".
            /// The empty string is returned on error, that is, when one of the tables or table fields does not exist.
            std::string GetCppClassNameFromEntityClassName(const std::string& EntClassName) const;

            /// Adds a new entity object/instance (a Lua table) to the script state.
            /// @param EntCppInstance   The pointer to (the C++ instance of) the entity for which a matching instance in the script state is to be created.
            /// @returns true on success, false on failure (e.g. when an object with the given name already exists).
            /// Note that   EntCppInstance->Name   will be the name of the newly created script object.
            bool AddEntityInstance(BaseEntityT* EntCppInstance);

            /// Removes the scripting instance related to EntCppInstance from the script state again
            /// (the Lua global variable with its name is reset to "nil").
            /// The method checks if the EntCppInstance has been added by the AddEntityInstance() method earlier,
            /// so that it can also be called for client-side entities or server-side entities that never got a script instance,
            /// in case of which it just does nothing. This is helpful for simplifying the caller code.
            void RemoveEntityInstance(BaseEntityT* EntCppInstance);

            /// Returns the underlying script state. (This is temporarly only.)
            UniScriptStateT& GetScriptState() { return m_ScriptState; }

            /// This method loads and runs all the command strings that were entered by the user via the "runMapCmd" console function
            /// since the last call (the last server think). This method must be called once while the server is thinking.
            void RunMapCmdsFromConsole();

            /// A console function that stores the given command string until the server "thinks" next.
            /// The RunMapCmdsFromConsole() method then runs the commands in the context of the current map/entity script.
            static int ConFunc_runMapCmd_Callback(lua_State* L);


            private:

            ScriptStateT(const ScriptStateT&);      ///< Use of the Copy Constructor    is not allowed.
            void operator = (const ScriptStateT&);  ///< Use of the Assignment Operator is not allowed.

            UniScriptStateT m_ScriptState;          ///< The script state of this script state. Yes, this is awkward -- temporary only!
        };
    }
}

#endif
