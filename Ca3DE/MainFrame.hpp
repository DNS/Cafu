/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MAIN_FRAME_HPP_INCLUDED
#define CAFU_MAIN_FRAME_HPP_INCLUDED

#include "wx/wx.h"


class GameInfoT;
class MainCanvasT;


/// This class represents the Cafu main frame.
class MainFrameT : public wxFrame
{
    public:

    /// The constructor.
    MainFrameT(const GameInfoT& GameInfo);


    private:

    void OnClose(wxCloseEvent& CE);     ///< Event handler for close events, e.g. after a system close button or command or a call to Close(). See wx Window Deletion Overview for more details.

    MainCanvasT* m_MainCanvas;

    DECLARE_EVENT_TABLE()
};

#endif
