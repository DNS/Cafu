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

#include "ToolbarMaterials.hpp"
#include "ChildFrame.hpp"
#include "MapDocument.hpp"
#include "MapEntityBase.hpp"
#include "GameConfig.hpp"
#include "EditorMaterial.hpp"
#include "DialogReplaceMaterials.hpp"
#include "DialogEditSurfaceProps.hpp"
#include "EditorMaterialManager.hpp"
#include "MaterialBrowser/DocAccess.hpp"
#include "MaterialBrowser/MaterialBrowserDialog.hpp"

#include "wx/image.h"


static const int PREVIEW_BITMAP_SIZE=128;


BEGIN_EVENT_TABLE(MaterialsToolbarT, wxPanel)
    EVT_CHOICE(MaterialsToolbarT::ID_CHOICE_CURRENT_MAT,  MaterialsToolbarT::OnSelChangeCurrentMat)
    EVT_BUTTON(MaterialsToolbarT::ID_BUTTON_BROWSE_MATS,  MaterialsToolbarT::OnButtonBrowse)
    EVT_BUTTON(MaterialsToolbarT::ID_BUTTON_REPLACE_MATS, MaterialsToolbarT::OnButtonReplace)
    EVT_UPDATE_UI_RANGE(MaterialsToolbarT::ID_CHOICE_CURRENT_MAT, MaterialsToolbarT::ID_BUTTON_REPLACE_MATS, MaterialsToolbarT::OnUpdateUI)
END_EVENT_TABLE()


MaterialsToolbarT::MaterialsToolbarT(wxWindow* Parent, MapDocumentT* MapDoc)
    : wxPanel(Parent, -1, wxDefaultPosition, wxDefaultSize),
      m_MapDoc(MapDoc),
      m_MatMan(MapDoc->GetGameConfig()->GetMatMan()),
      ChoiceCurrentMat(NULL),
      StaticTextCurrentMatSize(NULL),
      BitmapCurrentMat(NULL)
{
    // As we are now a wxAUI pane rather than a wxDialog, explicitly set that events are not propagated to our parent.
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);

    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );

    // wxStaticText *item3 = new wxStaticText(this, -1, wxT("Current Material (MRUs):"), wxDefaultPosition, wxDefaultSize, 0 );
    // item0->Add( item3, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );

    ChoiceCurrentMat= new wxChoice(this, ID_CHOICE_CURRENT_MAT, wxDefaultPosition, wxSize(100,-1), 0, NULL, 0 );
    item0->Add(ChoiceCurrentMat, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );

    StaticTextCurrentMatSize=new wxStaticText(this, -1, wxT("Size: a x b"), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE );
    item0->Add(StaticTextCurrentMatSize, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxTOP|wxLEFT|wxRIGHT, 5 );

    BitmapCurrentMat=new BitmapControlT(this, wxDefaultPosition, wxSize(PREVIEW_BITMAP_SIZE,PREVIEW_BITMAP_SIZE) );
    item0->Add(BitmapCurrentMat, 0, wxALIGN_CENTER|wxBOTTOM|wxLEFT|wxRIGHT, 5 );

    wxBoxSizer *item7 = new wxBoxSizer( wxHORIZONTAL );

    wxButton *item8 = new wxButton(this, ID_BUTTON_BROWSE_MATS, wxT("Browse"), wxDefaultPosition, wxSize(50,-1), 0 );
    item7->Add( item8, 1, wxALIGN_CENTER|wxLEFT|wxRIGHT|wxBOTTOM, 5 );

    wxButton *item9 = new wxButton(this, ID_BUTTON_REPLACE_MATS, wxT("Replace"), wxDefaultPosition, wxSize(50,-1), 0 );
    item9->Disable();
    item7->Add( item9, 1, wxALIGN_CENTER|wxRIGHT|wxBOTTOM, 5 );

    item0->Add( item7, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5 );

    this->SetSizer( item0 );
    item0->SetSizeHints(this);


    // Load MRU materials from world.
    const MapEntityBaseT* World   =m_MapDoc->GetEntities()[0];
    bool                  FirstMat=true;   // The material is the first one to be inserted into the list (and becomes the default material).

    // Get up to 10 MRU materials.
    for (unsigned long MatNr=0; MatNr<10; MatNr++)
    {
        const EntPropertyT* MRUProp=World->FindProperty(wxString::Format("mru_mat_%lu", MatNr));

        if (MRUProp==NULL) break;

        EditorMaterialI* Material=m_MatMan.FindMaterial(MRUProp->Value, false);

        if (!Material) continue;

        ChoiceCurrentMat->Append(MRUProp->Value, Material);

        if (FirstMat)
        {
            m_MatMan.SetDefaultMaterial(Material);
            FirstMat=false;
        }
    }

    // If no MRUs are stored in the world make sure that some default material is selected.
    if (ChoiceCurrentMat->IsEmpty())
    {
        EditorMaterialI* DefaultMat=m_MatMan.GetDefaultMaterial();

        ChoiceCurrentMat->Append(DefaultMat->GetName(), DefaultMat);
    }

    ChoiceCurrentMat->SetSelection(0);
    wxCommandEvent CE; OnSelChangeCurrentMat(CE);

    m_MapDoc->RegisterObserver(this);
}


/************************/
/*** Public Functions ***/
/************************/

void MaterialsToolbarT::NotifySubjectDies(SubjectT* Subject)
{
    wxASSERT(Subject==m_MapDoc);

    m_MapDoc=NULL;
}


bool MaterialsToolbarT::Show(bool show)
{
    if (show)
    {
        wxASSERT(m_MapDoc->GetChildFrame()->GetSurfacePropsDialog()!=NULL);

        // Synchronize with materials toolbar.
        ArrayT<EditorMaterialI*> MRUMaterials=m_MapDoc->GetChildFrame()->GetSurfacePropsDialog()->GetMRUMaterials();

        wxASSERT(MRUMaterials.Size()>0 && MRUMaterials.Size()<=10);

        ChoiceCurrentMat->Clear();

        for (unsigned long MatNr=0; MatNr<MRUMaterials.Size(); MatNr++)
            ChoiceCurrentMat->Append(MRUMaterials[MatNr]->GetName(), MRUMaterials[MatNr]);

        ChoiceCurrentMat->SetSelection(0);

        // Call this event method to repaint displayed material.
        wxCommandEvent CE; OnSelChangeCurrentMat(CE);
    }

    return wxPanel::Show(show);
}


ArrayT<EditorMaterialI*> MaterialsToolbarT::GetMRUMaterials() const
{
    ArrayT<EditorMaterialI*> MRUMaterials;

    for (unsigned long MatNr=0; MatNr<ChoiceCurrentMat->GetCount(); MatNr++)
        MRUMaterials.PushBack((EditorMaterialI*)ChoiceCurrentMat->GetClientData(MatNr));

    return MRUMaterials;
}


/**********************/
/*** Event Handlers ***/
/**********************/

// This looks at the current selection of the ChoiceCurrentMat,
// and updates the material preview bitmap and "Size: a x b" text accordingly.
// It also rearranges the contents of the ChoiceCurrentMat to reflect MRU behaviour.
void MaterialsToolbarT::OnSelChangeCurrentMat(wxCommandEvent& Event)
{
    int Index=ChoiceCurrentMat->GetSelection();

    wxASSERT(Index!=-1);

 // if (Index==-1)
 // {
 //     BitmapCurrentMat->m_Bitmap=wxNullBitmap;
 //     StaticTextCurrentMatSize->SetLabel("(No material selected.)");
 // }
 // else
    {
        EditorMaterialI* CurrentMaterial=(EditorMaterialI*)ChoiceCurrentMat->GetClientData(Index);

        if (Index!=0)
        {
            ChoiceCurrentMat->Delete(Index);
            ChoiceCurrentMat->Insert(CurrentMaterial->GetName(), 0, CurrentMaterial);
            ChoiceCurrentMat->SetSelection(0);
        }

        const int  w  =CurrentMaterial->GetWidth ();
        const int  h  =CurrentMaterial->GetHeight();
        const int  Max=w>h ? w : h;
        const bool Fit=(w<=PREVIEW_BITMAP_SIZE && h<=PREVIEW_BITMAP_SIZE);

        wxBitmap PreviewBitmap=Fit ? wxBitmap(CurrentMaterial->GetImage()) : wxBitmap(CurrentMaterial->GetImage().Scale(w*PREVIEW_BITMAP_SIZE/Max, h*PREVIEW_BITMAP_SIZE/Max));

        BitmapCurrentMat->m_Bitmap=PreviewBitmap;
        BitmapCurrentMat->Refresh();
        StaticTextCurrentMatSize->SetLabel(wxString::Format("Size: %ix%i", CurrentMaterial->GetWidth(), CurrentMaterial->GetHeight()));
        m_MatMan.SetDefaultMaterial(CurrentMaterial);
    }

    for (unsigned long i=0; i<ChoiceCurrentMat->GetCount(); i++)
    {
        EntPropertyT* MRUProp=m_MapDoc->GetEntities()[0]->FindProperty(wxString::Format("mru_mat_%lu", i), NULL, true);

        wxASSERT(MRUProp!=NULL);

        MRUProp->Value=ChoiceCurrentMat->GetString(i);
    }
}


void MaterialsToolbarT::OnButtonBrowse(wxCommandEvent& Event)
{
    int Index=ChoiceCurrentMat->GetSelection();

    MaterialBrowserDialogT MatBrowser(this, MaterialBrowser::MapDocAccessT(*m_MapDoc),
                                      Index!=-1 ? (EditorMaterialI*)ChoiceCurrentMat->GetClientData(Index) : NULL, "", false);

    if (MatBrowser.ShowModal()!=wxID_OK) return;

    EditorMaterialI* Mat=MatBrowser.GetCurrentMaterial();
    if (Mat==NULL) return;

    Index=ChoiceCurrentMat->FindString(Mat->GetName());
    if (Index==-1)
    {
        // If there are already 10 or more items in the control, delete the last one.
        while (ChoiceCurrentMat->GetCount()>=10)
            ChoiceCurrentMat->Delete(ChoiceCurrentMat->GetCount()-1);

        // Material not found in list, add it now.
        ChoiceCurrentMat->Append(Mat->GetName(), Mat);
        Index=ChoiceCurrentMat->GetCount()-1;
    }

    // The new material is inserted at the end of the list, OnSelChangeCurrentMat() makes sure that it is moved to the top of the list.
    ChoiceCurrentMat->SetSelection(Index);
    wxCommandEvent CE; OnSelChangeCurrentMat(CE);
}


void MaterialsToolbarT::OnButtonReplace(wxCommandEvent& Event)
{
    if (m_MapDoc)
    {
        ReplaceMaterialsDialogT ReplaceMatsDlg(m_MapDoc->GetSelection().Size()>0, *m_MapDoc, m_MatMan.GetDefaultMaterial()->GetName());
        ReplaceMatsDlg.ShowModal();
    }
}


void MaterialsToolbarT::OnUpdateUI(wxUpdateUIEvent& Event)
{
    if (Event.GetId()==ID_BUTTON_REPLACE_MATS) Event.Enable(m_MapDoc!=NULL);
                                          else Event.Enable(true);
}
