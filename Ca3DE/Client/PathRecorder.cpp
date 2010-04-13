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

#include "PathRecorder.hpp"
#include "../../Games/BaseEntity.hpp"
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

        Console->Print(cf::va("Stopped recording path of %lu points to pointfile %s ...", m_LineCount, m_FileName.c_str()));
    }
}


void PathRecorderT::WritePath(const EntityStateT* EntityState, float FrameTime)
{
    if (!EntityState) return;

    if (m_LineCount==0 || (m_Time-m_OldTime>=0.2f && length(EntityState->Origin-m_OldOrigin)>=250.0))
    {
        const Vector3dT WriteOrigin=EntityState->Origin/25.4;

        m_OutStream << "  { ";
     // m_OutStream << m_LineCount          << "; ";
        m_OutStream << m_Time               << "; " << "  ";
        m_OutStream << WriteOrigin.x        << ", ";
        m_OutStream << WriteOrigin.y        << ", ";
        m_OutStream << WriteOrigin.z        << "; " << "  ";
        m_OutStream << EntityState->Heading << "; ";
        m_OutStream << "\"\" },\n";

        m_OldTime   =m_Time;
        m_OldOrigin =EntityState->Origin;
        m_OldHeading=EntityState->Heading;

        m_LineCount++;
    }

    m_Time+=FrameTime;
}
