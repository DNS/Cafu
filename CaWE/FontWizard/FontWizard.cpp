/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "FontWizard.hpp"

#include "WelcomePage.hpp"
#include "SetupPage.hpp"
#include "PreviewPage.hpp"
#include "SavePage.hpp"
#include "LastPage.hpp"


FontWizardT::FontWizardT(wxWindow* Parent)
    : wxWizard(Parent, wxID_ANY, "New Font Wizard", wxBitmap("CaWE/res/FontWizard.png", wxBITMAP_TYPE_PNG), wxDefaultPosition, wxDEFAULT_DIALOG_STYLE),
      m_FirstPage(new WelcomePageT(this))
{
    SetupPageT*   SetupPage  =new SetupPageT(this);
    PreviewPageT* PreviewPage=new PreviewPageT(this);
    SavePageT*    SavePage   =new SavePageT(this);
    LastPageT*    LastPage   =new LastPageT(this);

    wxWizardPageSimple::Chain(m_FirstPage, SetupPage);
    wxWizardPageSimple::Chain(SetupPage, PreviewPage);
    wxWizardPageSimple::Chain(PreviewPage, SavePage);
    wxWizardPageSimple::Chain(SavePage, LastPage);
}


void FontWizardT::Run()
{
    RunWizard(m_FirstPage);
}


bool FontWizardT::GenerateFont(const wxString& FontFile, bool DebugPNGs)
{
    return m_FontGenerator.GenerateFont(FontFile, DebugPNGs);
}


void FontWizardT::SaveFont(const wxString& Directory, const wxString& MaterialBaseName) const
{
    m_FontGenerator.SaveFont(Directory, MaterialBaseName);
}


unsigned long FontWizardT::GetNrOfSizes() const
{
    return m_FontGenerator.GetNrOfSizes();
}


ArrayT<BitmapT*> FontWizardT::GetBitmaps(unsigned long SizeNr) const
{
    return m_FontGenerator.GetBitmaps(SizeNr);
}
