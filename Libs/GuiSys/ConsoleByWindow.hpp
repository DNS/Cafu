/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#ifndef CAFU_GUISYS_CONSOLE_BY_WINDOW_HPP_INCLUDED
#define CAFU_GUISYS_CONSOLE_BY_WINDOW_HPP_INCLUDED

#include "ConsoleCommands/Console.hpp"


namespace cf
{
    namespace GuiSys
    {
        class WindowT;


        /// This class implements the cf::ConsoleI interface by means of a cf::GuiSys::WindowT, thus providing us with a GuiSys-based console.
        /// It quasi acts as a kind of "mediator" between the cf::ConsoleI interface and the WindowT instance.
        /// Note that the target window must live longer than instances of this class!
        class ConsoleByWindowT : public cf::ConsoleI
        {
            public:

            /// Constructor for a console that is implemented by means of the given WindowT object.
            ConsoleByWindowT(WindowT* Win_);

            // Implementation of the cf::ConsoleI interface.
            void Print(const std::string& s);
            void DevPrint(const std::string& s);
            void Warning(const std::string& s);
            void DevWarning(const std::string& s);


            private:

            WindowT* Win;   ///< The "target" window that is supposed to receive the console output.
        };
    }
}

#endif
