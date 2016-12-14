/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "MainWindowWx.hpp"

#include <wx/frame.h>
#include <wx/glcanvas.h>


MainWindowWxT::MainWindowWxT(wxFrame* Frame, wxGLCanvas* Canvas)
    : MainWindowT(),
      m_Frame(Frame),
      m_Canvas(Canvas)
{
}


unsigned int MainWindowWxT::GetFrameBufferWidth() const
{
    return m_Frame->GetClientSize().GetWidth();
}


unsigned int MainWindowWxT::GetFrameBufferHeight() const
{
    return m_Frame->GetClientSize().GetWidth();
}


bool MainWindowWxT::IsKeyDown(unsigned int Key) const
{
    // Must translate the CaKeyboardEventT::CK_* key to a wxWidgets key.
    // TODO: int wxKey = translateCKToWx(Key);
    // return wxGetKeyState(wxKey);
    return false;
}


bool MainWindowWxT::LeftMB_IsDown() const
{
    return wxGetMouseState().LeftIsDown();
}


bool MainWindowWxT::MiddleMB_IsDown() const
{
    return wxGetMouseState().MiddleIsDown();
}


bool MainWindowWxT::RightMB_IsDown() const
{
    return wxGetMouseState().RightIsDown();
}


void MainWindowWxT::SwapBuffers()
{
    m_Canvas->SwapBuffers();
}
