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


class MainFrameT;


/// This class represents the Cafu Engine application.
class AppCafu : public wxApp
{
    public:

    AppCafu();

    MainFrameT* GetMainFrame() const { return m_MainFrame; }

    bool OnInit();
    int  OnExit();


    private:

    MainFrameT* m_MainFrame;
};


/// This macro provides the wxGetApp() function, which returns a reference to AppCafu, for use in other files.
DECLARE_APP(AppCafu)

#endif
