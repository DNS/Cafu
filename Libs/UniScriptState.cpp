/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "UniScriptState.hpp"
#include "TypeSys.hpp"
#include "ConsoleCommands/Console.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

#include <cassert>
#include <cstring>


using namespace cf;


unsigned int UniScriptStateT::CoroutineT::InstCount = 0;


ScriptBinderT::ScriptBinderT(lua_State* LuaState)
    : m_LuaState(LuaState)
{
}


void ScriptBinderT::InitState()
{
    // Add a table with name "__identity_to_object" to the registry:
    // REGISTRY["__identity_to_object"] = {}
    //
    // This table will be used to keep track of the bound C++ objects,
    // mapping C++ instance pointers (identities) to Lua tables/userdata.
    // It is used to find previously created Lua objects again by C++ pointer.
    lua_newtable(m_LuaState);

    // Set the new table as metatable of itself, configured for weak values.
    lua_pushstring(m_LuaState, "v");
    lua_setfield(m_LuaState, -2, "__mode");
    lua_pushvalue(m_LuaState, -1);
    lua_setmetatable(m_LuaState, -2);

    lua_setfield(m_LuaState, LUA_REGISTRYINDEX, "__identity_to_object");


    // Add a table with name "__has_ref_in_cpp" to the registry:
    // REGISTRY.__has_ref_in_cpp = {}
    lua_newtable(m_LuaState);
    lua_setfield(m_LuaState, LUA_REGISTRYINDEX, "__has_ref_in_cpp");
}


void ScriptBinderT::Anchor(int StackIndex)
{
    const StackCheckerT StackChecker(m_LuaState);

    StackIndex = abs_index(StackIndex);

    // Is the object at StackIndex eligible for anchoring?
    // (Is object:GetRefCount() >= 1 ?)
    lua_getfield(m_LuaState, StackIndex, "GetRefCount");

    if (!lua_isfunction(m_LuaState, -1))
    {
        lua_pop(m_LuaState, 1);
        return;
    }

    lua_pushvalue(m_LuaState, StackIndex);
    lua_call(m_LuaState, 1, 1);
    const int RefCount = lua_tointeger(m_LuaState, -1);
    lua_pop(m_LuaState, 1);

    if (RefCount < 1)
    {
        assert(false);
        return;
    }

    // Put the REGISTRY.__has_ref_in_cpp set onto the stack.
    lua_getfield(m_LuaState, LUA_REGISTRYINDEX, "__has_ref_in_cpp");

    // Insert the object: __has_ref_in_cpp[Object] = true
    lua_pushvalue(m_LuaState, StackIndex);
    lua_pushboolean(m_LuaState, 1);
    lua_rawset(m_LuaState, -3);

    // Remove the __has_ref_in_cpp table.
    lua_pop(m_LuaState, 1);
}


void ScriptBinderT::CheckCppRefs()
{
    const StackCheckerT StackChecker(m_LuaState);

    // Put the REGISTRY.__has_ref_in_cpp set onto the stack.
    lua_getfield(m_LuaState, LUA_REGISTRYINDEX, "__has_ref_in_cpp");
    assert(lua_istable(m_LuaState, -1));

    // The initial key for the traversal.
    lua_pushnil(m_LuaState);

    while (lua_next(m_LuaState, -2) != 0)
    {
        // The key is now at stack index -2, the value is at index -1.
        // Remove the unneeded boolean value (true), so that the key is at index -1.
        // The key is also needed at index -1 in order to seed the next iteration.
        lua_pop(m_LuaState, 1);

        // The key is the table that represents our object.
        assert(lua_istable(m_LuaState, -1));

        // Should the object at index -1 remain anchored?
        lua_getfield(m_LuaState, -1, "GetRefCount");

        if (lua_isfunction(m_LuaState, -1))
        {
            lua_pushvalue(m_LuaState, -2);
            lua_call(m_LuaState, 1, 1);
            const int RefCount = lua_tointeger(m_LuaState, -1);
            lua_pop(m_LuaState, 1);

            if (RefCount > 1)
            {
                // Keep this object anchored.
                continue;
            }
        }
        else
        {
            assert(lua_isnil(m_LuaState, -1));

            // Pop whatever is not a function.
            lua_pop(m_LuaState, 1);
        }

        // Remove the object: __has_ref_in_cpp[Object] = nil
        lua_pushvalue(m_LuaState, -1);
        lua_pushnil(m_LuaState);
        lua_rawset(m_LuaState, -4);

        // The key is kept for the next iteration.
    }

    // Remove the __has_ref_in_cpp table.
    lua_pop(m_LuaState, 1);
}


void ScriptBinderT::Init(const cf::TypeSys::TypeInfoManT& TIM)
{
    // For each class that the TIM knows about, add a (meta-)table to the registry of the Lua state.
    // The (meta-)table holds the Lua methods that the respective class implements in C++,
    // and is to be used as metatable for instances of this class.
    for (unsigned long RootNr=0; RootNr<TIM.GetTypeInfoRoots().Size(); RootNr++)
    {
        for (const cf::TypeSys::TypeInfoT* TI=TIM.GetTypeInfoRoots()[RootNr]; TI!=NULL; TI=TI->GetNext())
        {
            assert(lua_gettop(m_LuaState)==0);

            // Create a new table MT and add it into the registry table with TI->ClassName
            // (e.g. "cf::GuiSys::WindowT" or "cf::GameSys::EntMoverT") as the key and MT as the value.
            // This also leaves MT on top of the stack. See PiL2 chapter 28.2 for more details.
            luaL_newmetatable(m_LuaState, TI->ClassName);

            // Create a new table FT and register the functions in TI->MethodsList into it.
            // See PiL2 chapter 28.3 for more details. We intentionally don't merge FT into MT though,
            // or else Lua code could call the __gc method! See thread "__gc visible to Lua code" at
            // http://lua-users.org/lists/lua-l/2007-03/threads.html#00872 for more details.
            lua_newtable(m_LuaState);

            if (TI->MethodsList != NULL)
                luaL_setfuncs(m_LuaState, TI->MethodsList, 0);

            // If TI has a base class, model that relationship for FT, too, by setting the metatable of
            // the base class as the metatable for FT. Note that this works because the for-loop (over TI)
            // enumerates the base classes always before their child classes!
            if (TI->Base)
            {
                assert(strcmp(TI->BaseClassName, TI->Base->ClassName) == 0);

                // Get the metatable MT' with name TI->Base->ClassName (e.g. "cf::GameSys::BaseEntityT")
                // from the registry, and set it as metatable of FT.
                luaL_getmetatable(m_LuaState, TI->Base->ClassName);
                lua_setmetatable(m_LuaState, -2);
            }

            // MT.__index = FT
            lua_setfield(m_LuaState, -2, "__index");

            // This would be the right thing to do, but we don't know the proper type for Destruct<> here.
            // Therefore, MT.__gc is only set in Push().
            //
            // MT.__gc = Destruct
            // lua_pushcfunction(m_LuaState, Destruct<...>);
            // lua_setfield(m_LuaState, -2, "__gc");

            // Clear the stack.
            assert(lua_gettop(m_LuaState)==1);
            lua_pop(m_LuaState, 1);
        }
    }
}


bool ScriptBinderT::IsBound(void* Identity)
{
    const StackCheckerT StackChecker(m_LuaState);

    // Put the REGISTRY["__identity_to_object"] table onto the stack.
    lua_getfield(m_LuaState, LUA_REGISTRYINDEX, "__identity_to_object");

    // Put __identity_to_object[Identity] onto the stack.
    // This should be our table that represents the object.
    lua_pushlightuserdata(m_LuaState, Identity);
    lua_rawget(m_LuaState, -2);

    // Is the object in the __identity_to_object table?
    const bool Result = !lua_isnil(m_LuaState, -1);

    lua_pop(m_LuaState, 1);   // Pop the value.
    lua_pop(m_LuaState, 1);   // Pop the __identity_to_object table.

    return Result;
}


// This is essentially the opposite of Push().
void ScriptBinderT::Disconnect(void* Identity)
{
    const StackCheckerT StackChecker(m_LuaState);

    // Put the REGISTRY["__identity_to_object"] table onto the stack.
    lua_getfield(m_LuaState, LUA_REGISTRYINDEX, "__identity_to_object");

    // Put __identity_to_object[Identity] onto the stack.
    // This should be our table that represents the object.
    lua_pushlightuserdata(m_LuaState, Identity);
    lua_rawget(m_LuaState, -2);

    // If the object was not found in __identity_to_object, there is nothing to do.
    if (lua_isnil(m_LuaState, -1))
    {
        lua_pop(m_LuaState, 1);   // Pop the nil.
        lua_pop(m_LuaState, 1);   // Pop the __identity_to_object table.
        return;
    }

    // Put the contents of the "__userdata_cf" field on top of the stack.
    lua_pushstring(m_LuaState, "__userdata_cf");
    lua_rawget(m_LuaState, -2);

    // Get __gc from the metatable.
    lua_getmetatable(m_LuaState, -1);
    lua_pushstring(m_LuaState, "__gc");
    lua_rawget(m_LuaState, -2);
    lua_remove(m_LuaState, -2);

    // Run the __gc metamethod / the destructor.
    if (lua_iscfunction(m_LuaState, -1))
    {
        lua_pushvalue(m_LuaState, -2);
        lua_call(m_LuaState, 1, 0);
    }
    else
    {
        // Remove whatever was not a function (probably nil).
        lua_pop(m_LuaState, 1);
    }

    // Set the metatable to nil.
    lua_pushnil(m_LuaState);
    lua_setmetatable(m_LuaState, -2);

    // Pop the userdata.
    lua_pop(m_LuaState, 1);

    // Set the metatable to nil.
    lua_pushnil(m_LuaState);
    lua_setmetatable(m_LuaState, -2);

    // Pop the table.
    lua_pop(m_LuaState, 1);

    // Remove the table: __identity_to_object[Identity] = nil
    lua_pushlightuserdata(m_LuaState, Identity);
    lua_pushnil(m_LuaState);
    lua_rawset(m_LuaState, -3);

    // Pop the __identity_to_object table.
    lua_pop(m_LuaState, 1);
}


UniScriptStateT::CoroutineT::CoroutineT()
    : ID(InstCount++),
      State(0),
      NumParams(0),
      WaitTimeLeft(0)
{
}


UniScriptStateT::UniScriptStateT()
    : m_LuaState(NULL),
      m_CheckCppRefsCount(0)
{
    // Open (create, init) a new Lua state.
    m_LuaState = luaL_newstate();

    // Open (load, init) the Lua standard libraries.
    luaL_requiref(m_LuaState, "_G",            luaopen_base,      1); lua_pop(m_LuaState, 1);
    luaL_requiref(m_LuaState, LUA_LOADLIBNAME, luaopen_package,   1); lua_pop(m_LuaState, 1);
    luaL_requiref(m_LuaState, LUA_COLIBNAME,   luaopen_coroutine, 1); lua_pop(m_LuaState, 1);
    luaL_requiref(m_LuaState, LUA_TABLIBNAME,  luaopen_table,     1); lua_pop(m_LuaState, 1);
    luaL_requiref(m_LuaState, LUA_IOLIBNAME,   luaopen_io,        1); lua_pop(m_LuaState, 1);
    luaL_requiref(m_LuaState, LUA_OSLIBNAME,   luaopen_os,        1); lua_pop(m_LuaState, 1);
    luaL_requiref(m_LuaState, LUA_STRLIBNAME,  luaopen_string,    1); lua_pop(m_LuaState, 1);
    luaL_requiref(m_LuaState, LUA_BITLIBNAME,  luaopen_bit32,     1); lua_pop(m_LuaState, 1);
    luaL_requiref(m_LuaState, LUA_MATHLIBNAME, luaopen_math,      1); lua_pop(m_LuaState, 1);

    // Record a pointer to this UniScriptStateT C++ instance in the Lua state,
    // so that our C++-implemented global methods (like \c thread below) can get back to it.
    lua_pushlightuserdata(m_LuaState, this);
    lua_setfield(m_LuaState, LUA_REGISTRYINDEX, "cafu_script_state");

    // Run the one-time initializations of our binding strategy.
    cf::ScriptBinderT Binder(m_LuaState);
    Binder.InitState();

    // Add a table with name "__pending_coroutines_cf" to the registry.
    // This table will be used to keep track of the pending coroutines, making sure that Lua doesn't garbage collect them early.
    lua_newtable(m_LuaState);
    lua_setfield(m_LuaState, LUA_REGISTRYINDEX, "__pending_coroutines_cf");

    // Add an additional function "thread" that registers the given Lua function as a new thread.
    lua_pushcfunction(m_LuaState, RegisterThread);
    lua_setglobal(m_LuaState, "thread");

    // Run the equivalent to "wait=coroutine.yield;" and "waitFrame=coroutine.yield;", that is,
    // provide aliases for coroutine.yield as known from Doom3 map scripting.
    lua_getglobal(m_LuaState, "coroutine");
    lua_getfield(m_LuaState, -1, "yield");
    lua_setglobal(m_LuaState, "wait");
    lua_getfield(m_LuaState, -1, "yield");
    lua_setglobal(m_LuaState, "waitFrame");
    lua_pop(m_LuaState, 1);

    // Did everyone deal properly with the Lua stack so far?
    assert(lua_gettop(m_LuaState)==0);
}


UniScriptStateT::~UniScriptStateT()
{
    lua_close(m_LuaState);
}


namespace
{
    void CountHookFunction(lua_State* CrtState, lua_Debug* ar)
    {
        assert(ar->event==LUA_HOOKCOUNT);

        luaL_error(CrtState, "Instruction count exceeds the predefined limit (infinite loop?).");
    }
}


bool UniScriptStateT::DoString(const char* s, const char* Signature, ...)
{
    const StackCheckerT StackChecker(m_LuaState);

    // Load the string as a chunk, then put the compiled chunk as a function onto the stack.
    if (luaL_loadstring(m_LuaState, s) != 0)
    {
        // Note that we need an extra newline here, because (all?) the Lua error messages don't have one.
        Console->Print(std::string(lua_tostring(m_LuaState, -1))+"\n");

        lua_pop(m_LuaState, 1);   // Pop the error message.
        return false;
    }

    va_list vl;

    va_start(vl, Signature);
    const bool Result=StartNewCoroutine(0, Signature, vl, std::string("custom string: ") + s);
    va_end(vl);

    return Result;
}


bool UniScriptStateT::DoFile(const char* FileName, const char* Signature, ...)
{
    const StackCheckerT StackChecker(m_LuaState);

    // Load the file as a chunk, then put the compiled chunk as a function onto the stack.
    if (luaL_loadfile(m_LuaState, FileName) != 0)
    {
        // Note that we need an extra newline here, because (all?) the Lua error messages don't have one.
        // Console->Warning(std::string("Lua script \"")+FileName+"\" could not be loaded\n");
        // Console->Print(std::string("(")+lua_tostring(m_LuaState, -1)+").\n");
        Console->Print(std::string(lua_tostring(m_LuaState, -1))+"\n");

        lua_pop(m_LuaState, 1);   // Pop the error message.
        return false;
    }

    va_list vl;

    va_start(vl, Signature);
    const bool Result=StartNewCoroutine(0, Signature, vl, std::string("custom file: ") + FileName);
    va_end(vl);

    return Result;
}


bool UniScriptStateT::Call(const char* FuncName, const char* Signature, ...)
{
    // Note that when re-entrancy occurs, we do usually NOT have an empty stack here!
    // That is, when we first call a Lua function the stack is empty, but when the called Lua function
    // in turn calls back into our C++ code (e.g. a console function), and the C++ code in turn gets here,
    // we have a case of re-entrancy and the stack is not empty!
    const StackCheckerT StackChecker(m_LuaState);

    // Get the desired global function.
    lua_getglobal(m_LuaState, FuncName);

    if (!lua_isfunction(m_LuaState, -1))
    {
        // If we get here, this usually means that the value at -1 is just nil, i.e. the
        // function that we would like to call was just not defined in the Lua script.
        lua_pop(m_LuaState, 1);   // Pop whatever is not a function.
        return false;
    }

    va_list vl;

    va_start(vl, Signature);
    const bool Result=StartNewCoroutine(0, Signature, vl, std::string("global function ")+FuncName+"()");
    va_end(vl);

    return Result;
}


void UniScriptStateT::RunPendingCoroutines(float FrameTime)
{
    // Take the opportunity to check if the reference-counted objects are still referenced in C++ code.
    // If not, they're un-anchored, and thus can be garbage collected when they're unused in Lua as well.
    m_CheckCppRefsCount++;

    if (m_CheckCppRefsCount > 100)
    {
        ScriptBinderT Binder(m_LuaState);

        Binder.CheckCppRefs();
        m_CheckCppRefsCount = 0;
    }


    // Iterate over all elements in the REGISTRY["__pending_coroutines_cf"] table, which has all the pending coroutines.
    const int PENDING_COROUTINES_TABLE_IDX=1;
    lua_getfield(m_LuaState, LUA_REGISTRYINDEX, "__pending_coroutines_cf");   // Put REGISTRY["__pending_coroutines_cf"] onto the stack at index 1.
    assert(lua_gettop(m_LuaState)==1);
    assert(lua_istable(m_LuaState, PENDING_COROUTINES_TABLE_IDX));

    // This variable is used to handle the occurrence of re-entrancy.
    // Re-entrancy occurs when a script that is resumed via the call to lua_resume(Crt.State, 0) below
    // manages to add another entry into the PendingCoroutines array, e.g. by calling the game.startNewThread() function.
    unsigned long PendingCoroutines_Size=m_PendingCoroutines.Size();

    for (unsigned long PendingCrtNr=0; PendingCrtNr<PendingCoroutines_Size; PendingCrtNr++)
    {
        CoroutineT& Crt=m_PendingCoroutines[PendingCrtNr];

        Crt.WaitTimeLeft-=FrameTime;
        if (Crt.WaitTimeLeft>0) continue;

        // Set the hook function for the "count" event, so that we can detect and prevent infinite loops.
        // Must do this before the next call to lua_resume, or else the instruction count is *not* reset to zero.
        lua_sethook(Crt.State, CountHookFunction, LUA_MASKCOUNT, 10000);    // Should have a ConVar for the number of instruction counts!?

        // Wait time is over, resume the coroutine.
        const int Result=lua_resume(Crt.State, NULL, Crt.NumParams);

        if (Result==LUA_YIELD)
        {
            // The argument to the coroutine.yield() call is the wait time in seconds until the coroutine is supposed to be resumed.
            // If the argument is not given (or not a number), a wait time of 0 is assumed.
            // In any case, the earliest when the coroutine will be resumed is in the next (subsequent) server Think() frame.

            // Check the argument(s).
            if (lua_gettop(Crt.State)==0) lua_pushnumber(Crt.State, 0);
            if (!lua_isnumber(Crt.State, -1)) lua_pushnumber(Crt.State, 0);

            // Re-new the wait time.
            Crt.WaitTimeLeft=float(lua_tonumber(Crt.State, -1));
            Crt.NumParams   =0;
        }
        else
        {
            // The coroutine either completed normally (the body function returned), or an error occurred.
            // In both cases, we remove it from the list of pending coroutines, as it makes no sense to try to resume it once more.
            if (Result!=0)
            {
                // An error occurred when running the coroutine.
                // Note that we need an extra newline here, because (all?) the Lua error messages don't have one.
                Console->Print(std::string(lua_tostring(Crt.State, -1))+"\n");
            }

            // Remove the Crt.State from the __pending_coroutines_cf table, so that Lua will eventually garbage collect it.
            // __pending_coroutines_cf[Crt.ID]=nil;
            lua_pushnil(m_LuaState);
            lua_rawseti(m_LuaState, PENDING_COROUTINES_TABLE_IDX, Crt.ID);

            m_PendingCoroutines.RemoveAtAndKeepOrder(PendingCrtNr);
            PendingCrtNr--;
            PendingCoroutines_Size--;
        }
    }

    // Make sure that everyone dealt properly with the stack so far,
    // then remove the remaining __pending_coroutines_cf table.
    assert(lua_gettop(m_LuaState)==1);
    lua_pop(m_LuaState, 1);
}


/**
 * This method calls a Lua function in the context of the m_LuaState.
 *
 * As a prerequisite, the caller must have pushed the Lua function to be called onto the stack of m_LuaState,
 * followed by any extra arguments (that is, arguments that are not passed in the Signature/vl parameters) at subsequent stack positions
 * (as for example the alter ego instance of a window that is to be used as the "self" or "this" value of an object-oriented method call).
 * At stack positions "below" the function to be called there may be other stack values that this method will not touch.
 * Additional parameters to the Lua function are appended from the vl parameter list as described by the Signature.
 * Return values of the Lua function are returned to the vl parameter list as described by the Signature.
 *
 * The function call is implemented as the resumption of a coroutine (the coroutine instance is newly created),
 * so that the script code can call coroutine.yield() and its derivatives like wait(), waitFrame(), etc.
 */
bool UniScriptStateT::StartNewCoroutine(int NumExtraArgs, const char* Signature, va_list vl, const std::string& DbgName)
{
    const StackCheckerT StackChecker(m_LuaState, -(1 + NumExtraArgs));

    // Create a new coroutine for this function call (or else they cannot call coroutine.yield()).
    // The new coroutine is pushed onto the stack of m_LuaState as a value of type "thread".
    lua_State* NewThread=lua_newthread(m_LuaState);

    // Move the new thread from the top (-1) stack position to the position "below" the function and its extra parameters.
    lua_insert(m_LuaState, -(1+NumExtraArgs+1));

    // Move the function and its extra parameters to the stack of NewThread.
    lua_xmove(m_LuaState, NewThread, NumExtraArgs+1);


    // Put all other arguments for the function onto the stack of NewThread.
    // *********************************************************************

    const char* Results="";

    for (const char* c=Signature; *c; c++)
    {
        if (*c=='>')
        {
            Results=c+1;
            break;
        }

        switch (*c)
        {
            // According to the g++ compiler, bool is promoted to int, and float is promoted to double when passed through '...',
            // and therefore we should pass int and double to va_arg() instead of bool and float.
            case 'b': lua_pushboolean(NewThread, va_arg(vl, /*bool*/int    )); break;
            case 'i': lua_pushinteger(NewThread, va_arg(vl, int            )); break;
            case 'f': lua_pushnumber (NewThread, va_arg(vl, /*float*/double)); break;
            case 'd': lua_pushnumber (NewThread, va_arg(vl, double         )); break;
            case 's': lua_pushstring (NewThread, va_arg(vl, char*          )); break;
            case 'G': lua_getglobal  (NewThread, va_arg(vl, char*          )); break;

            // case 'O':    // TODO: This is what we really need here...
                // // Find given object in weak table...
                // break;

            default:
                Console->Warning(std::string("Invalid signature \"")+Signature+"\" in call to "+DbgName+".\n");
                lua_settop(NewThread, 0);   // Clear the stack of NewThread (the function and its arguments).
                lua_pop(m_LuaState,  1);    // Pop the thread (it will then be garbage collected).
                return false;
        }

        // WARNING: Do NOT issue a call like   lua_tostring(NewThread, -1)   here!
        // This is because "If the value is a number, then lua_tolstring also changes the actual value in the stack to a string.",
        // as described at http://www.lua.org/manual/5.1/manual.html#lua_tolstring
    }

    const int ResCount=int(strlen(Results));


    // Do the actual function call.
    // ****************************

    // Set the hook function for the "count" event, so that we can detect and prevent infinite loops.
    lua_sethook(NewThread, CountHookFunction, LUA_MASKCOUNT, 10000);    // Should have a ConVar for the number of instruction counts!?

    // Start the new coroutine.
    const int ThreadResult=lua_resume(NewThread, NULL, lua_gettop(NewThread)-1);

 /* if (lua_pcall(m_LuaState, 1+ArgCount, ResCount, 0)!=0)
    {
        Console->Warning(std::string("Calling Lua script method ")+MethodName+"() on window at 0x"+cf::va("%p", this)+" failed ("+lua_tostring(m_LuaState, -1)+").\n");
        Console->Print(std::string(lua_tostring(m_LuaState, -1))+"\n");   // Repeat the error message in the case the console doesn't line-wrap properly.
        lua_pop(m_LuaState, 2);   // Pop the error message and the original window table from the stack.
        return false;
    } */


    // Deal with the results.
    // **********************

    if (ThreadResult==0)
    {
        // The coroutine returned normally, now return the results to the caller.
        int StackIndex=1;

        // If we expect more arguments back than we got, push a single nil that will help to fill-up any number of missing arguments.
        if (ResCount>lua_gettop(NewThread)) lua_pushnil(NewThread);

        for (const char* c=Results; *c; c++)
        {
            switch (*c)
            {
                case 'b': *va_arg(vl, bool*  )=lua_toboolean(NewThread, StackIndex)!=0; break;
                case 'i': *va_arg(vl, int*   )=lua_tointeger(NewThread, StackIndex); break;
                case 'f': *va_arg(vl, float* )=float(lua_tonumber(NewThread, StackIndex)); break;
                case 'd': *va_arg(vl, double*)=lua_tonumber(NewThread, StackIndex); break;
             // case 'E': *va_arg(vl, BaseEntityT**)=(BaseEntityT*)ScriptStateT::GetCheckedObjectParam(NewThread, StackIndex, BaseEntityT::TypeInfo); break;

                case 's':
                {
                    const char*        s=lua_tostring(NewThread, StackIndex);
                    static const char* e="";

                    *va_arg(vl, const char**)=(s!=NULL) ? s : e;
                    break;
                }

                case 'S':
                {
                    const char* s=lua_tostring(NewThread, StackIndex);

                    *va_arg(vl, std::string*)=(s!=NULL) ? s : "";
                    break;
                }

                default:
                    Console->Warning(std::string("Invalid results signature \"")+Signature+"\" in call to "+DbgName+".\n");
                    break;
            }

            if (StackIndex<lua_gettop(NewThread)) StackIndex++;
        }

        lua_settop(NewThread, 0);   // Pop everything (the results) from the NewThread stack.
        lua_pop(m_LuaState, 1);     // Pop the thread (it will then be garbage collected).
        return true;
    }

    if (ThreadResult==LUA_YIELD)
    {
        // The argument to the coroutine.yield() call is the wait time in seconds until the coroutine is supposed to be resumed.
        // If the argument is not given (or not a number), a wait time of 0 is assumed.
        // In any case, the earliest when the coroutine will be resumed is in the next (subsequent) server Think() frame.

        // Check the argument(s).
        if (lua_gettop(NewThread)==0) lua_pushnumber(NewThread, 0);
        if (!lua_isnumber(NewThread, -1)) lua_pushnumber(NewThread, 0);

        CoroutineT Crt;

     // Crt.ID          =(already got a unique value assigned by CoroutineT ctor);
        Crt.State       =NewThread;
        Crt.NumParams   =0;
        Crt.WaitTimeLeft=float(lua_tonumber(NewThread, -1));

        m_PendingCoroutines.PushBack(Crt);

        // REGISTRY["__pending_coroutines_cf"][Crt.ID]=Crt.State;
        lua_getfield(m_LuaState, LUA_REGISTRYINDEX, "__pending_coroutines_cf");   // Put REGISTRY["__pending_coroutines_cf"] onto the stack (index -1).
        assert(lua_istable(m_LuaState, -1));  // Make sure that REGISTRY["__pending_coroutines_cf"] really is a table.
        lua_pushvalue(m_LuaState, -2);        // Duplicate the "thread" (==Ctr.State) value from index -2 to the top of the stack.
        lua_rawseti(m_LuaState, -2, Crt.ID);  // table[Crt.ID]=Crt.State;    -- Pops the value from the stack.
        lua_pop(m_LuaState, 1);               // Pop the table again.

        if (ResCount>0)
            Console->Warning("The call to "+DbgName+" yielded (expected return values matching signature \""+Signature+"\" instead).\n");

        lua_settop(NewThread, 0);   // Pop everything (the parameters to coroutine.yield()) from the NewThread stack.
        lua_pop(m_LuaState, 1);     // Pop the thread (it's kept inside the __pending_coroutines_cf table now).
        return ResCount==0;
    }

    // ThreadResult is not 0 and not LUA_YIELD, so an error occurred when running the coroutine.
    // Note that we need an extra newline here, because (all?) the Lua error messages don't have one.
    Console->Warning("Lua error in call to "+DbgName+":\n");
    Console->Print(std::string(lua_tostring(NewThread, -1))+"\n");

    // Note that the stack of NewThread was not unwound after the error (but we currently have no use for it).
    lua_settop(NewThread, 0);   // Just pop everything from the NewThread stack.
    lua_pop(m_LuaState, 1);     // Pop the thread (it will then be garbage collected).
    return false;
}


/*static*/ int UniScriptStateT::RegisterThread(lua_State* LuaState)
{
    lua_getfield(LuaState, LUA_REGISTRYINDEX, "cafu_script_state");
    UniScriptStateT* ScriptState=static_cast<UniScriptStateT*>(lua_touserdata(LuaState, -1));
    lua_pop(LuaState, 1);
    if (!ScriptState) return luaL_error(LuaState, "ScriptStateT C++ instance not found in LuaState");

    // Our stack (parameter list) comes with the function to be registered as a new thread, and the parameters for its initial call.
    luaL_argcheck(LuaState, lua_isfunction(LuaState, 1), 1, "function expected");
    const unsigned long StackSize=lua_gettop(LuaState);

    // Put the __pending_coroutines_cf table on top of the stack,
    // where we will insert ("anchor") the new thread below.
    lua_getfield(LuaState, LUA_REGISTRYINDEX, "__pending_coroutines_cf");   // Put REGISTRY["__pending_coroutines_cf"] onto the stack at index 1.
    assert(lua_istable(LuaState, -1));

    CoroutineT Crt;

 // Crt.ID          =(already got a unique value assigned by CoroutineT ctor);
    Crt.State       =lua_newthread(LuaState);   // Creates a new coroutine and puts it onto the stack of LuaState.
    Crt.NumParams   =StackSize-1;               // The number of function parameters in the stack of Crt.State for the upcoming call of lua_resume().
    Crt.WaitTimeLeft=0;                         // Run at next opportunity, i.e. at next call to RunPendingCoroutines().

    ScriptState->m_PendingCoroutines.PushBack(Crt);

    // The thread value is at the top (-1) of the stack, the __pending_coroutines_cf table directly below it at -2.
    // Now anchor the new thread at index Crt.ID in the __pending_coroutines_cf table.
    lua_rawseti(LuaState, -2, Crt.ID);          // __pending_coroutines_cf[Crt.ID]=Crt.State;    -- Pops the value from the stack.

    // Remove the __pending_coroutines_cf table from the stack again, restoring the stack to its original state.
    lua_pop(LuaState, 1);
    assert(lua_gettop(LuaState)==int(StackSize));

    // Preparing for the first lua_resume() call for Crt.State,
    // move the body ("main") function plus its paramters on the stack of Crt.State.
    lua_xmove(LuaState, Crt.State, StackSize);
    return 0;
}


#include <fstream>
#include <map>

/*static*/ void UniScriptStateT::CheckCallbackDoc(const cf::TypeSys::TypeInfoT* TI, const std::string& MethodName, int NumExtraArgs, const char* Signature)
{
    const std::string s = std::string(TI->ClassName) + "::" + MethodName + "(" + Signature + ")";

    while (TI)
    {
        if (TI->DocCallbacks)
        {
            for (unsigned int Nr = 0; TI->DocCallbacks[Nr].Name; Nr++)
            {
                if (MethodName == TI->DocCallbacks[Nr].Name)
                {
                    // TODO: The method name matches, but does Signature also match
                    // TI->DocCallbacks[Nr].Parameters?
                    return;
                }
            }
        }

        TI = TI->Base;
    }

    static std::map<std::string, bool> UniqueCallbacks;

    if (UniqueCallbacks[s]) return;
    UniqueCallbacks[s] = true;

    static std::ofstream LogFile("callbacks.txt", std::ios::app);

    if (!LogFile.bad())
    {
        LogFile << s << " is not documented!\n";
    }
}
