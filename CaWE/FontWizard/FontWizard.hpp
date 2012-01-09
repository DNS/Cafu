/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#ifndef _FONTWIZARD_FONT_WIZARD_HPP_
#define _FONTWIZARD_FONT_WIZARD_HPP_

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
