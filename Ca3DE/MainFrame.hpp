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

#ifndef _CAFU_MAIN_FRAME_HPP_
#define _CAFU_MAIN_FRAME_HPP_

#include "wx/wx.h"


class MainCanvasT;


/// This class represents the Cafu main frame.
class MainFrameT : public wxFrame
{
    public:

    /// The constructor.
    MainFrameT();

    /// Returns the main OpenGL 3D canvas.
    MainCanvasT* GetMainCanvas() { return m_MainCanvas; }


    private:

    void OnClose(wxCloseEvent& CE);     ///< Event handler for close events, e.g. after a system close button or command or a call to Close(). See wx Window Deletion Overview for more details.

    MainCanvasT* m_MainCanvas;

    DECLARE_EVENT_TABLE()
};

#endif
