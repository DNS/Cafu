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

#ifndef _CF_GAME_SCRIPTSTATE_HPP_
#define _CF_GAME_SCRIPTSTATE_HPP_

#include "Templates/Array.hpp"

#include <cstdarg>
#include <map>
#include <string>


class  BaseEntityT;
struct lua_State;


namespace cf
{
    namespace TypeSys
    {
        class TypeInfoT;
    }


    namespace GameSys
    {
        /// This class represents the state of the map/entity script of the map.
        class ScriptStateT
        {
            public:

            /// The constructor.
            /// This constructor *requires* that the global interface pointers are already initialized!
            ScriptStateT();

            /// The destructor.
            ~ScriptStateT();

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

            /// After all map entities of a map have been added to the script state (using the AddEntityInstance() method),
            /// this method is used to load the related user provided map script into the script state.
            void LoadMapScript(const std::string& FileName);

            /// Prints the global variables of the Lua script state to the console. Used for debugging.
            void PrintGlobalVars() const;

            /// Returns whether there are entity instances known/registered to the script state.
            /// This is mostly used for debugging, in order to make sure that on shutdown,
            /// all entities have been removed before the script state itself is deleted.
            bool HasEntityInstances() const;

            /// Runs the given command in the context of this script.
            /// @param Cmd   The command to be run.
            void RunCmd(const char* Cmd);

            /// Calls a (Lua-script provided) entity method (or a global function) with the given parameters.
            //   [[ THIS IS NOT YET IMPLEMENTED:
            //      The main purpose of this method is to call entity methods, but when NULL is given as the Entity pointer,
            //      the global function with the given MethodName is called instead. ]]
            /// Example: If Ent is a BaseEntityT object and Ent->Name=="Barney", then   CallEntityMethod(Ent, "OnTrigger", "f", 1.0);
            ///          calls the script method   Barney:OnTrigger(value)   where value is a number with value 1.0.
            /// FIXME / CHECK: What happens if MethodName is a method that is not defined by the script, but by C++ code???
            /// @param Entity       The entity instance whose Lua script method is to be called.
            /// @param MethodName   The name of the method that is to be called.
            /// @param Signature    The signature of the method (i.e. the in/out parameters). TODO: Explain this in greater detail!
            /// @param ...          The (variable number of) parameters to the method.
            /// @returns whether the method call was sucessful.
            /// Note that when a signature was provided that expects one or more return values and the called script code yields
            /// (calls coroutine.yield()), the returned values are undefined and thus the call is considered a failure and false is returned.
            /// Nonetheless, the related Lua thread is added to the list of pending coroutines for later resumption.
            bool CallEntityMethod(BaseEntityT* Entity, const std::string& MethodName, const char* Signature="", ...);

            /// As CallEntityMethod(), but with explicit argument list.
            /// @see CallEntityMethod().
            bool CallEntityMethod(BaseEntityT* Entity, const std::string& MethodName, const char* Signature, va_list vl);

            /// Runs the pending coroutines.
            void RunPendingCoroutines(float FrameTime);

            /// This method loads and runs all the command strings that were entered by the user via the "runMapCmd" console function
            /// since the last call (the last server think). This method must be called once while the server is thinking.
            void RunMapCmdsFromConsole();

            /// Checks if the value at the given stack index is an entity of type TypeInfo, and returns the userdata which is a pointer to the instance.
            static void* GetCheckedObjectParam(lua_State* LuaState, int StackIndex, const cf::TypeSys::TypeInfoT& TypeInfo);

            /// A console function that stores the given command string until the server "thinks" next.
            /// The RunMapCmdsFromConsole() method then runs the commands in the context of the current map/entity script.
            static int ConFunc_runMapCmd_Callback(lua_State* L);


            private:

            struct CoroutineT
            {
                unsigned long ID;           ///< The unique ID of this coroutine, used to anchor it in a table in the Lua registry.
                lua_State*    State;        ///< The state and stack of this coroutine.
                unsigned long NumParams;    ///< Number of parameters on the stack of State for the next call to lua_resume(), i.e. the parameters for the initial function call or the return values for the pending yield().
                float         WaitTimeLeft; ///< Wait time left until the next call to lua_resume().
            };

            ScriptStateT(const ScriptStateT&);      ///< Use of the Copy Constructor    is not allowed.
            void operator = (const ScriptStateT&);  ///< Use of the Assignment Operator is not allowed.

            lua_State*         LuaState;            ///< State of the Lua instance. This is what "really" represents the script.
            ArrayT<CoroutineT> PendingCoroutines;   ///< The list of active, pending coroutines.
            unsigned long      CoroutinesCount;     ///< Count of created coroutines, used for creating unique coroutine IDs.

            /// List of entities added to the script state by the AddEntityInstance() method until removed by the RemoveEntityInstance() method.
            ///
            /// NOTE: This member is needed for the proper implementation of the RemoveEntityInstance() method,
            /// because BaseEntityTs don't know themselves if they're client- or server-side, and if they have an associated script state.
            /// However, they probably should(!?!?!), and then this member would not be needed, because only the "right" entities would call
            /// RemoveEntityInstance(). Also see cf::GameSys::GameImplT::FreeBaseEntity() for some information.
            /// NOTE 2: On the one hand, the ScriptState is destroyed and recreated with each server map change, but on the other hand,
            ///         we cannot tolerate to *not* remove the Lua instance whose C++ instance has been deleted (dangling pointer in userdata).
            std::map<BaseEntityT*, std::string> KnownEntities;

            // An additional function that we provide for the map script.
            // It registers the given Lua script function as a new thread.
            static int RegisterThread(lua_State* LuaState);
        };
    }
}

#endif
