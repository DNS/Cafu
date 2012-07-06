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

#include <cstdarg>
#include <string>


namespace cf { namespace TypeSys { class TypeInfoT; } }
namespace cf { namespace TypeSys { class TypeInfoManT; } }
struct lua_State;


namespace cf
{
    /// This class implements and encapsulates the strategy with which we bind C++ objects to Lua.
    ///
    /// It is separate from class UniScriptStateT, because it can also be used "outside" of script states,
    /// e.g. in the CFunctions of the bound C++ classes. Moreover, it would be possible to derive from
    /// this class in order to implement alternative binding strategies, and to pass a concrete instance
    /// to the UniScriptStateT constructor in order to "configure" it for a particular strategy.
    ///
    /// Literature:
    ///   - "Game Programming Gems 6", chapter 4.2, "Binding C/C++ objects to Lua", W. Celes et al.
    ///   - 2008-04-01: http://thread.gmane.org/gmane.comp.lang.lua.general/46787
    class ScriptBinderT
    {
        public:

        /// The constructor.
        ScriptBinderT(lua_State* LuaState);

        /// Pushes the given C++ object onto the stack.
        /// The object must support the GetType() method (should we add a "const char* TypeName" parameter instead?).
        template<class T> bool Push(T* Object/*, bool Recreate*/);

        /// Checks if the value at the given stack index is an object of type TypeInfo,
        /// and returns the userdata which is a pointer to the instance.
        void* GetCheckedObjectParam(int StackIndex, const cf::TypeSys::TypeInfoT& TypeInfo);

        // /// If the given object still has an alter ego in the Lua state, calling this method
        // /// breaks the connection: Any attempt in Lua to access the C++-implemented methods
        // /// will trigger an error message, and the C++ code is free to delete the object.
        // void Disconnect(T* Object);


        private:

        friend class UniScriptStateT;

        /// Implements the one-time initialization of the Lua state for this binder.
        /// Called by the UniScriptStateT constructor.
        void InitState();

        lua_State* m_LuaState;  ///< The Lua state that this binder is used with.
    };


    /// This class represents the state of a script:
    /// the underlying Lua state, pending coroutines, metatables for C++ class hierarchies, etc.
    ///
    /// Its main features are:
    ///   - easy calling of Lua chunks, functions and object methods from C++ (Run() and Call()),
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

        /// Runs the chunk of Lua code that is created from the given string.
        /// (This acts very much like the stand-alone Lua interpreter.)
        bool Run(const char* Chunk);

        /// Calls the global script function with the given name.
        ///
        /// @param FuncName    The name of the global script function to be called.
        /// @param Signature   Describes the arguments to and results from the Lua function. See below for more details.
        /// @param ...         The arguments to the Lua function and the variables that receive its results as described by the Signature parameter.
        ///
        /// The Signature parameter is a string of individual letters, where each letter represents a variable and its type.
        /// The letters 'b' for bool, 'i' for int, 'f' for float, 'd' for double and 's' for const char* (string) can be used.
        /// A '>' separates the arguments from the results, and is optional if the function returns no results.
        /// For the results, additionally to the other letters, 'S' can be used for (address of) std::string.
        ///
        /// @returns whether the function call was successful.
        /// Note that when a signature was provided that expects one or more return values and the called script code yields
        /// (calls coroutine.yield()), the returned values are undefined and thus the call is considered a failure and false is returned.
        /// Nonetheless, the related Lua thread is added to the list of pending coroutines for later resumption.
        bool Call(const char* FuncName, const char* Signature="", ...);

        /// Runs the pending coroutines.
        void RunPendingCoroutines(float FrameTime);

        /// Returns the Lua state that implements this script state.
        lua_State* GetLuaState() const { return m_LuaState; }   // TODO: Remove the "const"

        // TODO: This method should be private.
        bool StartNewCoroutine(int NumExtraArgs, const char* Signature, va_list vl, const std::string& DbgName);


        private:

        class CoroutineT
        {
            public:

            CoroutineT();

            unsigned int ID;                ///< The unique ID of this coroutine, used to anchor it in a table in the Lua registry. Automatically set in the constructor, but not declared const so that CoroutineT objects can be kept in an ArrayT<>.
            lua_State*   State;             ///< The state and stack of this coroutine.
            unsigned int NumParams;         ///< Number of parameters on the stack of State for the next call to lua_resume(), i.e. the parameters for the initial function call or the return values for the pending yield().
            float        WaitTimeLeft;      ///< Wait time left until the next call to lua_resume().


            private:

            static unsigned int InstCount;  ///< Count of created instances, used for creating unique coroutine IDs.
        };

        UniScriptStateT(const UniScriptStateT&);    ///< Use of the Copy Constructor    is not allowed.
        void operator = (const UniScriptStateT&);   ///< Use of the Assignment Operator is not allowed.

        /// Like the public Run(), but can also pass parameters to the chunk, like Call().
        bool Run(const char* Chunk, const char* Signature, ...);

        /// A global Lua function that registers the given Lua function as a new thread.
        static int RegisterThread(lua_State* LuaState);

        lua_State*         m_LuaState;          ///< The Lua instance. This is what "really" represents the script.
        ArrayT<CoroutineT> m_PendingCoroutines; ///< The list of active, pending coroutines.
    };
}

#endif
