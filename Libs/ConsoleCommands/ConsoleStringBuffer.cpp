/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ConsoleStringBuffer.hpp"

using namespace cf;


ConsoleStringBufferT::ConsoleStringBufferT()
    : Buffer()
{
}


const std::string& ConsoleStringBufferT::GetBuffer() const
{
    return Buffer;
}


void ConsoleStringBufferT::ClearBuffer()
{
    Buffer="";
}


void ConsoleStringBufferT::Print(const std::string& s)
{
    Buffer+=s;
}


void ConsoleStringBufferT::DevPrint(const std::string& s)
{
    Buffer+="[Dev] ";
    Buffer+=s;
}


void ConsoleStringBufferT::Warning(const std::string& s)
{
    Buffer+="Warning: ";
    Buffer+=s;
}


void ConsoleStringBufferT::DevWarning(const std::string& s)
{
    Buffer+="[Dev] Warning: ";
    Buffer+=s;
}
