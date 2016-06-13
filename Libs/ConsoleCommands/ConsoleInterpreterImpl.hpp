/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CONSOLE_INTERPRETER_IMPL_HPP_INCLUDED
#define CAFU_CONSOLE_INTERPRETER_IMPL_HPP_INCLUDED

#include "ConsoleInterpreter.hpp"


/// This class provides an implementation for the ConsoleInterpreterI interface.
///
/// The key idea is that the console program state is represented by a Lua program / Lua state.
/// - Each ConFuncT that is registered with the interpreter is made available as a Lua function.
///   All these functions are grouped in a common table (used as a "namespace").
///   The name of that table is defined by the implementation, it's normally either "c" (Cafu-
///   specific functions), or "_G" (the table of global variables, this is more convenient).
///   When a ConFuncT is unregistered, it is of course removed again from the table.
/// - The ConVarTs also have a dedicated table that contains them, this is the same table as for
///   the functions. However, the situation is a bit more complicated here:
///   a) When a ConVarT that is not (yet) registered with the interpreter, the user can still use
///      and work with a value of that name in our common table. The value is then a native Lua value.
///   b) When a ConVarT is registered, the table value is set as the default value for the ConVarT,
///      the table value is cleared to nil, and all subsequent variable accesses (reads and writes)
///      the variable are (re-)routed to the ConVarT by the related metamethods.
///   c) When a ConVarT is unregistered again, its value at that time is re-established in the table
///      under the name of the ConVarT, so that users can transparently continue to use it.
///   In summary, registered ConVarTs are thought to "overlay" the corresponding Lua values.
///   This makes it possible to define default value before the ConVarT is registered for the first time,
///   and to preserve their values when a ConVarT becomes temporarily unregistered.
class ConsoleInterpreterImplT : public ConsoleInterpreterI
{
    public:

    ConsoleInterpreterImplT();
    ~ConsoleInterpreterImplT();

    // The methods from the ConsoleInterpreterI interface.
    void        Register(ConVarT* ConVar);
    void        Register(ConFuncT* ConFunc);
    void        Unregister(ConVarT* ConVar);
    void        Unregister(ConFuncT* ConFunc);
    ConFuncT*   FindFunc(const std::string& Name);
    ConVarT*    FindVar(const std::string& Name);
    std::string LineCompletion(const std::string& LineBegin, ArrayT<std::string>& Completions);
    bool        RunCommand(const std::string& Input);

    static int Lua_get_Callback(lua_State* LuaState);
    static int Lua_set_Callback(lua_State* LuaState);
    static int ConFunc_Help_Callback(lua_State* LuaState);
    static int ConFunc_List_Callback(lua_State* LuaState);


    private:

    lua_State*        LuaState;             ///< The Lua environment with the actual console state.
    ArrayT<ConVarT*>  RegisteredConVars;    ///< This is the global list of master convars.
    ArrayT<ConFuncT*> RegisteredConFuncs;   ///< This is the global list of master confuncs.
};

#endif
