/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GUIEDITOR_LIVE_PREVIEW_HPP_INCLUDED
#define CAFU_GUIEDITOR_LIVE_PREVIEW_HPP_INCLUDED

#include "Templates/Pointer.hpp"
#include "wx/wx.h"


namespace cf { class UniScriptStateT; }
namespace cf { namespace GuiSys { class GuiImplT; } }


namespace GuiEditor
{
    class PreviewCanvasT;


    class LivePreviewT : public wxDialog
    {
        public:

        LivePreviewT(wxWindow* Parent, cf::UniScriptStateT* ScriptState, IntrusivePtrT<cf::GuiSys::GuiImplT> Gui, const wxString& ScriptFileName);
        ~LivePreviewT();

        cf::UniScriptStateT* GetScriptState() { return m_ScriptState; }
        IntrusivePtrT<cf::GuiSys::GuiImplT> GetGui() { return m_Gui; }


        private:

        cf::UniScriptStateT*                m_ScriptState;
        IntrusivePtrT<cf::GuiSys::GuiImplT> m_Gui;
        PreviewCanvasT*                     m_Canvas;

        void OnClose(wxCloseEvent& CE);

        DECLARE_EVENT_TABLE()
    };
}

#endif
