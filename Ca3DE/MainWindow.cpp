/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "MainWindow.hpp"
#include "MainCanvas.hpp"
#include "MainFrame.hpp"

#include <wx/utils.h>


MainWindowT::MainWindowT(MainFrameT* MainFrame)
    : m_MainFrame(MainFrame)
{
}


MainWindowT::~MainWindowT()
{
}


unsigned int MainWindowT::GetFrameBufferWidth() const
{
    return m_MainFrame->GetClientSize().GetWidth();
}


unsigned int MainWindowT::GetFrameBufferHeight() const
{
    return m_MainFrame->GetClientSize().GetWidth();
}


bool MainWindowT::IsKeyDown(unsigned int Key) const
{
    // return wxGetKeyState(Key);
    return false;
}


bool MainWindowT::LeftMB_IsDown() const
{
    return wxGetMouseState().LeftIsDown();
}


bool MainWindowT::MiddleMB_IsDown() const
{
    return wxGetMouseState().MiddleIsDown();
}


bool MainWindowT::RightMB_IsDown() const
{
    return wxGetMouseState().RightIsDown();
}


void MainWindowT::SwapBuffers()
{
    m_MainFrame->GetMainCanvas()->SwapBuffers();
}
