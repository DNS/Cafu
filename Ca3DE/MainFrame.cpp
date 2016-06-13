/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "MainFrame.hpp"
#include "AppCafu.hpp"
#include "MainCanvas.hpp"


BEGIN_EVENT_TABLE(MainFrameT, wxFrame)
    EVT_CLOSE(MainFrameT::OnClose)
END_EVENT_TABLE()


MainFrameT::MainFrameT(const GameInfoT& GameInfo)
    : wxFrame(NULL /*parent*/, wxID_ANY, wxString("Cafu Engine - ") + __DATE__),
      m_MainCanvas(NULL)
{
#ifdef __WXMSW__
    SetIcon(wxIcon("aaaa", wxBITMAP_TYPE_ICO_RESOURCE));
#endif

    // Create sizer and insert canvas.
    wxBoxSizer* Sizer=new wxBoxSizer(wxHORIZONTAL);

    m_MainCanvas=new MainCanvasT(this, GameInfo);
    Sizer->Add(m_MainCanvas, 1, wxEXPAND, 0);

    this->SetSizer(Sizer);
    this->Layout();

    // Show the frame - it is not shown by default...
    if (wxGetApp().IsCustomVideoMode()) ShowFullScreen(true); else Show();
}


void MainFrameT::OnClose(wxCloseEvent& CE)
{
    if (!CE.CanVeto())
    {
        Destroy();
        return;
    }

    // All child frames were successfully closed.
    // It's safe now to also close this parent window, which will gracefully exit the application.
    Destroy();
}
