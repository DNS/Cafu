/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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
class ModelManagerT;
namespace cf { namespace GuiSys { class GuiResourcesT; } }
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
    enum LastMousePosT { IN_CLIENT_3D_GUI, IN_OTHER_2D_GUI };

    void Initialize();
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

    MainFrameT*                m_Parent;
    InitStateT                 m_InitState;     ///< Indicates whether initialization is still required, was attempted but failed, or completed successfully.
    wxGLContext*               m_GLContext;     ///< The OpenGL rendering context that represents our app-global OpenGL state.
    HMODULE                    m_RendererDLL;
    ModelManagerT*             m_ModelManager;
    cf::GuiSys::GuiResourcesT* m_GuiResources;
    HMODULE                    m_SoundSysDLL;
    HMODULE                    m_GameDLL;
    ClientT*                   m_Client;
    ServerT*                   m_Server;
    SvGuiCallbT*               m_SvGuiCallback;
    cf::ConsoleI*              m_ConByGuiWin;   ///< This points to an instance of cf::GuiSys::ConsoleByWindowT.
    TimerT                     m_Timer;
    double                     m_TotalTime;
    LastMousePosT              m_LastMousePos;  ///< Used to prevent unwanted changes to the players heading and pitch when we're switching back from a 2D GUI to the 3D client view.

    DECLARE_EVENT_TABLE()
};

#endif
