/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CONTROLS_BAR_HPP_INCLUDED
#define CAFU_CONTROLS_BAR_HPP_INCLUDED

#include "wx/wx.h"


namespace MaterialBrowser
{
    class DialogT;


    class ControlsBarT : public wxPanel
    {
        public:

        ControlsBarT(DialogT* Parent);


        private:

        DialogT*  m_Parent;
        wxChoice* m_DisplaySizeChoice;

        friend class DialogT;
    };
}

#endif
