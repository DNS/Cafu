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

#include "SkinsList.hpp"
#include "ChildFrame.hpp"
#include "ModelDocument.hpp"
#include "Commands/Add.hpp"
#include "Commands/Delete.hpp"
#include "Commands/Rename.hpp"
#include "Commands/Select.hpp"
#include "../ArtProvider.hpp"

#include "Models/Model_cmdl.hpp"


using namespace ModelEditor;


BEGIN_EVENT_TABLE(SkinsListT, wxListView)
    EVT_SET_FOCUS           (SkinsListT::OnFocus)
    EVT_CONTEXT_MENU        (SkinsListT::OnContextMenu)
    EVT_LIST_KEY_DOWN       (wxID_ANY, SkinsListT::OnKeyDown)
    EVT_LIST_ITEM_ACTIVATED (wxID_ANY, SkinsListT::OnItemActivated)
    EVT_LIST_ITEM_SELECTED  (wxID_ANY, SkinsListT::OnSelectionChanged)
    EVT_LIST_ITEM_DESELECTED(wxID_ANY, SkinsListT::OnSelectionChanged)
    EVT_LIST_END_LABEL_EDIT (wxID_ANY, SkinsListT::OnEndLabelEdit)
END_EVENT_TABLE()


SkinsListT::SkinsListT(ChildFrameT* MainFrame, wxWindow* Parent, const wxSize& Size, ModelElementTypeT Type)
    : wxListView(Parent, wxID_ANY, wxDefaultPosition, Size, wxLC_REPORT | wxLC_EDIT_LABELS),
      m_TYPE(Type),
      m_ModelDoc(MainFrame->GetModelDoc()),
      m_MainFrame(MainFrame),
      m_IsRecursiveSelfNotify(false)
{
    // TODO: Make it up to the caller code to call this?
    // // As we are now a wxAUI pane rather than a wxDialog, explicitly set that events are not propagated to our parent.
    // SetExtraStyle(wxWS_EX_BLOCK_EVENTS);

    InsertColumn(0, "Name");
    InsertColumn(1, "#");

    m_ModelDoc->RegisterObserver(this);
    InitListItems();
}


SkinsListT::~SkinsListT()
{
    if (m_ModelDoc)
        m_ModelDoc->UnregisterObserver(this);
}


void SkinsListT::Notify_SelectionChanged(SubjectT* Subject, ModelElementTypeT Type, const ArrayT<unsigned int>& OldSel, const ArrayT<unsigned int>& NewSel)
{
    if (m_IsRecursiveSelfNotify) return;
    if (Type!=m_TYPE) return;

    m_IsRecursiveSelfNotify=true;
    Freeze();

    for (long SelNr=GetFirstSelected(); SelNr!=-1; SelNr=GetNextSelected(SelNr))
        Select(SelNr, false);

    for (unsigned long SelNr=0; SelNr<NewSel.Size(); SelNr++)
        Select(NewSel[SelNr]+1);

    if (NewSel.Size()==0)
        Select(0);

    Thaw();
    m_IsRecursiveSelfNotify=false;
}


void SkinsListT::Notify_Created(SubjectT* Subject, ModelElementTypeT Type, const ArrayT<unsigned int>& Indices)
{
    if (m_IsRecursiveSelfNotify) return;
    if (Type!=m_TYPE) return;

    InitListItems();
}


void SkinsListT::Notify_Deleted(SubjectT* Subject, ModelElementTypeT Type, const ArrayT<unsigned int>& Indices)
{
    if (m_IsRecursiveSelfNotify) return;
    if (Type!=m_TYPE) return;

    InitListItems();
}


void SkinsListT::Notify_SkinChanged(SubjectT* Subject, unsigned int SkinNr)
{
    if (m_IsRecursiveSelfNotify) return;

    InitListItems();
}


void SkinsListT::Notify_SubjectDies(SubjectT* dyingSubject)
{
    wxASSERT(dyingSubject==m_ModelDoc);

    m_ModelDoc=NULL;

    DeleteAllItems();
}


void SkinsListT::InitListItems()
{
    const ArrayT<unsigned int>& Sel=m_ModelDoc->GetSelection(m_TYPE);

    Freeze();
    DeleteAllItems();

    switch (m_TYPE)
    {
        case SKIN:
            InsertItem(0, "default");
            SetItem(0, 1, "-1");
            if (Sel.Size()==0) Select(0);

            for (unsigned long ElemNr=0; ElemNr<m_ModelDoc->GetModel()->GetSkins().Size(); ElemNr++)
            {
                InsertItem(ElemNr+1, m_ModelDoc->GetModel()->GetSkins()[ElemNr].Name);
                SetItem(ElemNr+1, 1, wxString::Format("%lu", ElemNr));

                if (Sel.Find(ElemNr)!=-1) Select(ElemNr+1);
            }

            break;
    }

    // Set the widths of the columns to the width of their longest item.
    if (GetItemCount()>0)
        for (int ColNr=0; ColNr<GetColumnCount(); ColNr++)
            SetColumnWidth(ColNr, wxLIST_AUTOSIZE);

    Thaw();
}


void SkinsListT::OnFocus(wxFocusEvent& FE)
{
    m_MainFrame->SetLastUsedType(m_TYPE);
    FE.Skip();
}


void SkinsListT::OnContextMenu(wxContextMenuEvent& CE)
{
    // Note that GetPopupMenuSelectionFromUser() temporarily disables UI updates for the window,
    // so our menu IDs used below should be doubly clash-free.
    enum
    {
        ID_MENU_RENAME=wxID_HIGHEST+1+100,
        ID_MENU_ADD_NEW
    };

    wxMenu Menu;

    Menu.Append(ID_MENU_RENAME,  "Rename\tF2");
    Menu.Append(ID_MENU_ADD_NEW, "Add/create new");

    switch (GetPopupMenuSelectionFromUser(Menu))
    {
        case ID_MENU_RENAME:
        {
            const long SelNr=GetFirstSelected();

            // Only relabel custom skins (not the "default" skin).
            if (SelNr>0) EditLabel(SelNr);
            break;
        }

        case ID_MENU_ADD_NEW:
        {
            if (m_TYPE==SKIN)
            {
                CafuModelT::SkinT Skin;

                Skin.Name="New Skin";
                while (Skin.Materials.Size()       < m_ModelDoc->GetModel()->GetMeshes().Size()) Skin.Materials.PushBack(NULL);
                while (Skin.RenderMaterials.Size() < m_ModelDoc->GetModel()->GetMeshes().Size()) Skin.RenderMaterials.PushBack(NULL);

                m_MainFrame->SubmitCommand(new CommandAddT(m_ModelDoc, Skin));
            }
            break;
        }
    }
}


void SkinsListT::OnKeyDown(wxListEvent& LE)
{
    switch (LE.GetKeyCode())
    {
        case WXK_F2:
        {
            const long SelNr=LE.GetIndex();

            // Only relabel custom skins (not the "default" skin).
            if (SelNr>0) EditLabel(SelNr);
            break;
        }

        default:
            LE.Skip();
            break;
    }
}


void SkinsListT::OnItemActivated(wxListEvent& LE)
{
    // This is called when the item has been activated (ENTER or double click).
    if (m_ModelDoc==NULL) return;

    // Do nothing -- skins have no related inspector (instead, the materials
    // for the currently selected skin are edited via the mesh inspector.)
    //
    // // Make sure that the AUI pane for the inspector related to this elements list is shown.
    // m_MainFrame->ShowRelatedInspector(GetParent());
}


void SkinsListT::OnSelectionChanged(wxListEvent& LE)
{
    if (m_ModelDoc==NULL) return;
    if (m_IsRecursiveSelfNotify) return;

    m_IsRecursiveSelfNotify=true;

    // Get the currently selected list items and update the document selection accordingly.
    ArrayT<unsigned int> NewSel;

    for (long SelNr=GetFirstSelected(); SelNr!=-1; SelNr=GetNextSelected(SelNr))
        if (SelNr>0)    // Skip the "default" skin.
            NewSel.PushBack(SelNr-1);

    m_MainFrame->SubmitCommand(CommandSelectT::Set(m_ModelDoc, m_TYPE, NewSel));

    m_IsRecursiveSelfNotify=false;
}


void SkinsListT::OnEndLabelEdit(wxListEvent& LE)
{
    const unsigned int Index=LE.GetIndex();

    if (LE.IsEditCancelled()) return;
    if (Index==0) { LE.Veto(); return; }    // Cannot relabel the "default" skin.

    m_IsRecursiveSelfNotify=true;
    m_MainFrame->SubmitCommand(new CommandRenameT(m_ModelDoc, SKIN, Index-1, LE.GetLabel()));
    m_IsRecursiveSelfNotify=false;
}


BEGIN_EVENT_TABLE(SkinsPanelT, wxPanel)
    EVT_BUTTON(ID_BUTTON_ADD,    SkinsPanelT::OnButton)
    EVT_BUTTON(ID_BUTTON_UP,     SkinsPanelT::OnButton)
    EVT_BUTTON(ID_BUTTON_DOWN,   SkinsPanelT::OnButton)
    EVT_BUTTON(ID_BUTTON_DELETE, SkinsPanelT::OnButton)
    EVT_UPDATE_UI_RANGE(ID_BUTTON_ADD, ID_BUTTON_DELETE, SkinsPanelT::OnButtonUpdate)
END_EVENT_TABLE()


SkinsPanelT::SkinsPanelT(ChildFrameT* MainFrame, const wxSize& Size, ModelElementTypeT Type)
    : wxPanel(MainFrame, -1, wxDefaultPosition, Size),
      m_TYPE(Type),
      m_ModelDoc(MainFrame->GetModelDoc()),
      m_MainFrame(MainFrame),
      m_List(NULL)
{
    // As we are a wxAUI pane rather than a wxDialog, explicitly set that events are not propagated to our parent.
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);

    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer *item7 = new wxBoxSizer( wxHORIZONTAL );

    wxButton* button1 = new wxButton(this, ID_BUTTON_ADD, wxT("add"), wxDefaultPosition, wxSize(22, -1), wxBU_EXACTFIT | wxBU_NOTEXT );
    button1->SetBitmap(wxArtProvider::GetBitmap("list-add", wxART_BUTTON));
    item7->Add(button1, 1, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

    wxButton* button2 = new wxButton(this, ID_BUTTON_UP, wxT("up"), wxDefaultPosition, wxSize(22, -1), wxBU_EXACTFIT | wxBU_NOTEXT );
    button2->SetBitmap(wxArtProvider::GetBitmap("list-selection-up", wxART_BUTTON));
    item7->Add(button2, 1, wxALIGN_CENTER|wxRIGHT, 5 );

    wxButton* button3 = new wxButton(this, ID_BUTTON_DOWN, wxT("down"), wxDefaultPosition, wxSize(22, -1), wxBU_EXACTFIT | wxBU_NOTEXT );
    button3->SetBitmap(wxArtProvider::GetBitmap("list-selection-down", wxART_BUTTON));
    item7->Add(button3, 1, wxALIGN_CENTER|wxRIGHT, 5 );

    wxButton* button4 = new wxButton(this, ID_BUTTON_DELETE, wxT("del"), wxDefaultPosition, wxSize(22, -1), wxBU_EXACTFIT | wxBU_NOTEXT );
    button4->SetBitmap(wxArtProvider::GetBitmap("list-remove", wxART_BUTTON));
    item7->Add(button4, 1, wxALIGN_CENTER|wxRIGHT, 5 );

    item0->Add( item7, 0, wxEXPAND | wxTOP | wxBOTTOM, 3 );

    m_List=new SkinsListT(MainFrame, this, /*ID_LISTVIEW,*/ wxDefaultSize, m_TYPE);
    item0->Add(m_List, 1, wxEXPAND, 0 );

    this->SetSizer( item0 );
    item0->SetSizeHints(this);
}


void SkinsPanelT::OnButton(wxCommandEvent& Event)
{
    switch (Event.GetId())
    {
        case ID_BUTTON_ADD:
        {
            if (m_TYPE==SKIN)
            {
                CafuModelT::SkinT Skin;

                Skin.Name="New Skin";
                while (Skin.Materials.Size()       < m_ModelDoc->GetModel()->GetMeshes().Size()) Skin.Materials.PushBack(NULL);
                while (Skin.RenderMaterials.Size() < m_ModelDoc->GetModel()->GetMeshes().Size()) Skin.RenderMaterials.PushBack(NULL);

                m_MainFrame->SubmitCommand(new CommandAddT(m_ModelDoc, Skin));
            }
            break;
        }

        case ID_BUTTON_DELETE:
        {
            CommandDeleteT* DelCmd=new CommandDeleteT(m_ModelDoc, m_TYPE, m_ModelDoc->GetSelection(m_TYPE));
            bool            Result=DelCmd->Do();

            if (DelCmd->GetMessage()!="") wxMessageBox(DelCmd->GetMessage(), "Delete");
            if (Result) m_MainFrame->SubmitCommand(DelCmd); else delete DelCmd;
            break;
        }
    }
}


void SkinsPanelT::OnButtonUpdate(wxUpdateUIEvent& UE)
{
    switch (UE.GetId())
    {
        case ID_BUTTON_ADD:
        {
            UE.Enable(m_TYPE==SKIN || m_TYPE==GFIX);
            break;
        }

        case ID_BUTTON_UP:
        {
            UE.Enable(false);
            break;
        }

        case ID_BUTTON_DOWN:
        {
            UE.Enable(false);
            break;
        }

        case ID_BUTTON_DELETE:
        {
            // Are some skins but not the "default" skin selected?
            UE.Enable(m_ModelDoc->GetSelection(SKIN).Size()>0 && !m_List->IsSelected(0));
            break;
        }
    }
}
