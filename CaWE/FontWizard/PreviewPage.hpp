/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_FONTWIZARD_PREVIEW_PAGE_HPP_INCLUDED
#define CAFU_FONTWIZARD_PREVIEW_PAGE_HPP_INCLUDED

#include "wx/wx.h"
#include "wx/wizard.h"


class FontWizardT;


class PreviewPageT : public wxWizardPageSimple
{
    public:

    PreviewPageT(FontWizardT* Parent);
    ~PreviewPageT();

    private:

    FontWizardT* m_Parent;

    wxButton*       m_PreviewButton;
    wxStaticBitmap* m_PreviewBitmap;

    void OnPreviewButton(wxCommandEvent& CE);
    void OnWizardPageChanged(wxWizardEvent& WE);

    DECLARE_EVENT_TABLE()
};

#endif
