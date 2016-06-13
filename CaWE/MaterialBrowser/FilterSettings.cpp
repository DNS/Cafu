/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "FilterSettings.hpp"
#include "MaterialBrowserDialog.hpp"

#include "wx/statline.h"


using namespace MaterialBrowser;


FilterSettingsT::FilterSettingsT(DialogT* Parent)
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

    m_NameFilterCombobox = new wxComboBox( this, DialogT::ID_COMBO_NameFilter, wxT(""), wxDefaultPosition, wxSize(100,-1), 0, NULL, wxCB_DROPDOWN );
    for (unsigned long FilterNr=0; FilterNr<m_Parent->GetNameFilterHistory().Size(); FilterNr++)
        m_NameFilterCombobox->Append(m_Parent->GetNameFilterHistory()[FilterNr]);

    bSizer2->Add( m_NameFilterCombobox, 0, wxALL, 5 );

    bSizer1->Add( bSizer2, 0, wxEXPAND, 5 );

    m_OnlyShowUsedCheckbox = new wxCheckBox( this, DialogT::ID_CHECKBOX_OnlyShowUsed, wxT("Only show used materials"), wxDefaultPosition, wxDefaultSize, 0 );
    m_OnlyShowUsedCheckbox->SetValue(Parent->m_Config.m_OnlyShowUsed);

    bSizer1->Add( m_OnlyShowUsedCheckbox, 0, wxALL, 5 );

    if (!Parent->m_Config.m_NoFilterEditorMatsOnly)
    {
        m_OnlyShowEditorMaterials = new wxCheckBox( this, DialogT::ID_CHECKBOX_OnlyShowEditor, wxT("Only show editor materials"), wxDefaultPosition, wxDefaultSize, 0 );
        m_OnlyShowEditorMaterials->SetValue(true);

        bSizer1->Add( m_OnlyShowEditorMaterials, 0, wxALL, 5 );
    }

    this->SetSizer( bSizer1 );
    this->Layout();
}


wxString FilterSettingsT::GetNameFilterValue() const
{
    return m_NameFilterCombobox->GetValue();
}


void FilterSettingsT::SetNameFilterValue(const wxString& s)
{
    m_NameFilterCombobox->SetValue(s);
}
