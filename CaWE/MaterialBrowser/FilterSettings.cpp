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

#include "FilterSettings.hpp"

#include "MaterialBrowserDialog.hpp"

#include "wx/statline.h"


FilterSettingsT::FilterSettingsT(MaterialBrowserDialogT* Parent)
    : wxPanel(Parent, wxID_ANY),
      m_Parent(Parent),
      m_NameFilterCombobox(NULL),
      m_OnlyShowUsedCheckbox(NULL),
      m_OnlyShowEditorMaterials(NULL)
{
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* m_staticText1 = new wxStaticText( this, wxID_ANY, wxT("Filter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bSizer2->Add( m_staticText1, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_NameFilterCombobox = new wxComboBox( this, MaterialBrowserDialogT::ID_COMBO_NameFilter, wxT(""), wxDefaultPosition, wxSize(100,-1), 0, NULL, wxCB_DROPDOWN );
    for (unsigned long FilterNr=0; FilterNr<m_Parent->GetNameFilterHistory().Size(); FilterNr++)
        m_NameFilterCombobox->Append(m_Parent->GetNameFilterHistory()[FilterNr]);

	bSizer2->Add( m_NameFilterCombobox, 0, wxALL, 5 );

	bSizer1->Add( bSizer2, 0, wxEXPAND, 5 );

	m_OnlyShowUsedCheckbox = new wxCheckBox( this, MaterialBrowserDialogT::ID_CHECKBOX_OnlyShowUsed, wxT("Only show used materials"), wxDefaultPosition, wxDefaultSize, 0 );

	bSizer1->Add( m_OnlyShowUsedCheckbox, 0, wxALL, 5 );

	m_OnlyShowEditorMaterials = new wxCheckBox( this, MaterialBrowserDialogT::ID_CHECKBOX_OnlyShowEditor, wxT("Only show editor materials"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OnlyShowEditorMaterials->SetValue(true);

	bSizer1->Add( m_OnlyShowEditorMaterials, 0, wxALL, 5 );

	this->SetSizer( bSizer1 );
	this->Layout();
}


wxString FilterSettingsT::GetNameFilterValue() const
{
    return m_NameFilterCombobox->GetValue();
}
