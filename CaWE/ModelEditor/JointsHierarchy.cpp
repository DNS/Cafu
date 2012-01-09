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

#include "JointsHierarchy.hpp"
#include "ChildFrame.hpp"
#include "ModelDocument.hpp"
#include "Commands/Rename.hpp"
#include "Commands/Select.hpp"
#include "Models/Model_cmdl.hpp"

#include "wx/wx.h"


namespace
{
    class JointsTreeItemT : public wxTreeItemData
    {
        public:

        JointsTreeItemT(unsigned int JointNr) : m_JointNr(JointNr) {};

        unsigned int GetJointNr() const { return m_JointNr; }


        private:

        unsigned int m_JointNr;
    };


    long GetCustomStyle()
    {
        int Major;
        int Minor;
        const wxOperatingSystemId OsId=wxGetOsVersion(&Major, &Minor);

        if ((OsId & wxOS_WINDOWS) && (Major>=6))
        {
            // Twist buttons really should be used without lines, so use them only if they are supported.
            return wxTR_TWIST_BUTTONS | wxTR_NO_LINES;
        }

        return 0;
    }
}


using namespace ModelEditor;


BEGIN_EVENT_TABLE(JointsHierarchyT, wxTreeCtrl)
    EVT_SET_FOCUS            (JointsHierarchyT::OnFocus)
    EVT_KEY_DOWN             (JointsHierarchyT::OnKeyDown)
    // EVT_LEFT_DOWN         (JointsHierarchyT::OnTreeLeftClick)
    // EVT_LEFT_DCLICK       (JointsHierarchyT::OnTreeLeftClick)  // Handle double clicks like normal left clicks when it comes to clicks on tree item icons (otherwise double clicks are handled normally).
    EVT_TREE_ITEM_ACTIVATED  (wxID_ANY, JointsHierarchyT::OnItemActivated)
    EVT_TREE_SEL_CHANGED     (wxID_ANY, JointsHierarchyT::OnSelectionChanged)
    EVT_TREE_END_LABEL_EDIT  (wxID_ANY, JointsHierarchyT::OnLabelChanged)
    EVT_TREE_ITEM_RIGHT_CLICK(wxID_ANY, JointsHierarchyT::OnTreeItemRightClick)
END_EVENT_TABLE()


JointsHierarchyT::JointsHierarchyT(ChildFrameT* Parent, const wxSize& Size)
    : wxTreeCtrl(Parent, wxID_ANY, wxDefaultPosition, Size, wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT | wxTR_MULTIPLE | wxTR_EDIT_LABELS | ::GetCustomStyle()),
      m_ModelDoc(Parent->GetModelDoc()),
      m_Parent(Parent),
      m_IsRecursiveSelfNotify(false)
{
    m_ModelDoc->RegisterObserver(this);
    RefreshTree();
}


JointsHierarchyT::~JointsHierarchyT()
{
    if (m_ModelDoc)
        m_ModelDoc->UnregisterObserver(this);
}


const wxTreeItemId JointsHierarchyT::FindTreeItem(const wxTreeItemId& StartingItem, unsigned int JointNr) const
{
    // If the item to start with is invalid, return it so the result of this function call is invalid too.
    if (!StartingItem.IsOk()) return StartingItem;

    // Starting item is valid, so check if it is related to the Window.
    JointsTreeItemT* JointItem=(JointsTreeItemT*)GetItemData(StartingItem);

    if (JointItem->GetJointNr()==JointNr) return StartingItem;

    // No match, so recursively check all children of the starting item.
    wxTreeItemIdValue Cookie; // This cookie is needed for iteration over a trees children.

    wxTreeItemId Result=FindTreeItem(GetFirstChild(StartingItem, Cookie), JointNr); // If there is no first child (the TreeItemId passed to FindTreeItem is invalid).

    if (Result.IsOk()) return Result; // If we found a match, return it.

    for (unsigned long i=1; i<GetChildrenCount(StartingItem); i++)
    {
        Result=FindTreeItem(GetNextChild(StartingItem, Cookie), JointNr);

        if (Result.IsOk()) return Result; // If we found a match, return it.
    }

    wxASSERT(!Result.IsOk());

    return Result; // No match for this item and its children, so return the invalid result.
}


void JointsHierarchyT::GetTreeItems(const wxTreeItemId& StartingItem, ArrayT<wxTreeItemId>& Items)
{
    if (!StartingItem.IsOk()) return;

    Items.PushBack(StartingItem);

    if (!ItemHasChildren(StartingItem)) return;

    wxTreeItemIdValue Cookie; // This cookie is needed for iteration over a trees children.

    GetTreeItems(GetFirstChild(StartingItem, Cookie), Items);

    for (unsigned long i=1; i<GetChildrenCount(StartingItem); i++)
        GetTreeItems(GetNextChild(StartingItem, Cookie), Items);
}


void JointsHierarchyT::Notify_SelectionChanged(SubjectT* Subject, ModelElementTypeT Type, const ArrayT<unsigned int>& OldSel, const ArrayT<unsigned int>& NewSel)
{
    if (m_IsRecursiveSelfNotify) return;
    if (Type!=JOINT) return;

    // Both UnselectAll() and SelectItem() result in EVT_TREE_SEL_CHANGED event,
    // see <http://thread.gmane.org/gmane.comp.lib.wxwidgets.general/72754> for details.
    // m_IsRecursiveSelfNotify makes sure that the tree isn't unintentionally updated recursively.
    m_IsRecursiveSelfNotify=true;

    // Reset tree selection and update it according to new selection.
    UnselectAll();

    for (unsigned long NewSelNr=0; NewSelNr<NewSel.Size(); NewSelNr++)
    {
        wxTreeItemId Result=FindTreeItem(GetRootItem(), NewSel[NewSelNr]);

        if (Result.IsOk())
        {
            SelectItem(Result);
            EnsureVisible(Result);
        }
    }

    m_IsRecursiveSelfNotify=false;
}


void JointsHierarchyT::Notify_Created(SubjectT* Subject, ModelElementTypeT Type, const ArrayT<unsigned int>& Indices)
{
    if (m_IsRecursiveSelfNotify) return;
    if (Type!=JOINT) return;

    RefreshTree();
}


void JointsHierarchyT::Notify_Deleted(SubjectT* Subject, ModelElementTypeT Type, const ArrayT<unsigned int>& Indices)
{
    if (m_IsRecursiveSelfNotify) return;
    if (Type!=JOINT) return;

    RefreshTree();
}


void JointsHierarchyT::Notify_JointChanged(SubjectT* Subject, unsigned int JointNr)
{
    if (m_IsRecursiveSelfNotify) return;
    if (m_ModelDoc==NULL) return;

    wxTreeItemId Item=FindTreeItem(GetRootItem(), JointNr);
    if (!Item.IsOk()) return;

    const wxString Label    =GetItemText(Item);
    const wxString JointName=m_ModelDoc->GetModel()->GetJoints()[JointNr].Name;
    if (Label==JointName) return;

    SetItemText(Item, JointName);
    EnsureVisible(Item);
}


void JointsHierarchyT::Notify_SubjectDies(SubjectT* dyingSubject)
{
    wxASSERT(dyingSubject==m_ModelDoc);

    m_ModelDoc=NULL;

    DeleteAllItems();
}


void JointsHierarchyT::RefreshTree()
{
    if (m_ModelDoc==NULL) return;

    // Get all currently opened tree items and reopen them after the refresh.
    ArrayT<wxTreeItemId> TreeItems;
    ArrayT<unsigned int> ExpandedJoints;

    // Note that this may well produce TreeItems whose joints have been deleted from the document already.
    GetTreeItems(GetRootItem(), TreeItems);

    for (unsigned long ItemNr=0; ItemNr<TreeItems.Size(); ItemNr++)
        if (IsExpanded(TreeItems[ItemNr]))
            ExpandedJoints.PushBack(((JointsTreeItemT*)GetItemData(TreeItems[ItemNr]))->GetJointNr());

    Freeze();
    DeleteAllItems();

    // Add all joints to the tree.
    const ArrayT<CafuModelT::JointT>& Joints=m_ModelDoc->GetModel()->GetJoints();
    ArrayT<wxTreeItemId>              JointItemIds;

    for (unsigned long JointNr=0; JointNr<Joints.Size(); JointNr++)
    {
        const CafuModelT::JointT& J=Joints[JointNr];

        JointItemIds.PushBack(Joints[JointNr].Parent<0
            ? AddRoot(J.Name, -1, -1, new JointsTreeItemT(JointNr))
            : AppendItem(JointItemIds[J.Parent], J.Name, -1, -1, new JointsTreeItemT(JointNr)));
    }

    // Re-select selected joints in the tree.
    const ArrayT<unsigned int>& SelectedJoints=m_ModelDoc->GetSelection(JOINT);

    for (unsigned long SelNr=0; SelNr<SelectedJoints.Size(); SelNr++)
    {
        wxTreeItemId Result=FindTreeItem(JointItemIds[0], SelectedJoints[SelNr]);

        if (Result.IsOk())
        {
            m_IsRecursiveSelfNotify=true;
            SelectItem(Result);     // SelectItem() results in EVT_TREE_SEL_CHANGED event. m_IsRecursiveSelfNotify makes sure the tree isn't updated recursively.
            m_IsRecursiveSelfNotify=false;
            Expand(Result);

            // Make sure parents are also expanded.
            while (GetItemParent(Result).IsOk())
            {
                Result=GetItemParent(Result);
                Expand(Result);
            }
        }
    }

    Expand(JointItemIds[0]); // Always expand root joint.

    // Expand all previously expanded items.
    for (unsigned long ExJointNr=0; ExJointNr<ExpandedJoints.Size(); ExJointNr++)
    {
        wxTreeItemId Result=FindTreeItem(JointItemIds[0], ExpandedJoints[ExJointNr]);

        if (Result.IsOk())
            Expand(Result);
    }

    Thaw();
}


/*void JointsHierarchyT::OnTreeLeftClick(wxMouseEvent& ME)
{
    // Check if we hit an tree item icon.
    int HitFlag=0;

    wxTreeItemId ClickedItem=HitTest(ME.GetPosition(), HitFlag);

    // If a icon was hit, toggle visibility of the associated gui window.
    if (HitFlag & wxTREE_HITTEST_ONITEMICON)
    {
        cf::GuiSys::WindowT* ClickedWindow=((JointsTreeItemT*)GetItemData(ClickedItem))->GetWindow();

        m_IsRecursiveSelfNotify=true;

        if (ClickedWindow->ShowWindow)
        {
            m_Parent->SubmitCommand(new CommandModifyWindowT(m_GuiDocument, ClickedWindow, "Visible", ClickedWindow->GetMemberVar("show"), 0));
            SetItemImage(ClickedItem, 1); // Invisible icon.
        }
        else
        {
            m_Parent->SubmitCommand(new CommandModifyWindowT(m_GuiDocument, ClickedWindow, "Visible", ClickedWindow->GetMemberVar("show"), 1));
            SetItemImage(ClickedItem, 0); // Visible icon.
        }

        m_IsRecursiveSelfNotify=false;
    }
    else
    {
        // Skip event if no icon was hit to preserve wxTreeCtrl functionality.
        ME.Skip();
    }
}*/


void JointsHierarchyT::OnFocus(wxFocusEvent& FE)
{
    m_Parent->SetLastUsedType(JOINT);
    FE.Skip();
}


void JointsHierarchyT::OnItemActivated(wxTreeEvent& TE)
{
    // This is called when the item has been activated (ENTER or double click).
    if (m_ModelDoc==NULL) return;

    // Make sure that the AUI pane for the inspector related to this joints hierarchy is shown.
    m_Parent->ShowRelatedInspector(this);
}


void JointsHierarchyT::OnSelectionChanged(wxTreeEvent& TE)
{
    if (m_ModelDoc==NULL) return;
    if (m_IsRecursiveSelfNotify) return;

    m_IsRecursiveSelfNotify=true;

    // Get the currently selected tree items and update the document selection accordingly.
    wxArrayTreeItemIds SelectedItems;
    GetSelections(SelectedItems);

    ArrayT<unsigned int> NewSel;
    for (size_t SelNr=0; SelNr<SelectedItems.GetCount(); SelNr++)
        NewSel.PushBack(((JointsTreeItemT*)GetItemData(SelectedItems[SelNr]))->GetJointNr());

    m_Parent->SubmitCommand(CommandSelectT::Set(m_ModelDoc, JOINT, NewSel));

    m_IsRecursiveSelfNotify=false;
}


void JointsHierarchyT::OnKeyDown(wxKeyEvent& KE)
{
    switch (KE.GetKeyCode())
    {
        case WXK_F2:
        {
            wxTreeItemId Item=GetFocusedItem();

            if (Item.IsOk()) EditLabel(Item);
            break;
        }

        default:
            KE.Skip();
            break;
    }
}


void JointsHierarchyT::OnLabelChanged(wxTreeEvent& TE)
{
    // Emtpy string means the user has either not changed the label at all or
    // deleted the whole label string.
    if (TE.GetLabel()=="")
    {
        TE.Veto();  // Reset value.
        return;
    }

    const unsigned int JointNr=((JointsTreeItemT*)GetItemData(TE.GetItem()))->GetJointNr();

    m_IsRecursiveSelfNotify=true;

    if (!m_Parent->SubmitCommand(new CommandRenameT(m_ModelDoc, JOINT, JointNr, TE.GetLabel())))
    {
        TE.Veto();  // Reset value if not valid.
    }

    m_IsRecursiveSelfNotify=false;
}


void JointsHierarchyT::OnTreeItemRightClick(wxTreeEvent& TE)
{
    // Note that GetPopupMenuSelectionFromUser() temporarily disables UI updates for the window,
    // so our menu IDs used below should be doubly clash-free.
    enum
    {
        ID_MENU_INSPECT_EDIT=wxID_HIGHEST+1+100,
        ID_MENU_RENAME,
        ID_MENU_EXPAND_RECURSIVELY
    };

    wxMenu Menu;

    Menu.Append(ID_MENU_INSPECT_EDIT,       "Inspect / Edit\tEnter");
    Menu.Append(ID_MENU_RENAME,             "Rename\tF2");
    Menu.Append(ID_MENU_EXPAND_RECURSIVELY, "Expand all");

    switch (GetPopupMenuSelectionFromUser(Menu))
    {
        case ID_MENU_INSPECT_EDIT:
            // Make sure that the AUI pane for the inspector related to this joints hierarchy is shown.
            m_Parent->ShowRelatedInspector(this);
            break;

        case ID_MENU_RENAME:
            EditLabel(TE.GetItem());
            break;

        case ID_MENU_EXPAND_RECURSIVELY:
            ExpandAllChildren(TE.GetItem());
            break;
    }
}
