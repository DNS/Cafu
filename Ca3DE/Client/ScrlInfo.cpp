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

#include <stdio.h>
#include <stdarg.h>

#include "ScrlInfo.hpp"
#include "Fonts/Font.hpp"
#include "ConsoleCommands/ConVar.hpp"

#if defined(_WIN32)
    #if defined(_MSC_VER)
        #define vsnprintf _vsnprintf
    #endif
#endif


static ConVarT TimeToLive("cl_MsgShowTime", 6.0, ConVarT::FLAG_MAIN_EXE, "How long (in seconds) info and chat messages are shown.", 0.0f, 30.0);


ScrollInfoT::ScrollInfoT()
{
    assert(TimeToLive.GetType()==ConVarT::Double);

    MAX_LINES =8;
    FirstLine =0;
    NrOfLines =0;
    TimeLeft  =float(TimeToLive.GetValueDouble());

    InfoLine.PushBackEmpty(MAX_LINES);
    for (char LineNr=0; LineNr<MAX_LINES; LineNr++)
    {
        InfoLine[LineNr].PushBackEmpty(256);
        InfoLine[LineNr][0]=0;
    }
}


void ScrollInfoT::Print(const char* PrintString, ...)
{
    va_list ArgList;
    char    InfoLineString[256];

    if (!PrintString) return;

    va_start(ArgList, PrintString);
        vsnprintf(InfoLineString, 256, PrintString, ArgList);
    va_end(ArgList);

    if (NrOfLines==MAX_LINES)
    {
        sprintf(&InfoLine[FirstLine][0], "%s", InfoLineString);
        FirstLine=(FirstLine+1) % MAX_LINES;
        TimeLeft=float(TimeToLive.GetValueDouble());
        return;
    }

    sprintf(&InfoLine[(FirstLine+NrOfLines) % MAX_LINES][0], "%s", InfoLineString);
    NrOfLines++;
}


void ScrollInfoT::Draw(FontT& Font, unsigned long PosX, unsigned long PosY, float FrameWidth, float FrameHeight) const
{
    if (NrOfLines==0) return;

    Font.AccPrintBegin(FrameWidth, FrameHeight);

    for (char LineNr=0; LineNr<NrOfLines; LineNr++)
        Font.AccPrint(PosX, PosY+20*LineNr, 0x00BBDDFF, "%s", &InfoLine[(FirstLine+LineNr) % MAX_LINES][0]);

    Font.AccPrintEnd();
}


void ScrollInfoT::AdvanceTime(float FrameTime)
{
    if (NrOfLines==0) return;

    TimeLeft-=FrameTime;
    if (TimeLeft>0) return;

    // Oberste Zeile entfernen
    FirstLine=(FirstLine+1) % MAX_LINES;
    NrOfLines--;
    TimeLeft=float(TimeToLive.GetValueDouble());
}
