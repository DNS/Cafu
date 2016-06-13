/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
