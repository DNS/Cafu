/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "wx/wx.h"
#include "wx/config.h"
#include "wx/confbase.h"
#include "wx/fileconf.h"

#include "DialogCustomCompile.hpp"


BEGIN_EVENT_TABLE(CustomCompileDialogT, wxDialog)
    EVT_BUTTON(wxID_OK, CustomCompileDialogT::OnOK)
END_EVENT_TABLE()


wxSizer* CustomCompileDialogT::CustomCompileDialogInit(wxWindow* parent, bool call_fit, bool set_sizer)
{
    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );

    wxStaticBox *item2 = new wxStaticBox( parent, -1, "Custom Map Compile Options");
    wxStaticBoxSizer *item1 = new wxStaticBoxSizer( item2, wxVERTICAL );

    wxBoxSizer *item3 = new wxBoxSizer( wxVERTICAL );

    item3->Add( 300, 5, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 0 );

    wxStaticText *item4 = new wxStaticText( parent, -1, "CaBSP ("+wxConfigBase::Get()->Read("General/BSP Executable")+"):", wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add( item4, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 5 );

    CaBSPOptions = new wxTextCtrl( parent, -1, wxConfigBase::Get()->Read("CustomCompileOptions/CaBSP", ""), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add( CaBSPOptions, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL);

    wxStaticText *item6 = new wxStaticText( parent, -1, "CaPVS ("+wxConfigBase::Get()->Read("General/PVS Executable")+"):", wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add( item6, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 5 );

    CaPVSOptions = new wxTextCtrl( parent, -1, wxConfigBase::Get()->Read("CustomCompileOptions/CaPVS", ""), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add( CaPVSOptions, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL);

    wxStaticText *item8 = new wxStaticText( parent, -1, "CaLight ("+wxConfigBase::Get()->Read("General/Light Executable")+"):", wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add( item8, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 5 );

    CaLightOptions = new wxTextCtrl( parent, -1, wxConfigBase::Get()->Read("CustomCompileOptions/CaLight", ""), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add( CaLightOptions, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL);

    wxStaticText *item10 = new wxStaticText( parent, -1, "Cafu Engine ("+wxConfigBase::Get()->Read("General/Engine Executable")+"):", wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add( item10, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 5 );

    EngineOptions = new wxTextCtrl( parent, -1, wxConfigBase::Get()->Read("CustomCompileOptions/Engine", ""), wxDefaultPosition, wxDefaultSize, 0 );
    item3->Add( EngineOptions, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL);

    item1->Add( item3, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxBOTTOM, 5 );

    wxBoxSizer *item12 = new wxBoxSizer( wxHORIZONTAL );

    wxButton *item13 = new wxButton( parent, wxID_OK, "OK", wxDefaultPosition, wxDefaultSize, 0 );
    item13->SetDefault();
    item12->Add( item13, 0, wxALIGN_CENTER|wxALL, 5 );

    wxButton *item14 = new wxButton( parent, wxID_CANCEL, "Cancel", wxDefaultPosition, wxDefaultSize, 0 );
    item12->Add( item14, 0, wxALIGN_CENTER|wxALL, 5 );

    item1->Add( item12, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP, 5 );

    item0->Add( item1, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    if (set_sizer)
    {
        parent->SetSizer( item0 );
        if (call_fit)
            item0->SetSizeHints( parent );
    }

    return item0;
}

CustomCompileDialogT::CustomCompileDialogT(wxWindow* Parent)
    : wxDialog(Parent, -1, wxString("Custom Map Compile Options"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    CustomCompileDialogInit(this);
}


// On OK button, values of all textfields are stored in the global configuration file.
void CustomCompileDialogT::OnOK(wxCommandEvent&)
{
    wxConfigBase::Get()->SetPath("CustomCompileOptions");

    wxConfigBase::Get()->Write("CaBSP",   CaBSPOptions  ->GetValue());
    wxConfigBase::Get()->Write("CaPVS",   CaPVSOptions  ->GetValue());
    wxConfigBase::Get()->Write("CaLight", CaLightOptions->GetValue());
    wxConfigBase::Get()->Write("Engine",  EngineOptions ->GetValue());

    wxConfigBase::Get()->SetPath("..");

    EndModal(wxID_OK);
}
