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

#ifndef CAFU_DIALOG_CUSTOM_COMPILE_HPP_INCLUDED
#define CAFU_DIALOG_CUSTOM_COMPILE_HPP_INCLUDED

#include "wx/dialog.h"


/**
 * Dialog to configure map compile options.
 *
 * These options are read from the global configuration file and stored in it
 * along with possible changes made by the user.
 */
class CustomCompileDialogT : public wxDialog
{
    public:

    CustomCompileDialogT(wxWindow* parent);

    // Methods to access the compile options, that are set in the dialog.
    wxString GetCaBSPOptions()   { return CaBSPOptions  ->GetValue(); }
    wxString GetCaPVSOptions()   { return CaPVSOptions  ->GetValue(); }
    wxString GetCaLightOptions() { return CaLightOptions->GetValue(); }
    wxString GetEngineOptions()  { return EngineOptions ->GetValue(); }


    private:

    wxSizer* CustomCompileDialogInit(wxWindow* parent, bool call_fit=true, bool set_sizer=true);

    wxTextCtrl* CaBSPOptions;
    wxTextCtrl* CaPVSOptions;
    wxTextCtrl* CaLightOptions;
    wxTextCtrl* EngineOptions;

    void OnOK(wxCommandEvent&);

    DECLARE_EVENT_TABLE()
};

#endif
