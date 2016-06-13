/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CONSOLE_FILE_HPP_INCLUDED
#define CAFU_CONSOLE_FILE_HPP_INCLUDED

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
