/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_FONTWIZARD_FONT_WIZARD_HPP_INCLUDED
#define CAFU_FONTWIZARD_FONT_WIZARD_HPP_INCLUDED

#include "wx/wizard.h"

#include "FontGenerator.hpp"


class FontWizardT : public wxWizard
{
    public:

    FontWizardT(wxWindow* Parent);

    void Run();

    // Interface to FontGeneratorT.
    bool             GenerateFont(const wxString& FontFile, bool DebugPNGs=false);
    void             SaveFont(const wxString& Directory, const wxString& MaterialBaseName="") const;
    unsigned long    GetNrOfSizes() const;
    ArrayT<BitmapT*> GetBitmaps(unsigned long SizeNr) const;

    wxString FontName;
    wxString DefaultFontName;


    private:

    FontGeneratorT      m_FontGenerator;
    wxWizardPageSimple* m_FirstPage;
};

#endif
