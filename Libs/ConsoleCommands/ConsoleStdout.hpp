/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CONSOLE_STDOUT_HPP_INCLUDED
#define CAFU_CONSOLE_STDOUT_HPP_INCLUDED

#include "Console.hpp"


namespace cf
{
    /// This class implements the ConsoleI interface by printing the console output to stdout.
    class ConsoleStdoutT : public ConsoleI
    {
        public:

        /// Constructor for creating an instance of a ConsoleStdoutT.
        /// @param AutoFlush_   Whether auto-flushing is enabled or not.
        ConsoleStdoutT(bool AutoFlush_=false);

        /// Enables or diables auto-flushing.
        /// If enabled, each call to the ConsoleI output functions is followed by a flush of the stdout stream.
        void EnableAutoFlush(bool AutoFlush_);

        /// Flushes the stdout stream.
        void Flush();

        // Methods of the ConsoleI interface.
        void Print(const std::string& s);
        void DevPrint(const std::string& s);
        void Warning(const std::string& s);
        void DevWarning(const std::string& s);


        private:

        bool AutoFlush;     ///< Whether auto-flushing is enabled or not.
    };
}

#endif
