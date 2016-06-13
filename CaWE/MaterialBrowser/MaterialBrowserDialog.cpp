/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "MaterialBrowserDialog.hpp"

#include "ScrolledMaterialWin.hpp"
#include "ControlsBar.hpp"
#include "MaterialTree.hpp"
#include "MaterialProperties.hpp"
#include "FilterSettings.hpp"

#include "../DocumentAdapter.hpp"
#include "../EditorMaterial.hpp"

#include "wx/confbase.h"


using namespace MaterialBrowser;


ConfigT::ConfigT()
    : m_InitialMaterial(NULL),
      m_InitialNameFilter(""),
      m_OnlyShowUsed(false),
      m_NoFilterEditorMatsOnly(false),
      m_NoButtonMark(false),
      m_NoButtonReplace(false)
{
}


static ArrayT<wxString> NameFilterHistory;      // Updated on each dialog exit.


BEGIN_EVENT_TABLE(DialogT, wxDialog)
 // EVT_?????   (DialogT::ID_SCROLLED_MaterialWindow, DialogT::OnScrolled_MaterialWindow)
    EVT_CHOICE  (DialogT::ID_CHOICE_DisplaySize,      DialogT::OnChoice_DisplaySize)
    EVT_BUTTON  (DialogT::ID_BUTTON_Mark,             DialogT::OnButton_Mark)
    EVT_BUTTON  (DialogT::ID_BUTTON_Replace,          DialogT::OnButton_Replace)
    EVT_BUTTON  (DialogT::ID_BUTTON_ExportDiffMaps,   DialogT::OnButton_ExportDiffMaps)
    EVT_BUTTON  (wxID_CANCEL,                         DialogT::OnButton_Cancel)
    EVT_COMBOBOX(DialogT::ID_COMBO_NameFilter,        DialogT::OnCombobox_NameFilterSelection)
    EVT_TEXT    (DialogT::ID_COMBO_NameFilter,        DialogT::OnCombobox_NameFilterTextChange)
 // EVT_?????   (DialogT::ID_COMBO_KeywordFilter,     DialogT::OnXXX_KeywordFilter)
    EVT_CHECKBOX(DialogT::ID_CHECKBOX_OnlyShowUsed,   DialogT::OnCheckbox_OnlyShowUsed)
    EVT_CHECKBOX(DialogT::ID_CHECKBOX_OnlyShowEditor, DialogT::OnCheckbox_OnlyShowEditor)
END_EVENT_TABLE()


void DialogT::Init(const ArrayT<EditorMaterialI*>& Materials)
{
    // Set this dialog to be managed by wxAUI.
    m_AUIManager.SetManagedWindow(this);

    // This sets the cursor to the busy cursor in its ctor, and back to the default cursor in the dtor.
    // Needed here because the material manager might not yet have finished caching in all materials in idle time,
    // so opening the material browser might cause a delay that would seem strange without the busy cursor.
    wxBusyCursor BusyCursor;

    m_ScrolledMatWin    =new ScrolledMaterialWindowT(this, wxID_ANY, Materials);
    m_ControlsBar       =new ControlsBarT(this);
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
    m_FilterSettings->SetNameFilterValue(m_Config.m_InitialNameFilter=="" && NameFilterHistory.Size()>0 ? NameFilterHistory[0] : m_Config.m_InitialNameFilter);

    // Setup the "Display Size" choice.
    const int SelectionIndex=wxConfigBase::Get()->Read("Material Browser Dialog/DisplaySizeSelection", 0l);
    m_ControlsBar->m_DisplaySizeChoice->SetSelection(SelectionIndex>=0 ? SelectionIndex : 0);
    wxCommandEvent CE1; OnChoice_DisplaySize(CE1);

    // Setup (initiaize for) the "Only show used" checkbox.
    // ( Now in filter ctor: m_FilterSettings->m_OnlyShowUsedCheckbox->SetValue(m_Config.m_OnlyShowUsed); )
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


DialogT::DialogT(wxWindow* Parent, const DocAdapterI& DocAccess, const ConfigT& Config)
    : wxDialog(Parent, -1, wxString("CaWE Material Browser"), wxDefaultPosition, wxSize(800, 600), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX),
      m_DocAccess(DocAccess),       // Must use a fixed size in place of wxDefaultSize, see <http://trac.wxwidgets.org/ticket/12490> for details.
      m_Config(Config),
      m_ScrolledMatWin(NULL),
      m_ControlsBar(NULL),
      m_MaterialTree(NULL),
      m_MaterialProperties(NULL),
      m_FilterSettings(NULL),
      DisplaySize(0),
      m_CurrentMaterial(Config.m_InitialMaterial),
      m_UsedMaterialsList(NULL)
{
    Init(m_DocAccess.GetMaterials());
}


DialogT::~DialogT()
{
    m_AUIManager.UnInit();
}


EditorMaterialI* DialogT::GetCurrentMaterial() const
{
    return m_CurrentMaterial;
}


const ArrayT<wxString>& DialogT::GetNameFilterHistory()
{
    return NameFilterHistory;
}


void DialogT::SelectMaterial(EditorMaterialI* Material)
{
    m_CurrentMaterial=Material;

    m_MaterialTree      ->SelectMaterial(Material);
    m_ScrolledMatWin    ->SelectMaterial(Material);
    m_MaterialProperties->ShowMaterial(Material);
}


void DialogT::SaveAndQuitDialog(int ReturnValue)
{
    // Update the NameFilterHistory with the current string in the NameFilterCombobox.
    // If the current name filter is already in the history, delete it.
    for (unsigned long FilterNr=0; FilterNr<NameFilterHistory.Size(); FilterNr++)
        if (wxStricmp(NameFilterHistory[FilterNr], m_FilterSettings->GetNameFilterValue())==0)
        {
            NameFilterHistory.RemoveAtAndKeepOrder(FilterNr);
            break;
        }

    // Now insert the current name filter at NameFilterHistory[0].
    NameFilterHistory.InsertAt(0, m_FilterSettings->GetNameFilterValue());


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

void DialogT::OnChoice_DisplaySize(wxCommandEvent& Event)
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


void DialogT::OnButton_Mark(wxCommandEvent& Event)
{
    if (m_CurrentMaterial==NULL) { wxMessageBox("Please select a material first!", "Currently no material is selected."); return; }

    m_DocAccess.OnMarkMaterial(m_CurrentMaterial);
    SaveAndQuitDialog(wxID_OK);
}


void DialogT::OnButton_Replace(wxCommandEvent& Event)
{
    if (m_CurrentMaterial==NULL) { wxMessageBox("Please select a material first!", "Currently no material is selected."); return; }

    m_DocAccess.OnReplaceMaterial(m_CurrentMaterial);
    if (m_FilterSettings->OnlyShowUsed()) { wxCommandEvent CE; OnCheckbox_OnlyShowUsed(CE); }
}


void DialogT::OnButton_ExportDiffMaps(wxCommandEvent& Event)
{
    m_ScrolledMatWin->ExportDiffuseMaps(wxDirSelector("Please select the destination directory for the export.", "."));
}


void DialogT::OnButton_Cancel(wxCommandEvent& Event)
{
    SaveAndQuitDialog(wxID_CANCEL);
}


void DialogT::OnCombobox_NameFilterSelection(wxCommandEvent& Event)
{
    m_ScrolledMatWin->UpdateVirtualSize();
    m_ScrolledMatWin->SelectMaterial(m_CurrentMaterial);
    m_ScrolledMatWin->Refresh();      // If m_CurrentMaterial==NULL, m_ScrolledMatWin->SelectMaterial() doesn't refresh, so make sure it is done here.
}


void DialogT::OnCombobox_NameFilterTextChange(wxCommandEvent& Event)
{
    m_ScrolledMatWin->UpdateVirtualSize();
    m_ScrolledMatWin->SelectMaterial(m_CurrentMaterial);
    m_ScrolledMatWin->Refresh();      // If m_CurrentMaterial==NULL, m_ScrolledMatWin->SelectMaterial() doesn't refresh, so make sure it is done here.
}


void DialogT::OnCheckbox_OnlyShowUsed(wxCommandEvent& Event)
{
    if (m_FilterSettings->OnlyShowUsed())
    {
        static ArrayT<EditorMaterialI*> UsedMatList;

        m_DocAccess.GetUsedMaterials(UsedMatList);
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


void DialogT::OnCheckbox_OnlyShowEditor(wxCommandEvent& Event)
{
    m_ScrolledMatWin->UpdateVirtualSize();
    m_ScrolledMatWin->SelectMaterial(m_CurrentMaterial);
    m_ScrolledMatWin->Refresh();      // If m_CurrentMaterial==NULL, m_ScrolledMatWin->SelectMaterial() doesn't refresh, so make sure it is done here.
}
