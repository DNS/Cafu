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


using namespace cf;


unsigned int UniScriptStateT::CoroutineT::InstCount = 0;


UniScriptStateT::CoroutineT::CoroutineT()
    : ID(InstCount++),
      State(0),
      NumParams(0),
      WaitTimeLeft(0)
{
}


UniScriptStateT::UniScriptStateT()
    : m_LuaState(NULL)
{
    // Open (create, init) a new Lua state.
    m_LuaState=lua_open();

    // Open (load, init) the Lua standard libraries.
    lua_pushcfunction(m_LuaState, luaopen_base);    lua_pushstring(m_LuaState, "");              lua_call(m_LuaState, 1, 0);  // Opens the basic library.
    lua_pushcfunction(m_LuaState, luaopen_package); lua_pushstring(m_LuaState, LUA_LOADLIBNAME); lua_call(m_LuaState, 1, 0);  // Opens the package library.
    lua_pushcfunction(m_LuaState, luaopen_table);   lua_pushstring(m_LuaState, LUA_TABLIBNAME);  lua_call(m_LuaState, 1, 0);  // Opens the table library.
    lua_pushcfunction(m_LuaState, luaopen_io);      lua_pushstring(m_LuaState, LUA_IOLIBNAME);   lua_call(m_LuaState, 1, 0);  // Opens the I/O library.
    lua_pushcfunction(m_LuaState, luaopen_os);      lua_pushstring(m_LuaState, LUA_OSLIBNAME);   lua_call(m_LuaState, 1, 0);  // Opens the OS library.
    lua_pushcfunction(m_LuaState, luaopen_string);  lua_pushstring(m_LuaState, LUA_STRLIBNAME);  lua_call(m_LuaState, 1, 0);  // Opens the string lib.
    lua_pushcfunction(m_LuaState, luaopen_math);    lua_pushstring(m_LuaState, LUA_MATHLIBNAME); lua_call(m_LuaState, 1, 0);  // Opens the math lib.

    // Record a pointer to this UniScriptStateT C++ instance in the Lua state,
    // so that our C++-implemented global methods (like \c thread below) can get back to it.
    lua_pushlightuserdata(m_LuaState, this);
    lua_setfield(m_LuaState, LUA_REGISTRYINDEX, "cafu_script_state");

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


void UniScriptStateT::Init(const cf::TypeSys::TypeInfoManT& TIM)
{
    // For each class that the TIM knows about, add a (meta-)table to the registry of the Lua state.
    // The (meta-)table holds the Lua methods that the respective class implements in C++,
    // and is to be used as metatable for instances of this class.
    for (unsigned long RootNr=0; RootNr<TIM.GetTypeInfoRoots().Size(); RootNr++)
    {
        for (const cf::TypeSys::TypeInfoT* TI=TIM.GetTypeInfoRoots()[RootNr]; TI!=NULL; TI=TI->GetNext())
        {
            assert(lua_gettop(m_LuaState)==0);

            // Create a new table T and add it into the registry table with TI->ClassName (e.g. "cf::GuiSys::WindowT" or "cf::GameSys::EntMoverT") as the key and T as the value.
            // This also leaves T on top of the stack. See PiL2 chapter 28.2 for more details.
            luaL_newmetatable(m_LuaState, TI->ClassName);

            // See PiL2 chapter 28.3 for a great explanation on what is going on here.
            // Essentially, we set T.__index = T (the luaL_newmetatable() function left T on the top of the stack).
            lua_pushvalue(m_LuaState, -1);              // Pushes/duplicates the new table T on the stack.
            lua_setfield(m_LuaState, -2, "__index");    // T.__index = T;

            // Now insert the functions listed in TI->MethodsList into T (the table on top of the stack).
            if (TI->MethodsList!=NULL)
                luaL_register(m_LuaState, NULL, TI->MethodsList);

            // If TI has a base class, model that relationship for T, too, by setting the metatable of the base class as the metatable for T.
            // Note that this works because the for-loop (over TI) enumerates the base classes always before their child classes!
            if (TI->Base)
            {
                assert(strcmp(TI->BaseClassName, TI->Base->ClassName)==0);

                // Get the metatable M with name (key) TI->Base->ClassName (e.g. "cf::GameSys::BaseEntityT")
                // from the registry, and set it as metatable of T.
                luaL_getmetatable(m_LuaState, TI->Base->ClassName);
                lua_setmetatable(m_LuaState, -2);
            }

            // Clear the stack.
            assert(lua_gettop(m_LuaState)==1);
            lua_pop(m_LuaState, 1);
        }
    }
}


namespace
{
    void CountHookFunction(lua_State* CrtState, lua_Debug* ar)
    {
        assert(ar->event==LUA_HOOKCOUNT);

        luaL_error(CrtState, "Instruction count exceeds the predefined limit (infinite loop?).");
    }
}


bool UniScriptStateT::Run(const char* Chunk)
{
    // Load the chunk as a function onto the stack.
    if (luaL_loadstring(m_LuaState, Chunk) != 0)
    {
        // Note that we need an extra newline here, because (all?) the Lua error messages don't have one.
        Console->Print(std::string(lua_tostring(m_LuaState, -1))+"\n");

        lua_pop(m_LuaState, 1);   // Pop the error message.
        return false;
    }

    va_list     vl;
    const char* Signature="";

    va_start(vl, Signature);
    const bool Result=StartNewCoroutine(0, Signature, vl, std::string("custom chunk: ") + Chunk);
    va_end(vl);

    return Result;
}


bool UniScriptStateT::Call(const char* FuncName, const char* Signature, ...)
{
    // Note that when re-entrancy occurs, we do usually NOT have an empty stack here!
    // That is, when we first call a Lua function the stack is empty, but when the called Lua function
    // in turn calls back into our C++ code (e.g. a console function), and the C++ code in turn gets here,
    // we have a case of re-entrancy and the stack is not empty!
    // That is, the assert() statement in the next line does not generally hold.
    // assert(lua_gettop(LuaState)==0);

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
        const int Result=lua_resume(Crt.State, Crt.NumParams);

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
    const int ThreadResult=lua_resume(NewThread, lua_gettop(NewThread)-1);

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
