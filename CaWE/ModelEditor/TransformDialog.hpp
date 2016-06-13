/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MODELEDITOR_TRANSFORM_DIALOG_HPP_INCLUDED
#define CAFU_MODELEDITOR_TRANSFORM_DIALOG_HPP_INCLUDED

#include "ObserverPattern.hpp"
#include "wx/wx.h"


class wxNotebook;


namespace ModelEditor
{
    class ChildFrameT;
    class ModelDocumentT;

    class TransformDialogT : public wxPanel, public ObserverT
    {
        public:

        TransformDialogT(ChildFrameT* Parent, const wxSize& Size);
        ~TransformDialogT();

        // ObserverT implementation.
        void Notify_SelectionChanged(SubjectT* Subject, ModelElementTypeT Type, const ArrayT<unsigned int>& OldSel, const ArrayT<unsigned int>& NewSel);
        void Notify_SubjectDies(SubjectT* dyingSubject);


        private:

        /// IDs for the controls whose events we are interested in.
        enum
        {
            ID_BUTTON_RESET=wxID_HIGHEST+1,
            ID_BUTTON_APPLY
        };

        void OnButton(wxCommandEvent& Event);

        ModelDocumentT* m_ModelDoc;
        ChildFrameT*    m_Parent;
        wxNotebook*     m_Notebook;

        float           m_Values[3][3];     ///< Translate, rotate and scale: three values each.

        DECLARE_EVENT_TABLE()
    };
}

#endif
