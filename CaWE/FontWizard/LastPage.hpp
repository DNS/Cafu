/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_FONTWIZARD_LAST_PAGE_HPP_INCLUDED
#define CAFU_FONTWIZARD_LAST_PAGE_HPP_INCLUDED

#include "wx/wx.h"
#include "wx/wizard.h"


class FontWizardT;


class LastPageT : public wxWizardPageSimple
{
    public:

    LastPageT(FontWizardT* Parent);


    private:

    FontWizardT* m_Parent;

    wxStaticText* m_FontName;

    void OnWizardPageChanged(wxWizardEvent& WE);
    void OnWizardPageChanging(wxWizardEvent& WE);

    DECLARE_EVENT_TABLE()
};

#endif
