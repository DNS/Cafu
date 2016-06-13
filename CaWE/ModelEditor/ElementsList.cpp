/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ElementsList.hpp"
#include "ChildFrame.hpp"
#include "ModelDocument.hpp"
#include "Commands/Add.hpp"
#include "Commands/Delete.hpp"
#include "Commands/Rename.hpp"
#include "Commands/Select.hpp"
#include "Commands/UpdateUVCoords.hpp"
#include "../ArtProvider.hpp"

#include "MaterialSystem/Material.hpp"
#include "Models/Model_cmdl.hpp"

#include "wx/numdlg.h"


using namespace ModelEditor;


BEGIN_EVENT_TABLE(ElementsListT, wxListView)
    EVT_SET_FOCUS           (ElementsListT::OnFocus)
    EVT_CONTEXT_MENU        (ElementsListT::OnContextMenu)
    EVT_LIST_KEY_DOWN       (wxID_ANY, ElementsListT::OnKeyDown)
    EVT_LIST_ITEM_ACTIVATED (wxID_ANY, ElementsListT::OnItemActivated)
    EVT_LIST_ITEM_SELECTED  (wxID_ANY, ElementsListT::OnSelectionChanged)
    EVT_LIST_ITEM_DESELECTED(wxID_ANY, ElementsListT::OnSelectionChanged)
    EVT_LIST_END_LABEL_EDIT (wxID_ANY, ElementsListT::OnEndLabelEdit)
END_EVENT_TABLE()


ElementsListT::ElementsListT(ChildFrameT* MainFrame, wxWindow* Parent, const wxSize& Size, ModelElementTypeT Type)
    : wxListView(Parent, wxID_ANY, wxDefaultPosition, Size, wxLC_REPORT | wxLC_EDIT_LABELS),
      m_TYPE(Type),
      m_NUM_DEFAULT_ITEMS(m_TYPE==SKIN || m_TYPE==CHAN ? 1 : 0),
      m_ModelDoc(MainFrame->GetModelDoc()),
      m_MainFrame(MainFrame),
      m_IsRecursiveSelfNotify(false)
{
    wxASSERT(m_TYPE==MESH || m_TYPE==SKIN || m_TYPE==GFIX || m_TYPE==ANIM || m_TYPE==CHAN);

    // TODO: Make it up to the caller code to call this?
    // // As we are now a wxAUI pane rather than a wxDialog, explicitly set that events are not propagated to our parent.
    // SetExtraStyle(wxWS_EX_BLOCK_EVENTS);

    InsertColumn(0, "Name");
    InsertColumn(1, "#");
    if (m_TYPE==MESH) InsertColumn(2, "Material");

    m_ModelDoc->RegisterObserver(this);
    InitListItems();
}


ElementsListT::~ElementsListT()
{
    if (m_ModelDoc)
        m_ModelDoc->UnregisterObserver(this);
}


bool ElementsListT::AreDefaultItemsSelected() const
{
    for (int ItemNr=0; ItemNr<m_NUM_DEFAULT_ITEMS; ItemNr++)
        if (IsSelected(ItemNr))
            return true;

    return false;
}


void ElementsListT::Notify_SelectionChanged(SubjectT* Subject, ModelElementTypeT Type, const ArrayT<unsigned int>& OldSel, const ArrayT<unsigned int>& NewSel)
{
    if (m_IsRecursiveSelfNotify) return;
    if (m_TYPE==MESH && Type==SKIN) { InitListItems(); return; }
    if (Type!=m_TYPE) return;

    m_IsRecursiveSelfNotify=true;
    Freeze();

    for (long SelNr=GetFirstSelected(); SelNr!=-1; SelNr=GetNextSelected(SelNr))
        Select(SelNr, false);

    for (unsigned long SelNr=0; SelNr<NewSel.Size(); SelNr++)
        Select(NewSel[SelNr] + m_NUM_DEFAULT_ITEMS);

    if (NewSel.Size()==0 && m_NUM_DEFAULT_ITEMS>0)
        Select(0);

    Thaw();
    m_IsRecursiveSelfNotify=false;
}


void ElementsListT::Notify_Created(SubjectT* Subject, ModelElementTypeT Type, const ArrayT<unsigned int>& Indices)
{
    if (m_IsRecursiveSelfNotify) return;
    if (Type!=m_TYPE) return;

    InitListItems();
}


void ElementsListT::Notify_Deleted(SubjectT* Subject, ModelElementTypeT Type, const ArrayT<unsigned int>& Indices)
{
    if (m_IsRecursiveSelfNotify) return;
    if (Type!=m_TYPE) return;

    InitListItems();
}


void ElementsListT::Notify_MeshChanged(SubjectT* Subject, unsigned int MeshNr)
{
    if (m_IsRecursiveSelfNotify) return;
    if (m_TYPE!=MESH) return;

    InitListItems();
}


void ElementsListT::Notify_SkinChanged(SubjectT* Subject, unsigned int SkinNr)
{
    if (m_IsRecursiveSelfNotify) return;

    // Update the list of meshes also when a skin changed, as with each
    // mesh we display the used material in the currently selected skin.
    if (m_TYPE!=SKIN && m_TYPE!=MESH) return;

    InitListItems();
}


void ElementsListT::Notify_GuiFixtureChanged(SubjectT* Subject, unsigned int GuiFixtureNr)
{
    if (m_IsRecursiveSelfNotify) return;
    if (m_TYPE!=GFIX) return;

    InitListItems();
}


void ElementsListT::Notify_AnimChanged(SubjectT* Subject, unsigned int AnimNr)
{
    if (m_IsRecursiveSelfNotify) return;
    if (m_TYPE!=ANIM) return;

    InitListItems();
}


void ElementsListT::Notify_ChannelChanged(SubjectT* Subject, unsigned int ChannelNr)
{
    if (m_IsRecursiveSelfNotify) return;
    if (m_TYPE!=CHAN) return;

    InitListItems();
}


void ElementsListT::Notify_SubjectDies(SubjectT* dyingSubject)
{
    wxASSERT(dyingSubject==m_ModelDoc);

    m_ModelDoc=NULL;

    DeleteAllItems();
}


void ElementsListT::InitListItems()
{
    const ArrayT<unsigned int>& Sel=m_ModelDoc->GetSelection(m_TYPE);

    // When a command is done or undone, it can indirectly call our Notify_*() methods,
    // which in turn call InitListItems(). Our calls to Select() unfortunately cause a
    // wxEVT_COMMAND_LIST_ITEM_SELECTED event, which in turn calls our OnSelectionChanged()
    // handler. Thus we set m_IsRecursiveSelfNotify in order to prevent the handler
    // from inadvertently submitting another command.
    m_IsRecursiveSelfNotify=true;
    Freeze();
    DeleteAllItems();

    switch (m_TYPE)
    {
        case JOINT:
            wxASSERT(false);
            break;

        case MESH:
        {
            wxListItem Col;

            GetColumn(2, Col);
            Col.SetText(wxString::Format("Material (%s)", m_ModelDoc->GetSelSkinString()));
            SetColumn(2, Col);

            for (unsigned long ElemNr=0; ElemNr<m_ModelDoc->GetModel()->GetMeshes().Size(); ElemNr++)
            {
                const MaterialT* Mat=m_ModelDoc->GetModel()->GetMaterial(ElemNr, m_ModelDoc->GetSelSkinNr());

                InsertItem(ElemNr, m_ModelDoc->GetModel()->GetMeshes()[ElemNr].Name);
                SetItem(ElemNr, 1, wxString::Format("%lu", ElemNr));
                SetItem(ElemNr, 2, Mat ? Mat->Name : "<NULL>");

                if (Sel.Find(ElemNr)!=-1) Select(ElemNr);
            }
            break;
        }

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

        case GFIX:
            for (unsigned long ElemNr=0; ElemNr<m_ModelDoc->GetModel()->GetGuiFixtures().Size(); ElemNr++)
            {
                InsertItem(ElemNr, m_ModelDoc->GetModel()->GetGuiFixtures()[ElemNr].Name);
                SetItem(ElemNr, 1, wxString::Format("%lu", ElemNr));

                if (Sel.Find(ElemNr)!=-1) Select(ElemNr);
            }
            break;

        case ANIM:
            for (unsigned long ElemNr=0; ElemNr<m_ModelDoc->GetModel()->GetAnims().Size(); ElemNr++)
            {
                InsertItem(ElemNr, m_ModelDoc->GetModel()->GetAnims()[ElemNr].Name);
                SetItem(ElemNr, 1, wxString::Format("%lu", ElemNr));

                if (Sel.Find(ElemNr)!=-1) Select(ElemNr);
            }
            break;

        case CHAN:
            InsertItem(0, "all (default)");
            SetItem(0, 1, "-1");
            if (Sel.Size()==0) Select(0);

            for (unsigned long ElemNr=0; ElemNr<m_ModelDoc->GetModel()->GetChannels().Size(); ElemNr++)
            {
                InsertItem(ElemNr+1, m_ModelDoc->GetModel()->GetChannels()[ElemNr].Name);
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
    m_IsRecursiveSelfNotify=false;
}


void ElementsListT::OnFocus(wxFocusEvent& FE)
{
    m_MainFrame->SetLastUsedType(m_TYPE);
    FE.Skip();
}


void ElementsListT::OnContextMenu(wxContextMenuEvent& CE)
{
    // Note that GetPopupMenuSelectionFromUser() temporarily disables UI updates for the window,
    // so our menu IDs used below should be doubly clash-free.
    enum
    {
        ID_MENU_INSPECT_EDIT=wxID_HIGHEST+1+100,
        ID_MENU_RENAME,
        ID_MENU_ADD_NEW,
        ID_MENU_PROJECT_UV_COORDS
    };

    wxMenu Menu;

    if (m_TYPE!=SKIN) Menu.Append(ID_MENU_INSPECT_EDIT, "Inspect / Edit\tEnter");
    Menu.Append(ID_MENU_RENAME, "Rename\tF2");
    if (m_TYPE==GFIX || m_TYPE==SKIN || m_TYPE==CHAN) Menu.Append(ID_MENU_ADD_NEW, "Add/create new");
    if (m_TYPE==ANIM) Menu.Append(ID_MENU_ADD_NEW, "Import...");

    if (m_TYPE==MESH)
    {
        Menu.AppendSeparator();
        // Menu.Append(..., "Remove unused Vertices");
        // Menu.Append(..., "Remove unused Weights");
        Menu.Append(ID_MENU_PROJECT_UV_COORDS, "Project new UV-coords...", "Project UV-coordinates onto the mesh");
    }

    switch (GetPopupMenuSelectionFromUser(Menu))
    {
        case ID_MENU_INSPECT_EDIT:
            // Make sure that the AUI pane for the inspector related to this elements list is shown.
            m_MainFrame->ShowRelatedInspector(GetParent());
            break;

        case ID_MENU_RENAME:
        {
            const long SelNr=GetFirstSelected();

            // Only relabel custom elements (not the "default" ones).
            if (SelNr>=m_NUM_DEFAULT_ITEMS) EditLabel(SelNr);
            break;
        }

        case ID_MENU_ADD_NEW:
        {
            switch (m_TYPE)
            {
                case SKIN: m_MainFrame->SubmitNewSkin();       break;
                case GFIX: m_MainFrame->SubmitNewGuiFixture(); break;
                case ANIM: m_MainFrame->SubmitImportAnims();   break;
                case CHAN: m_MainFrame->SubmitNewChannel();    break;
                default: break;
            }
            break;
        }

        case ID_MENU_PROJECT_UV_COORDS:
        {
            if (m_TYPE!=MESH) break;
            if (m_ModelDoc->GetSelection(m_TYPE).Size()==0) break;

            const unsigned int MeshNr=m_ModelDoc->GetSelection(m_TYPE)[0];

            const long Scale=wxGetNumberFromUser(
                "This is a tool for fixing models that did not bring proper UV-coordinates, or none at all.\n"
                "The right solution is to assign UV-coordinates in your favourite 3D mesh modelling software\n"
                "and then to re-import the model, whereas this tool is meant as a very quick and very simple\n"
                "means for a hot-fix (that sometimes may suffice nevertheless).",
                "Enter UV-vector scale:",
                wxString::Format("Project UV-coordinates onto mesh %u", MeshNr),
                12,
                1,
                1024);

            m_MainFrame->SubmitCommand(new CommandUpdateUVCoordsT(m_ModelDoc,
                MeshNr,
                m_ModelDoc->GetAnimState().Pose,
                Vector3fT(Scale, 0, 0),
                Vector3fT(0, 0, Scale)));
            break;
        }
    }
}


void ElementsListT::OnKeyDown(wxListEvent& LE)
{
    switch (LE.GetKeyCode())
    {
        case WXK_F2:
        {
            const long SelNr=LE.GetIndex();

            // Only relabel custom elements (not the "default" ones).
            if (SelNr>=m_NUM_DEFAULT_ITEMS) EditLabel(SelNr);
            break;
        }

        default:
            LE.Skip();
            break;
    }
}


void ElementsListT::OnItemActivated(wxListEvent& LE)
{
    // This is called when the item has been activated (ENTER or double click).
    if (m_ModelDoc==NULL) return;

    // Make sure that the AUI pane for the inspector related to this elements list is shown.
    m_MainFrame->ShowRelatedInspector(GetParent());
}


void ElementsListT::OnSelectionChanged(wxListEvent& LE)
{
    if (m_ModelDoc==NULL) return;
    if (m_IsRecursiveSelfNotify) return;

    m_IsRecursiveSelfNotify=true;

    // Get the currently selected list items and update the document selection accordingly.
    ArrayT<unsigned int> NewSel;

    for (long SelNr=GetFirstSelected(); SelNr!=-1; SelNr=GetNextSelected(SelNr))
        if (SelNr>=m_NUM_DEFAULT_ITEMS)     // Skip the "default" elements.
            NewSel.PushBack(SelNr - m_NUM_DEFAULT_ITEMS);

    m_MainFrame->SubmitCommand(CommandSelectT::Set(m_ModelDoc, m_TYPE, NewSel));

    m_IsRecursiveSelfNotify=false;
}


void ElementsListT::OnEndLabelEdit(wxListEvent& LE)
{
    const long Index=LE.GetIndex();

    if (LE.IsEditCancelled()) return;
    if (Index < m_NUM_DEFAULT_ITEMS) { LE.Veto(); return; }   // Cannot relabel the "default" elements.

    m_IsRecursiveSelfNotify=true;
    m_MainFrame->SubmitCommand(new CommandRenameT(m_ModelDoc, m_TYPE, Index - m_NUM_DEFAULT_ITEMS, LE.GetLabel()));
    m_IsRecursiveSelfNotify=false;
}


BEGIN_EVENT_TABLE(ElementsPanelT, wxPanel)
    EVT_BUTTON(ID_BUTTON_ADD,    ElementsPanelT::OnButton)
    EVT_BUTTON(ID_BUTTON_UP,     ElementsPanelT::OnButton)
    EVT_BUTTON(ID_BUTTON_DOWN,   ElementsPanelT::OnButton)
    EVT_BUTTON(ID_BUTTON_DELETE, ElementsPanelT::OnButton)
    EVT_UPDATE_UI_RANGE(ID_BUTTON_ADD, ID_BUTTON_DELETE, ElementsPanelT::OnButtonUpdate)
END_EVENT_TABLE()


ElementsPanelT::ElementsPanelT(ChildFrameT* MainFrame, const wxSize& Size, ModelElementTypeT Type)
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

    m_List=new ElementsListT(MainFrame, this, /*ID_LISTVIEW,*/ wxDefaultSize, m_TYPE);
    item0->Add(m_List, 1, wxEXPAND, 0 );

    this->SetSizer( item0 );
    item0->SetSizeHints(this);
}


void ElementsPanelT::OnButton(wxCommandEvent& Event)
{
    switch (Event.GetId())
    {
        case ID_BUTTON_ADD:
        {
            switch (m_TYPE)
            {
                case SKIN: m_MainFrame->SubmitNewSkin();       break;
                case GFIX: m_MainFrame->SubmitNewGuiFixture(); break;
                case ANIM: m_MainFrame->SubmitImportAnims();   break;
                case CHAN: m_MainFrame->SubmitNewChannel();    break;
                default: break;
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


void ElementsPanelT::OnButtonUpdate(wxUpdateUIEvent& UE)
{
    switch (UE.GetId())
    {
        case ID_BUTTON_ADD:
        {
            UE.Enable(m_TYPE==SKIN || m_TYPE==GFIX || m_TYPE==ANIM || m_TYPE==CHAN);
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
            // Are some elements but not the "default" elements selected?
            UE.Enable(m_ModelDoc->GetSelection(m_TYPE).Size()>0 && !m_List->AreDefaultItemsSelected());
            break;
        }
    }
}
