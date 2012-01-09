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

#include "wx/wx.h"
#include "wx/spinctrl.h"
#include "wx/valgen.h"
#include "wx/confbase.h"
#include "DialogPasteSpecial.hpp"


// TODO: Add a RESET button that resets all the spin controls!
// TODO: Employ a validator that validates, wxGenericValidator doesn't!
BEGIN_EVENT_TABLE(PasteSpecialDialogT, wxDialog)
    EVT_BUTTON(PasteSpecialDialogT::ID_BUTTON_GET_WIDTH , PasteSpecialDialogT::OnButtonGetWidth )
    EVT_BUTTON(PasteSpecialDialogT::ID_BUTTON_GET_DEPTH , PasteSpecialDialogT::OnButtonGetDepth )
    EVT_BUTTON(PasteSpecialDialogT::ID_BUTTON_GET_HEIGHT, PasteSpecialDialogT::OnButtonGetHeight)
    EVT_BUTTON(wxID_OK                                  , PasteSpecialDialogT::OnButtonOK       )
END_EVENT_TABLE()


PasteSpecialDialogT::PasteSpecialDialogT(const BoundingBox3fT& ObjectsBox)
    : wxDialog(NULL, -1, "Paste Special", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE /*| wxRESIZE_BORDER*/),
      NrOfCopies      (wxConfigBase::Get()->Read("Paste Special Dialog/Number Of Copies"  , 1l)),
      CenterAtOriginal(wxConfigBase::Get()->Read("Paste Special Dialog/Center At Original", 1l)!=0),
      GroupCopies     (wxConfigBase::Get()->Read("Paste Special Dialog/Group Copies"      , 0l)!=0),
      TranslateX      (wxConfigBase::Get()->Read("Paste Special Dialog/TranslateX"        , 0l)),
      TranslateY      (wxConfigBase::Get()->Read("Paste Special Dialog/TranslateY"        , 0l)),
      TranslateZ      (wxConfigBase::Get()->Read("Paste Special Dialog/TranslateZ"        , 0l)),
      RotateX         (wxConfigBase::Get()->Read("Paste Special Dialog/RotateX"           , 0l)),
      RotateY         (wxConfigBase::Get()->Read("Paste Special Dialog/RotateY"           , 0l)),
      RotateZ         (wxConfigBase::Get()->Read("Paste Special Dialog/RotateZ"           , 0l)),
      ObjectsSizeX((int)(ObjectsBox.Max[0]-ObjectsBox.Min[0])),
      ObjectsSizeY((int)(ObjectsBox.Max[1]-ObjectsBox.Min[1])),
      ObjectsSizeZ((int)(ObjectsBox.Max[2]-ObjectsBox.Min[2]))
{
    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer *item1 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText *item2 = new wxStaticText(this, -1, wxT("Number of copies to paste:"), wxDefaultPosition, wxDefaultSize, 0 );
    item1->Add( item2, 0, wxALIGN_CENTER|wxALL, 5 );

    wxSpinCtrl *item3 = new wxSpinCtrl(this, -1, wxT("1"), wxDefaultPosition, wxSize(60,-1), wxSP_ARROW_KEYS, 1, 100, 1 );
    item3->SetValidator(wxGenericValidator(&NrOfCopies));
    item1->Add( item3, 0, wxALIGN_CENTER|wxALL, 5 );

    item0->Add( item1, 0, wxALIGN_CENTER_VERTICAL, 5 );

    wxCheckBox *item4 = new wxCheckBox(this, -1, wxT("Start at center of original"), wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&CenterAtOriginal));
    item4->SetValue( true );
    item0->Add( item4, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxCheckBox *item5 = new wxCheckBox(this, -1, wxT("Group copies"), wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&GroupCopies));
    item0->Add( item5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxBoxSizer *item6 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticBox *item8 = new wxStaticBox(this, -1, wxT("Translation (per copy)") );
    wxStaticBoxSizer *item7 = new wxStaticBoxSizer( item8, wxVERTICAL );

    wxBoxSizer *item9 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText *item10 = new wxStaticText(this, -1, wxT("x:"), wxDefaultPosition, wxDefaultSize, 0 );
    item9->Add( item10, 0, wxALIGN_CENTER|wxALL, 5 );

    SpinCtrlTranslateX=new wxSpinCtrl(this, -1, wxT("0"), wxDefaultPosition, wxSize(60,-1), wxSP_ARROW_KEYS, -9999, 9999, 0 );
    SpinCtrlTranslateX->SetValidator(wxGenericValidator(&TranslateX));
    item9->Add(SpinCtrlTranslateX, 1, wxALIGN_CENTER|wxALL, 5 );

    wxButton *item12 = new wxButton(this, ID_BUTTON_GET_WIDTH, wxT("W"), wxDefaultPosition, wxSize(20,-1), 0 );
    item9->Add( item12, 0, wxALIGN_CENTER|wxALL, 5 );

    item7->Add( item9, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    wxBoxSizer *item13 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText *item14 = new wxStaticText(this, -1, wxT("y:"), wxDefaultPosition, wxDefaultSize, 0 );
    item13->Add( item14, 0, wxALIGN_CENTER|wxALL, 5 );

    SpinCtrlTranslateY=new wxSpinCtrl(this, -1, wxT("0"), wxDefaultPosition, wxSize(60,-1), wxSP_ARROW_KEYS, -9999, 9999, 0 );
    SpinCtrlTranslateY->SetValidator(wxGenericValidator(&TranslateY));
    item13->Add(SpinCtrlTranslateY, 1, wxALIGN_CENTER|wxALL, 5 );

    wxButton *item16 = new wxButton(this, ID_BUTTON_GET_DEPTH, wxT("D"), wxDefaultPosition, wxSize(20,-1), 0 );
    item13->Add( item16, 0, wxALIGN_CENTER|wxALL, 5 );

    item7->Add( item13, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    wxBoxSizer *item17 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText *item18 = new wxStaticText(this, -1, wxT("z:"), wxDefaultPosition, wxDefaultSize, 0 );
    item17->Add( item18, 0, wxALIGN_CENTER|wxALL, 5 );

    SpinCtrlTranslateZ= new wxSpinCtrl(this, -1, wxT("0"), wxDefaultPosition, wxSize(60,-1), wxSP_ARROW_KEYS, -9999, 9999, 0 );
    SpinCtrlTranslateZ->SetValidator(wxGenericValidator(&TranslateZ));
    item17->Add(SpinCtrlTranslateZ, 1, wxALIGN_CENTER|wxALL, 5 );

    wxButton *item20 = new wxButton(this, ID_BUTTON_GET_HEIGHT, wxT("H"), wxDefaultPosition, wxSize(20,-1), 0 );
    item17->Add( item20, 0, wxALIGN_CENTER|wxALL, 5 );

    item7->Add( item17, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    item6->Add( item7, 1, wxGROW|wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );

    wxStaticBox *item22 = new wxStaticBox(this, -1, wxT("Rotation (per copy)") );
    wxStaticBoxSizer *item21 = new wxStaticBoxSizer( item22, wxVERTICAL );

    wxBoxSizer *item23 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText *item24 = new wxStaticText(this, -1, wxT("x:"), wxDefaultPosition, wxDefaultSize, 0 );
    item23->Add( item24, 0, wxALIGN_CENTER|wxALL, 5 );

    wxSpinCtrl *item25 = new wxSpinCtrl(this, -1, wxT("0"), wxDefaultPosition, wxSize(60,-1), wxSP_ARROW_KEYS, -360, 360, 0 );
    item25->SetValidator(wxGenericValidator(&RotateX));
    item23->Add( item25, 0, wxALIGN_CENTER|wxALL, 5 );

    item21->Add( item23, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    wxBoxSizer *item26 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText *item27 = new wxStaticText(this, -1, wxT("y:"), wxDefaultPosition, wxDefaultSize, 0 );
    item26->Add( item27, 0, wxALIGN_CENTER|wxALL, 5 );

    wxSpinCtrl *item28 = new wxSpinCtrl(this, -1, wxT("0"), wxDefaultPosition, wxSize(60,-1), wxSP_ARROW_KEYS, -360, 360, 0 );
    item28->SetValidator(wxGenericValidator(&RotateY));
    item26->Add( item28, 0, wxALIGN_CENTER|wxALL, 5 );

    item21->Add( item26, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    wxBoxSizer *item29 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText *item30 = new wxStaticText(this, -1, wxT("z:"), wxDefaultPosition, wxDefaultSize, 0 );
    item29->Add( item30, 0, wxALIGN_CENTER|wxALL, 5 );

    wxSpinCtrl *item31 = new wxSpinCtrl(this, -1, wxT("0"), wxDefaultPosition, wxSize(60,-1), wxSP_ARROW_KEYS, -360, 360, 0 );
    item31->SetValidator(wxGenericValidator(&RotateZ));
    item29->Add( item31, 0, wxALIGN_CENTER|wxALL, 5 );

    item21->Add( item29, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    item6->Add( item21, 0, wxGROW|wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );

    item0->Add( item6, 1, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

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


void PasteSpecialDialogT::OnButtonGetWidth(wxCommandEvent& Event)
{
    SpinCtrlTranslateX->SetValue(SpinCtrlTranslateX->GetValue()==ObjectsSizeX ? -ObjectsSizeX : ObjectsSizeX);
}


void PasteSpecialDialogT::OnButtonGetDepth(wxCommandEvent& Event)
{
    SpinCtrlTranslateY->SetValue(SpinCtrlTranslateY->GetValue()==ObjectsSizeY ? -ObjectsSizeY : ObjectsSizeY);
}


void PasteSpecialDialogT::OnButtonGetHeight(wxCommandEvent& Event)
{
    SpinCtrlTranslateZ->SetValue(SpinCtrlTranslateZ->GetValue()==ObjectsSizeZ ? -ObjectsSizeZ : ObjectsSizeZ);
}


void PasteSpecialDialogT::OnButtonOK(wxCommandEvent& Event)
{
    if (!Validate()) return;
    if (!TransferDataFromWindow()) return;

    wxConfigBase::Get()->Write("Paste Special Dialog/Number Of Copies"  , NrOfCopies      );
    wxConfigBase::Get()->Write("Paste Special Dialog/Center At Original", CenterAtOriginal);
    wxConfigBase::Get()->Write("Paste Special Dialog/Group Copies"      , GroupCopies     );
    wxConfigBase::Get()->Write("Paste Special Dialog/TranslateX"        , TranslateX      );
    wxConfigBase::Get()->Write("Paste Special Dialog/TranslateY"        , TranslateY      );
    wxConfigBase::Get()->Write("Paste Special Dialog/TranslateZ"        , TranslateZ      );
    wxConfigBase::Get()->Write("Paste Special Dialog/RotateX"           , RotateX         );
    wxConfigBase::Get()->Write("Paste Special Dialog/RotateY"           , RotateY         );
    wxConfigBase::Get()->Write("Paste Special Dialog/RotateZ"           , RotateZ         );

    EndModal(wxID_OK);
}
