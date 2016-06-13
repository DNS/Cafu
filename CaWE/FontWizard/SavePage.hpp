/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_FONTWIZARD_SAVE_PAGE_HPP_INCLUDED
#define CAFU_FONTWIZARD_SAVE_PAGE_HPP_INCLUDED

#include "wx/wx.h"
#include "wx/wizard.h"


class FontWizardT;


class SavePageT : public wxWizardPageSimple
{
    public:

    SavePageT(FontWizardT* Parent);
    ~SavePageT();

    private:

    FontWizardT* m_Parent;

	wxTextCtrl*   m_FontName;
	wxStaticText* m_FontDir;

	wxString      m_BaseDir;

    void OnFontNameChange(wxCommandEvent& CE);
    void OnWizardPageChanged(wxWizardEvent& WE);
    void OnWizardPageChanging(wxWizardEvent& WE);

    DECLARE_EVENT_TABLE()
};

#endif
