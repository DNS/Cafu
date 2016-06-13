/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "DialogGotoPrimitive.hpp"

#include "wx/valgen.h"
#include "wxExt/valTextNumber.hpp"


GotoPrimitiveDialogT::GotoPrimitiveDialogT(wxWindow *parent)
    : wxDialog(parent, -1, wxString(_T("Goto Primitive/Entity")), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_EntityNumber(0),
      m_PrimitiveNumber(0)
{
    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );

    wxGridSizer *item1 = new wxGridSizer( 3, 0, 0 );

    wxStaticText *item2 = new wxStaticText( this, -1, wxT("Entity Number:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
    item1->Add( item2, 0, wxALIGN_CENTER|wxALL, 2 );

    wxTextCtrl *item3 = new wxTextCtrl( this, -1, wxT(""), wxDefaultPosition, wxSize(50,-1), 0, textNumberValidator(&m_EntityNumber) );
    item1->Add( item3, 0, wxALIGN_LEFT|wxALL, 2 );

    wxButton *item4 = new wxButton( this, wxID_OK, wxT("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    item1->Add( item4, 0, wxALIGN_CENTER|wxALL, 2 );

    wxStaticText *item5 = new wxStaticText( this, -1, wxT("Primitive Number:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
    item1->Add( item5, 0, wxALIGN_CENTER|wxALL, 2 );

    wxTextCtrl *item6 = new wxTextCtrl( this, -1, wxT(""), wxDefaultPosition, wxSize(50,-1), 0, textNumberValidator(&m_PrimitiveNumber) );
    item1->Add( item6, 0, wxALIGN_LEFT|wxALL, 2 );

    wxButton *item7 = new wxButton( this, wxID_CANCEL, wxT("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    item1->Add( item7, 0, wxALIGN_CENTER|wxALL, 2 );

    item0->Add( item1, 0, wxALIGN_CENTER|wxALL, 5 );

    // wxCheckBox *item8 = new wxCheckBox( this, -1, wxT("Search visible primitives only"), wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&m_VisiblesOnly) );
    // item0->Add( item8, 0, wxALIGN_CENTER|wxALL, 5 );

    this->SetSizer( item0 );
    item0->SetSizeHints( this );
}
