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

/*******************/
/*** Scroll Info ***/
/*******************/

#ifndef _CLIENT_SCROLLINFO_HPP_
#define _CLIENT_SCROLLINFO_HPP_

#include "Templates/Array.hpp"


class FontT;


class ScrollInfoT
{
    private:

    char  MAX_LINES;
    char  FirstLine;
    char  NrOfLines;
    float TimeLeft;
    ArrayT< ArrayT<char> > InfoLine;


    public:

    ScrollInfoT();
    void Print(const char* PrintString, ...);
    void Draw(FontT& Font, unsigned long PosX, unsigned long PosY) const;
    void AdvanceTime(float FrameTime);
};

#endif
