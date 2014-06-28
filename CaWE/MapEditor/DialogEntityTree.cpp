/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2014 Carsten Fuchs Software.

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

#include "DialogEntityTree.hpp"
#include "../ChildFrame.hpp"
#include "../CompMapEntity.hpp"
#include "../MapDocument.hpp"
#include "../MapElement.hpp"

#include "Commands/ChangeEntityHierarchy.hpp"
#include "../MapCommands/NewEntity.hpp"
#include "../MapCommands/Select.hpp"

#include "../SetCompVar.hpp"

#include "GameSys/Entity.hpp"

#include "wx/artprov.h"


namespace
{
    class EntityTreeItemT : public wxTreeItemData
    {
        public:

        EntityTreeItemT(IntrusivePtrT<cf::GameSys::EntityT> Entity) : m_Entity(Entity) {};

        IntrusivePtrT<cf::GameSys::EntityT> GetEntity() { return m_Entity; }


        private:

        IntrusivePtrT<cf::GameSys::EntityT> m_Entity;
    };
}


using namespace MapEditor;


BEGIN_EVENT_TABLE(EntityTreeDialogT, wxTreeCtrl)
    EVT_KEY_DOWN             (EntityTreeDialogT::OnKeyDown)
    EVT_LEFT_DOWN            (EntityTreeDialogT::OnTreeLeftClick)
    EVT_LEFT_DCLICK          (EntityTreeDialogT::OnTreeLeftClick)   // Handle double clicks like normal left clicks when it comes to clicks on tree item icons (otherwise double clicks are handled normally).
    EVT_TREE_SEL_CHANGED     (wxID_ANY, EntityTreeDialogT::OnSelectionChanged)
    EVT_TREE_END_LABEL_EDIT  (wxID_ANY, EntityTreeDialogT::OnEndLabelEdit)
    EVT_TREE_ITEM_RIGHT_CLICK(wxID_ANY, EntityTreeDialogT::OnTreeItemRightClick)
    EVT_TREE_BEGIN_DRAG      (wxID_ANY, EntityTreeDialogT::OnBeginDrag)
    EVT_TREE_END_DRAG        (wxID_ANY, EntityTreeDialogT::OnEndDrag)
END_EVENT_TABLE()


EntityTreeDialogT::EntityTreeDialogT(ChildFrameT* Parent, const wxSize& Size)
    : wxTreeCtrl(Parent, wxID_ANY, wxDefaultPosition, Size, wxTR_HAS_BUTTONS|wxTR_LINES_AT_ROOT|wxTR_MULTIPLE|wxSUNKEN_BORDER|wxTR_EDIT_LABELS),
      m_MapDoc(Parent->GetDoc()),
      m_Parent(Parent),
      m_IsRecursiveSelfNotify(false),
      m_DraggedEntity(NULL)
{
    // Build list of entity tree icons.
    wxImageList* TreeIcons = new wxImageList(16, 16);

    TreeIcons->Add(wxBitmap("CaWE/res/checked.png", wxBITMAP_TYPE_PNG));
    TreeIcons->Add(wxBitmap("CaWE/res/unchecked.png", wxBITMAP_TYPE_PNG));

    AssignImageList(TreeIcons);   // Note: wxTreeCtrl takes ownership of this list and deletes it on window destroy.

    m_MapDoc->RegisterObserver(this);
    RefreshTree();
}


EntityTreeDialogT::~EntityTreeDialogT()
{
    if (m_MapDoc) m_MapDoc->UnregisterObserver(this);
}


// Automatically fills children of tree item using the items client data.
void EntityTreeDialogT::AddChildren(const wxTreeItemId& Item, bool Recursive)
{
    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > Children;

    // Read children of this object.
    // We can safely cast this since we have set the data ourselves.
    EntityTreeItemT* EntityItem = (EntityTreeItemT*)GetItemData(Item);

    EntityItem->GetEntity()->GetChildren(Children);

    for (unsigned long i = 0; i < Children.Size(); i++)
    {
        wxTreeItemId ID = AppendItem(Item, Children[i]->GetBasics()->GetEntityName(), -1, -1, new EntityTreeItemT(Children[i]));

        SetItemImage(ID, Children[i]->GetBasics()->IsShown() ? 0 : 1);  // Set the "is visible" or "is invisible" icon.

        if (Recursive) AddChildren(ID, true);
    }
}


const wxTreeItemId EntityTreeDialogT::FindTreeItem(const wxTreeItemId& StartingItem, IntrusivePtrT<cf::GameSys::EntityT> Entity) const
{
    // If the item to start with is invalid, return it so the result of this function call is invalid too.
    if (!StartingItem.IsOk()) return StartingItem;

    // Starting item is valid, so check if it is related to the Entity.
    EntityTreeItemT* EntityItem = (EntityTreeItemT*)GetItemData(StartingItem);

    if (EntityItem->GetEntity() == Entity) return StartingItem;

    // No match, so recursively check all children of the starting item.
    wxTreeItemIdValue Cookie; // This cookie is needed for iteration over a trees children.

    wxTreeItemId Result = FindTreeItem(GetFirstChild(StartingItem, Cookie), Entity); // If there is no first child (the TreeItemId passed to FindTreeItem is invalid).

    if (Result.IsOk()) return Result; // If we found a match, return it.

    for (unsigned long i = 1; i < GetChildrenCount(StartingItem); i++)
    {
        Result = FindTreeItem(GetNextChild(StartingItem, Cookie), Entity);

        if (Result.IsOk()) return Result; // If we found a match, return it.
    }

    wxASSERT(!Result.IsOk());

    return Result; // No match for this item and its children, so return the invalid result.
}


void EntityTreeDialogT::GetTreeItems(const wxTreeItemId& StartingItem, ArrayT<wxTreeItemId>& Items)
{
    if (!StartingItem.IsOk()) return;

    Items.PushBack(StartingItem);

    if (!ItemHasChildren(StartingItem)) return;

    wxTreeItemIdValue Cookie;   // This cookie is needed for the iteration over a tree's children.

    GetTreeItems(GetFirstChild(StartingItem, Cookie), Items);

    for (unsigned long i = 1; i < GetChildrenCount(StartingItem); i++)
        GetTreeItems(GetNextChild(StartingItem, Cookie), Items);
}


void EntityTreeDialogT::NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection)
{
    if (m_IsRecursiveSelfNotify) return;

    // Both UnselectAll() and SelectItem() result in EVT_TREE_SEL_CHANGED event,
    // see <http://thread.gmane.org/gmane.comp.lib.wxwidgets.general/72754> for details.
    // m_IsRecursiveSelfNotify makes sure that the tree isn't unintentionally updated recursively.
    m_IsRecursiveSelfNotify = true;

    // Reset tree selection and update it according to new selection.
    UnselectAll();

    for (unsigned long NewSelNr = 0; NewSelNr < NewSelection.Size(); NewSelNr++)
    {
        if (NewSelection[NewSelNr]->GetParent() == NULL) continue;

        wxTreeItemId Result = FindTreeItem(GetRootItem(), NewSelection[NewSelNr]->GetParent()->GetEntity());

        if (Result.IsOk())
        {
            SelectItem(Result);
            EnsureVisible(Result);
        }
    }

    m_IsRecursiveSelfNotify = false;
}


void EntityTreeDialogT::NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities)
{
    if (m_IsRecursiveSelfNotify) return;

    RefreshTree();
}


void EntityTreeDialogT::NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities)
{
    if (m_IsRecursiveSelfNotify) return;

    RefreshTree();
}


void EntityTreeDialogT::Notify_EntChanged(SubjectT* Subject, const ArrayT< IntrusivePtrT<MapEditor::CompMapEntityT> >& Entities, EntityModDetailE Detail)
{
    if (m_IsRecursiveSelfNotify) return;
    if (Detail != EMD_HIERARCHY) return;

    // TODO: can we handle order changes more precisely (e.g. move the tree item to its new order position)?
    RefreshTree();
}


void EntityTreeDialogT::Notify_VarChanged(SubjectT* Subject, const cf::TypeSys::VarBaseT& Var)
{
    if (m_IsRecursiveSelfNotify) return;

    ArrayT<wxTreeItemId> TreeItems;

    GetTreeItems(GetRootItem(), TreeItems);

    for (unsigned long ItemNr = 0; ItemNr < TreeItems.Size(); ItemNr++)
    {
        IntrusivePtrT<cf::GameSys::ComponentBasicsT> Basics = ((EntityTreeItemT*)GetItemData(TreeItems[ItemNr]))->GetEntity()->GetBasics();

        if (&Var == Basics->GetMemberVars().Find("Name"))
        {
            SetItemText(TreeItems[ItemNr], Basics->GetEntityName());
            return;
        }

        if (&Var == Basics->GetMemberVars().Find("Show"))
        {
            SetItemImage(TreeItems[ItemNr], Basics->IsShown() ? 0 : 1);   // Set the "is visible" or "is invisible" icon.
            return;
        }
    }
}


void EntityTreeDialogT::NotifySubjectDies(SubjectT* dyingSubject)
{
    wxASSERT(dyingSubject == m_MapDoc);

    m_MapDoc = NULL;

    DeleteAllItems();
}


void EntityTreeDialogT::RefreshTree()
{
    if (m_MapDoc == NULL) return;

    // Get all currently opened tree items and reopen them after the refresh.
    ArrayT<wxTreeItemId>                          TreeItems;
    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > ExpandedEntities;

    // Note that this may well produce TreeItems whose entities have been deleted from the document already.
    GetTreeItems(GetRootItem(), TreeItems);

    for (unsigned long ItemNr = 0; ItemNr < TreeItems.Size(); ItemNr++)
        if (IsExpanded(TreeItems[ItemNr]))
            ExpandedEntities.PushBack(((EntityTreeItemT*)GetItemData(TreeItems[ItemNr]))->GetEntity());

    Freeze();

    // Even if there was only a minor change, just re-build the entire tree from scratch.
    // An attempt to update any existing tree items is probably a lot more complicated.
    DeleteAllItems();

    // First add the root entity to the tree.
    IntrusivePtrT<cf::GameSys::EntityT> RootEntity = m_MapDoc->GetRootMapEntity()->GetEntity();
    wxTreeItemId ID = AddRoot(RootEntity->GetBasics()->GetEntityName(), -1, -1, new EntityTreeItemT(RootEntity));

    SetItemImage(ID, RootEntity->GetBasics()->IsShown() ? 0 : 1);   // Set the "is visible" or "is invisible" icon.

    // Add all children of root recursively to the tree.
    AddChildren(ID, true);

    // Re-select selected entities in the tree.
    const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > Selection = m_MapDoc->GetSelectedEntities();

    for (unsigned long SelNr = 0; SelNr < Selection.Size(); SelNr++)
    {
        wxTreeItemId Result = FindTreeItem(ID, Selection[SelNr]);

        if (Result.IsOk())
        {
            m_IsRecursiveSelfNotify = true;
            SelectItem(Result);     // SelectItem() results in EVT_TREE_SEL_CHANGED event. m_IsRecursiveSelfNotify makes sure the tree isn't updated recursively.
            m_IsRecursiveSelfNotify = false;
            Expand(Result);

            // Make sure parents are also expanded.
            while (GetItemParent(Result).IsOk())
            {
                Result = GetItemParent(Result);
                Expand(Result);
            }
        }
    }

    Expand(ID); // Always expand root entity.

    // Expand all previously expanded items.
    for (unsigned long ExWinNr = 0; ExWinNr < ExpandedEntities.Size(); ExWinNr++)
    {
        wxTreeItemId Result = FindTreeItem(GetRootItem(), ExpandedEntities[ExWinNr]);

        if (Result.IsOk())
            Expand(Result);
    }

    Thaw();
}


void EntityTreeDialogT::OnKeyDown(wxKeyEvent& KE)
{
    switch (KE.GetKeyCode())
    {
        case WXK_F2:
        {
            wxTreeItemId Item = GetFocusedItem();

            if (Item.IsOk()) EditLabel(Item);
            break;
        }

        default:
            KE.Skip();
            break;
    }
}


void EntityTreeDialogT::OnTreeLeftClick(wxMouseEvent& ME)
{
    // Check if we hit an tree item icon.
    int HitFlag = 0;

    wxTreeItemId ClickedItem = HitTest(ME.GetPosition(), HitFlag);

    // If a icon was hit, toggle visibility of the associated entity.
    if (HitFlag & wxTREE_HITTEST_ONITEMICON)
    {
        IntrusivePtrT<cf::GameSys::EntityT> ClickedEntity = ((EntityTreeItemT*)GetItemData(ClickedItem))->GetEntity();
        cf::TypeSys::VarT<bool>* Show = dynamic_cast<cf::TypeSys::VarT<bool>*>(ClickedEntity->GetBasics()->GetMemberVars().Find("Show"));

        m_IsRecursiveSelfNotify = true;

        if (Show)
        {
            m_Parent->SubmitCommand(new CommandSetCompVarT<bool>(m_MapDoc->GetAdapter(), *Show, !Show->Get()));
            SetItemImage(ClickedItem, Show->Get() ? 0 : 1);   // Set the "is visible" or "is invisible" icon.
        }

        m_IsRecursiveSelfNotify = false;
    }
    else
    {
        // Skip event if no icon was hit to preserve wxTreeCtrl functionality.
        ME.Skip();
    }
}


void EntityTreeDialogT::OnSelectionChanged(wxTreeEvent& TE)
{
    if (m_MapDoc == NULL || m_IsRecursiveSelfNotify) return;

    m_IsRecursiveSelfNotify = true;

    // Get the currently selected tree items and update the document selection accordingly.
    wxArrayTreeItemIds SelectedItems;
    GetSelections(SelectedItems);

    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > NewSelection;
    for (size_t SelNr = 0; SelNr < SelectedItems.GetCount(); SelNr++)
        NewSelection.PushBack(((EntityTreeItemT*)GetItemData(SelectedItems[SelNr]))->GetEntity());

    m_Parent->SubmitCommand(CommandSelectT::Set(m_MapDoc, NewSelection));

    m_IsRecursiveSelfNotify = false;
}


void EntityTreeDialogT::OnEndLabelEdit(wxTreeEvent& TE)
{
    IntrusivePtrT<cf::GameSys::EntityT> Entity = ((EntityTreeItemT*)GetItemData(TE.GetItem()))->GetEntity();
    cf::TypeSys::VarT<std::string>* Name = dynamic_cast<cf::TypeSys::VarT<std::string>*>(Entity->GetBasics()->GetMemberVars().Find("Name"));

    if (TE.IsEditCancelled()) return;

    m_IsRecursiveSelfNotify = true;

    if (Name)
    {
        m_Parent->SubmitCommand(new CommandSetCompVarT<std::string>(m_MapDoc->GetAdapter(), *Name, std::string(TE.GetLabel())));
    }

    m_IsRecursiveSelfNotify = false;

    // The command may well have set a name different from TE.GetLabel().
    TE.Veto();
    SetItemText(TE.GetItem(), Entity->GetBasics()->GetEntityName());
}


// This function has been duplicated into other modules, too... can we reconcile them?
static wxMenuItem* AppendMI(wxMenu& Menu, int MenuID, const wxString& Label, const wxArtID& ArtID, bool Active=true, const wxString& Help="")
{
    wxMenuItem* MI = new wxMenuItem(&Menu, MenuID, Label, Help);

    // Under wxMSW (2.9.2), the bitmap must be set before the menu item is added to the menu.
    if (ArtID != "")
        MI->SetBitmap(wxArtProvider::GetBitmap(ArtID, wxART_MENU));

    // Under wxGTK (2.9.2), the menu item must be added to the menu before we can call Enable().
    Menu.Append(MI);

    MI->Enable(Active);

    return MI;
}


void EntityTreeDialogT::OnTreeItemRightClick(wxTreeEvent& TE)
{
    // Note that GetPopupMenuSelectionFromUser() temporarily disables UI updates for the entity,
    // so our menu IDs used below should be doubly clash-free.
    enum
    {
        ID_MENU_CREATE_ENTITY = wxID_HIGHEST + 1,
        ID_MENU_RENAME
    };

    wxMenu Menu;

    // Create context menus.
    AppendMI(Menu, ID_MENU_CREATE_ENTITY, "Create Entity", "window-new");
    AppendMI(Menu, ID_MENU_RENAME,        "Rename\tF2", "" /*"textfield_rename"*/);

    switch (GetPopupMenuSelectionFromUser(Menu))
    {
        case ID_MENU_CREATE_ENTITY:
            m_Parent->SubmitCommand(new CommandNewEntityT(*m_MapDoc, ((EntityTreeItemT*)GetItemData(TE.GetItem()))->GetEntity(), false /*don't auto-select the new entity*/));
            break;

        case ID_MENU_RENAME:
            EditLabel(TE.GetItem());
            break;
    }
}


void EntityTreeDialogT::OnBeginDrag(wxTreeEvent& TE)
{
    wxASSERT(m_DraggedEntity == NULL);

    // Unfortunately we need this array in order to call the method below.
    // This is the only way to get the number of selected tree items.
    wxArrayTreeItemIds SelectedItems;

    if (GetSelections(SelectedItems) > 1)
    {
        wxMessageBox("Sorry, you can only drag one entity at a time.");
        return;
    }

    if (!TE.GetItem().IsOk())   // Should never happen.
    {
        wxMessageBox("Sorry, this entity cannot be dragged.");
        return;
    }

    if (TE.GetItem() == GetRootItem())
    {
        wxMessageBox("Sorry, the root entity cannot be dragged.");
        return;
    }

    m_DraggedEntity = ((EntityTreeItemT*)GetItemData(TE.GetItem()))->GetEntity();
    TE.Allow();
}


void EntityTreeDialogT::OnEndDrag(wxTreeEvent& TE)
{
    wxASSERT(!m_DraggedEntity.IsNull());
    if (m_DraggedEntity.IsNull()) return;
    IntrusivePtrT<cf::GameSys::EntityT> SourceEntity = m_DraggedEntity;
    m_DraggedEntity = NULL;

    if (!TE.GetItem().IsOk()) return;
    IntrusivePtrT<cf::GameSys::EntityT> TargetEntity = ((EntityTreeItemT*)GetItemData(TE.GetItem()))->GetEntity();


    // If SourceEntity is already an immediate child of TargetEntity, do nothing.
    if (SourceEntity->GetParent() == TargetEntity) return;

    // Make sure that TargetEntity is not in the subtree of SourceEntity (or else the reparenting would create invalid cycles).
    // Although the command below does the same check redundantly again, we also want to have it here for clarity.
    // Note that the TargetEntity can still be a child in a different subtree of SourceEntity->Parent.
    {
        ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > SubTree;

        SubTree.PushBack(SourceEntity);
        SourceEntity->GetChildren(SubTree, true /*recurse*/);

        if (SubTree.Find(TargetEntity) >= 0) return;
    }


    // Note that the "|| TargetEntity->Parent == NULL" half of the if-condition is actually only for safety,
    // because TargetEntity->Parent == NULL only if TargetEntity is the root, but the root entity always has children.
    // If it hadn't, then SourceEntity == TargetEntity, and we had not gotten here.
    if (TargetEntity->GetChildren().Size() > 0 || TargetEntity->GetParent() == NULL)
    {
        // Make SourceEntity a child of TargetEntity.
        const unsigned long NewPos = TargetEntity->GetChildren().Size();

        m_Parent->SubmitCommand(new CommandChangeEntityHierarchyT(m_MapDoc, SourceEntity, TargetEntity, NewPos));
    }
    else
    {
        wxASSERT(!TargetEntity->GetParent().IsNull());   // This condition has been established in the if-branch above.

        // Make SourceEntity a sibling of TargetEntity.
        const unsigned long NewPos = TargetEntity->GetParent()->GetChildren().Find(TargetEntity);

        m_Parent->SubmitCommand(new CommandChangeEntityHierarchyT(m_MapDoc, SourceEntity, TargetEntity->GetParent(), NewPos));
    }
}
