/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
