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

/**************************************/
/***                                ***/
/***          Cafu Engine           ***/
/***                                ***/
/*** Dass ich erkenne, was die Welt ***/
/*** im Innersten zusammenhält.     ***/
/*** (Faust)                        ***/
/***                                ***/
/**************************************/

#ifndef _APP_CAFU_HPP_
#define _APP_CAFU_HPP_

#include "wx/app.h"
#include "wx/display.h"


class MainFrameT;


/// This class represents the Cafu Engine application.
class AppCafu : public wxApp
{
    public:

    AppCafu();

    /// Returns whether we successfully set a custom video mode (screen resolution) during initialization.
    bool IsCustomVideoMode() const { return m_IsCustomVideoMode; }

    /// This method returns the current video mode, which may be identical to the desktops video mode
    /// (in which case the mode as not switched at all at app init), or any custom mode.
    const wxVideoMode& GetCurrentMode() const { return m_CurrentMode; }

    /// Returns the main frame of the Cafu application.
    MainFrameT* GetMainFrame() const { return m_MainFrame; }

    bool OnInit();
    int  OnExit();


    private:

    void OnInitCmdLine(wxCmdLineParser& Parser);
    bool OnCmdLineParsed(wxCmdLineParser& Parser);

    bool        m_IsCustomVideoMode;  ///< Whether we successfully set a custom video mode (screen resolution) during initialization.
    wxVideoMode m_CurrentMode;        ///< The video mode that we're currently using.
    MainFrameT* m_MainFrame;          ///< The Cafu application main frame.
};


/// This macro provides the wxGetApp() function, which returns a reference to AppCafu, for use in other files.
DECLARE_APP(AppCafu)

#endif
