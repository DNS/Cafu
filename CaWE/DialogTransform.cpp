/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#include "DialogTransform.hpp"

#include "wx/spinctrl.h"
#include "wx/valgen.h"
#include "wxExt/valTextNumber.hpp"


TransformDialogT::TransformDialogT(wxWindow *parent)
    : wxDialog(parent, -1, wxString(_T("Transform Type-In")), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_Value(),
      m_Mode(0)
{
    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer *item1 = new wxBoxSizer( wxHORIZONTAL );

    wxString strs[] =
    {
        wxT("Move"),
        wxT("Scale"),
        wxT("Rotate")
    };

    wxRadioBox *item2 = new wxRadioBox( this, -1, wxT("Mode"), wxDefaultPosition, wxDefaultSize, 3, strs, 1, wxRA_SPECIFY_COLS, wxGenericValidator(&m_Mode));
    item1->Add( item2, 0, wxALIGN_CENTER|wxALL|wxEXPAND, 2 );

    wxStaticBox *item4 = new wxStaticBox( this, -1, wxT("Values") );
    wxStaticBoxSizer *item3 = new wxStaticBoxSizer( item4, wxVERTICAL );

    wxBoxSizer *item5 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText *item6 = new wxStaticText( this, -1, wxT("X:"), wxDefaultPosition, wxDefaultSize, 0 );
    item5->Add( item6, 0, wxALIGN_CENTER | wxALL, 2 );

    wxTextCtrl *item7 = new wxTextCtrl( this, -1, wxT(""), wxDefaultPosition, wxSize(80,-1), 0, textNumberValidator(&m_Value.x));
    item5->Add( item7, 0, wxALIGN_CENTER|wxALL, 2 );

    item3->Add( item5, 0, wxALIGN_CENTER|wxALL, 2 );

    wxBoxSizer *item8 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText *item9 = new wxStaticText( this, -1, wxT("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
    item8->Add( item9, 0, wxALIGN_CENTER|wxALL, 2 );

    wxTextCtrl *item10 = new wxTextCtrl( this, -1, wxT(""), wxDefaultPosition, wxSize(80,-1), 0, textNumberValidator(&m_Value.y));
    item8->Add( item10, 0, wxALIGN_CENTER|wxALL, 2 );

    item3->Add( item8, 0, wxALIGN_CENTER|wxALL, 2);

    //
    wxBoxSizer *item11 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText *item12 = new wxStaticText( this, -1, wxT("Z:"), wxDefaultPosition, wxDefaultSize, 0 );
    item11->Add( item12, 0, wxALIGN_CENTER|wxALL, 2 );

    wxTextCtrl *item13 = new wxTextCtrl( this, -1, wxT(""), wxDefaultPosition, wxSize(80,-1), 0, textNumberValidator(&m_Value.z));
    item11->Add( item13, 0, wxALIGN_CENTER|wxALL, 2 );

    item3->Add( item11, 0, wxALIGN_CENTER|wxALL, 2 );

    item1->Add( item3, 0, wxALIGN_CENTER|wxALL, 2 );
    item0->Add( item1, 0, wxALIGN_CENTER|wxALL, 2 );
    wxBoxSizer *item14 = new wxBoxSizer( wxHORIZONTAL );

    wxButton *item15 = new wxButton( this, wxID_CANCEL, wxT("Close"), wxDefaultPosition, wxDefaultSize, 0 );
    item14->Add( item15, 0, wxALIGN_CENTER|wxALL, 2 );

    wxButton *item16 = new wxButton( this, wxID_OK, wxT("Apply"), wxDefaultPosition, wxDefaultSize, 0 );
    item14->Add( item16, 0, wxALIGN_CENTER|wxALL, 2 );

    item0->Add( item14, 0, wxALIGN_CENTER|wxALL, 2 );

    this->SetSizer(item0);
    item0->SetSizeHints(this);
}
