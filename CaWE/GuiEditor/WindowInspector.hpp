/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GUIEDITOR_WINDOW_INSPECTOR_HPP_INCLUDED
#define CAFU_GUIEDITOR_WINDOW_INSPECTOR_HPP_INCLUDED

#include "ObserverPattern.hpp"

#include "wx/wx.h"
#include "wx/propgrid/manager.h"


namespace cf { namespace GuiSys { class ComponentBaseT; } }
namespace cf { namespace GuiSys { class WindowT; } }
namespace cf { namespace TypeSys { class VarBaseT; } }


namespace GuiEditor
{
    class ChildFrameT;
    class GuiDocumentT;

    class WindowInspectorT : public wxPropertyGridManager, public ObserverT
    {
        public:

        WindowInspectorT(ChildFrameT* Parent, const wxSize& Size);

        // ObserverT implementation.
        void NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& OldSelection, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& NewSelection);
        void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows);
        void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows, WindowModDetailE Detail);
        void Notify_Changed(SubjectT* Subject, const cf::TypeSys::VarBaseT& Var);
        void NotifySubjectDies(SubjectT* dyingSubject);

        void RefreshPropGrid();


        private:

        GuiDocumentT*                      m_GuiDocument;
        ChildFrameT*                       m_Parent;
        IntrusivePtrT<cf::GuiSys::WindowT> m_SelectedWindow;
        bool                               m_IsRecursiveSelfNotify;

        void AppendComponent(IntrusivePtrT<cf::GuiSys::ComponentBaseT> Comp);

        void OnPropertyGridChanging(wxPropertyGridEvent& Event);
        void OnPropertyGridChanged(wxPropertyGridEvent& Event);
        void OnPropertyGridRightClick(wxPropertyGridEvent& Event);

        DECLARE_EVENT_TABLE()
    };
}

#endif
