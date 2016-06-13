/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
