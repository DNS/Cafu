/**
 * @defgroup Common Common Libraries
 *
 * This module groups auxiliary libraries (e.g.\ script classes and object instances)
 * that are available and can be used in all Cafu scripts, such as map scripts, GUI scripts, etc.
 *
 * Note that besides the libraries listed here, (most of) the
 * <a href="http://www.lua.org/manual/5.1/manual.html#5">Lua standard libraries</a>
 * are loaded and available in Cafu scripts as well.
 *
 * @{
 */


/// Use the methods of this library for printing strings to the Cafu
/// <a href="http://www.cafu.de/wiki/usermanual:running#the_command_console">in-game console</a>.
///
/// Note that you don't have to create instances of this class yourself.
/// Instead this class is used like a (Lua) library: A global instance @c Console is automatically available.
/// See the function details below for examples on usage.
class Console
{
    public:

    /// Prints a message to the in-game console.
    ///
    /// \par Example:
    /// \code
    ///     Console.Print("Hello world!\n")     -- Prints "Hello world!" to the in-game console.
    /// \endcode
    ///
    /// @param s   The message to print to the console.
    Print(string s);

    /// Prints a developer message to the in-game console.
    /// If the \c "developer" console variable is set, this prints the string \c s, prepended by \c "[Dev] ",
    /// to the Cafu in-game console. If the \c "developer" console variable is not set, nothing is printed.
    ///
    /// \par Example:
    /// \code
    ///     Console.DevPrint("Hello world!\n")  -- Prints "[Dev] Hello world!" to the in-game console (in developer mode).
    /// \endcode
    ///
    /// @param s   The developer message to print to the console.
    DevPrint(string s);

    /// Prints a warning to the in-game console.
    /// This is similar to Print(), except that the text \c "Warning: " is automatically prepended to the output.
    ///
    /// \par Example:
    /// \code
    ///     Console.Warning("Problem here!\n")  -- Prints "Warning: Problem here!" to the in-game console.
    /// \endcode
    ///
    /// @param s   The warning to print to the console.
    Warning(string s);

    /// Prints a developer warning to the in-game console.
    /// If the \c "developer" console variable is set, this prints the string \c s, prepended by \c "[Dev] Warning: ",
    /// to the Cafu in-game console. If the \c "developer" console variable is not set, nothing is printed.
    ///
    /// \par Example:
    /// \code
    ///     Console.DevWarning("Hello world!\n")    -- Prints "[Dev] Warning: Hello world!"
    ///                                             -- to the in-game console (in developer mode).
    /// \endcode
    ///
    /// @param s   The developer warning to print to the console.
    DevWarning(string s);

    /// Returns an array (a table) with the file and directory entries of the given directory.
    ///   - If the string \c "f" is passed as a second parameter, only entries of type \emph{file} are returned,
    ///   - if the string \c "d" is passed as a second parameter, only entries of type \emph{directory} are returned,
    ///   - all entries are returned in all other cases.
    ///
    /// \par TODO:
    ///       This function should be implemented \emph{independently} from the "Console" interface (i.e. elsewhere where
    ///       it is a better topical fit), but for now the "Console" interface is the only interface that is included
    ///       in all of our Lua instances (console interpreter, GUIs, map scripts, ...)!
    /// \par IDEA:
    ///       Can we make ConVars and ConFuncs directly available not only in the console Lua instance, but in arbitrary
    ///       many Lua instances?? Then the GUI and map scripts could directly access ConVars and ConFuncs, too...!
    ///       (And GetDir() would be a regular ConFunc.)
    ///
    /// @param dir    The name of the directory to get the entries for.
    /// @param mode   Whether only file entries (\c "f"), only directory entries (\c "d"), or all entries should be returned. See above for details.
    table GetDir(string dir, string mode="");
};


/// Class \c ci provides access to the Cafu
/// <a href="http://www.cafu.de/wiki/usermanual:running#the_command_console">in-game console</a>
/// from other scripts (e.g.\ map scripts, GUI scripts, ...).
///
/// Note that you don't have to create instances of this class yourself.
/// Instead this class is used like a (Lua) library: A global instance @c ci is automatically available.
/// See the function details below for examples on usage.
class ci
{
    public:

    /// Retrieves the value of the specified console variable.
    ///
    /// \par Example:
    /// \code
    ///     showFPS=ci.GetValue("showFPS")
    /// \endcode
    ///
    /// @param name   The name of the console variable whose value is to be retrieved.
    ///
    /// @returns The value of the given console variable.
    ///     The returned value can be of type \c string, \c integer, \c boolean or \c number,
    ///     depending on the type of the console variable, or \c nil if the console variable
    ///     has a different type or does not exist.
    any GetValue(string name);

    /// Sets the console variable of the given name to the given value.
    ///
    /// \par Example:
    /// \code
    ///     ci.SetValue("showFPS", true)
    /// \endcode
    ///
    /// @param name    The name of the console variable whose value is to be set.
    /// @param value   The value that the console variable is set to.
    ///    Note that \c value can be of any type; it is cast to the type of the console variable.
    ///    If the cast fails, some default value is assigned to the console variable.
    SetValue(string name, any value);

    /// This method provides command-line completion for this console interpreter.
    /// It returns all available completions for the last token in the given parameter string \c LineBegin.
    /// Note that the completions not only cover the registered C++ ConFuncTs and ConVarTs, but also any other user-
    /// and implementation-defined symbols.
    ///
    /// @param LineBegin     The incomplete command-line string for whose last token the function is supposed to provide completions for.
    ///                      For example, if \c LineBegin is \c "a=GetValueX()*Get", the method is supposed to look for completions for \c "Get".
    ///
    /// @returns A <tt>(string, table)</tt> tuple whose first element is the longest expansion substring for
    ///     \c LineBegin that works with all completions.
    ///     The caller can simply concatenate this string to \c LineBegin to implement incremental completions.
    ///     The second element is the table with all found completions as complete tokens.
    tuple LineCompletion(string LineBegin);

    /// Runs the console command \c command as if the user had typed it into the Cafu in-game console.
    /// Use this to run console functions and to set the values of console variables.
    ///
    /// \par Examples:
    /// \code
    ///     ci.RunCommand("quit=true")                  -- Same as ci.SetValue("quit", true)
    ///     ci.RunCommand("changeLevel('TechDemo')")
    ///     ci.RunCommand("SetMasterVolume(0.8)")
    /// \endcode
    /// Also see the <tt>*.cgui</tt> files in \c DeathMatch/GUIs for many additional examples.
    ///
    /// @param command   The command (including paramaters etc.) to be run in the in-game console.
    RunCommand(string command);
};

/** @} */   // End of group Common.
