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

#ifndef _CF_CONSOLE_INTERPRETER_HPP_
#define _CF_CONSOLE_INTERPRETER_HPP_

#include "Templates/Array.hpp"
#include <string>


class  ConFuncT;
class  ConVarT;
struct lua_State;


/// This class is an interface to the console interpreter.
/// User code can register its convars and confuncs with it so that they can be used in the context of the interpreter.
/// For each application, there is only one, global, application-wide, unique implementation of the console interpreter,
/// typically in the main exe, and the exe and all dlls each have a pointer to it.
class ConsoleInterpreterI
{
    public:

    /// Virtual dtor so that derived classes properly destroy.
    virtual ~ConsoleInterpreterI() { }

    /// Registers a convar with the console interpreter.
    /// This method is normally not called by user code, but by the implementation of the ConVarT ctors and ConVarT::RegisterStaticList().
    /// @param ConVar   The console variable to register with this interpreter.
    virtual void Register(ConVarT* ConVar)=0;

    /// Registers a confunc with the console interpreter.
    /// This method is normally not called by user code, but by the implementation of the ConFuncT ctors and ConFuncT::RegisterStaticList().
    /// @param ConFunc  The console function to register with this interpreter.
    virtual void Register(ConFuncT* ConFunc)=0;

    /// Unregisters the given convar from the interpreter again.
    /// @param ConVar   The console variable to unregister from this interpreter.
    virtual void Unregister(ConVarT* ConVar)=0;

    /// Unregisters the given confunc from the interpreter again.
    /// @param ConFunc   The console function to unregister from this interpreter.
    virtual void Unregister(ConFuncT* ConFunc)=0;

    /// Finds the confunc with the given name.
    /// @param Name   The name of the confunc to find.
    /// @returns the pointer to the confunc, or NULL if a confunc with that name does not exist.
    virtual ConFuncT* FindFunc(const std::string& Name)=0;

    /// Finds the convar with the given name.
    /// @param Name   The name of the convar to find.
    /// @returns the pointer to the convar, or NULL if a convar with that name does not exist.
    virtual ConVarT* FindVar(const std::string& Name)=0;

    /// This method provides command-line completion for this interpreter.
    /// It returns all available completions for the last token in the given string LineBegin in the Completions array.
    /// Note that the completions not only cover the registered ConFuncTs and ConVarTs, but also any other user-
    /// and implementation-defined symbols.
    /// @param LineBegin     The incomplete command-line string for whose last token the method is supposed to provide completions for.
    ///                      For example, if LineBegin is "a=GetValueX()*Get", the method is supposed to look for completions for "Get".
    /// @param Completions   The found completions are returned here as complete tokens.
    ///                      For example, if LineBegin is "a=GetValueX()*Get", Completions may contain the strings "GetValueX" and "GetValueY".
    ///                      The caller can therefore not do much (i.e. concat to input string) with the returned strings but print them out.
    /// @returns the longest expansion substring for LineBegin that works with all completions.
    ///     The caller can simply concatenate this string to LineBegin to implement incremental completions.
    ///     In the context of the above examples, "Value" would be the expected return string.
    virtual std::string LineCompletion(const std::string& LineBegin, ArrayT<std::string>& Completions)=0;

    // TODO: Some commands can possibly not be executed immediately.
    //       For example, a "map" command for changing the level can probably not be executed in mid-frame from a Think()ing entity,
    //       but should only be executed between frames, e.g. after Think()ing returned.

    /// Compiles and runs the given Lua statements.
    /// @param Input   The string with the Lua statements that is to be compiled and run within this console state.
    /// @returns true if the command has been successfully loaded (compiled) and run, false otherwise.
    virtual bool RunCommand(const std::string& Input)=0;


    /// Registers the methods of this interface with LuaState as a Lua module as described in the PiL2 book, chapter 26.2.
    /// The key idea is that all methods are called via the global ConsoleInterpreter variable defined below,
    /// and therefore we may consider them as a collection of C-style functions (no OO involved),
    /// so that putting them in a Lua table as described in chapter 26 of the PiL2 book is straightforward.
    static void RegisterLua(lua_State* LuaState);
};


/// A global pointer to an implementation of the ConsoleInterpreterI interface.
///
/// Each module (exe or dll) that uses this pointer must somewhere provide exactly one definition for it (none is provided by the ConsoleInterpreter library).
/// That is, typically the main.cpp or similar file of each exe and dll must contain a line like
///     ConsoleInterpreterI* ConsoleInterpreter=NULL;
/// or else the module will not link successfully due to an undefined symbol.
///
/// Exe files will then want to reset this pointer to an instance of a ConsoleInterpreterImplT during their initialization
/// e.g. by code like:   ConsoleInterpreter=new ConsoleInterpreterImplT;
/// Note that the ConsoleInterpreterImplT ctor may require that other interfaces (e.g. the Console) have been inited first.
///
/// Dlls typically get one of their init functions called immediately after they have been loaded.
/// By doing so, the exe passes a pointer to its above instance to the dll, which in turn copies it to its ConsoleInterpreter variable.
extern ConsoleInterpreterI* ConsoleInterpreter;

#endif
