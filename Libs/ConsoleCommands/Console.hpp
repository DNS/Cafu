/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CONSOLE_HPP_INCLUDED
#define CAFU_CONSOLE_HPP_INCLUDED

#include <string>


namespace cf
{
    /// This class is an interface to the application console.
    /// Google search for "console input non blocking" yields interesting insights for non-blocking console input under Linux.
    /// The unix_main.c file from Q3 has something, too.
    class ConsoleI
    {
        public:

        /// Virtual dtor so that derived classes properly destroy.
        virtual ~ConsoleI() { }

        // Methods for console output.
        virtual void Print(const std::string& s)=0;      ///< Print message to console.
        virtual void DevPrint(const std::string& s)=0;   ///< Print dev message to console.
        virtual void Warning(const std::string& s)=0;    ///< Print warning to console.
        virtual void DevWarning(const std::string& s)=0; ///< Print dev warning to console.
    };


    /// Builds a std::string from a printf-like format string and the variable argument list.
    std::string va(const char* FormatString, ...);
}


/// A global pointer to an implementation of the ConsoleI interface.
///
/// Each module (exe or dll) that uses this pointer must somewhere provide exactly one definition for it (none is provided by the Console library).
/// That is, typically the main.cpp or similar file of each exe and dll must contain a line like
///     cf::ConsoleI* Console=NULL;
/// or else the module will not link successfully due to an undefined symbol.
///
/// Exe files will then want to reset this pointer to an instance of e.g. a ConsoleStdoutT during their initialization
/// e.g. by code like:   Console=new cf::ConsoleStdoutT;
///
/// Dlls typically get one of their init functions called immediately after they have been loaded.
/// By doing so, the exe passes a pointer to its above instance to the dll, which in turn copies it to its Console variable.
extern cf::ConsoleI* Console;

#endif
