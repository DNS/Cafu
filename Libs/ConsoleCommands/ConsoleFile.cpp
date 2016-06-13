/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ConsoleFile.hpp"


using namespace cf;


ConsoleFileT::ConsoleFileT(const std::string& FileName)
    : m_File(FileName.c_str()),
      m_AutoFlush(false)
{
}


void ConsoleFileT::Flush()
{
    m_File.flush();
}


void ConsoleFileT::Print(const std::string& s)
{
    m_File << s;
    if (m_AutoFlush) Flush();
}


void ConsoleFileT::DevPrint(const std::string& s)
{
    m_File << "[Dev] " << s;
    if (m_AutoFlush) Flush();
}


void ConsoleFileT::Warning(const std::string& s)
{
    m_File << "Warning: " << s;
    if (m_AutoFlush) Flush();
}


void ConsoleFileT::DevWarning(const std::string& s)
{
    m_File << "[Dev] Warning: " << s;
    if (m_AutoFlush) Flush();
}
