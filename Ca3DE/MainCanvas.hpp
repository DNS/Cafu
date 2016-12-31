/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MAIN_CANVAS_HPP_INCLUDED
#define CAFU_MAIN_CANVAS_HPP_INCLUDED

#include "MainWindow/MainWindowWx.hpp"
#include "Util/Util.hpp"
#include "wx/wx.h"
#include "wx/glcanvas.h"
#include "Resources.hpp"


class MainFrameT;
class GameInfoT;
class wxGLContext;


/// This class represents the Cafu main OpenGL 3D canvas.
class MainCanvasT : public wxGLCanvas
{
    public:

    /// The constructor.
    MainCanvasT(MainFrameT* Parent, const GameInfoT& GameInfo);

    /// The destructor.
    ~MainCanvasT();


    private:

    enum LastMousePosT { IN_CLIENT_3D_GUI, IN_OTHER_2D_GUI };

    void TakeScreenshot() const;

    void OnPaint(wxPaintEvent& PE);
    void OnSize(wxSizeEvent& SE);
    void OnIdle(wxIdleEvent& IE);   ///< The idle event handler runs one frame of the Cafu Engine (client and/or server).

    void OnMouseMove (wxMouseEvent& ME);
    void OnMouseWheel(wxMouseEvent& ME);
    void OnLMouseDown(wxMouseEvent& ME);
    void OnLMouseUp  (wxMouseEvent& ME);
    void OnRMouseDown(wxMouseEvent& ME);
    void OnRMouseUp  (wxMouseEvent& ME);

    void OnKeyDown(wxKeyEvent& KE);
    void OnKeyUp  (wxKeyEvent& KE);
    void OnKeyChar(wxKeyEvent& KE);

    MainFrameT*      m_Parent;
    const GameInfoT& m_GameInfo;
    ResourcesT*      m_Resources;
    bool             m_ResInitFailed; ///< Prevent repeated init attempts when it has already failed once.
    wxGLContext*     m_GLContext;     ///< The OpenGL rendering context that represents our app-global OpenGL state.
    TimerT           m_Timer;
    double           m_TotalTime;
    LastMousePosT    m_LastMousePos;  ///< Used to prevent unwanted changes to the players heading and pitch when we're switching back from a 2D GUI to the 3D client view.
    MainWindowWxT    m_MainWin;

    DECLARE_EVENT_TABLE()
};

#endif
