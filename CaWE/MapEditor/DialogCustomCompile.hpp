/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
