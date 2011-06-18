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

#include "ElementsList.hpp"
#include "ChildFrame.hpp"
#include "ModelDocument.hpp"
#include "Commands/Select.hpp"

#include "MaterialSystem/Material.hpp"
#include "Models/Model_cmdl.hpp"


namespace
{
    class ListContextMenuT : public wxMenu
    {
        public:

        enum
        {
            ID_MENU_INSPECT_EDIT=wxID_HIGHEST+1,
            ID_MENU_RENAME
        };

        ListContextMenuT() : wxMenu(), ID(-1)
        {
            Append(ID_MENU_INSPECT_EDIT, "Inspect / Edit\tEnter");
            Append(ID_MENU_RENAME,       "Rename\tF2")->Enable(false);

            /* if (Type==MESH)
            {
                Append(..., "Remove unused Vertices");
                Append(..., "Remove unused Weights");
            } */
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


using namespace ModelEditor;


BEGIN_EVENT_TABLE(ElementsListT, wxListView)
    EVT_CONTEXT_MENU        (ElementsListT::OnContextMenu)
 // EVT_LIST_KEY_DOWN       (wxID_ANY, ElementsListT::OnKeyDown)
    EVT_LIST_ITEM_ACTIVATED (wxID_ANY, ElementsListT::OnItemActivated)
    EVT_LIST_ITEM_SELECTED  (wxID_ANY, ElementsListT::OnSelectionChanged)
    EVT_LIST_ITEM_DESELECTED(wxID_ANY, ElementsListT::OnSelectionChanged)
 // EVT_LIST_END_LABEL_EDIT (wxID_ANY, ElementsListT::OnEndLabelEdit)
END_EVENT_TABLE()


ElementsListT::ElementsListT(ChildFrameT* Parent, const wxSize& Size, ModelElementTypeT Type)
    : wxListView(Parent, wxID_ANY, wxDefaultPosition, Size, wxLC_REPORT /*| wxLC_EDIT_LABELS*/),
      m_TYPE(Type),
      m_ModelDoc(Parent->GetModelDoc()),
      m_Parent(Parent),
      m_IsRecursiveSelfNotify(false)
{
    wxASSERT(m_TYPE==ANIM || m_TYPE==MESH);

    // TODO: Make it up to the caller code to call this?
    // // As we are now a wxAUI pane rather than a wxDialog, explicitly set that events are not propagated to our parent.
    // SetExtraStyle(wxWS_EX_BLOCK_EVENTS);

    InsertColumn(0, "#");
    InsertColumn(1, "Name");
    if (m_TYPE==MESH) InsertColumn(2, "Material");

    m_ModelDoc->RegisterObserver(this);
    InitListItems();
}


ElementsListT::~ElementsListT()
{
    if (m_ModelDoc)
        m_ModelDoc->UnregisterObserver(this);
}


void ElementsListT::Notify_SelectionChanged(SubjectT* Subject, ModelElementTypeT Type, const ArrayT<unsigned int>& OldSel, const ArrayT<unsigned int>& NewSel)
{
    if (m_IsRecursiveSelfNotify) return;
    if (Type!=m_TYPE) return;

    m_IsRecursiveSelfNotify=true;
    Freeze();

    for (long SelNr=GetFirstSelected(); SelNr!=-1; SelNr=GetNextSelected(SelNr))
        Select(SelNr, false);

    for (unsigned long SelNr=0; SelNr<NewSel.Size(); SelNr++)
        Select(NewSel[SelNr]);

    Thaw();
    m_IsRecursiveSelfNotify=false;
}


void ElementsListT::Notify_MeshChanged(SubjectT* Subject, unsigned int MeshNr)
{
    if (m_IsRecursiveSelfNotify) return;
    if (m_TYPE!=MESH) return;

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
    const unsigned long         NumElems=(m_TYPE==MESH) ? m_ModelDoc->GetModel()->GetMeshes().Size() : m_ModelDoc->GetModel()->GetAnims().Size();
    const ArrayT<unsigned int>& Sel     =m_ModelDoc->GetSelection(m_TYPE);

    DeleteAllItems();

    for (unsigned long ElemNr=0; ElemNr<NumElems; ElemNr++)
    {
        InsertItem(ElemNr, wxString::Format("%lu", ElemNr));
        SetItem(ElemNr, 1, wxString(m_TYPE==MESH ? "Mesh" : "Anim") + wxString::Format(" %lu", ElemNr));
        if (m_TYPE==MESH) SetItem(ElemNr, 2, m_ModelDoc->GetModel()->GetMeshes()[ElemNr].Material->Name);

        if (Sel.Find(ElemNr)!=-1) Select(ElemNr);
    }

    // Set the widths of the columns to the width of their longest item.
    for (int ColNr=0; ColNr<GetColumnCount(); ColNr++)
        SetColumnWidth(ColNr, wxLIST_AUTOSIZE);
}


void ElementsListT::OnContextMenu(wxContextMenuEvent& CE)
{
    ListContextMenuT ContextMenu;

    PopupMenu(&ContextMenu);

    switch (ContextMenu.GetClickedMenuItem())
    {
        case ListContextMenuT::ID_MENU_INSPECT_EDIT:
            // Make sure that the AUI pane for the inspector related to this joints hierarchy is shown.
            m_Parent->ShowRelatedInspector(this);
            break;

        case ListContextMenuT::ID_MENU_RENAME:
            // EditLabel(...);
            break;
    }
}


/*void ElementsListT::OnKeyDown(wxListEvent& LE)
{
    switch (LE.GetKeyCode())
    {
        case WXK_F2:
        {
            const long SelNr=LE.GetIndex();

            if (SelNr!=-1) EditLabel(SelNr);
            break;
        }

        default:
            LE.Skip();
            break;
    }
}*/


void ElementsListT::OnItemActivated(wxListEvent& LE)
{
    // This is called when the item has been activated (ENTER or double click).
    if (m_ModelDoc==NULL) return;

    // Make sure that the AUI pane for the inspector related to this elements list is shown.
    m_Parent->ShowRelatedInspector(this);
}


void ElementsListT::OnSelectionChanged(wxListEvent& LE)
{
    if (m_ModelDoc==NULL) return;
    if (m_IsRecursiveSelfNotify) return;

    m_IsRecursiveSelfNotify=true;

    // Get the currently selected list items and update the document selection accordingly.
    ArrayT<unsigned int> NewSel;

    for (long SelNr=GetFirstSelected(); SelNr!=-1; SelNr=GetNextSelected(SelNr))
        NewSel.PushBack(SelNr);

    m_Parent->SubmitCommand(CommandSelectT::Set(m_ModelDoc, m_TYPE, NewSel));

    m_IsRecursiveSelfNotify=false;
}


/*void ElementsListT::OnEndLabelEdit(wxListEvent& LE)
{
    const unsigned long Index=LE.GetIndex();

    if (LE.IsEditCancelled()) return;
    // if (Index>=m_ModelDoc->GetGroups().Size()) return;

    m_IsRecursiveSelfNotify=true;
    // m_ModelDoc->GetHistory().SubmitCommand(new CommandGroupSetPropT(*MapDoc, MapDoc->GetGroups()[Index], LE.GetLabel()));
    m_IsRecursiveSelfNotify=false;
}*/
