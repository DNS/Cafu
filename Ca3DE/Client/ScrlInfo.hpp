/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CLIENT_SCROLLINFO_HPP_INCLUDED
#define CAFU_CLIENT_SCROLLINFO_HPP_INCLUDED

#include "Templates/Array.hpp"
#include <string>


class FontT;


class ScrollInfoT
{
    public:

    ScrollInfoT();

    void Print(const std::string& Line);
    void Draw(FontT& Font, unsigned long PosX, unsigned long PosY, float FrameWidth, float FrameHeight) const;
    void AdvanceTime(float FrameTime);


    private:

    const unsigned int  m_MAX_LINES;
    float               m_TimeLeft;
    ArrayT<std::string> m_InfoLines;
};

#endif
