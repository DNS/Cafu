/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_FONTWIZARD_SETUP_PAGE_HPP_INCLUDED
#define CAFU_FONTWIZARD_SETUP_PAGE_HPP_INCLUDED

#include "wx/wx.h"
#include "wx/wizard.h"

#include "Templates/Array.hpp"


class FontWizardT;
struct BitmapT;


class SetupPageT : public wxWizardPageSimple
{
    public:

    SetupPageT(FontWizardT* Parent);
    ~SetupPageT();


    private:

    FontWizardT* m_Parent;

    wxTextCtrl* m_FontFile;
    wxButton*   m_FileButton;
    wxCheckBox* m_DebugCheckBox;

    void OnChooseFile(wxCommandEvent& CE);
    void OnWizardCancel(wxWizardEvent& WE);
    void OnWizardPageChanging(wxWizardEvent& WE);

    DECLARE_EVENT_TABLE()
};

#endif
