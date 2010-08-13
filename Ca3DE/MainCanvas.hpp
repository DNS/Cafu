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

#ifndef _CAFU_MAIN_CANVAS_HPP_
#define _CAFU_MAIN_CANVAS_HPP_

#include "Util/Util.hpp"
#include "wx/wx.h"
#include "wx/glcanvas.h"

#if __linux__
#define HMODULE void*
#endif


class MainFrameT;
class wxGLContext;
class ClientT;
class ServerT;
class SvGuiCallbT;
namespace cf { class ConsoleI; }


/// This class represents the Cafu main OpenGL 3D canvas.
class MainCanvasT : public wxGLCanvas
{
    public:

    /// The constructor.
    MainCanvasT(MainFrameT* Parent);

    /// The destructor.
    ~MainCanvasT();


    private:

    enum InitStateT { INIT_REQUIRED, INIT_FAILED, INIT_SUCCESS };

    void Initialize();

    void OnPaint(wxPaintEvent& PE);
    void OnSize(wxSizeEvent& SE);
    void OnIdle(wxIdleEvent& IE);   ///< The idle event handler runs one frame of the Cafu Engine (client and/or server).
 // void OnShow(wxShowEvent& SE);   ///< Event handler for "has been shown" events.

    void OnMouseMove (wxMouseEvent& ME);
    void OnMouseWheel(wxMouseEvent& ME);
    void OnLMouseDown(wxMouseEvent& ME);
    void OnLMouseUp  (wxMouseEvent& ME);
    void OnRMouseDown(wxMouseEvent& ME);
    void OnRMouseUp  (wxMouseEvent& ME);

    void OnKeyDown(wxKeyEvent& KE);
    void OnKeyUp  (wxKeyEvent& KE);
    void OnKeyChar(wxKeyEvent& KE);

    MainFrameT*   m_Parent;
    InitStateT    m_InitState;      ///< Indicates whether initialization is still required, was attempted but failed, or completed successfully.
    wxGLContext*  m_GLContext;      ///< The OpenGL rendering context that represents our app-global OpenGL state.
    HMODULE       m_RendererDLL;
    HMODULE       m_SoundSysDLL;
    HMODULE       m_GameDLL;
    ClientT*      m_Client;
    ServerT*      m_Server;
    SvGuiCallbT*  m_SvGuiCallback;
    cf::ConsoleI* m_PrevConsole;
    cf::ConsoleI* m_ConByGuiWin;    ///< This points to an instance of cf::GuiSys::ConsoleByWindowT.
    TimerT        m_Timer;
    double        m_TotalTime;

    DECLARE_EVENT_TABLE()
};

#endif
