/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#ifndef _CF_CONSOLE_STDOUT_HPP_
#define _CF_CONSOLE_STDOUT_HPP_

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
