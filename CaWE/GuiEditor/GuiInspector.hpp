/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GUIEDITOR_GUI_INSPECTOR_HPP_INCLUDED
#define CAFU_GUIEDITOR_GUI_INSPECTOR_HPP_INCLUDED

#include "ObserverPattern.hpp"

#include "wx/wx.h"
#include "wx/propgrid/manager.h"


namespace GuiEditor
{
    class ChildFrameT;
    class GuiDocumentT;

    class GuiInspectorT : public wxPropertyGridManager, public ObserverT
    {
        public:

        GuiInspectorT(ChildFrameT* Parent, const wxSize& Size);

        // ObserverT implementation.
        void NotifySubjectChanged_GuiPropertyModified(SubjectT* Subject);
        void NotifySubjectDies(SubjectT* dyingSubject);

        void RefreshPropGrid();


        private:

        GuiDocumentT*        m_GuiDocument;
        ChildFrameT*         m_Parent;
        bool                 m_IsRecursiveSelfUpdate;

        void OnPropertyGridChanged(wxPropertyGridEvent& Event);

        DECLARE_EVENT_TABLE()
    };
}

#endif
