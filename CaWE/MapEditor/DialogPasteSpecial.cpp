/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "DialogPasteSpecial.hpp"


PasteSpecialDialogT::PasteSpecialDialogT()
    : wxDialog(NULL, -1, "Paste Special", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE /*| wxRESIZE_BORDER*/),
      m_PastePos(NULL)
{
    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );

    wxString Choices[] =
    {
        "normally (at a well-visible location, e.g. centered in the 2D views)",
        "with original parent-space transforms (pasted objects relate to the \n"
        "new parent as they related to their original parent)"
    };

    m_PastePos = new wxRadioBox(this, wxID_ANY, "Position the pasted elements:",
        wxDefaultPosition, wxDefaultSize, 2, Choices, 1);

    item0->Add(m_PastePos, 0, wxEXPAND | wxALL, 5);


    wxBoxSizer *item32 = new wxBoxSizer( wxHORIZONTAL );

    wxButton *item33 = new wxButton(this, wxID_OK, wxT("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    item33->SetDefault();
    item32->Add( item33, 0, wxALIGN_CENTER|wxALL, 5 );

    wxButton *item34 = new wxButton(this, wxID_CANCEL, wxT("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    item32->Add( item34, 0, wxALIGN_CENTER|wxALL, 5 );

    item0->Add( item32, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

    this->SetSizer(item0);
    item0->SetSizeHints(this);
}


bool PasteSpecialDialogT::MakePastedElementsWellVisible() const
{
    return m_PastePos->GetSelection() == 0;
}
