/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#ifndef CAFU_APP_CAFU_HPP_INCLUDED
#define CAFU_APP_CAFU_HPP_INCLUDED

#include "wx/app.h"
#include "wx/display.h"


namespace cf { class CompositeConsoleT; }
namespace cf { class ConsoleFileT; }
namespace cf { class ConsoleStringBufferT; }
class MainFrameT;


/// This class represents the Cafu Engine application.
class AppCafuT : public wxApp
{
    public:

    AppCafuT();
    ~AppCafuT();

    /// Returns the composite console that is also available via the global Console pointer.
    cf::CompositeConsoleT& GetConComposite() const;

    /// Returns the console that buffers all output.
    cf::ConsoleStringBufferT& GetConBuffer() const { return *m_ConBuffer; }

    // /// Returns the console that logs all output into a file (can be NULL if not used).
    // cf::ConsoleFileT& GetConFile() const { return *m_ConFile; }

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

    wxLocale*                 m_Locale;
    cf::ConsoleStringBufferT* m_ConBuffer;          ///< The console that buffers all output.
    cf::ConsoleFileT*         m_ConFile;            ///< The console that logs all output into a file (can be NULL if not used).
    bool                      m_IsCustomVideoMode;  ///< Whether we successfully set a custom video mode (screen resolution) during initialization.
    wxVideoMode               m_CurrentMode;        ///< The video mode that we're currently using.
    MainFrameT*               m_MainFrame;          ///< The Cafu application main frame.
};


/// This macro provides the wxGetApp() function which returns a reference to our AppCafuT instance.
DECLARE_APP(AppCafuT)

#endif
