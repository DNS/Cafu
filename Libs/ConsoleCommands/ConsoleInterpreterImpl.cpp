/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

////////////////////////////////////////////////////////////////////////////////////////////
// This compilation unit (source code file) must *only* be linked to the main executable, //
// *not* to any (subsequently loaded) dll! This is because dlls don't have their own      //
// ConsoleInterpreterImplT instance, but rather get a pointer to that of the executable!  //
////////////////////////////////////////////////////////////////////////////////////////////

#include "ConsoleInterpreterImpl.hpp"
#include "ConFunc.hpp"
#include "ConVar.hpp"
#include "Console.hpp"
#include "Console_Lua.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

#include <algorithm>
#include <sstream>
#include <cassert>


// The name of the Lua table we register all our ConVarTs and ConFuncTs in.
// This is usually "c" (short for "Cafu" or "console") or "_G" (the table of global variables).
// "_G" is less orderly, but a lot more convenient for users of course.  :-)
static const char* CAFU_TABLE="_G";

// This implementation of the ConsoleInterpreterImplT class maintains the invariant that
// at the end of each method, the CAFU_TABLE is left as the only element on the Lua stack.
// This in turn makes the implementation of the methods a lot easier and more convenient,
// because it makes the ongoing getting and verification of the table unnecessary.
#define StackHasCafuTable()    (lua_gettop(LuaState)==1 && lua_istable(LuaState, 1))


ConsoleInterpreterImplT::ConsoleInterpreterImplT()
    : LuaState(NULL)
{
    // Initialize Lua.
    LuaState = luaL_newstate();

    luaL_requiref(LuaState, "_G",            luaopen_base,      1); lua_pop(LuaState, 1);
    luaL_requiref(LuaState, LUA_LOADLIBNAME, luaopen_package,   1); lua_pop(LuaState, 1);
    luaL_requiref(LuaState, LUA_COLIBNAME,   luaopen_coroutine, 1); lua_pop(LuaState, 1);
    luaL_requiref(LuaState, LUA_TABLIBNAME,  luaopen_table,     1); lua_pop(LuaState, 1);
    luaL_requiref(LuaState, LUA_IOLIBNAME,   luaopen_io,        1); lua_pop(LuaState, 1);
    luaL_requiref(LuaState, LUA_OSLIBNAME,   luaopen_os,        1); lua_pop(LuaState, 1);
    luaL_requiref(LuaState, LUA_STRLIBNAME,  luaopen_string,    1); lua_pop(LuaState, 1);
    luaL_requiref(LuaState, LUA_BITLIBNAME,  luaopen_bit32,     1); lua_pop(LuaState, 1);
    luaL_requiref(LuaState, LUA_MATHLIBNAME, luaopen_math,      1); lua_pop(LuaState, 1);

    // Load the console library. (Adds a global table with name "Console" to the LuaState with the functions of the ConsoleI interface.)
    cf::Console_RegisterLua(LuaState);


    // Make sure that the table with name CAFU_TABLE exists.
    // This table will be the "proxy" table for getting and setting the console variables,
    // see PiL2 chapter 13.4 for details about table access metamethods.
    lua_getglobal(LuaState, CAFU_TABLE);

    if (!lua_istable(LuaState, -1))
    {
        if (!lua_isnil(LuaState, -1))
        {
            // No table at name CAFU_TABLE, but something else??
            Console->Warning(cf::va("Global variable \"%s\" is not a table or nil, but a \"%s\" - overwriting with table.\n",
                CAFU_TABLE, lua_typename(LuaState, lua_type(LuaState, -1))));
        }

        lua_pop(LuaState, 1);                   // Pop whatever it was.
        lua_newtable(LuaState);                 // Create a new table.
        lua_pushvalue(LuaState, -1);            // Pushes/duplicates the new table on the stack.
        lua_setglobal(LuaState, CAFU_TABLE);    // Pop the copy and set it as a global variable with name CAFU_TABLE.
    }

    // This leaves the CAFU_TABLE on the stack.
    assert(StackHasCafuTable());


    // Create a new table T and add it into the registry table with "ConVars_Mediator" as the key and T as the value.
    // This also leaves T on top of the stack. See PiL2 chapter 28.2 for more details.
    // T will be used as the metatable for the CAFU_TABLE.
    luaL_newmetatable(LuaState, "ConVars_Mediator");

    static const luaL_Reg MediatorMethods[]=
    {
        { "__index",    Lua_get_Callback },
        { "__newindex", Lua_set_Callback },
     // { "__tostring", toString },
        { NULL, NULL }
    };

    // Insert the functions listed in MediatorMethods into T (the table on top of the stack).
    luaL_setfuncs(LuaState, MediatorMethods, 0);

    // Now set T as the metatable of CAFU_TABLE, "CAFU_TABLE.__metatable=T;".
    // This removes T from the stack, leaving only the CAFU_TABLE.
    lua_setmetatable(LuaState, -2);

    // Note that "our" table is intentionally left on the stack, for convenient access by other methods below.
    // See the comment for the StackHasCafuTable() macro for more details.
    assert(StackHasCafuTable());
}


ConsoleInterpreterImplT::~ConsoleInterpreterImplT()
{
    // Unfortunately we cannot   assert(RegisteredConVars.Size()==0);   here, because some
    // static ConVarTs might be destructed after us (if this ConsoleInterpreterImplT was static, too).

    // Unfortunately we cannot   assert(RegisteredConFuncs.Size()==0);   here, because some
    // static ConFuncTs might be destructed after us (if this ConsoleInterpreterImplT was static, too).

    // Close Lua.
    lua_close(LuaState);
}


void ConsoleInterpreterImplT::Register(ConVarT* ConVar)
{
    // Make sure that we have no console variable with the same name in our list yet.
    for (unsigned long ConVarNr=0; ConVarNr<RegisteredConVars.Size(); ConVarNr++)
    {
        // If already registered, don't register again.
        if (RegisteredConVars[ConVarNr]==ConVar) return;

        if (RegisteredConVars[ConVarNr]->Name==ConVar->Name)
        {
            Console->Warning(std::string("Duplicate definition attempt:\nConsole variable with name \"")+ConVar->Name+"\" already defined!\n");
            return;
        }
    }

    RegisteredConVars.PushBack(ConVar);


    // Note that "our" CAFU_TABLE should always be on the stack.
    assert(StackHasCafuTable());

    // See if there is already a value with this name in the table (raw access, no metamethod), that is, see if CAFU_TABLE[ConVar->Name]!=nil.
    // If so, consider this value as a default value, set it as the value of ConVar, then raw-set CAFU_TABLE[ConVar->Name]=nil so that future
    // accesses of CAFU_TABLE[ConVar->Name] occur via the metamethods.
    // In short, "move" a possibly preexisting value in CAFU_TABLE from there into the ConVarT.
    lua_pushstring(LuaState, ConVar->Name.c_str());
    lua_rawget(LuaState, -2);

    if (!lua_isnil(LuaState, -1))
    {
        switch (ConVar->GetType())
        {
            case ConVarT::String:
            {
                const char* Value=lua_tostring(LuaState, -1);

                if (Value==NULL) Console->Warning(std::string("Cannot convert the default value for ")+ConVar->Name+" to type \"string\".\n");
                ConVar->SetValue(Value!=NULL ? std::string(Value) : "");
                break;
            }

            case ConVarT::Integer:
                ConVar->SetValue(lua_tointeger(LuaState, -1));
                break;

            case ConVarT::Bool:
                // I also want to treat the number 0 as false, not just "false" and "nil".
                ConVar->SetValue(lua_isnumber(LuaState, -1) ? lua_tointeger(LuaState, -1)!=0 : lua_toboolean(LuaState, -1)!=0);
                break;

            case ConVarT::Double:
                ConVar->SetValue(lua_tonumber(LuaState, -1));
                break;
        }

        // Pop the old value of CAFU_TABLE[ConVar->Name] from the stack. (This leaves only the CAFU_TABLE.)
        lua_pop(LuaState, 1);
        assert(lua_gettop(LuaState)==1);

        // "Delete" the ConVar->Name entry from the CAFU_TABLE, that is, raw-set CAFU_TABLE[ConVar->Name] to nil.
        lua_pushstring(LuaState, ConVar->Name.c_str());
        lua_pushnil(LuaState);
        lua_rawset(LuaState, -3);
    }
    else
    {
        // There was no entry in CAFU_TABLE for ConVar->Name, that is, the raw value of cv[ConVar->Name] was nil.
        // So just pop the nil from the stack and be done.
        lua_pop(LuaState, 1);
    }

    assert(StackHasCafuTable());
}


void ConsoleInterpreterImplT::Register(ConFuncT* ConFunc)
{
    // Make sure that we have no console function with the same name in our list yet.
    for (unsigned long ConFuncNr=0; ConFuncNr<RegisteredConFuncs.Size(); ConFuncNr++)
    {
        // If already registered, don't register again.
        if (RegisteredConFuncs[ConFuncNr]==ConFunc) return;

        if (RegisteredConFuncs[ConFuncNr]->Name==ConFunc->Name)
        {
            Console->Warning(std::string("Duplicate definition attempt:\nConsole function with name \"")+ConFunc->Name+"\" already defined!\n");
            return;
        }
    }

    RegisteredConFuncs.PushBack(ConFunc);


    // Register the function with Lua in the CAFU_TABLE.
    // Using luaL_setfuncs() is not possible here, because it uses metamethods (is this still true with Lua 5.2?).
    lua_pushstring(LuaState, ConFunc->GetName().c_str());
    lua_pushcfunction(LuaState, ConFunc->LuaCFunction);
    lua_rawset(LuaState, -3);

    assert(StackHasCafuTable());
}


void ConsoleInterpreterImplT::Unregister(ConVarT* ConVar)
{
    for (unsigned long ConVarNr=0; ConVarNr<RegisteredConVars.Size(); ConVarNr++)
        if (RegisteredConVars[ConVarNr]==ConVar)
        {
            RegisteredConVars.RemoveAt(ConVarNr);
            break;
        }


    // Save the value of ConVar by storing it in the CAFU_TABLE, using raw-set.
    // This keeps the variable available to the script even though the ConVarT has been unregistered.
    // Even better, if ConVar is registered again later (see Register() method), the value will then
    // be "moved back" from the CAFU_TABLE to the ConVarT as the default value!
    lua_pushstring(LuaState, ConVar->Name.c_str());

    switch (ConVar->Type)
    {
        case ConVarT::String:  lua_pushstring (LuaState, ConVar->GetValueString().c_str()); break;
        case ConVarT::Integer: lua_pushinteger(LuaState, ConVar->GetValueInt()           ); break;
        case ConVarT::Bool:    lua_pushboolean(LuaState, ConVar->GetValueBool()          ); break;
        case ConVarT::Double:  lua_pushnumber (LuaState, ConVar->GetValueDouble()        ); break;
    }

    lua_rawset(LuaState, -3);

    assert(StackHasCafuTable());
}


void ConsoleInterpreterImplT::Unregister(ConFuncT* ConFunc)
{
    for (unsigned long ConFuncNr=0; ConFuncNr<RegisteredConFuncs.Size(); ConFuncNr++)
        if (RegisteredConFuncs[ConFuncNr]==ConFunc)
        {
            RegisteredConFuncs.RemoveAt(ConFuncNr);
            break;
        }


    // Remove the console function from the CAFU_TABLE by setting CAFU_TABLE.FuncName to nil.
    lua_pushstring(LuaState, ConFunc->GetName().c_str());
    lua_pushnil(LuaState);
    lua_rawset(LuaState, -3);

    assert(StackHasCafuTable());
}


ConFuncT* ConsoleInterpreterImplT::FindFunc(const std::string& Name)
{
    for (unsigned long ConFuncNr=0; ConFuncNr<RegisteredConFuncs.Size(); ConFuncNr++)
        if (RegisteredConFuncs[ConFuncNr]->GetName()==Name)
            return RegisteredConFuncs[ConFuncNr];

    return NULL;
}


ConVarT* ConsoleInterpreterImplT::FindVar(const std::string& Name)
{
    for (unsigned long ConVarNr=0; ConVarNr<RegisteredConVars.Size(); ConVarNr++)
        if (RegisteredConVars[ConVarNr]->GetName()==Name)
            return RegisteredConVars[ConVarNr];

    return NULL;
}


std::string ConsoleInterpreterImplT::LineCompletion(const std::string& LineBegin, ArrayT<std::string>& Completions)
{
    std::string::size_type PartialToken1st=LineBegin.length();

    while (PartialToken1st>0 && (isalnum(LineBegin[PartialToken1st-1]) || LineBegin[PartialToken1st-1]=='_'))
    {
        PartialToken1st--;
    }

    const std::string            PartialToken   =std::string(LineBegin, PartialToken1st);
    const std::string::size_type PartialTokenLen=PartialToken.length();

    for (unsigned long ConVarNr=0; ConVarNr<RegisteredConVars.Size(); ConVarNr++)
        if (RegisteredConVars[ConVarNr]->GetName().compare(0, PartialTokenLen, PartialToken)==0)
            Completions.PushBack(RegisteredConVars[ConVarNr]->GetName());

#if 0
    for (unsigned long ConFuncNr=0; ConFuncNr<RegisteredConFuncs.Size(); ConFuncNr++)
        if (RegisteredConFuncs[ConFuncNr]->GetName().compare(0, PartialTokenLen, PartialToken)==0)
            Completions.PushBack(RegisteredConFuncs[ConFuncNr]->GetName());
#else
    lua_pushnil(LuaState);  // Push the initial key.
    while (lua_next(LuaState, -2)!=0)
    {
        // The key is now at stack index -2, the value is at index -1.
        // Remove the value right now, because we never need it in this loop.
        // Note that this moves the key to stack index -1.
        lua_pop(LuaState, 1);

        if (!lua_isstring(LuaState, -1))
        {
            Console->DevWarning(std::string("Skipped unexpected key type \"")+lua_typename(LuaState, lua_type(LuaState, -1))+"\" while traversing table \""+CAFU_TABLE+"\".\n");
            continue;
        }

        const std::string Key=lua_tostring(LuaState, -1);

        if (Key.compare(0, PartialTokenLen, PartialToken)==0)
            Completions.PushBack(Key);
    }
#endif


    // Determine the prefix that is common to all completions.
    if (Completions.Size()==0) return "";

    std::string CommonPrefix=Completions[0];

    for (unsigned long CompletionNr=1; CompletionNr<Completions.Size(); CompletionNr++)
    {
        const std::string::size_type MaxLen=std::min(CommonPrefix.length(), Completions[CompletionNr].length());
        std::string NewCP;

        for (unsigned long c=0; c<MaxLen && CommonPrefix[c]==Completions[CompletionNr][c]; c++)
            NewCP+=CommonPrefix[c];

        CommonPrefix=NewCP;
    }

    assert(StackHasCafuTable());

    // Of the CommonPrefix, return only the part that is right of the PartialToken.
    return std::string(CommonPrefix, PartialTokenLen);
}


// Note that this function can be reentrant (call itself recursively), e.g. when the Main Menu GUI
// calls RunCommand("changeLevel()"), the changeLevel() implementation in turn calls back into the
// Main Menu GUI (letting it know the new server state), and from there the Main Menu GUI in turn
// calls e.g. RunCommand("MusicLoad()").
bool ConsoleInterpreterImplT::RunCommand(const std::string& Input)
{
#ifndef NDEBUG
    static unsigned long ReentrancyCount=0;
    ReentrancyCount++;
#endif

    if (luaL_loadstring(LuaState, Input.c_str())!=0 || lua_pcall(LuaState, 0, 0, 0)!=0)
    {
        const char* ErrorMsg=lua_tostring(LuaState, -1);

        // Note that we need an extra newline here, because (all?) the Lua error messages don't have one.
        Console->Print(std::string(ErrorMsg!=NULL ? ErrorMsg : "Unknown error.")+"\n");

        lua_pop(LuaState, 1);
#ifndef NDEBUG
    ReentrancyCount--;
#endif
        assert(ReentrancyCount>0 || StackHasCafuTable());
        return false;
    }

#ifdef DEBUG
    ReentrancyCount--;
#endif
    assert(ReentrancyCount>0 || StackHasCafuTable());
    return true;
}


int ConsoleInterpreterImplT::Lua_get_Callback(lua_State* LuaState)
{
    // This function serves as the __index metamethod of the CAFU_TABLE.
    // We are given the CAFU_TABLE instance as the first and the key (name of the ConVar) as the second parameter.
    const char* ConVarName=luaL_checkstring(LuaState, 2);
    ConVarT*    ConVar    =ConsoleInterpreter->FindVar(ConVarName);

    if (ConVar==NULL) return luaL_error(LuaState, "Unknown identifier \"%s\".\n", ConVarName);

    switch (ConVar->Type)
    {
        case ConVarT::String:  lua_pushstring (LuaState, ConVar->GetValueString().c_str()); break;
        case ConVarT::Integer: lua_pushinteger(LuaState, ConVar->GetValueInt()           ); break;
        case ConVarT::Bool:    lua_pushboolean(LuaState, ConVar->GetValueBool()          ); break;
        case ConVarT::Double:  lua_pushnumber (LuaState, ConVar->GetValueDouble()        ); break;
    }

    return 1;
}


int ConsoleInterpreterImplT::Lua_set_Callback(lua_State* LuaState)
{
    // This function serves as the __newindex metamethod of the CAFU_TABLE.
    // We are given the CAFU_TABLE instance as the first, the key (name of the ConVar) as the second, and the new value as the third parameter.
    const char* ConVarName=luaL_checkstring(LuaState, 2);
    ConVarT*    ConVar    =ConsoleInterpreter->FindVar(ConVarName);

    if (ConVar==NULL)
    {
        // Okay, we have no registered ConVarT with this name,
        // so simply raw-write the value into the CAFU_TABLE instead!
        // This is the key step that allows the user to work quasi normally with the CAFU_TABLE.
        lua_rawset(LuaState, -3);
        return 0;
    }

    switch (ConVar->Type)
    {
        case ConVarT::String:
        {
            const char* Value=lua_tostring(LuaState, 3);

            ConVar->SetValue(Value!=NULL ? std::string(Value) : "");
            break;
        }

        case ConVarT::Integer:
            ConVar->SetValue(lua_tointeger(LuaState, 3));
            break;

        case ConVarT::Bool:
            // I also want to treat the number 0 as false, not just "false" and "nil".
            ConVar->SetValue(lua_isnumber(LuaState, 3) ? lua_tointeger(LuaState, 3)!=0 : lua_toboolean(LuaState, 3)!=0);
            break;

        case ConVarT::Double:
            ConVar->SetValue(lua_tonumber(LuaState, 3));
            break;
    }

    return 0;
}


int ConsoleInterpreterImplT::ConFunc_Help_Callback(lua_State* LuaState)
{
    const std::string str_CAFU_TABLE=CAFU_TABLE;

    if (lua_gettop(LuaState)==0)
    {
        Console->Print("The   help(param)   function provides help on the specified console function or variable.\n");
        Console->Print("\"param\" must be a string with the name of a console function or variable to provide help for.\n");
        Console->Print("For example,   help(\"quit\")   provides help on the console variable \"quit\".\n");
        Console->Print("\n");
        Console->Print("The   list()   function shows a list of all available console functions and variables.\n");
        Console->Print("Passing an optional string parameter to   list()   reduces the output to functions and variables\n");
        Console->Print("whose name begins with the given string.\n");
        Console->Print("\n");
        return 0;
    }

    if (!lua_isstring(LuaState, 1))
    {
        return luaL_error(LuaState, "The parameter is not a \"string\". Did you write help(x) when you intended to write help(\"x\")?\n");
    }

    const char* VarName=luaL_checkstring(LuaState, 1);

    // 1. See if VarName refers to a console function.
    {
        ConFuncT* ConFunc=ConsoleInterpreter->FindFunc(VarName);

        if (ConFunc!=NULL)
        {
            if (str_CAFU_TABLE!="_G") Console->Print(str_CAFU_TABLE+".");

            Console->Print(ConFunc->GetName()+"() is a console function.\n");
            Console->Print(cf::va("Flags: 0x%X\n", ConFunc->GetFlags()));
            Console->Print("Description: "+ConFunc->GetDescription()+"\n");
            return 0;
        }
    }

    // 2. See if VarName refers to a console variable.
    {
        ConVarT* ConVar=ConsoleInterpreter->FindVar(VarName);

        if (ConVar!=NULL)
        {
            if (str_CAFU_TABLE!="_G") Console->Print(str_CAFU_TABLE+".");

            Console->Print(ConVar->GetName()+" is a console variable.\n");

            switch (ConVar->GetType())
            {
                case ConVarT::String:  Console->Print(cf::va("Value: \"%s\"\n", ConVar->GetValueString().c_str()         )); Console->Print("Type: string\n"); break;
                case ConVarT::Integer: Console->Print(cf::va("Value: %i\n",     ConVar->GetValueInt()                    )); Console->Print("Type: int\n");    break;
                case ConVarT::Bool:    Console->Print(cf::va("Value: %s\n",     ConVar->GetValueBool() ? "true" : "false")); Console->Print("Type: bool\n");   break;
                case ConVarT::Double:  Console->Print(cf::va("Value: %f\n",     ConVar->GetValueDouble()                 )); Console->Print("Type: double\n"); break;
            }

            Console->Print(cf::va("Flags: 0x%X\n", ConVar->GetFlags()));
            Console->Print("Description: "+ConVar->GetDescription()+"\n");
            return 0;
        }
    }

    // 3. VarName was neither a registered ConVarT nor a ConFuncT, now let's see if we find something in the CAFU_TABLE.
    lua_getglobal(LuaState, CAFU_TABLE);
    lua_pushstring(LuaState, VarName);
    lua_rawget(LuaState, -2);

    if (!lua_isnil(LuaState, -1))
    {
        // Note that the value cannot be one of our C functions of the ConFuncTs,
        // because we already checked them above.
        if (str_CAFU_TABLE!="_G") Console->Print(str_CAFU_TABLE+".");

        Console->Print(cf::va("%s is a native Lua value of type \"%s\".\n", VarName, lua_typename(LuaState, lua_type(LuaState, -1))));
        return 0;
    }

    // 4. Found nothing for VarName, give up.
    return luaL_error(LuaState, "Unknown identifier \"%s\". Did you write help(x) when you intended to write help(\"x\")?\n", VarName);
}

static ConFuncT ConFunc_Help("help", ConsoleInterpreterImplT::ConFunc_Help_Callback, ConFuncT::FLAG_MAIN_EXE, "Provides help on the given function or variable.");


/// If no parameter is given, this function lists all known console functions and variables.
/// Otherwise, the output is limited to matches with the given prefix.
int ConsoleInterpreterImplT::ConFunc_List_Callback(lua_State* LuaState)
{
    const std::string            Prefix   =lua_gettop(LuaState)==0 ? "" : luaL_checkstring(LuaState, 1);
    const std::string::size_type PrefixLen=Prefix.length();

    // 1. List the functions.
    const ArrayT<ConFuncT*>& RegConFuncs=static_cast<ConsoleInterpreterImplT*>(ConsoleInterpreter)->RegisteredConFuncs;
    unsigned long OutCount=0;

    for (unsigned long ConFuncNr=0; ConFuncNr<RegConFuncs.Size(); ConFuncNr++)
        if (RegConFuncs[ConFuncNr]->GetName().compare(0, PrefixLen, Prefix)==0)
        {
            const ConFuncT* ConFunc=RegConFuncs[ConFuncNr];

            if (OutCount==0)
                Console->Print("\nFunctions:\n");

            if (OutCount>0)
                Console->Print(OutCount % 3==0 ? ",\n" : ", ");

            Console->Print(ConFunc->GetName()+"()");
            OutCount++;
        }

    if (OutCount>0)
        Console->Print("\n");

    // 2. List the variables.
    const ArrayT<ConVarT*>& RegConVars=static_cast<ConsoleInterpreterImplT*>(ConsoleInterpreter)->RegisteredConVars;
    OutCount=0;

    for (unsigned long ConVarNr=0; ConVarNr<RegConVars.Size(); ConVarNr++)
        if (RegConVars[ConVarNr]->GetName().compare(0, PrefixLen, Prefix)==0)
        {
            const ConVarT* ConVar=RegConVars[ConVarNr];

            if (OutCount==0)
                Console->Print("\nVariables:\n");

            if (OutCount>0)
                Console->Print(OutCount % 2==0 ? ",\n" : ", ");

            Console->Print(ConVar->GetName());

            switch (ConVar->GetType())
            {
                case ConVarT::String:  Console->Print(cf::va(" == \"%s\" [string]", ConVar->GetValueString().c_str()         )); break;
                case ConVarT::Integer: Console->Print(cf::va(" == %i [int]",        ConVar->GetValueInt()                    )); break;
                case ConVarT::Bool:    Console->Print(cf::va(" == %s [bool]",       ConVar->GetValueBool() ? "true" : "false")); break;
                case ConVarT::Double:  Console->Print(cf::va(" == %f [double]",     ConVar->GetValueDouble()                 )); break;
            }

            OutCount++;
        }

    if (OutCount>0)
        Console->Print("\n");

    // 3. List the variables that are true (raw) members of the CAFU_TABLE (no ConVarT is registered for them, they are not in the RegisteredConVars).
    lua_getglobal(LuaState, CAFU_TABLE);
    OutCount=0;

    lua_pushnil(LuaState);  // Push the initial key.
    while (lua_next(LuaState, -2)!=0)
    {
        // The key is now at stack index -2, the value is at index -1.
        if (!lua_isstring(LuaState, -2))
        {
            Console->Warning(std::string("Skipped unexpected key type \"")+lua_typename(LuaState, lua_type(LuaState, -2))+"\" while traversing table \""+CAFU_TABLE+"\".\n");

            // Remove the value, but keep the key for the next iteration.
            lua_pop(LuaState, 1);
            continue;
        }

        const std::string Key=lua_tostring(LuaState, -2);

        // Skip all prefix mismatches.
        if (Key.compare(0, PrefixLen, Prefix)!=0)
        {
            // Remove the value, but keep the key for the next iteration.
            lua_pop(LuaState, 1);
            continue;
        }

        // Skip the registered ConFuncTs.
        if (ConsoleInterpreter->FindFunc(Key)!=NULL)
        {
            // Remove the value, but keep the key for the next iteration.
            lua_pop(LuaState, 1);
            continue;
        }

        if (OutCount==0)
            Console->Print("\nOther:\n");

        if (OutCount>0)
            Console->Print(OutCount % 5==0 ? ",\n" : ", ");

        Console->Print(Key);

        switch (lua_type(LuaState, -1))
        {
            case LUA_TBOOLEAN: Console->Print(cf::va(" == %s",     lua_toboolean(LuaState, -1)!=0 ? "true" : "false")); break;
            case LUA_TSTRING:  Console->Print(cf::va(" == \"%s\"", lua_tostring(LuaState, -1))); break;
            case LUA_TNUMBER:  Console->Print(cf::va(" == %f",     lua_tonumber(LuaState, -1))); break;

            default:
                Console->Print(" == ...");
                break;
        }

        Console->Print(cf::va(" [%s]", lua_typename(LuaState, lua_type(LuaState, -1))));
        OutCount++;

        // Remove the value, but keep the key for the next iteration.
        lua_pop(LuaState, 1);
    }

    if (OutCount>0)
        Console->Print("\n");

    lua_pop(LuaState, 1);

    // 4. Done.
    return 0;
}

static ConFuncT ConFunc_List("list", ConsoleInterpreterImplT::ConFunc_List_Callback, ConFuncT::FLAG_MAIN_EXE, "Lists all console functions and variables, or only those matching a given prefix.");
