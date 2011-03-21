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

#ifndef _CF_CONSOLE_FILE_HPP_
#define _CF_CONSOLE_FILE_HPP_

#include "Console.hpp"

#include <fstream>


namespace cf
{
    /// This class implements the ConsoleI interface by writing the console output into a text file.
    class ConsoleFileT : public ConsoleI
    {
        public:

        /// Constructor for creating an instance of a ConsoleFileT.
        /// @param FileName   Path and name of the file that the console output is to be written to.
        ConsoleFileT(const std::string& FileName);

        /// Sets if the console buffer should be auto-flushed after each call to one of the Print() or Warning() methods.
        void SetAutoFlush(bool AutoFlush) { m_AutoFlush=AutoFlush; }

        /// Flushes the buffers so that all contents is immediately written into the file.
        void Flush();

        // Methods of the ConsoleI interface.
        void Print(const std::string& s);
        void DevPrint(const std::string& s);
        void Warning(const std::string& s);
        void DevWarning(const std::string& s);


        private:

        std::ofstream m_File;       ///< The filestream output is logged to.
        bool          m_AutoFlush;  ///< If the console buffer should be auto-flushed after each call to one of the Print() or Warning() methods.
    };
}

#endif
