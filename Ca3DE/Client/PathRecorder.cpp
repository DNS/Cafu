/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "PathRecorder.hpp"
#include "ConsoleCommands/Console.hpp"


static std::string GetFullPath(const std::string& FileName)
{
    if (FileName.find(".")==std::string::npos) return FileName+".pts";

    return FileName;
}


PathRecorderT::PathRecorderT(const std::string& FileName)
    : m_FileName(GetFullPath(FileName)),
      m_OutStream(m_FileName.c_str()),
      m_Time(0.0f),
      m_LineCount(0)
{
    if (m_OutStream)
    {
        Console->Print(cf::va("Recording path to pointfile %s ...", m_FileName.c_str()));

        m_OutStream << "Points=\n";
        m_OutStream << "{\n";
    }
}


PathRecorderT::~PathRecorderT()
{
    if (m_OutStream)
    {
        m_OutStream << "}\n";
        m_OutStream << "\nColors={ \"#888888\", \"Red\", \"Green\", \"#AAAAAA\" }\n";

        Console->Print(cf::va("Stopped recording path of %lu points to pointfile %s ...", m_LineCount, m_FileName.c_str()));
    }
}


void PathRecorderT::WritePath(const Vector3dT& Origin, unsigned short Heading, float FrameTime)
{
    if (m_LineCount==0 || (m_Time-m_OldTime>=0.2f && length(Origin-m_OldOrigin)>=8.0))
    {
        m_OutStream << "  { ";
     // m_OutStream << m_LineCount << "; ";
        m_OutStream << m_Time      << "; " << "  ";
        m_OutStream << Origin.x    << ", ";
        m_OutStream << Origin.y    << ", ";
        m_OutStream << Origin.z    << "; " << "  ";
        m_OutStream << Heading     << "; ";
        m_OutStream << "\"\" },\n";

        m_OldTime   =m_Time;
        m_OldOrigin =Origin;
        m_OldHeading=Heading;

        m_LineCount++;
    }

    m_Time+=FrameTime;
}
