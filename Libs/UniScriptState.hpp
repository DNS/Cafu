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

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

#include <cstdarg>
#include <string>


namespace cf { namespace TypeSys { class TypeInfoT; } }
namespace cf { namespace TypeSys { class TypeInfoManT; } }
struct lua_State;


namespace cf
{
    /// This class checks if the Lua stack has the same size at the start and the end of a function.
    class StackCheckerT
    {
        public:

        StackCheckerT(lua_State* LuaState, int Change=0)
            : m_LuaState(LuaState),
              m_StartTop(lua_gettop(m_LuaState) + Change)
        {
        }

        ~StackCheckerT()
        {
            assert(m_StartTop == lua_gettop(m_LuaState));
        }


        private:

        lua_State* m_LuaState;  ///< The Lua state we're checking the stack for.
        const int  m_StartTop;  ///< The initial size of the stack.
    };


    /// This class implements and encapsulates the strategy with which we bind C++ objects to Lua.
    ///
    /// It is separate from class UniScriptStateT, because it can also be used "outside" of script states,
    /// e.g. in the CFunctions of the bound C++ classes. Moreover, it would be possible to derive from
    /// this class in order to implement alternative binding strategies, and to pass a concrete instance
    /// to the UniScriptStateT constructor in order to "configure" it for a particular strategy.
    ///
    /// Literature:
    ///   - Programming in Lua, 2nd edition, Roberto Ierusalimschy
    ///   - Game Programming Gems 6, chapter 4.2, "Binding C/C++ objects to Lua", W. Celes et al.
    ///   - 2008-04-01: http://thread.gmane.org/gmane.comp.lang.lua.general/46787
    class ScriptBinderT
    {
        public:

        /// The constructor.
        ScriptBinderT(lua_State* LuaState);

        /// Pushes the given C++ object onto the stack.
        /// The object must support the GetType() method (should we add a "const char* TypeName" parameter instead?).
        template<class T> bool Push(T* Object/*, bool Recreate*/);

        /// Returns if the given object is currently bound to the Lua state,
        /// i.e. whether for the C++ object there is an alter ego in Lua.
        bool IsBound(void* Object);

        /// Checks if the value at the given stack index is an object of type TypeInfo,
        /// and returns the userdata which is a pointer to the instance.
        void* GetCheckedObjectParam(int StackIndex, const cf::TypeSys::TypeInfoT& TypeInfo);

        /// Breaks the connection between a C++ object and its alter ego in Lua.
        /// If the given object still has an alter ego in the Lua state, calling this method
        /// essentially removes all C++ parts from it: the metatable is reset to nil,
        /// and the userdata's destructor is called (Lua will collect it later).
        /// After this method, any attempt to access the C++-implemented methods in Lua
        /// yields a (safe and well-defined) error message.
        void Disconnect(void* Object);


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

        /// Loads the given string as a Lua chunk, then runs it.
        /// (This acts very much like the stand-alone Lua interpreter.)
        bool DoString(const char* s);

        /// Loads the given file as a Lua chunk, then runs it.
        bool DoFile(const char* FileName);

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

        /// Calls a method with the given name of the given object.
        ///
        /// @param Object      The object whose script method is to be called.
        /// @param MethodName  The name of the method to be called.
        /// @param Signature   Describes the arguments to and results from the Lua method.
        /// @param ...         The arguments to the Lua method and the variables that receive its results as described by the Signature parameter.
        ///
        /// For more details about the parameters and return value, see Call().
        ///
        /// Example:
        /// If the variable Obj is bound to Object, then   CallMethod(Object, "OnTrigger", "f", 1.0);
        /// calls the script method   Obj:OnTrigger(value)   where value is a number with value 1.0.
        template<class T> bool CallMethod(T* Object, const std::string& MethodName, const char* Signature="", ...);

        /// Runs the pending coroutines.
        void RunPendingCoroutines(float FrameTime);

        /// Returns the Lua state that implements this script state.
        lua_State* GetLuaState() { return m_LuaState; }

        // TODO: This method should be private.
        /// This method calls a Lua function in the context of the Lua state.
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

        /// Like the public DoString(), but can also pass parameters to the chunk, like Call().
        bool DoString(const char* s, const char* Signature, ...);

        /// Like the public DoFile(), but can also pass parameters to the chunk, like Call().
        bool DoFile(const char* FileName, const char* Signature, ...);

        /// A global Lua function that registers the given Lua function as a new thread.
        static int RegisterThread(lua_State* LuaState);

        lua_State*         m_LuaState;          ///< The Lua instance. This is what "really" represents the script.
        ArrayT<CoroutineT> m_PendingCoroutines; ///< The list of active, pending coroutines.
    };
}


template<class T> inline bool cf::ScriptBinderT::Push(T* Object/*, bool Recreate*/)
{
    const StackCheckerT StackChecker(m_LuaState, 1);

    // Put the REGISTRY["__cpp_anchors_cf"] table onto the stack.
    lua_getfield(m_LuaState, LUA_REGISTRYINDEX, "__cpp_anchors_cf");

    // Put __cpp_anchors_cf[Object] onto the stack.
    // This should be our table that represents the object.
    lua_pushlightuserdata(m_LuaState, Object);
    lua_rawget(m_LuaState, -2);

    // If the object was not found in __cpp_anchors_cf, create it anew.
    if (lua_isnil(m_LuaState, -1))
    {
        // Remove the nil.
        lua_pop(m_LuaState, 1);

        // Stack indices of the table and userdata that we process here.
        const int USERDATA_INDEX=lua_gettop(m_LuaState) + 2;
        const int TABLE_INDEX   =lua_gettop(m_LuaState) + 1;

        // Create a new table T, which is pushed on the stack and thus at stack index TABLE_INDEX.
        lua_newtable(m_LuaState);

        // Create a new user datum UD, which is pushed on the stack and thus at stack index USERDATA_INDEX.
        T** UserData=(T**)lua_newuserdata(m_LuaState, sizeof(T*));

        // Initialize the memory allocated by the lua_newuserdata() function.
        *UserData=Object;

        // T["__userdata_cf"] = UD
        lua_pushvalue(m_LuaState, USERDATA_INDEX);    // Duplicate the userdata on top of the stack.
        lua_setfield(m_LuaState, TABLE_INDEX, "__userdata_cf");

        // Get the table with name (key) Object->GetType()->ClassName from the registry,
        // and set it as metatable of the newly created table.
        // This is the crucial step that establishes the main functionality of our new table.
        luaL_getmetatable(m_LuaState, Object->GetType()->ClassName);
        lua_setmetatable(m_LuaState, TABLE_INDEX);

        // Get the table with name (key) Object->GetType()->ClassName from the registry,
        // and set it as metatable of the newly created userdata item.
        // This is important for userdata type safety (see PiL2, chapter 28.2) and to have automatic garbage collection work
        // (contrary to the text in the "Game Programming Gems 6" book, chapter 4.2, a __gc method in the metatable
        //  is only called for full userdata, see my email to the Lua mailing list on 2008-Apr-01 for more details).
        luaL_getmetatable(m_LuaState, Object->GetType()->ClassName);
        lua_setmetatable(m_LuaState, USERDATA_INDEX);

        // Remove UD from the stack, so that now the new table T is on top of the stack.
        lua_pop(m_LuaState, 1);

        // Anchor the table: __cpp_anchors_cf[Object] = T
        lua_pushlightuserdata(m_LuaState, Object);
        lua_pushvalue(m_LuaState, TABLE_INDEX);   // Duplicate the table on top of the stack.
        lua_rawset(m_LuaState, -4);
    }

    // Remove the __cpp_anchors_cf table.
    lua_remove(m_LuaState, -2);

    // The requested table/userdata is now at the top of the stack.
    return true;
}


template<class T> inline bool cf::UniScriptStateT::CallMethod(T* Object, const std::string& MethodName, const char* Signature, ...)
{
    const StackCheckerT StackChecker(m_LuaState);
    cf::ScriptBinderT   Binder(m_LuaState);

    Binder.Push(Object);

    // Put the desired method (from the object table) onto the stack of LuaState.
    #if 1
        lua_getfield(m_LuaState, -1, MethodName.c_str());
    #else
        // lua_getfield(LuaState, -1, MethodName.c_str()); or lua_gettable() instead of lua_rawget() just doesn't work,
        // it results in a "PANIC: unprotected error in call to Lua API (loop in gettable)" abortion.
        // I don't know exactly why this is so.
        lua_pushstring(m_LuaState, MethodName.c_str());
        lua_rawget(m_LuaState, -2);
    #endif

    if (!lua_isfunction(m_LuaState, -1))
    {
        // If we get here, this usually means that the value at -1 is just nil, i.e. the
        // function that we would like to call was just not defined in the Lua script.
        lua_pop(m_LuaState, 2);   // Pop whatever is not a function, and the object table.
        return false;
    }

    // Swap the object table and the function.
    // ***************************************

    // The current stack contents of LuaState is
    //      2  function to be called
    //      1  object table
    // Now just swap the two, because the object table is not needed any more but for the first argument to the function
    // (the "self" or "this" value for the object-oriented method call), and having the function at index 1 means that
    // after the call to lua_resume(), the stack is populated only with results (no remains from our code here).
    lua_insert(m_LuaState, -2);   // Inserting the function at index -2 shifts the object table to index -1.

    va_list vl;

    va_start(vl, Signature);
    const bool Result=StartNewCoroutine(1, Signature, vl, std::string("method ") + MethodName + "()");
    va_end(vl);

    return Result;
}

#endif
