/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CONSOLE_STRINGBUFFER_HPP_INCLUDED
#define CAFU_CONSOLE_STRINGBUFFER_HPP_INCLUDED

#include "Console.hpp"


namespace cf
{
    /// This class implements the ConsoleI interface by printing the console output to stdout.
    class ConsoleStringBufferT : public ConsoleI
    {
        public:

        /// Constructor for creating an instance of a ConsoleStringBufferT.
        ConsoleStringBufferT();

        /// Returns the (contents of the) buffer.
        /// @returns the (contents of the) buffer.
        const std::string& GetBuffer() const;

        /// Clears (empties) the buffer.
        void ClearBuffer();

        // Methods of the ConsoleI interface.
        void Print(const std::string& s);
        void DevPrint(const std::string& s);
        void Warning(const std::string& s);
        void DevWarning(const std::string& s);


        private:

        std::string Buffer;     ///< The string where we buffer all output.
    };
}

#endif
