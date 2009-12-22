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

#include "MaterialBrowserDialog.hpp"

#include "ScrolledMaterialWin.hpp"
#include "ControlsBar.hpp"
#include "MaterialTree.hpp"
#include "MaterialProperties.hpp"
#include "FilterSettings.hpp"

#include "../EditorMaterial.hpp"
#include "../GameConfig.hpp"
#include "../MapDocument.hpp"
#include "../DialogReplaceMaterials.hpp"
#include "../EditorMaterialManager.hpp"
#include "../MapCommands/ReplaceMat.hpp"
#include "../GuiEditor/GuiDocument.hpp"

#include "wx/confbase.h"


static ArrayT<wxString> NameFilterHistory;      // Updated on each dialog exit.


BEGIN_EVENT_TABLE(MaterialBrowserDialogT, wxDialog)
 // EVT_?????   (MaterialBrowserDialogT::ID_SCROLLED_MaterialWindow, MaterialBrowserDialogT::OnScrolled_MaterialWindow)
    EVT_CHOICE  (MaterialBrowserDialogT::ID_CHOICE_DisplaySize,      MaterialBrowserDialogT::OnChoice_DisplaySize)
    EVT_BUTTON  (MaterialBrowserDialogT::ID_BUTTON_Mark,             MaterialBrowserDialogT::OnButton_Mark)
    EVT_BUTTON  (MaterialBrowserDialogT::ID_BUTTON_Replace,          MaterialBrowserDialogT::OnButton_Replace)
    EVT_BUTTON  (MaterialBrowserDialogT::ID_BUTTON_ExportDiffMaps,   MaterialBrowserDialogT::OnButton_ExportDiffMaps)
    EVT_BUTTON  (wxID_CANCEL,                                        MaterialBrowserDialogT::OnButton_Cancel)
    EVT_COMBOBOX(MaterialBrowserDialogT::ID_COMBO_NameFilter,        MaterialBrowserDialogT::OnCombobox_NameFilterSelection)
    EVT_TEXT    (MaterialBrowserDialogT::ID_COMBO_NameFilter,        MaterialBrowserDialogT::OnCombobox_NameFilterTextChange)
 // EVT_?????   (MaterialBrowserDialogT::ID_COMBO_KeywordFilter,     MaterialBrowserDialogT::OnXXX_KeywordFilter)
    EVT_CHECKBOX(MaterialBrowserDialogT::ID_CHECKBOX_OnlyShowUsed,   MaterialBrowserDialogT::OnCheckbox_OnlyShowUsed)
    EVT_CHECKBOX(MaterialBrowserDialogT::ID_CHECKBOX_OnlyShowEditor, MaterialBrowserDialogT::OnCheckbox_OnlyShowEditor)
END_EVENT_TABLE()


void MaterialBrowserDialogT::Init(const ArrayT<EditorMaterialI*>& Materials, const wxString& InitialNameFilter, bool OnlyShowUsed)
{
    // Set this dialog to be managed by wxAUI.
    m_AUIManager.SetManagedWindow(this);

    // This sets the cursor to the busy cursor in its ctor, and back to the default cursor in the dtor.
    // Needed here because the material manager might not yet have finished caching in all materials in idle time,
    // so opening the material browser might cause a delay that would seem strange without the busy cursor.
    wxBusyCursor BusyCursor;

    m_ScrolledMatWin    =new ScrolledMaterialWindowT(this, wxID_ANY, Materials);
    m_ControlsBar       =new ControlsBarT(this, true);
    m_MaterialTree      =new MaterialTreeT(this, Materials);
    m_MaterialProperties=new MaterialPropertiesT(this);
    m_FilterSettings    =new FilterSettingsT(this);

    // Set the dialog position and size according to the settings in the config file.
    if (wxConfigBase::Get()->Read("Material Browser Dialog/IsMaximized", true))
    {
        Maximize(true);
    }
    else
    {
        const int PosX  =wxConfigBase::Get()->Read("Material Browser Dialog/PosX"  , -1);
        const int PosY  =wxConfigBase::Get()->Read("Material Browser Dialog/PosY"  , -1);
        const int Width =wxConfigBase::Get()->Read("Material Browser Dialog/Width" , -1);
        const int Height=wxConfigBase::Get()->Read("Material Browser Dialog/Height", -1);

        SetSize(PosX, PosY, Width, Height);
    }

    // If no explicit initial name filter was specified but we have some history record,
    // set the first entry of the history, otherwise set the (possibly empty) initial name filter.
    m_FilterSettings->m_NameFilterCombobox->SetValue(InitialNameFilter=="" && NameFilterHistory.Size()>0 ? NameFilterHistory[0] : InitialNameFilter);

    // Setup the "Display Size" choice.
    const int SelectionIndex=wxConfigBase::Get()->Read("Material Browser Dialog/DisplaySizeSelection", 0l);
    m_ControlsBar->m_DisplaySizeChoice->SetSelection(SelectionIndex>=0 ? SelectionIndex : 0);
    wxCommandEvent CE1; OnChoice_DisplaySize(CE1);

    // Setup the "Only show used" checkbox.
    m_FilterSettings->m_OnlyShowUsedCheckbox->SetValue(OnlyShowUsed);
    wxCommandEvent CE2; OnCheckbox_OnlyShowUsed(CE2);

    // Note that the sizes set here are merely hints for wxAUI on how to size the panes.
    // In fact the three panes to the left will maintain their width (200), but their
    // height is evenly distributed along the height of the parent dialog.
    // In consequence the filter settings pane has the same size as the tree or property
    // pane, even if defined smaller here.
    // There seems to be no way to programatically change the real size of a docked pane
    // and the only work around would be to call LoadPerspective with a hard coded string
    // that changes the size according to our preferences.
    m_AUIManager.AddPane(m_ScrolledMatWin, wxAuiPaneInfo().
                         Name("MaterialWindow").Caption("Material Window").
                         CenterPane().CloseButton(false).BestSize(1024, 1024));
    m_AUIManager.AddPane(m_ControlsBar, wxAuiPaneInfo().
                         Name("MaterialToolbar").Caption("Material Toolbar").
                         Bottom().Layer(0).CloseButton(false).BestSize(600, 70));
    m_AUIManager.AddPane(m_MaterialTree, wxAuiPaneInfo().
                         Name("MaterialTree").Caption("Material Tree").
                         Left().Layer(1).Position(0).CloseButton(false).BestSize(200, 600));
    m_AUIManager.AddPane(m_FilterSettings, wxAuiPaneInfo().
                         Name("FilterSettings").Caption("Filter Settings").
                         Left().Layer(1).Position(1).CloseButton(false).BestSize(200, 80));
    m_AUIManager.AddPane(m_MaterialProperties, wxAuiPaneInfo().
                         Name("MaterialProperties").Caption("Material Properties").
                         Left().Layer(1).Position(2).CloseButton(false).BestSize(200, 600));

    m_AUIManager.Update();

    SelectMaterial(m_CurrentMaterial);
}


MaterialBrowserDialogT::MaterialBrowserDialogT(wxWindow* Parent, MapDocumentT* MapDoc, EditorMaterialI* InitialMaterial, const wxString& InitialNameFilter_, bool OnlyShowUsed_)
    : wxDialog(Parent, -1, wxString("CaWE Material Browser"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX),
      m_MapDoc(MapDoc),
      m_GuiDoc(NULL),
      m_ScrolledMatWin(NULL),
      m_ControlsBar(NULL),
      m_MaterialTree(NULL),
      m_MaterialProperties(NULL),
      m_FilterSettings(NULL),
      DisplaySize(0),
      m_CurrentMaterial(InitialMaterial),
      m_UsedMaterialsList(NULL)
{
    Init(m_MapDoc->GetGameConfig()->GetMatMan().GetMaterials(), InitialNameFilter_, OnlyShowUsed_);
}


MaterialBrowserDialogT::MaterialBrowserDialogT(wxWindow* Parent, GuiEditor::GuiDocumentT* GuiDoc, EditorMaterialI* InitialMaterial, const wxString& InitialNameFilter_, bool OnlyShowUsed_)
    : wxDialog(Parent, -1, wxString("CaWE Material Browser"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX),
      m_MapDoc(NULL),
      m_GuiDoc(GuiDoc),
      m_ScrolledMatWin(NULL),
      m_ControlsBar(NULL),
      m_MaterialTree(NULL),
      m_MaterialProperties(NULL),
      m_FilterSettings(NULL),
      DisplaySize(0),
      m_CurrentMaterial(InitialMaterial),
      m_UsedMaterialsList(NULL)
{
    Init(m_GuiDoc->GetGameConfig()->GetMatMan().GetMaterials(), InitialNameFilter_, OnlyShowUsed_);
}


MaterialBrowserDialogT::~MaterialBrowserDialogT()
{
    m_AUIManager.UnInit();
}


EditorMaterialI* MaterialBrowserDialogT::GetCurrentMaterial()
{
    return m_CurrentMaterial;
}


const ArrayT<wxString>& MaterialBrowserDialogT::GetNameFilterHistory()
{
    return NameFilterHistory;
}


void MaterialBrowserDialogT::SelectMaterial(EditorMaterialI* Material)
{
    m_CurrentMaterial=Material;

    m_MaterialTree      ->SelectMaterial(Material);
    m_ScrolledMatWin    ->SelectMaterial(Material);
    m_MaterialProperties->ShowMaterial(Material);
}


void MaterialBrowserDialogT::SaveAndQuitDialog(int ReturnValue)
{
    // Update the NameFilterHistory with the current string in the NameFilterCombobox.
    // If the current name filter is already in the history, delete it.
    for (unsigned long FilterNr=0; FilterNr<NameFilterHistory.Size(); FilterNr++)
        if (wxStricmp(NameFilterHistory[FilterNr], m_FilterSettings->m_NameFilterCombobox->GetValue())==0)
        {
            NameFilterHistory.RemoveAtAndKeepOrder(FilterNr);
            break;
        }

    // Now insert the current name filter at NameFilterHistory[0].
    NameFilterHistory.InsertAt(0, m_FilterSettings->m_NameFilterCombobox->GetValue());


    // Save the dialogs position and size in the config file.
    const wxRect DialogRect=GetRect();

    wxConfigBase::Get()->Write("Material Browser Dialog/IsMaximized"         , IsMaximized());
    wxConfigBase::Get()->Write("Material Browser Dialog/PosX"                , DialogRect.x);
    wxConfigBase::Get()->Write("Material Browser Dialog/PosY"                , DialogRect.y);
    wxConfigBase::Get()->Write("Material Browser Dialog/Width"               , DialogRect.width);
    wxConfigBase::Get()->Write("Material Browser Dialog/Height"              , DialogRect.height);
    wxConfigBase::Get()->Write("Material Browser Dialog/DisplaySizeSelection", m_ControlsBar->m_DisplaySizeChoice->GetSelection());

    EndModal(ReturnValue);
}


/*******************************/
/*** Event Handler Functions ***/
/*******************************/

void MaterialBrowserDialogT::OnChoice_DisplaySize(wxCommandEvent& Event)
{
    switch (m_ControlsBar->m_DisplaySizeChoice->GetSelection())
    {
        // This must correlate with the content strings of the choicebox.
        case 0: DisplaySize=  0; break;     // 1:1
        case 1: DisplaySize=128; break;
        case 2: DisplaySize=256; break;
        case 3: DisplaySize=512; break;
    }

    if (DisplaySize>0) wxMessageBox("Please note that all display sizes\n           except 1:1\n"
                                    "currently do not properly work.\nWe will try now the size that you selected anyway,\n"
                                    "but you'll probably want to switch back to 1:1.", "Information");

    m_ScrolledMatWin->UpdateVirtualSize();
    m_ScrolledMatWin->SelectMaterial(m_CurrentMaterial);
}


void MaterialBrowserDialogT::OnButton_Mark(wxCommandEvent& Event)
{
    if (m_MapDoc==NULL) return;
    if (m_CurrentMaterial==NULL) { wxMessageBox("Please select a material first!", "Currently no material is selected."); return; }

    CommandReplaceMatT* Command=new CommandReplaceMatT(
        *m_MapDoc,
        m_MapDoc->GetSelection(),
        m_CurrentMaterial->GetName().c_str(),
        "",
        CommandReplaceMatT::ExactMatches,
        true,   // Only mark found faces/brushes/patches.
        false,  // Search whole world, not only in selection.
        true,   // Include brushes in the search.
        true,   // Include bezier patches in the search.
        false); // Do not include hidden objects (makes no sense for marking anyway).

    m_MapDoc->GetHistory().SubmitCommand(Command);
    wxMessageBox(Command->GetResultString());
    SaveAndQuitDialog(wxID_OK);
}


void MaterialBrowserDialogT::OnButton_Replace(wxCommandEvent& Event)
{
    if (m_MapDoc==NULL) return;
    if (m_CurrentMaterial==NULL) { wxMessageBox("Please select a material first!", "Currently no material is selected."); return; }

    ReplaceMaterialsDialogT ReplaceMatsDlg(m_MapDoc->GetSelection().Size()>0, m_MapDoc, m_CurrentMaterial->GetName());  //todo: Pass in m_MapDoc by reference rather than by pointer?
    ReplaceMatsDlg.ShowModal();

    if (m_FilterSettings->m_OnlyShowUsedCheckbox->IsChecked()) { wxCommandEvent CE; OnCheckbox_OnlyShowUsed(CE); }
}


void MaterialBrowserDialogT::OnButton_ExportDiffMaps(wxCommandEvent& Event)
{
    m_ScrolledMatWin->ExportDiffuseMaps(wxDirSelector("Please select the destination directory for the export.", "."));
}


void MaterialBrowserDialogT::OnButton_Cancel(wxCommandEvent& Event)
{
    SaveAndQuitDialog(wxID_CANCEL);
}


void MaterialBrowserDialogT::OnCombobox_NameFilterSelection(wxCommandEvent& Event)
{
    m_ScrolledMatWin->UpdateVirtualSize();
    m_ScrolledMatWin->SelectMaterial(m_CurrentMaterial);
    m_ScrolledMatWin->Refresh();      // If m_CurrentMaterial==NULL, m_ScrolledMatWin->SelectMaterial() doesn't refresh, so make sure it is done here.
}


void MaterialBrowserDialogT::OnCombobox_NameFilterTextChange(wxCommandEvent& Event)
{
    m_ScrolledMatWin->UpdateVirtualSize();
    m_ScrolledMatWin->SelectMaterial(m_CurrentMaterial);
    m_ScrolledMatWin->Refresh();      // If m_CurrentMaterial==NULL, m_ScrolledMatWin->SelectMaterial() doesn't refresh, so make sure it is done here.
}


void MaterialBrowserDialogT::OnCheckbox_OnlyShowUsed(wxCommandEvent& Event)
{
    if (m_MapDoc==NULL && m_GuiDoc==NULL) return;

    if (m_FilterSettings->m_OnlyShowUsedCheckbox->IsChecked())
    {
        static ArrayT<EditorMaterialI*> UsedMatList;

        if (m_MapDoc) m_MapDoc->GetUsedMaterials(UsedMatList);
                 else m_GuiDoc->GetUsedMaterials(UsedMatList);

        m_UsedMaterialsList=&UsedMatList;
    }
    else
    {
        m_UsedMaterialsList=NULL;
    }

    m_ScrolledMatWin->UpdateVirtualSize();
    m_ScrolledMatWin->SelectMaterial(m_CurrentMaterial);
    m_ScrolledMatWin->Refresh();      // If m_CurrentMaterial==NULL, m_ScrolledMatWin->SelectMaterial() doesn't refresh, so make sure it is done here.
}


void MaterialBrowserDialogT::OnCheckbox_OnlyShowEditor(wxCommandEvent& Event)
{
    if (m_MapDoc==NULL && m_GuiDoc==NULL) return;

    m_ScrolledMatWin->UpdateVirtualSize();
    m_ScrolledMatWin->SelectMaterial(m_CurrentMaterial);
    m_ScrolledMatWin->Refresh();      // If m_CurrentMaterial==NULL, m_ScrolledMatWin->SelectMaterial() doesn't refresh, so make sure it is done here.
}
