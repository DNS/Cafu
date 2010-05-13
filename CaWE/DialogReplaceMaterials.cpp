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

#include "DialogReplaceMaterials.hpp"
#include "AppCaWE.hpp"
#include "GameConfig.hpp"
#include "ParentFrame.hpp"
#include "ChildFrame.hpp"
#include "MapDocument.hpp"
#include "CommandHistory.hpp"
#include "EditorMaterial.hpp"
#include "EditorMaterialManager.hpp"

#include "MaterialBrowser/DocAccess.hpp"
#include "MaterialBrowser/MaterialBrowserDialog.hpp"
#include "MapCommands/ReplaceMat.hpp"
#include "MapCommands/Select.hpp"

#include "wx/image.h"


BEGIN_EVENT_TABLE(BitmapControlT, wxWindow)
    EVT_PAINT(BitmapControlT::OnPaint)
END_EVENT_TABLE()


BitmapControlT::BitmapControlT(wxWindow* Parent, const wxPoint& Pos, const wxSize& Size)
    : wxWindow(Parent, -1, Pos, wxDefaultSize, wxSUNKEN_BORDER),
      m_Bitmap()
{
    SetClientSize(Size);
}


void BitmapControlT::OnPaint(wxPaintEvent& PE)
{
    wxPaintDC dc(this);

    if (m_Bitmap.GetWidth()>0 && m_Bitmap.GetHeight()>0)
        dc.DrawBitmap(m_Bitmap, 0, 0, false);
}


static const int PREVIEW_BITMAP_SIZE=128;


BEGIN_EVENT_TABLE(ReplaceMaterialsDialogT, wxDialog)
    EVT_BUTTON(wxID_OK, ReplaceMaterialsDialogT::OnOK)
    EVT_BUTTON(ReplaceMaterialsDialogT::ID_BUTTON_BROWSE_FIND, ReplaceMaterialsDialogT::OnButtonBrowseFind)
    EVT_BUTTON(ReplaceMaterialsDialogT::ID_BUTTON_BROWSE_REPLACE, ReplaceMaterialsDialogT::OnButtonBrowseReplace)
    EVT_CHECKBOX(ReplaceMaterialsDialogT::ID_CHECKBOX_FINDONLY, ReplaceMaterialsDialogT::OnCheckboxFindOnly)
    EVT_RADIOBUTTON(ReplaceMaterialsDialogT::ID_RADIOBUTTON_SEARCH_IN_SELECTION, ReplaceMaterialsDialogT::OnRadioButtonSearchIn)
    EVT_RADIOBUTTON(ReplaceMaterialsDialogT::ID_RADIOBUTTON_SEARCH_IN_WHOLEWORLD, ReplaceMaterialsDialogT::OnRadioButtonSearchIn)
    EVT_TEXT(ReplaceMaterialsDialogT::ID_TEXTCTRL_FINDMATNAME, ReplaceMaterialsDialogT::OnTextUpdateFindMatName)
    EVT_TEXT(ReplaceMaterialsDialogT::ID_TEXTCTRL_REPLACEMATNAME, ReplaceMaterialsDialogT::OnTextUpdateReplaceMatName)
END_EVENT_TABLE()


ReplaceMaterialsDialogT::ReplaceMaterialsDialogT(bool IsSomethingSelected, MapDocumentT& MapDoc, const wxString& InitialFindMatName)
    : wxDialog(NULL, -1, wxString("Replace Materials"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_MapDoc(MapDoc),
      TextCtrlFindMatName(NULL),
      TextCtrlReplaceMatName(NULL),
      RadioButtonSearchInSelection(NULL),
      RadioButtonSearchInWholeWorld(NULL),
      CheckBoxInclusiveBrushes(NULL),
      CheckBoxInclusiveBPs(NULL),
      CheckBoxInclusiveHidden(NULL),
      RadioBoxSearchFor(NULL),
      RadioBoxReplaceRescaleMode(NULL),
      CheckBoxFindOnly(NULL),
      BitmapFindMat(NULL),
      BitmapReplaceMat(NULL),
      StaticBoxReplace(NULL),
      ButtonBrowseReplace(NULL)
{
    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );

    wxStaticBox *item2 = new wxStaticBox(this, -1, wxT("Find") );
    wxStaticBoxSizer *item1 = new wxStaticBoxSizer( item2, wxVERTICAL );

    wxBoxSizer *item3 = new wxBoxSizer( wxHORIZONTAL );

    // Warning: Setting a non-empty default text here does cause a text update event,
    // and thus an early call to the event handler that in turn will cause a crash, as we are still in mid-constructor!
    TextCtrlFindMatName=new wxTextCtrl(this, ID_TEXTCTRL_FINDMATNAME, wxT(""), wxDefaultPosition, wxSize(80,-1), 0 );
    item3->Add(TextCtrlFindMatName, 1, wxALIGN_CENTER|wxALL, 5 );

    wxButton *item5 = new wxButton(this, ID_BUTTON_BROWSE_FIND, wxT("Browse"), wxDefaultPosition, wxSize(50,-1), 0 );
    item3->Add( item5, 0, wxALIGN_CENTER|wxALL, 5 );

    item1->Add( item3, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    wxBoxSizer *item6 = new wxBoxSizer( wxHORIZONTAL );

    BitmapFindMat=new BitmapControlT(this, wxDefaultPosition, wxSize(PREVIEW_BITMAP_SIZE, PREVIEW_BITMAP_SIZE));
    item6->Add(BitmapFindMat, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );

    wxBoxSizer *item8 = new wxBoxSizer( wxVERTICAL );

    wxStaticBox *item10 = new wxStaticBox(this, -1, wxT("Search in") );
    wxStaticBoxSizer *item9 = new wxStaticBoxSizer( item10, wxVERTICAL );

    RadioButtonSearchInSelection=new wxRadioButton(this, ID_RADIOBUTTON_SEARCH_IN_SELECTION, wxT("selected objects"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    RadioButtonSearchInSelection->SetValue(IsSomethingSelected);
    item9->Add(RadioButtonSearchInSelection, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP, 5 );

    RadioButtonSearchInWholeWorld=new wxRadioButton(this, ID_RADIOBUTTON_SEARCH_IN_WHOLEWORLD, wxT("all objects (whole world)"), wxDefaultPosition, wxDefaultSize, 0 );
    RadioButtonSearchInWholeWorld->SetValue(!IsSomethingSelected);
    item9->Add(RadioButtonSearchInWholeWorld, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP, 5 );

    wxBoxSizer *item13 = new wxBoxSizer( wxVERTICAL );

    CheckBoxInclusiveBrushes=new wxCheckBox(this, -1, wxT("inclusive brushes"), wxDefaultPosition, wxDefaultSize, 0 );
    CheckBoxInclusiveBrushes->SetValue(true);
    CheckBoxInclusiveBrushes->Enable(RadioButtonSearchInWholeWorld->GetValue());
    item13->Add(CheckBoxInclusiveBrushes, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );

    CheckBoxInclusiveBPs=new wxCheckBox(this, -1, wxT("inclusive bezier patches"), wxDefaultPosition, wxDefaultSize, 0 );
    CheckBoxInclusiveBPs->SetValue(true);
    CheckBoxInclusiveBPs->Enable(RadioButtonSearchInWholeWorld->GetValue());
    item13->Add(CheckBoxInclusiveBPs, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );

    CheckBoxInclusiveHidden=new wxCheckBox(this, -1, wxT("also in hidden groups"), wxDefaultPosition, wxDefaultSize, 0 );
    CheckBoxInclusiveHidden->SetValue(false);
    CheckBoxInclusiveHidden->Enable(RadioButtonSearchInWholeWorld->GetValue());
    item13->Add(CheckBoxInclusiveHidden, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    item9->Add( item13, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 25 );

    item8->Add( item9, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxString strs17[] =
    {
        wxT("exact matches"),
        wxT("partial matches")
    };
    RadioBoxSearchFor=new wxRadioBox(this, -1, wxT("Search for"), wxDefaultPosition, wxDefaultSize, 2, strs17, 1, wxRA_SPECIFY_COLS );
    item8->Add(RadioBoxSearchFor, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    item6->Add( item8, 1, wxALIGN_CENTER, 5 );

    item1->Add( item6, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    item0->Add( item1, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    CheckBoxFindOnly= new wxCheckBox(this, ID_CHECKBOX_FINDONLY, wxT("Find only (mark found faces, don't replace)"), wxDefaultPosition, wxDefaultSize, 0 );
    item0->Add(CheckBoxFindOnly, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    StaticBoxReplace= new wxStaticBox(this, -1, wxT("Replace") );
    wxStaticBoxSizer *item19 = new wxStaticBoxSizer(StaticBoxReplace, wxVERTICAL );

    wxBoxSizer *item21 = new wxBoxSizer( wxHORIZONTAL );

    // Warning: Setting a non-empty default text here does cause a text update event,
    // and thus an early call to the event handler that in turn will cause a crash, as we are still in mid-constructor!
    TextCtrlReplaceMatName= new wxTextCtrl(this, ID_TEXTCTRL_REPLACEMATNAME, wxT(""), wxDefaultPosition, wxSize(80,-1), 0 );
    item21->Add(TextCtrlReplaceMatName, 1, wxALIGN_CENTER|wxALL, 5 );

    ButtonBrowseReplace= new wxButton(this, ID_BUTTON_BROWSE_REPLACE, wxT("Browse"), wxDefaultPosition, wxSize(50,-1), 0 );
    item21->Add(ButtonBrowseReplace, 0, wxALIGN_CENTER|wxALL, 5 );

    item19->Add( item21, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    wxBoxSizer *item24 = new wxBoxSizer( wxHORIZONTAL );

    BitmapReplaceMat=new BitmapControlT(this, wxDefaultPosition, wxSize(PREVIEW_BITMAP_SIZE, PREVIEW_BITMAP_SIZE));
    item24->Add(BitmapReplaceMat, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );

    wxString strs26[] =
    {
        wxT("do rescale (texture is repeated/tiled as often as before)"),
        wxT("do not rescale (pixels don't change their size)")
    };
    RadioBoxReplaceRescaleMode= new wxRadioBox(this, -1, wxT("Replace and"), wxDefaultPosition, wxDefaultSize, 2, strs26, 1, wxRA_SPECIFY_COLS );
    RadioBoxReplaceRescaleMode->Disable();
    item24->Add(RadioBoxReplaceRescaleMode, 1, wxALIGN_TOP|wxALL, 5 );

    item19->Add( item24, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    item0->Add( item19, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxStaticText *item27 = new wxStaticText(this, -1,
        wxT("Hint: You may also replace materials by loading the cmap\n")
        wxT("file into a text editor and use the search and replace function there.\n")
        wxT("Only recommended for experts, and only after a backup!"),
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
    item0->Add( item27, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxBoxSizer *item28 = new wxBoxSizer( wxHORIZONTAL );

    wxButton *item29 = new wxButton(this, wxID_OK, wxT("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    item29->SetDefault();
    item28->Add( item29, 0, wxALIGN_CENTER|wxALL, 5 );

    wxButton *item30 = new wxButton(this, wxID_CANCEL, wxT("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    item28->Add( item30, 0, wxALIGN_CENTER|wxALL, 5 );

    item0->Add( item28, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    this->SetSizer( item0 );
    item0->SetSizeHints( this );


    // This does also trigger a text update event (and thus a function call to OnTextUpdateFindMatName()).
    TextCtrlFindMatName->SetValue(InitialFindMatName);
}


void ReplaceMaterialsDialogT::OnOK(wxCommandEvent& Event)
{
    // Clear selection before marking new stuff.
    if (CheckBoxFindOnly->IsChecked()) m_MapDoc.GetHistory().SubmitCommand(CommandSelectT::Clear(&m_MapDoc));

    // CF: The big TODO question with this dialog is:
    // If I select something, and then hide it in a group, is it still selected?
    CommandReplaceMatT* Command=new CommandReplaceMatT(
        m_MapDoc,
        m_MapDoc.GetSelection(),
        TextCtrlFindMatName->GetValue(),
        TextCtrlReplaceMatName->GetValue(),
        CommandReplaceMatT::ReplaceActionT(RadioBoxSearchFor->GetSelection()),
        CheckBoxFindOnly->IsChecked(),
        RadioButtonSearchInSelection->GetValue(), // Search in selection? Or everywhere (whole world)?
        CheckBoxInclusiveBrushes->IsChecked(),    // Include brushes?
        CheckBoxInclusiveBPs->IsChecked(),        // Include bezier patches?
        CheckBoxInclusiveHidden->IsChecked());    // Search for hidden if "all objects (inclusive hidden objects)" is selected.

    m_MapDoc.GetHistory().SubmitCommand(Command);
    wxMessageBox(Command->GetResultString());

    // Tell wxWidgets to continue processing the default behavior for wxID_OK
    // (which is to call all wxValidators, handle any data transfer with those validators, and close the dialog).
    Event.Skip();
}


void ReplaceMaterialsDialogT::OnButtonBrowseFind(wxCommandEvent& Event)
{
    MaterialBrowserDialogT MatBrowser(this, MaterialBrowser::MapDocAccessT(m_MapDoc),
                                      m_MapDoc.GetGameConfig()->GetMatMan().FindMaterial(TextCtrlFindMatName->GetValue(), false), "", true);

    if (MatBrowser.ShowModal()==wxID_OK)
        TextCtrlFindMatName->SetValue(MatBrowser.GetCurrentMaterial()!=NULL ? MatBrowser.GetCurrentMaterial()->GetName() : "");
}


void ReplaceMaterialsDialogT::OnButtonBrowseReplace(wxCommandEvent& Event)
{
    MaterialBrowserDialogT MatBrowser(this, MaterialBrowser::MapDocAccessT(m_MapDoc),
                                      m_MapDoc.GetGameConfig()->GetMatMan().FindMaterial(TextCtrlReplaceMatName->GetValue(), false), "", false);

    if (MatBrowser.ShowModal()==wxID_OK)
        TextCtrlReplaceMatName->SetValue(MatBrowser.GetCurrentMaterial()!=NULL ? MatBrowser.GetCurrentMaterial()->GetName() : "");
}


void ReplaceMaterialsDialogT::OnCheckboxFindOnly(wxCommandEvent& Event)
{
    StaticBoxReplace      ->Enable(!Event.IsChecked());
    TextCtrlReplaceMatName->Enable(!Event.IsChecked());
    ButtonBrowseReplace   ->Enable(!Event.IsChecked());
 // StaticBoxReplace      ->Enable(!Event.IsChecked());
}


static wxBitmap GetScaledBitmapFromMaterial(EditorMaterialI* MaterialPtr)
{
    if (MaterialPtr==NULL) return wxNullBitmap;

    const int w  =MaterialPtr->GetWidth ();
    const int h  =MaterialPtr->GetHeight();
    const int Max=w>h ? w : h;

    if (w<=PREVIEW_BITMAP_SIZE && h<=PREVIEW_BITMAP_SIZE) return wxBitmap(MaterialPtr->GetImage());

    return wxBitmap(MaterialPtr->GetImage().Scale(w*PREVIEW_BITMAP_SIZE/Max, h*PREVIEW_BITMAP_SIZE/Max));
}


void ReplaceMaterialsDialogT::OnRadioButtonSearchIn(wxCommandEvent& Event)
{
    CheckBoxInclusiveBrushes->Enable(Event.GetId()==ID_RADIOBUTTON_SEARCH_IN_WHOLEWORLD);
    CheckBoxInclusiveBPs    ->Enable(Event.GetId()==ID_RADIOBUTTON_SEARCH_IN_WHOLEWORLD);
    CheckBoxInclusiveHidden ->Enable(Event.GetId()==ID_RADIOBUTTON_SEARCH_IN_WHOLEWORLD);
}


void ReplaceMaterialsDialogT::OnTextUpdateFindMatName(wxCommandEvent& Event)
{
    BitmapFindMat->m_Bitmap=GetScaledBitmapFromMaterial(m_MapDoc.GetGameConfig()->GetMatMan().FindMaterial(TextCtrlFindMatName->GetValue(), false));
    BitmapFindMat->Refresh();
}


void ReplaceMaterialsDialogT::OnTextUpdateReplaceMatName(wxCommandEvent& Event)
{
    BitmapReplaceMat->m_Bitmap=GetScaledBitmapFromMaterial(m_MapDoc.GetGameConfig()->GetMatMan().FindMaterial(TextCtrlReplaceMatName->GetValue(), false));
    BitmapReplaceMat->Refresh();
}
