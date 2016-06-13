/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ControlsBar.hpp"
#include "MaterialBrowserDialog.hpp"

#include "wx/statline.h"
#include "wx/confbase.h"


using namespace MaterialBrowser;


ControlsBarT::ControlsBarT(DialogT* Parent)
    : wxPanel(Parent, wxID_ANY),
      m_Parent(Parent),
      m_DisplaySizeChoice(NULL)
{
    wxBoxSizer *item2 = new wxBoxSizer( wxHORIZONTAL );

    wxBoxSizer *item3 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer *item4 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText *item5 = new wxStaticText( this, -1, wxT("Display Size:"), wxDefaultPosition, wxDefaultSize, 0 );
    item4->Add( item5, 0, wxALIGN_CENTER|wxALL, 5 );

    wxString strs6[] =
    {
        wxT("1:1 (original)"),
        wxT("max. 128 x 128"),
        wxT("max. 256 x 256"),
        wxT("max. 512 x 512")
    };
    m_DisplaySizeChoice=new wxChoice( this, DialogT::ID_CHOICE_DisplaySize, wxDefaultPosition, wxDefaultSize, 4, strs6, 0 );
    item4->Add(m_DisplaySizeChoice, 0, wxALIGN_CENTER|wxALL, 5 );

    item3->Add( item4, 0, wxALIGN_LEFT, 5 );

    item2->Add( item3, 0, wxALIGN_CENTER, 5 );

    item2->Add(0, 0, 1, wxEXPAND, 5);

    wxBoxSizer *item17 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer *item18 = new wxBoxSizer( wxHORIZONTAL );

    if (wxConfigBase::Get()->Read("General/Activate Hidden", 0L)==0x1978)
    {
        wxButton* ExportDiffuseMapsButton=new wxButton(this, DialogT::ID_BUTTON_ExportDiffMaps, wxT("Export Diff-Maps"), wxDefaultPosition, wxDefaultSize, 0 );
        item18->Add(ExportDiffuseMapsButton, 0, wxALL, 5 );
    }

    if (!Parent->m_Config.m_NoButtonMark)
    {
        wxButton *item20 = new wxButton( this, DialogT::ID_BUTTON_Mark, wxT("Mark"), wxDefaultPosition, wxDefaultSize, 0 );
        item18->Add( item20, 0, wxALIGN_CENTER|wxALL, 5 );
    }

    if (!Parent->m_Config.m_NoButtonReplace)
    {
        wxButton *item21 = new wxButton( this, DialogT::ID_BUTTON_Replace, wxT("Replace"), wxDefaultPosition, wxDefaultSize, 0 );
        item18->Add( item21, 0, wxALIGN_CENTER|wxALL, 5 );
    }

    item17->Add( item18, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5 );

    wxBoxSizer *item22 = new wxBoxSizer( wxHORIZONTAL );

    wxButton *item23 = new wxButton( this, wxID_OK, wxT("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    item22->Add( item23, 0, wxALIGN_CENTER|wxALL, 5 );

    wxButton *item24 = new wxButton( this, wxID_CANCEL, wxT("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    item22->Add( item24, 0, wxALIGN_CENTER|wxALL, 5 );

    item17->Add( item22, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5 );

    item2->Add( item17, 0, wxALIGN_CENTER, 5 );

    this->SetSizer( item2 );
    this->Layout();
}
