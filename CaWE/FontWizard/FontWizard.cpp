/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

Cafu is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

Cafu is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Cafu. If not, see <http://www.gnu.org/licenses/>.

For support and more information about Cafu, visit us at <http://www.cafu.de>.
=================================================================================
*/

#include "FontWizard.hpp"

#include "WelcomePage.hpp"
#include "SetupPage.hpp"
#include "PreviewPage.hpp"
#include "SavePage.hpp"
#include "LastPage.hpp"


FontWizardT::FontWizardT(wxWindow* Parent)
    : wxWizard(Parent, wxID_ANY, "New Font Wizard", wxBitmap::wxBitmap("CaWE/res/FontWizard.png", wxBITMAP_TYPE_PNG), wxDefaultPosition, wxDEFAULT_DIALOG_STYLE),
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
