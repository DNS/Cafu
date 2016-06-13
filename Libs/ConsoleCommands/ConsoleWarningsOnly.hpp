/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CONSOLE_WARNINGS_ONLY_HPP_INCLUDED
#define CAFU_CONSOLE_WARNINGS_ONLY_HPP_INCLUDED

#include "Console.hpp"


namespace cf
{
    /// This class implements a console that only prints warnings (and filters normal output).
    /// The output is directed to another given console.
    class ConsoleWarningsOnlyT : public ConsoleI
    {
        public:

        /// Constructor for the warnings only console.
        /// @param Console_   The target console on which the warnings should be printed.
        ConsoleWarningsOnlyT(ConsoleI* Console_);

        // Methods of the ConsoleI interface.
        void Print(const std::string& s);
        void DevPrint(const std::string& s);
        void Warning(const std::string& s);
        void DevWarning(const std::string& s);


        private:

        ConsoleI* Console;  ///< The console to direct all output to.
    };
}

#endif
