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

#include "ScrlInfo.hpp"
#include "Fonts/Font.hpp"
#include "ConsoleCommands/ConVar.hpp"


static ConVarT TimeToLive("cl_MsgShowTime", 6.0, ConVarT::FLAG_MAIN_EXE, "How long (in seconds) info and chat messages are shown.", 0.0, 30.0);


ScrollInfoT::ScrollInfoT()
    : m_MAX_LINES(8)
{
    m_TimeLeft=float(TimeToLive.GetValueDouble());
}


void ScrollInfoT::Print(const std::string& Line)
{
    m_InfoLines.PushBack(Line);

    while (m_InfoLines.Size()>m_MAX_LINES)
    {
        m_InfoLines.RemoveAtAndKeepOrder(0);
        m_TimeLeft=float(TimeToLive.GetValueDouble());
    }
}


void ScrollInfoT::Draw(FontT& Font, unsigned long PosX, unsigned long PosY, float FrameWidth, float FrameHeight) const
{
    if (m_InfoLines.Size()==0) return;

    Font.AccPrintBegin(FrameWidth, FrameHeight);

    for (unsigned long LineNr=0; LineNr<m_InfoLines.Size(); LineNr++)
        Font.AccPrint(PosX, PosY+20*LineNr, 0x00BBDDFF, m_InfoLines[LineNr]);

    Font.AccPrintEnd();
}


void ScrollInfoT::AdvanceTime(float FrameTime)
{
    if (m_InfoLines.Size()==0) return;

    m_TimeLeft-=FrameTime;
    if (m_TimeLeft>0) return;

    m_InfoLines.RemoveAtAndKeepOrder(0);
    m_TimeLeft=float(TimeToLive.GetValueDouble());
}
