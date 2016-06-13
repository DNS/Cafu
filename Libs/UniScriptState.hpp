/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_UNI_SCRIPT_STATE_HPP_INCLUDED
#define CAFU_UNI_SCRIPT_STATE_HPP_INCLUDED

#include "Templates/Array.hpp"
#include "Templates/Pointer.hpp"
#include "TypeSys.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

#include <cstdarg>
#include <string>


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
    /// Key idea: we only pass by value!
    ///   - object is copied
    ///   - Lua "owns" it and thus determines its lifetime
    ///   - per default, always create new userdata
    ///     - simple and clear if the object has no identity: push an xyz-vector twice --> two different userdata
    ///     - if the object is a smart pointer, and has identity (entities or windows):
    ///       it would work, too, with one detail issue: per-object extensions added by Lua code (e.g. attributes or callbacks) would not be consistent:
    ///       one userdata representing the identity would have them, but not another
    ///       ==> no problem, treat smart pointers as special case
    ///
    /// Each type(name) can only be used with *one* kind of binding:
    ///     MyClassT                A();
    ///     IntrusivePtrT<MyClassT> B(new MyClassT());
    ///
    ///     Push(A);
    ///     Push(B);    // This is not possible!
    /// This is because we can only have one __gc function per type: for this typename, would we call
    /// Destruct<T>(...) with T == MyClassT or with T == IntrusivePtrT<MyClassT> ?
    /// and because the type's CFunction callbacks (for example, BaseEntityT::GetOrigin()) would similary not know with which T to call GetCheckedObjectParam<T>().
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

        /// This method registers all C++ classes known to the TIM in the related Lua state.
        ///
        /// Subsequently created Lua instances of these classes will use the related information
        /// so that script code can call the C++-implemented methods, e.g. "obj:myCppFunc()".
        /// The method also takes inheritance into account: Lua instances of derived classes can
        /// access the attributes and call the methods of their base classes.
        void Init(const cf::TypeSys::TypeInfoManT& TIM);

        /// Pushes the given C++ object onto the stack.
        /// The object must support the GetType() method (should we add a "const char* TypeName" parameter instead?).
        template<class T> bool Push(T Object);

        /// Checks if the value at the given stack index is a Lua object of type T::TypeInfo,
        /// or a subclass derived from it, and returns a reference to the related userdata.
        /// (If T is really an IntrusivePtrT<U>, the method checks for type U::TypeInfo,
        ///  or a subclass derived from it.)
        template<class T> T& GetCheckedObjectParam(int StackIndex);

        /// Returns if the object with the given identity is currently bound to the Lua state,
        /// i.e. whether for the C++ object there is an alter ego in Lua.
        bool IsBound(void* Identity);

        /// Breaks the connection between a C++ object and its alter ego in Lua.
        /// If the object with the given identity still has an alter ego in the Lua state,
        /// calling this method essentially removes all C++ parts from it: the metatable is
        /// reset to nil, and the userdata's destructor is called (Lua will collect it later).
        /// After this method, any attempt to access the C++-implemented methods in Lua
        /// yields a (safe and well-defined) error message.
        void Disconnect(void* Identity);


        private:

        /// Use traits for obtaining information from objects of any given type T.
        /// If we have an instance of T, call GetType() instead, which returns the proper type
        /// even if T is a base class pointer.
        /// See http://erdani.com/publications/traits.html for a nice intro to traits.
        template<class T> class TraitsT
        {
            public:

            static T* GetIdentity(T& Object) { return &Object; }
            static const cf::TypeSys::TypeInfoT& GetTypeInfo() { return T::TypeInfo; }
            static const cf::TypeSys::TypeInfoT& GetTypeInfo(const T& Object) { return *Object.GetType(); }
            static bool IsRefCounted() { return false; }
        };

        /// Specialization of TraitsT for IntrusivePtrTs to T.
        template<class T> class TraitsT< IntrusivePtrT<T> >
        {
            public:

            static T* GetIdentity(IntrusivePtrT<T> Object) { return Object.get(); }
            static const cf::TypeSys::TypeInfoT& GetTypeInfo() { return T::TypeInfo; }
            static const cf::TypeSys::TypeInfoT& GetTypeInfo(IntrusivePtrT<T> Object) { return *Object->GetType(); }
            static bool IsRefCounted() { return true; }
        };

        friend class UniScriptStateT;

        /// Implements the one-time initialization of the Lua state for this binder.
        /// Called by the UniScriptStateT constructor.
        void InitState();

        /// If the object at the given stack index is an IntrusivePtrT, this method anchors it in a separate table
        /// so that it cannot be garbage collected in Lua while it has siblings in C++.
        ///
        /// The method first checks if the object at the given stack index is an IntrusivePtrT.
        /// Its reference count is expected to be at least 2 if the IntrusivePtrT was just passed in from C++ code
        /// and a copy of it was bound to Lua, or at least 1 if Lua has the only instance (that however might soon
        /// be returned to C++ code, where it can be copied and kept, thereby increasing the reference count).
        /// In any case, if the object is an IntrusivePtrT, it is added to the REGISTRY.__has_ref_in_cpp set.
        ///
        /// This prevents the object, when it becomes (otherwise) unused in Lua, from being garbage collected.
        /// It would normally not be a problem at all for an IntrusivePtrT object being collected, and it would be
        /// perfectly possible to push another copy of an IntrusivePtrT to the same C++ object later.
        ///
        /// The only downside with garbage collecting IntrusivePtrT's that are still referenced in C++ is that any
        /// custom Lua data that is attached to it, such as event callback functions, gets lost.
        /// See http://thread.gmane.org/gmane.comp.lang.lua.general/92550 for details.
        ///
        /// Addressing this problem is in fact the sole reason for this method.
        /// See CheckCppRefs() for the complementary method that un-anchors the objects again.
        void Anchor(int StackIndex);

        /// This method un-anchors IntrusivePtrT objects that no longer have any siblings in C++.
        ///
        /// For such IntrusivePtrT's, the Lua instance is the only remaining reference to the C++ object.
        /// Removing such objects from the REGISTRY.__has_ref_in_cpp set ensures that they can normally be
        /// garbage collected as soon as they become unused in Lua as well.
        ///
        /// The user must call this method periodically (typically every n-th game frame).
        ///
        /// In summary, the key idea of the whole anchoring process is:
        ///   1) When an object is newly pushed, anchor it.
        ///   2) Every now and then, run our own "pseudo garbage collection":
        ///        a) When the object becomes unused in C++, un-anchor it.
        ///        b) If the object is passed back to C++ again, re-anchor it.
        ///
        /// See Anchor() for the complementary method that (re-)anchors IntrusivePtrT objects.
        void CheckCppRefs();

        /// If i is a negative stack index (relative to the top), returns the related absolute index.
        int abs_index(int i) const
        {
            return (i > 0 || i <= LUA_REGISTRYINDEX) ? i : lua_gettop(m_LuaState) + i + 1;
        }

        /// An extra object method for objects of type IntrusivePtrT<X>.
        template<class T> static int GetRefCount(lua_State* LuaState);

        /// The callback for the __gc metamethod.
        template<class T> static int Destruct(lua_State* LuaState);

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

        /// Loads the given string as a Lua chunk, then runs it.
        /// (This acts very much like the stand-alone Lua interpreter.)
        bool DoString(const char* s, const char* Signature = "", ...);

        /// Loads the given file as a Lua chunk, then runs it.
        bool DoFile(const char* FileName, const char* Signature = "", ...);

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
        template<class T> bool CallMethod(T Object, const std::string& MethodName, const char* Signature="", ...);

        /// Like CallMethod() above, but the arguments and results are passed via vl rather than "...",
        /// and if any extra arguments have been pushed on the stack, their number must be given.
        /// Note that the "extra arguments" are a work-around that was not necessary if we could use
        /// variadic templates for the implementation of CallMethod().
        template<class T> bool CallMethod_Impl(T Object, const std::string& MethodName, int NumExtraArgs, const char* Signature, va_list vl);

        /// Runs the pending coroutines.
        void RunPendingCoroutines(float FrameTime);

        /// Returns the Lua state that implements this script state.
        lua_State* GetLuaState() { return m_LuaState; }


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

        /// This method calls a Lua function in the context of the Lua state.
        bool StartNewCoroutine(int NumExtraArgs, const char* Signature, va_list vl, const std::string& DbgName);

        /// A global Lua function that registers the given Lua function as a new thread.
        static int RegisterThread(lua_State* LuaState);

        /// A helper function for checking if a called Lua function is documented.
        static void CheckCallbackDoc(const cf::TypeSys::TypeInfoT* TI, const std::string& MethodName, int NumExtraArgs, const char* Signature);

        lua_State*         m_LuaState;          ///< The Lua instance. This is what "really" represents the script.
        ArrayT<CoroutineT> m_PendingCoroutines; ///< The list of active, pending coroutines.
        unsigned int       m_CheckCppRefsCount; ///< Call Binder.CheckCppRefs() only every n-th frame.
    };
}


template<class T> int cf::ScriptBinderT::GetRefCount(lua_State* LuaState)
{
    // Cannot use Binder.GetCheckedObjectParam() here, because it would cause infinite
    // recursion (via the call to Anchor()).
    lua_getfield(LuaState, -1, "__userdata_cf");

    // Note that T is an IntrusivePtrT<X>.
    T* UserData=(T*)lua_touserdata(LuaState, -1);

    assert(UserData);
    lua_pushinteger(LuaState, UserData ? (*UserData)->GetRefCount() : 0);
    return 1;
}


template<class T> int cf::ScriptBinderT::Destruct(lua_State* LuaState)
{
    T* UserData=(T*)lua_touserdata(LuaState, 1);

    if (UserData)
    {
        // Explicitly call the destructor for the placed object.
        UserData->~T();
    }

    return 0;
}


template<class T> inline bool cf::ScriptBinderT::Push(T Object)
{
    const StackCheckerT StackChecker(m_LuaState, 1);

    // Put the REGISTRY["__identity_to_object"] table onto the stack.
    lua_getfield(m_LuaState, LUA_REGISTRYINDEX, "__identity_to_object");

    // Put __identity_to_object[Identity] onto the stack.
    // This should be our table that represents the object.
    lua_pushlightuserdata(m_LuaState, TraitsT<T>::GetIdentity(Object));   // Need the raw "identity" pointer here.
    lua_rawget(m_LuaState, -2);

    // If the object was not found in __identity_to_object, create it anew.
    if (lua_isnil(m_LuaState, -1))
    {
        // Remove the nil.
        lua_pop(m_LuaState, 1);

        // Stack indices of the table and userdata that we process here.
        const int USERDATA_INDEX=lua_gettop(m_LuaState) + 2;
        const int TABLE_INDEX   =lua_gettop(m_LuaState) + 1;

        // Create a new object table OT, which is pushed on the stack and thus at stack index TABLE_INDEX.
        lua_newtable(m_LuaState);

        // Create a new user datum UD, which is pushed on the stack and thus at stack index USERDATA_INDEX.
        new (lua_newuserdata(m_LuaState, sizeof(T))) T(Object);

        // OT["__userdata_cf"] = UD
        lua_pushvalue(m_LuaState, USERDATA_INDEX);    // Duplicate the userdata on top of the stack.
        lua_setfield(m_LuaState, TABLE_INDEX, "__userdata_cf");

        // Get the table with name (key) TraitsT<T>::GetTypeInfo(Object).ClassName from the registry,
        // and check if its __gc metamethod is already set.
        // Note that starting with Lua 5.2, the __gc field must be set *before* the table is set as metatable (below),
        // or else the finalizer will not be called even if it is set later (see §2.5.1 in the Lua Reference Manual).
        luaL_getmetatable(m_LuaState, TraitsT<T>::GetTypeInfo(Object).ClassName);
        assert(lua_istable(m_LuaState, -1));
        lua_getfield(m_LuaState, -1, "__gc");
        if (lua_isnil(m_LuaState, -1))
        {
            lua_pushcfunction(m_LuaState, Destruct<T>);
            lua_setfield(m_LuaState, -3, "__gc");
        }
        lua_pop(m_LuaState, 2);

        // Get the table with name TraitsT<T>::GetTypeInfo(Object).ClassName from the registry,
        // and set it as metatable of the newly created table.
        // This is the crucial step that establishes the main functionality of our new table.
        luaL_getmetatable(m_LuaState, TraitsT<T>::GetTypeInfo(Object).ClassName);
        assert(lua_istable(m_LuaState, -1));
        lua_setmetatable(m_LuaState, TABLE_INDEX);

        // Get the table with name (key) TraitsT<T>::GetTypeInfo(Object).ClassName from the registry,
        // and set it as metatable of the newly created userdata item.
        // This is important for userdata type safety (see PiL2, chapter 28.2) and to have automatic garbage collection work
        // (contrary to the text in the "Game Programming Gems 6" book, chapter 4.2, a __gc method in the metatable
        //  is only called for full userdata, see my email to the Lua mailing list on 2008-Apr-01 for more details).
        luaL_getmetatable(m_LuaState, TraitsT<T>::GetTypeInfo(Object).ClassName);
        assert(lua_istable(m_LuaState, -1));
        lua_setmetatable(m_LuaState, USERDATA_INDEX);

        // Get the table for the root of TraitsT<T>::GetTypeInfo(Object) from the registry,
        // get its __index table, and check if its GetRefCount method is already set.
        if (TraitsT<T>::IsRefCounted())
        {
            const cf::TypeSys::TypeInfoT* TI = &TraitsT<T>::GetTypeInfo(Object);

            while (TI->Base)
                TI = TI->Base;

            luaL_getmetatable(m_LuaState, TI->ClassName);
            lua_getfield(m_LuaState, -1, "__index");
            lua_getfield(m_LuaState, -1, "GetRefCount");
            if (lua_isnil(m_LuaState, -1))
            {
                lua_pushcfunction(m_LuaState, GetRefCount<T>);
                lua_setfield(m_LuaState, -3, "GetRefCount");
            }
            lua_pop(m_LuaState, 3);
        }

        // Remove UD from the stack, so that now the new table OT is on top of the stack.
        lua_pop(m_LuaState, 1);

        // Record the table: __identity_to_object[Identity] = OT
        lua_pushlightuserdata(m_LuaState, TraitsT<T>::GetIdentity(Object));   // Need the raw "identity" pointer here.
        lua_pushvalue(m_LuaState, TABLE_INDEX);                               // Duplicate the table on top of the stack.
        lua_rawset(m_LuaState, -4);

        // Anchor the object (table OT).
        // Note that this is not necessary if the object was found in __identity_to_object above,
        // because then, clearly a copy in C++ and a copy in Lua existed beforehand, so that
        // consequently the object must also be anchored.
        Anchor(TABLE_INDEX);
    }

    // Remove the __identity_to_object table.
    lua_remove(m_LuaState, -2);

    // The requested table/userdata is now at the top of the stack.
    return true;
}


template<class T> inline T& cf::ScriptBinderT::GetCheckedObjectParam(int StackIndex)
{
    // Don't bother with stack checking, because here it can only work in the case of success.
    // If there is an error, not only do we not clean up the stack, but the error message itself is
    // a problem as well: See http://thread.gmane.org/gmane.comp.lang.lua.general/103390 for details.
    // const StackCheckerT StackChecker(m_LuaState);

    StackIndex = abs_index(StackIndex);

    // First make sure that the table that represents the object itself is at StackIndex.
    luaL_argcheck(m_LuaState, lua_istable(m_LuaState, StackIndex), StackIndex, "Expected a table that represents an object." /*of type TypeInfo.ClassName*/);

    // Put the contents of the "__userdata_cf" field on top of the stack (other values may be between it and the table at position StackIndex).
    lua_getfield(m_LuaState, StackIndex, "__userdata_cf");

#if 1
    // This approach takes inheritance properly into account by "manually traversing up the inheritance hierarchy".
    // See the "Game Programming Gems 6" book, page 353 for the inspiration for this code.

    // Put the metatable of the desired type on top of the stack.
    luaL_getmetatable(m_LuaState, TraitsT<T>::GetTypeInfo().ClassName);

    // Put the metatable for the given userdata on top of the stack (it may belong to a derived class).
    if (!lua_getmetatable(m_LuaState, -2)) lua_pushnil(m_LuaState);     // Don't have it push nothing in case of failure.

    while (lua_istable(m_LuaState, -1))
    {
        if (lua_rawequal(m_LuaState, -1, -2))
        {
            T* UserData=(T*)lua_touserdata(m_LuaState, -3);

            if (UserData==NULL)
                luaL_error(m_LuaState, "NULL userdata in object table.");

            // Pop the two matching metatables and the userdata.
            lua_pop(m_LuaState, 3);

            // We pass the object back to C++, fully expecting that it will keep a copy
            // and, if it is an IntrusivePtrT, increase its reference count.
            Anchor(StackIndex);

            return *UserData;
        }

        // Replace the metatable MT on top of the stack with the metatable of MT.__index.
        lua_getfield(m_LuaState, -1, "__index");
        if (!lua_getmetatable(m_LuaState, -1)) lua_pushnil(m_LuaState);     // Don't have it push nothing in case of failure.
        lua_remove(m_LuaState, -2);
        lua_remove(m_LuaState, -2);
    }

 // luaL_typerror(m_LuaState, StackIndex, TraitsT<T>::GetTypeInfo().ClassName);
    luaL_argerror(m_LuaState, StackIndex, lua_pushfstring(m_LuaState, "%s expected, got %s", TraitsT<T>::GetTypeInfo().ClassName, luaL_typename(m_LuaState, StackIndex)));

    static T* Invalid = NULL;
    return *Invalid;
#else
    // This approach is too simplistic, it doesn't work when inheritance is used.
    T* UserData=(T*)luaL_checkudata(m_LuaState, -1, TraitsT<T>::GetTypeInfo().ClassName);

    if (UserData==NULL)
        luaL_error(m_LuaState, "NULL userdata in object table.");

    // Pop the userdata from the stack again. Not necessary though as it doesn't hurt there.
    // lua_pop(m_LuaState, 1);
    return *UserData;
#endif
}


template<class T> inline bool cf::UniScriptStateT::CallMethod(T Object, const std::string& MethodName, const char* Signature, ...)
{
    va_list vl;

    va_start(vl, Signature);
    const bool Result=CallMethod_Impl(Object, MethodName, 0, Signature, vl);
    va_end(vl);

    return Result;
}


template<class T> inline bool cf::UniScriptStateT::CallMethod_Impl(T Object, const std::string& MethodName, int NumExtraArgs, const char* Signature, va_list vl)
{
    const StackCheckerT StackChecker(m_LuaState, -NumExtraArgs);
    cf::ScriptBinderT   Binder(m_LuaState);

#ifdef DEBUG
    CheckCallbackDoc(Object->GetType(), MethodName, NumExtraArgs, Signature);
#endif

    Binder.Push(Object);

    // Put the desired method (directly from the object's table or
    // from its metatables __index table) onto the stack of LuaState.
    lua_getfield(m_LuaState, -1, MethodName.c_str());

    if (!lua_isfunction(m_LuaState, -1))
    {
        // If we get here, this usually means that the value at -1 is just nil, i.e. the
        // function that we would like to call was just not defined in the Lua script.
        // Pop whatever is not a function, the object table, and any extra arguments.
        lua_pop(m_LuaState, 2 + NumExtraArgs);
        return false;
    }

    // Rearrange the stack from "method, Object, extra arguments" to "extra arguments, Object, method",
    // so that the function call sees the Object as its first argument (the "self" or "this" value for
    // the object-oriented method call), followed by any extra arguments.
    lua_insert(m_LuaState, -2 - NumExtraArgs);  // Move the method below the Object and the extra args.
    lua_insert(m_LuaState, -1 - NumExtraArgs);  // Move the Object below the extra args.

    // The stack is now prepared as required by the StartNewCoroutine() method.
    return StartNewCoroutine(1 + NumExtraArgs, Signature, vl, std::string("method ") + MethodName + "()");
}

#endif
