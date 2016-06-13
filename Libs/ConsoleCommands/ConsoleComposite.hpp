/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMPOSITE_CONSOLE_HPP_INCLUDED
#define CAFU_COMPOSITE_CONSOLE_HPP_INCLUDED

#include "Console.hpp"
#include "Templates/Array.hpp"


namespace cf
{
    /// This class implements the ConsoleI interface as a composite console
    /// by sending all output to its attached sub-consoles.
    class CompositeConsoleT : public ConsoleI
    {
        public:

        /// Constructor for creating a composite console.
        CompositeConsoleT();

        /// Attaches the given console c to the set of sub-consoles.
        bool Attach(ConsoleI* c);

        /// Removes the given console c from the set of sub-consoles.
        bool Detach(ConsoleI* c);

        // Methods of the ConsoleI interface.
        void Print(const std::string& s);
        void DevPrint(const std::string& s);
        void Warning(const std::string& s);
        void DevWarning(const std::string& s);


        private:

        ArrayT<ConsoleI*> m_Consoles;   ///< The filestream output is logged to.
    };
}

#endif
