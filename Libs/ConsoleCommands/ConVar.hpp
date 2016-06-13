/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/*************************/
/*** Console Variables ***/
/*************************/

#ifndef CAFU_CONSOLE_VARIABLES_HPP_INCLUDED
#define CAFU_CONSOLE_VARIABLES_HPP_INCLUDED

#include <string>
#include "Templates/Array.hpp"

#undef Bool


// TODO: - Argument completion callbacks.

/// This class implements Console Variables ("convars").
/// A convar is a special variable that can be modified by the user at the game console and is globally accessible in the entire application.
///
/// In user code, a convar can be declared as a global variable or as a local, static variable.
/// Use as a non-static local variable or on the heap (with new and delete) is also possible,
/// but comes at a performance cost and is against the (gobal) nature of convars.
/// Such a local variable would also never be visible at the console, and thus it makes no sense to have it.
///
/// a) Before ConVarT::RegisterStaticList() is called, all instantiated convars register themselves in a list that is global to the module,
///    that is, the exe and each dll has its own list.
/// b) Then, the ConsoleInterpreter pointer is set to point to the implementation of the ConsoleInterpreterI in the exe.
///    (There is only exactly one "global" singleton ConsoleInterpreterI instance for the entire application.)
/// c) Calling ConVarT::RegisterStaticList() then iterates over all (pointers to) convars in the per-module list,
///    and for each tries to find a convar with the same name in the unique, application-global list inside the ConsoleInterpreter.
/// d) Convars that are instantiated after the call to ConVarT::RegisterStaticList(), e.g. local static variables that are reached by the
///    control-flow for the first time, automatically register themselves with the ConsoleInterpreter directly, rather than attempting to
///    add themselves to the obsolete list from step a).
class ConVarT
{
    public:

    enum TypeT { String, Integer, Bool, Double };

    enum FlagT
    {
        FLAG_ALL          =     -1,

        FLAG_MAIN_EXE     = 0x0001,
        FLAG_MATSYS       = 0x0002,
        FLAG_SOUNDSYS     = 0x0004,
        FLAG_GAMEDLL      = 0x0008,

        FLAG_USERINFO     = 0x0010,   ///< Sent from clients to the server.
        FLAG_SERVERINFO   = 0x0020,   ///< Sent from servers to the clients.
        FLAG_NETWORK_SYNC = 0x0040,   ///< Value of this variable is sync'ed from the server to the clients.
        FLAG_CHEAT        = 0x0080,   ///< Use of this variable is considered a cheat.

        FLAG_FOR_INIT     = 0x0100,   ///< This variable is used during initialization, and can thus only be changed at the command-line, not at the console.
        FLAG_READ_ONLY    = 0x0200,   ///< This variable is read-only, i.e. it cannot be modified by the user (neither in console nor at command-line).
        FLAG_BY_CODE      = 0x0400,   ///< This variable has been statically declared by code, not by a user via the "set var xy" command.
        FLAG_PERSISTENT   = 0x0800    ///< This variable (and its value) is archived in a config file.
    };


    // Constructors.
    ConVarT(const std::string& Name_, const std::string& Value_, const int Flags_, const std::string& Description_, const char** AllowedValues_=NULL);
    ConVarT(const std::string& Name_, const char*        Value_, const int Flags_, const std::string& Description_, const char** AllowedValues_=NULL);  // This constructor is required for cases when "" is given for Value_, which the VC++ 2005 compiler seems to cast and match to the "bool" constructor...
    ConVarT(const std::string& Name_, const int          Value_, const int Flags_, const std::string& Description_, const int   MinValue_=   1, const int   MaxValue_=   -1);
    ConVarT(const std::string& Name_, const bool         Value_, const int Flags_, const std::string& Description_);
    ConVarT(const std::string& Name_, const double       Value_, const int Flags_, const std::string& Description_, const double MinValue_=1.0, const double MaxValue_=-1.0);

    // Destructor.
    ~ConVarT();

    // Get methods.
    const std::string& GetName() const { return Name; }
    const std::string& GetDescription() const { return Description; }
    int                GetFlags() const { return Flags; }
    TypeT              GetType() const { return Type; }
    const std::string& GetValueString() const { assert(Type==String); return ValueString; }
    int                GetValueInt() const { assert(Type==Integer); return ValueInt; }
    bool               GetValueBool() const { assert(Type==Bool); return ValueInt!=0; }
    double             GetValueDouble() const { assert(Type==Double); return ValueDouble; }

    // Set methods.
    void               SetValue(const std::string& Value_);
    void               SetValue(const char*        Value_);     // Without this, literal strings are cast to bool rather than std::string.
    void               SetValue(const int          Value_);
    void               SetValue(const bool         Value_);
    void               SetValue(const double       Value_);

    void               operator = (const std::string& Value_) { SetValue(Value_); }
    void               operator = (const char*        Value_) { SetValue(Value_); }
    void               operator = (const int          Value_) { SetValue(Value_); }
    void               operator = (const bool         Value_) { SetValue(Value_); }
    void               operator = (const double       Value_) { SetValue(Value_); }

    // Special methods for dealing with the IsModified flag.
    bool               HasBeenModified() const { return IsModified; }
 // void               SetModified()   { IsModified=true; } // Should probably be private, but instead I wrote IsModified=true; everywhere directly.
    void               ClearModified() { IsModified=false; }


    /// Registers all convars in the StaticList with the ConsoleInterpreter and invalidates the StaticList.
    /// This has to be called after the ConsoleInterpreter pointer is set when the dll is first initialized!
    /// The ctors of all convars that are instantiated after this call (those that are declared as local (static) variables)
    /// then automatically register themselves with the ConsoleInterpreter rather than with the StaticList.
    static void RegisterStaticList();


    private:

    /// Give the implementation of the console interpreter full access.
    friend class ConsoleInterpreterImplT;

    const std::string         Name;             ///< The name of this console variable.
    const std::string         Description;      ///< The description (i.e. user help text) for this console variable.
    const int                 Flags;            ///< The flags for this convar.
    const TypeT               Type;             ///< The type of this convar. Cannot be changed later.
    std::string               ValueString;      ///< The value of this convar if it is of type String.
    int                       ValueInt;         ///< The value of this convar if it is of type Integer or Bool.
    double                    ValueDouble;      ///< The value of this convar if it is of type Double.
    const double              MinValue;         ///< The minimum value of this convar if Type is Double or Integer.
    const double              MaxValue;         ///< The maximum value of this convar if Type is Double or Integer.
    const ArrayT<std::string> AllowedValues;    ///< The list of allowed values if Type is String.
    bool                      IsModified;       ///< Whether this variable has been modified. Set by the SetValue() methods.
};

#endif
