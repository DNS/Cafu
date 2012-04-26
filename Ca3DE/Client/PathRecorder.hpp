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

#ifndef CAFU_CLIENT_PATH_RECORDER_HPP_INCLUDED
#define CAFU_CLIENT_PATH_RECORDER_HPP_INCLUDED

#include "Math3D/Vector3.hpp"
#include <fstream>


/// This class records the path that an entity takes through a level into a file.
class PathRecorderT
{
    public:

    PathRecorderT(const std::string& FileName);
    ~PathRecorderT();

    const std::string& GetFileName() const { return m_FileName; }
    void WritePath(const Vector3dT& Origin, unsigned short Heading, float FrameTime);


    private:

    const std::string m_FileName;
    std::ofstream     m_OutStream;
    float             m_Time;
    unsigned long     m_LineCount;
    float             m_OldTime;
    Vector3dT         m_OldOrigin;
    unsigned short    m_OldHeading;
};

#endif
