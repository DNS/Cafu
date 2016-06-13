/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ConsoleWarningsOnly.hpp"
#include <iostream>

using namespace cf;


ConsoleWarningsOnlyT::ConsoleWarningsOnlyT(ConsoleI* Console_)
    : Console(Console_)
{
}


void ConsoleWarningsOnlyT::Print(const std::string& s)
{
}


void ConsoleWarningsOnlyT::DevPrint(const std::string& s)
{
}


void ConsoleWarningsOnlyT::Warning(const std::string& s)
{
    Console->Warning(s);
}


void ConsoleWarningsOnlyT::DevWarning(const std::string& s)
{
    Console->DevWarning(s);
}
