/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#include "ConsoleStdout.hpp"
#include <iostream>

using namespace cf;


ConsoleStdoutT::ConsoleStdoutT(bool AutoFlush_)
    : AutoFlush(AutoFlush_)
{
}


void ConsoleStdoutT::EnableAutoFlush(bool AutoFlush_)
{
    AutoFlush=AutoFlush_;
}


void ConsoleStdoutT::Flush()
{
    std::cout << std::flush;
}


void ConsoleStdoutT::Print(const std::string& s)
{
    std::cout << s;

    if (AutoFlush) std::cout << std::flush;
}


void ConsoleStdoutT::DevPrint(const std::string& s)
{
    std::cout << "[Dev] " << s;

    if (AutoFlush) std::cout << std::flush;
}


void ConsoleStdoutT::Warning(const std::string& s)
{
    std::cout << "Warning: " << s;

    if (AutoFlush) std::cout << std::flush;
}


void ConsoleStdoutT::DevWarning(const std::string& s)
{
    std::cout << "[Dev] Warning: " << s;

    if (AutoFlush) std::cout << std::flush;
}
