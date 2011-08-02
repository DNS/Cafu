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

#include "SubmodelsList.hpp"
#include "ChildFrame.hpp"
#include "ModelDocument.hpp"
#include "Commands/Add.hpp"
#include "Commands/Delete.hpp"
#include "Commands/Rename.hpp"
#include "Commands/Select.hpp"
#include "../ArtProvider.hpp"
#include "../GameConfig.hpp"

#include "MaterialSystem/Material.hpp"
#include "Models/Model_cmdl.hpp"

#include "wx/filename.h"


using namespace ModelEditor;


namespace
{
    class ListContextMenuT : public wxMenu
    {
        public:

        enum
        {
            ID_MENU_LOAD=wxID_HIGHEST+1,
            ID_MENU_UNLOAD
        };

        ListContextMenuT() : wxMenu(), ID(-1)
        {
            Append(ID_MENU_LOAD,   "Load...");
            Append(ID_MENU_UNLOAD, "Unload");
        }

        int GetClickedMenuItem() { return ID; }


        protected:

        void OnMenuClick(wxCommandEvent& CE) { ID=CE.GetId(); }


        private:

        int ID;

        DECLARE_EVENT_TABLE()
    };


    BEGIN_EVENT_TABLE(ListContextMenuT, wxMenu)
        EVT_MENU(wxID_ANY, ListContextMenuT::OnMenuClick)
    END_EVENT_TABLE()
}


BEGIN_EVENT_TABLE(SubmodelsListT, wxListView)
    EVT_CONTEXT_MENU       (SubmodelsListT::OnContextMenu)
    EVT_LIST_ITEM_ACTIVATED(wxID_ANY, SubmodelsListT::OnItemActivated)
END_EVENT_TABLE()


SubmodelsListT::SubmodelsListT(ChildFrameT* MainFrame, wxWindow* Parent, const wxSize& Size)
    : wxListView(Parent, wxID_ANY, wxDefaultPosition, Size, wxLC_REPORT /*| wxLC_EDIT_LABELS*/),
      m_ModelDoc(MainFrame->GetModelDoc()),
      m_MainFrame(MainFrame),
      m_IsRecursiveSelfNotify(false)
{
    // TODO: Make it up to the caller code to call this?
    // // As we are now a wxAUI pane rather than a wxDialog, explicitly set that events are not propagated to our parent.
    // SetExtraStyle(wxWS_EX_BLOCK_EVENTS);

    InsertColumn(0, "Filename");

    m_ModelDoc->RegisterObserver(this);
    InitListItems();
}


SubmodelsListT::~SubmodelsListT()
{
    if (m_ModelDoc)
        m_ModelDoc->UnregisterObserver(this);
}


void SubmodelsListT::Notify_SubmodelsChanged(SubjectT* Subject)
{
    if (m_IsRecursiveSelfNotify) return;

    InitListItems();
}


void SubmodelsListT::Notify_SubjectDies(SubjectT* dyingSubject)
{
    wxASSERT(dyingSubject==m_ModelDoc);

    m_ModelDoc=NULL;

    DeleteAllItems();
}


void SubmodelsListT::InitListItems()
{
    Freeze();
    DeleteAllItems();

    for (unsigned long ElemNr=0; ElemNr<m_ModelDoc->GetSubmodels().Size(); ElemNr++)
    {
        wxFileName fn(m_ModelDoc->GetSubmodels()[ElemNr]->GetFilename());

        fn.MakeRelativeTo(m_ModelDoc->GetGameConfig()->ModDir);

        // The trailing spaces add some extra width that help with seeing the last character when the user is horizontally scrolling.
        InsertItem(ElemNr, fn.GetFullPath() + "  ");
    }

    // Set the widths of the columns to the width of their longest item.
    if (GetItemCount()>0)
        for (int ColNr=0; ColNr<GetColumnCount(); ColNr++)
            SetColumnWidth(ColNr, wxLIST_AUTOSIZE);

    Thaw();
}


void SubmodelsListT::LoadSubmodel()
{
    wxFileDialog FileDialog(this, "Load submodel", "", "", "Model files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (FileDialog.ShowModal()==wxID_OK)
    {
        m_ModelDoc->LoadSubmodel(FileDialog.GetPath());
        m_ModelDoc->UpdateAllObservers_SubmodelsChanged();
    }
}


void SubmodelsListT::UnloadSelectedSubmodels()
{
    // We have to make the "detour" via the DelSM array, because unloading any submodel potentially modifies the indices of the rest.
    ArrayT<ModelDocumentT::SubmodelT*> DelSM;

    for (long SelNr=GetFirstSelected(); SelNr!=-1; SelNr=GetNextSelected(SelNr))
        DelSM.PushBack(m_ModelDoc->GetSubmodels()[SelNr]);

    for (unsigned long SMNr=0; SMNr<DelSM.Size(); SMNr++)
        m_ModelDoc->UnloadSubmodel(m_ModelDoc->GetSubmodels().Find(DelSM[SMNr]));

    m_ModelDoc->UpdateAllObservers_SubmodelsChanged();
}


void SubmodelsListT::OnContextMenu(wxContextMenuEvent& CE)
{
    ListContextMenuT ContextMenu;

    PopupMenu(&ContextMenu);

    switch (ContextMenu.GetClickedMenuItem())
    {
        case ListContextMenuT::ID_MENU_LOAD:
        {
            LoadSubmodel();
            break;
        }

        case ListContextMenuT::ID_MENU_UNLOAD:
        {
            UnloadSelectedSubmodels();
            break;
        }
    }
}


void SubmodelsListT::OnItemActivated(wxListEvent& LE)
{
    // This is called when the item has been activated (ENTER or double click).
    if (m_ModelDoc==NULL) return;

    // Make sure that the AUI pane for the inspector related to this elements list is shown.
    // m_MainFrame->ShowRelatedInspector(GetParent());
}


BEGIN_EVENT_TABLE(SubmodelsPanelT, wxPanel)
    EVT_BUTTON(ID_BUTTON_LOAD,   SubmodelsPanelT::OnButton)
    EVT_BUTTON(ID_BUTTON_UP,     SubmodelsPanelT::OnButton)
    EVT_BUTTON(ID_BUTTON_DOWN,   SubmodelsPanelT::OnButton)
    EVT_BUTTON(ID_BUTTON_UNLOAD, SubmodelsPanelT::OnButton)
    EVT_UPDATE_UI_RANGE(ID_BUTTON_LOAD, ID_BUTTON_UNLOAD, SubmodelsPanelT::OnButtonUpdate)
END_EVENT_TABLE()


SubmodelsPanelT::SubmodelsPanelT(ChildFrameT* MainFrame, const wxSize& Size)
    : wxPanel(MainFrame, -1, wxDefaultPosition, Size),
      m_ModelDoc(MainFrame->GetModelDoc()),
      m_MainFrame(MainFrame),
      m_List(NULL)
{
    // As we are a wxAUI pane rather than a wxDialog, explicitly set that events are not propagated to our parent.
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);

    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer *item7 = new wxBoxSizer( wxHORIZONTAL );

    wxButton* button1 = new wxButton(this, ID_BUTTON_LOAD, wxT("load"), wxDefaultPosition, wxSize(22, -1), wxBU_EXACTFIT | wxBU_NOTEXT );
    button1->SetBitmap(wxArtProvider::GetBitmap("list-add", wxART_BUTTON));
    item7->Add(button1, 1, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

    wxButton* button2 = new wxButton(this, ID_BUTTON_UP, wxT("up"), wxDefaultPosition, wxSize(22, -1), wxBU_EXACTFIT | wxBU_NOTEXT );
    button2->SetBitmap(wxArtProvider::GetBitmap("list-selection-up", wxART_BUTTON));
    item7->Add(button2, 1, wxALIGN_CENTER|wxRIGHT, 5 );

    wxButton* button3 = new wxButton(this, ID_BUTTON_DOWN, wxT("down"), wxDefaultPosition, wxSize(22, -1), wxBU_EXACTFIT | wxBU_NOTEXT );
    button3->SetBitmap(wxArtProvider::GetBitmap("list-selection-down", wxART_BUTTON));
    item7->Add(button3, 1, wxALIGN_CENTER|wxRIGHT, 5 );

    wxButton* button4 = new wxButton(this, ID_BUTTON_UNLOAD, wxT("unload"), wxDefaultPosition, wxSize(22, -1), wxBU_EXACTFIT | wxBU_NOTEXT );
    button4->SetBitmap(wxArtProvider::GetBitmap("list-remove", wxART_BUTTON));
    item7->Add(button4, 1, wxALIGN_CENTER|wxRIGHT, 5 );

    item0->Add( item7, 0, wxEXPAND | wxTOP | wxBOTTOM, 3 );

    m_List=new SubmodelsListT(MainFrame, this, /*ID_LISTVIEW,*/ wxDefaultSize);
    item0->Add(m_List, 1, wxEXPAND, 0 );

    this->SetSizer( item0 );
    item0->SetSizeHints(this);
}


void SubmodelsPanelT::OnButton(wxCommandEvent& Event)
{
    switch (Event.GetId())
    {
        case ID_BUTTON_LOAD:
        {
            m_List->LoadSubmodel();
            break;
        }

        case ID_BUTTON_UNLOAD:
        {
            m_List->UnloadSelectedSubmodels();
            break;
        }
    }
}


void SubmodelsPanelT::OnButtonUpdate(wxUpdateUIEvent& UE)
{
    switch (UE.GetId())
    {
        case ID_BUTTON_LOAD:
        {
            UE.Enable(true);
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

        case ID_BUTTON_UNLOAD:
        {
            UE.Enable(m_List->GetSelectedItemCount()>0);
            break;
        }
    }
}
