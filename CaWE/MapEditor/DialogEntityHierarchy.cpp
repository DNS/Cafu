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

#include "DialogEntityHierarchy.hpp"
#include "ChildFrame.hpp"
#include "CompMapEntity.hpp"
#include "MapDocument.hpp"
#include "MapElement.hpp"
#include "MapEntRepres.hpp"
#include "MapPrimitive.hpp"

#include "../SetCompVar.hpp"

#include "Commands/ChangeEntityHierarchy.hpp"
#include "Commands/NewEntity.hpp"
#include "Commands/Select.hpp"

#include "GameSys/Entity.hpp"
#include "GameSys/EntityCreateParams.hpp"

#include "wx/artprov.h"
#include "wx/imaglist.h"


using namespace MapEditor;


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


    wxString GetEntityLabel(IntrusivePtrT<cf::GameSys::EntityT> Entity)
    {
        wxString            Label    = Entity->GetBasics()->GetEntityName();
        const unsigned long NumPrims = GetMapEnt(Entity)->GetPrimitives().Size();

        if (NumPrims > 0)
            Label += wxString::Format(" (%lu)", NumPrims);

        return Label;
    }
}


BEGIN_EVENT_TABLE(EntityHierarchyDialogT, wxTreeCtrl)
    EVT_KEY_DOWN             (EntityHierarchyDialogT::OnKeyDown)
    EVT_LEFT_DOWN            (EntityHierarchyDialogT::OnTreeLeftClick)
    EVT_LEFT_DCLICK          (EntityHierarchyDialogT::OnTreeLeftClick)   // Handle double clicks like normal left clicks when it comes to clicks on tree item icons (otherwise double clicks are handled normally).
    EVT_TREE_SEL_CHANGED     (wxID_ANY, EntityHierarchyDialogT::OnSelectionChanged)
    EVT_TREE_BEGIN_LABEL_EDIT(wxID_ANY, EntityHierarchyDialogT::OnBeginLabelEdit)
    EVT_TREE_END_LABEL_EDIT  (wxID_ANY, EntityHierarchyDialogT::OnEndLabelEdit)
    EVT_TREE_ITEM_RIGHT_CLICK(wxID_ANY, EntityHierarchyDialogT::OnTreeItemRightClick)
    EVT_TREE_BEGIN_DRAG      (wxID_ANY, EntityHierarchyDialogT::OnBeginDrag)
    EVT_TREE_END_DRAG        (wxID_ANY, EntityHierarchyDialogT::OnEndDrag)
END_EVENT_TABLE()


EntityHierarchyDialogT::EntityHierarchyDialogT(ChildFrameT* Parent, wxWindow* WinParent)
    : wxTreeCtrl(WinParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS|wxTR_LINES_AT_ROOT|wxTR_MULTIPLE|wxBORDER_NONE|wxTR_EDIT_LABELS),
      m_MapDoc(Parent->GetDoc()),
      m_Parent(Parent),
      m_IsRecursiveSelfNotify(false),
      m_DraggedEntity(NULL)
{
    // Build list of entity hierarchy icons.
    wxImageList* TreeIcons = new wxImageList(16, 16);

    TreeIcons->Add(wxBitmap("CaWE/res/checked.png", wxBITMAP_TYPE_PNG));
    TreeIcons->Add(wxBitmap("CaWE/res/unchecked.png", wxBITMAP_TYPE_PNG));

    AssignImageList(TreeIcons);   // Note: wxTreeCtrl takes ownership of this list and deletes it on window destroy.

    m_MapDoc->RegisterObserver(this);
    RefreshTree();
}


EntityHierarchyDialogT::~EntityHierarchyDialogT()
{
    if (m_MapDoc) m_MapDoc->UnregisterObserver(this);
}


// Automatically fills children of tree item using the items client data.
void EntityHierarchyDialogT::AddChildren(const wxTreeItemId& Item, bool Recursive)
{
    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > Children;

    // Read children of this object.
    // We can safely cast this since we have set the data ourselves.
    EntityTreeItemT* EntityItem = (EntityTreeItemT*)GetItemData(Item);

    EntityItem->GetEntity()->GetChildren(Children);

    for (unsigned long i = 0; i < Children.Size(); i++)
    {
        wxTreeItemId ID = AppendItem(Item, GetEntityLabel(Children[i]), -1, -1, new EntityTreeItemT(Children[i]));

        SetItemImage(ID, Children[i]->GetBasics()->IsShown() ? 0 : 1);  // Set the "is visible" or "is invisible" icon.

        if (Recursive) AddChildren(ID, true);
    }
}


const wxTreeItemId EntityHierarchyDialogT::FindTreeItem(const wxTreeItemId& StartingItem, IntrusivePtrT<cf::GameSys::EntityT> Entity) const
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


void EntityHierarchyDialogT::GetTreeItems(const wxTreeItemId& StartingItem, ArrayT<wxTreeItemId>& Items)
{
    if (!StartingItem.IsOk()) return;

    Items.PushBack(StartingItem);

    if (!ItemHasChildren(StartingItem)) return;

    wxTreeItemIdValue Cookie;   // This cookie is needed for the iteration over a tree's children.

    GetTreeItems(GetFirstChild(StartingItem, Cookie), Items);

    for (unsigned long i = 1; i < GetChildrenCount(StartingItem); i++)
        GetTreeItems(GetNextChild(StartingItem, Cookie), Items);
}


void EntityHierarchyDialogT::UpdateAllLabels()
{
    ArrayT<wxTreeItemId> TreeItems;

    GetTreeItems(GetRootItem(), TreeItems);

    for (unsigned long ItemNr = 0; ItemNr < TreeItems.Size(); ItemNr++)
    {
        IntrusivePtrT<cf::GameSys::EntityT> Entity = ((EntityTreeItemT*)GetItemData(TreeItems[ItemNr]))->GetEntity();

        SetItemText(TreeItems[ItemNr], GetEntityLabel(Entity));
    }
}


void EntityHierarchyDialogT::NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection)
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
        // If the selected element is something other than an entity representation
        // (that is, a map primitive), don't consider it.
        if (NewSelection[NewSelNr]->GetParent()->GetRepres() != NewSelection[NewSelNr]) continue;

        wxTreeItemId Result = FindTreeItem(GetRootItem(), NewSelection[NewSelNr]->GetParent()->GetEntity());

        if (Result.IsOk())
        {
            SelectItem(Result);
            EnsureVisible(Result);
        }
    }

    m_IsRecursiveSelfNotify = false;
}


void EntityHierarchyDialogT::NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities)
{
    if (m_IsRecursiveSelfNotify) return;

    RefreshTree();
}


void EntityHierarchyDialogT::NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT<MapPrimitiveT*>& Primitives)
{
    if (m_IsRecursiveSelfNotify) return;

    UpdateAllLabels();
}


void EntityHierarchyDialogT::NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities)
{
    if (m_IsRecursiveSelfNotify) return;

    RefreshTree();
}


void EntityHierarchyDialogT::NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT<MapPrimitiveT*>& Primitives)
{
    if (m_IsRecursiveSelfNotify) return;

    UpdateAllLabels();
}


void EntityHierarchyDialogT::NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail)
{
    if (m_IsRecursiveSelfNotify) return;

    if (Detail == MEMD_ASSIGN_PRIM_TO_ENTITY)
    {
        UpdateAllLabels();
    }
}


void EntityHierarchyDialogT::Notify_EntChanged(SubjectT* Subject, const ArrayT< IntrusivePtrT<MapEditor::CompMapEntityT> >& Entities, EntityModDetailE Detail)
{
    if (m_IsRecursiveSelfNotify) return;
    if (Detail != EMD_HIERARCHY) return;

    // TODO: can we handle order changes more precisely (e.g. move the tree item to its new order position)?
    RefreshTree();
}


void EntityHierarchyDialogT::Notify_VarChanged(SubjectT* Subject, const cf::TypeSys::VarBaseT& Var)
{
    if (m_IsRecursiveSelfNotify) return;

    ArrayT<wxTreeItemId> TreeItems;

    GetTreeItems(GetRootItem(), TreeItems);

    for (unsigned long ItemNr = 0; ItemNr < TreeItems.Size(); ItemNr++)
    {
        IntrusivePtrT<cf::GameSys::ComponentBasicsT> Basics = ((EntityTreeItemT*)GetItemData(TreeItems[ItemNr]))->GetEntity()->GetBasics();

        if (&Var == Basics->GetMemberVars().Find("Name"))
        {
            SetItemText(TreeItems[ItemNr], GetEntityLabel(Basics->GetEntity()));
            return;
        }

        if (&Var == Basics->GetMemberVars().Find("Show"))
        {
            SetItemImage(TreeItems[ItemNr], Basics->IsShown() ? 0 : 1);   // Set the "is visible" or "is invisible" icon.
            return;
        }
    }
}


void EntityHierarchyDialogT::NotifySubjectDies(SubjectT* dyingSubject)
{
    wxASSERT(dyingSubject == m_MapDoc);

    m_MapDoc = NULL;

    DeleteAllItems();
}


void EntityHierarchyDialogT::RefreshTree()
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
    wxTreeItemId ID = AddRoot(GetEntityLabel(RootEntity), -1, -1, new EntityTreeItemT(RootEntity));

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


void EntityHierarchyDialogT::OnKeyDown(wxKeyEvent& KE)
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


void EntityHierarchyDialogT::OnTreeLeftClick(wxMouseEvent& ME)
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


void EntityHierarchyDialogT::OnSelectionChanged(wxTreeEvent& TE)
{
    if (m_MapDoc == NULL || m_IsRecursiveSelfNotify) return;

    m_IsRecursiveSelfNotify = true;

    // Get the currently selected tree items and update the document selection accordingly.
    wxArrayTreeItemIds SelectedItems;
    GetSelections(SelectedItems);

    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > NewSelection;

    for (size_t SelNr = 0; SelNr < SelectedItems.GetCount(); SelNr++)
        NewSelection.PushBack(((EntityTreeItemT*)GetItemData(SelectedItems[SelNr]))->GetEntity());

    m_Parent->SubmitCommand(CommandSelectT::Set(m_MapDoc, NewSelection, false /*WithEntPrims*/));

    m_IsRecursiveSelfNotify = false;
}


void EntityHierarchyDialogT::OnBeginLabelEdit(wxTreeEvent& TE)
{
    IntrusivePtrT<cf::GameSys::EntityT> Entity = ((EntityTreeItemT*)GetItemData(TE.GetItem()))->GetEntity();

    SetItemText(TE.GetItem(), Entity->GetBasics()->GetEntityName());
}


void EntityHierarchyDialogT::OnEndLabelEdit(wxTreeEvent& TE)
{
    IntrusivePtrT<cf::GameSys::EntityT> Entity = ((EntityTreeItemT*)GetItemData(TE.GetItem()))->GetEntity();
    cf::TypeSys::VarT<std::string>* Name = dynamic_cast<cf::TypeSys::VarT<std::string>*>(Entity->GetBasics()->GetMemberVars().Find("Name"));

    if (TE.IsEditCancelled())
    {
        SetItemText(TE.GetItem(), GetEntityLabel(Entity));
        return;
    }

    m_IsRecursiveSelfNotify = true;

    if (Name)
    {
        m_Parent->SubmitCommand(new CommandSetCompVarT<std::string>(m_MapDoc->GetAdapter(), *Name, std::string(TE.GetLabel())));
    }

    m_IsRecursiveSelfNotify = false;

    // The command may well have set a name different from TE.GetLabel().
    TE.Veto();
    SetItemText(TE.GetItem(), GetEntityLabel(Entity));
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


void EntityHierarchyDialogT::OnTreeItemRightClick(wxTreeEvent& TE)
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
    AppendMI(Menu, ID_MENU_CREATE_ENTITY, "Create Entity", "MapEditor/tool-new-entity");
    AppendMI(Menu, ID_MENU_RENAME,        "Rename\tF2", "textfield_rename");
    Menu.AppendSeparator();
    AppendMI(Menu, wxID_CUT,                                  "Cut", wxART_CUT);
    AppendMI(Menu, wxID_COPY,                                 "Copy", wxART_COPY);
    AppendMI(Menu, wxID_PASTE,                                "Paste", wxART_PASTE);
    AppendMI(Menu, ChildFrameT::ID_MENU_EDIT_PASTE_SPECIAL,   "Paste Special...", "");
    AppendMI(Menu, ChildFrameT::ID_MENU_EDIT_DELETE,          "Delete", wxART_DELETE);
    Menu.AppendSeparator();
    AppendMI(Menu, ChildFrameT::ID_MENU_VIEW_CENTER_2D_VIEWS, "Center 2D Views on Selection", "");
    AppendMI(Menu, ChildFrameT::ID_MENU_VIEW_CENTER_3D_VIEWS, "Center 3D Views on Selection", "");

    Menu.UpdateUI(m_Parent);

    const int MenuSelID = GetPopupMenuSelectionFromUser(Menu);

    switch (MenuSelID)
    {
        case ID_MENU_CREATE_ENTITY:
        {
            IntrusivePtrT<cf::GameSys::EntityT> Parent = ((EntityTreeItemT*)GetItemData(TE.GetItem()))->GetEntity();
            IntrusivePtrT<cf::GameSys::EntityT> NewEnt = new cf::GameSys::EntityT(cf::GameSys::EntityCreateParamsT(m_MapDoc->GetScriptWorld()));
            IntrusivePtrT<CompMapEntityT>       MapEnt = new CompMapEntityT(*m_MapDoc);

            if (Parent->GetParent() == NULL)
            {
                NewEnt->GetTransform()->SetOriginWS(m_MapDoc->SnapToGrid(m_MapDoc->GetMostRecentSelBB().GetCenter(), false /*Toggle*/, -1 /*AxisNoSnap*/));
            }
            else
            {
                // NewEnt->GetBasics()->SetMember("Static", true);
                NewEnt->GetTransform()->SetOriginPS(Vector3fT(8, 8, 8));
            }

            NewEnt->SetApp(MapEnt);

            m_Parent->SubmitCommand(new CommandNewEntityT(
                *m_MapDoc,
                NewEnt,
                Parent,
                true /*select the new entity*/));
            break;
        }

        case ID_MENU_RENAME:
            EditLabel(TE.GetItem());
            break;

        default:
        {
            // This is neither elegant nor entirely right.
            //   - It is not elegant because of the awkward event forwarding to the ChildFrameT.
            //   - It is not entirely right because the context menu should (primarily) act on the tree event's item,
            //     i.e. `((EntityTreeItemT*)GetItemData(TE.GetItem()))->GetEntity()`, not on the general mapdoc's
            //     selection, as the ChildFrameT's menus do.
            //   - I'm not sure if duplicating functionality that is already in the ChildFrameT's main toolbars and
            //     menus makes sense.
            //     It seems better to only have functionality that is *specific* to the entity hierarchy here!
            //   - Well, this *was* done because the "Entity Hierarchy" dialog is intended as a substitute for the old
            //     (pre-component-system) "Entity Report", which had, beside several filter options, also buttons
            //     "Go to" and "Delete", which we reproduce here in an attempt to preserve the old dialog's set of
            //     features.
            wxCommandEvent CE(wxEVT_COMMAND_MENU_SELECTED, MenuSelID);

            m_Parent->GetEventHandler()->ProcessEvent(CE);
            break;
        }
    }
}


void EntityHierarchyDialogT::OnBeginDrag(wxTreeEvent& TE)
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


void EntityHierarchyDialogT::OnEndDrag(wxTreeEvent& TE)
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


/*****************************/
/*** EntityHierarchyModelT ***/
/*****************************/

EntityHierarchyModelT::EntityHierarchyModelT(IntrusivePtrT<cf::GameSys::EntityT> RootEntity)
    : m_RootEntity(RootEntity)
{
    wxASSERT(m_RootEntity != NULL);
    wxASSERT(m_RootEntity->GetParent() == NULL);
}


unsigned int EntityHierarchyModelT::GetColumnCount() const
{
    return NR_OF_COLUMNS;
}


wxString EntityHierarchyModelT::GetColumnType(unsigned int col) const
{
    switch (col)
    {
        case COLUMN_ENTITY_NAME:    return "string";
        case COLUMN_NUM_PRIMITIVES: return "string";    // Not "long", because we use "" for "0".
        case COLUMN_VISIBILITY:     return "wxBitmap";  // Would be a "bool" for "visibility".
        case COLUMN_SELECTION_MODE: return "wxBitmap";  // Would be a "long" for "selection mode".
        case NR_OF_COLUMNS: wxASSERT(false); break;
    }

    // Just a fallback, should never get here.
    return "string";
}


void EntityHierarchyModelT::GetValue(wxVariant& Variant, const wxDataViewItem& Item, unsigned int col) const
{
    cf::GameSys::EntityT* Entity = static_cast<cf::GameSys::EntityT*>(Item.GetID());

    if (!Entity) return;

    switch (col)
    {
        case COLUMN_ENTITY_NAME:
        {
            Variant = Entity->GetBasics()->GetEntityName();
            break;
        }

        case COLUMN_NUM_PRIMITIVES:
        {
            wxString NumPrims;
            NumPrims << GetMapEnt(Entity)->GetPrimitives().Size();

            Variant = (NumPrims == "0") ? wxString("") : NumPrims;
            break;
        }

        case COLUMN_VISIBILITY:
        {
            Variant << wxArtProvider::GetBitmap(Entity->GetBasics()->IsShown() ? "eye_open" : "eye_grey", wxART_MENU);
            break;
        }

        case COLUMN_SELECTION_MODE:
        {
            using namespace cf::GameSys;

            switch (Entity->GetBasics()->GetSelMode())
            {
                case ComponentBasicsT::SINGLE: Variant << wxBitmap("CaWE/res/GroupSelect-Indiv.png", wxBITMAP_TYPE_PNG); break;
                case ComponentBasicsT::GROUP:  Variant << wxBitmap("CaWE/res/GroupSelect-AsOne.png", wxBITMAP_TYPE_PNG); break;
                case ComponentBasicsT::LOCKED: Variant << wxArtProvider::GetBitmap("changes-prevent", wxART_MENU); break;
            }

            break;
        }

        default:
        {
            wxASSERT(false);
            break;
        }
    }
}


bool EntityHierarchyModelT::SetValue(const wxVariant& Variant, const wxDataViewItem& Item, unsigned int col)
{
    return false;   // ??? TODO!!!
}


wxDataViewItem EntityHierarchyModelT::GetParent(const wxDataViewItem& Item) const
{
    cf::GameSys::EntityT* Entity = static_cast<cf::GameSys::EntityT*>(Item.GetID());

    if (!Entity) return wxDataViewItem(NULL);

    return wxDataViewItem(Entity->GetParent().get());
}


bool EntityHierarchyModelT::IsContainer(const wxDataViewItem& Item) const
{
    cf::GameSys::EntityT* Entity = static_cast<cf::GameSys::EntityT*>(Item.GetID());

    // The wxDataViewCtrl's root item is always invisible and comes with `Item.GetID() == NULL`.
    // It can have children (in our case, exactly one, namely the `m_RootEntity`), and thus we
    // have to return `true` here.
    if (!Entity) return true;

    return Entity->GetChildren().Size() > 0;
}


bool EntityHierarchyModelT::HasContainerColumns(const wxDataViewItem& Item) const
{
    // Our "containers" are not only headlines or categories, but have the same set of
    // detail columns as our "items" -- they're all entities.
    return true;
}


unsigned int EntityHierarchyModelT::GetChildren(const wxDataViewItem& Item, wxDataViewItemArray& Children) const
{
    cf::GameSys::EntityT* Entity = static_cast<cf::GameSys::EntityT*>(Item.GetID());

    if (!Entity)
    {
        Children.Add(wxDataViewItem(m_RootEntity.get()));
    }
    else
    {
        for (unsigned int ChildNr = 0; ChildNr < Entity->GetChildren().Size(); ChildNr++)
            Children.Add(wxDataViewItem(Entity->GetChildren()[ChildNr].get()));
    }

    return Children.GetCount();
}


/****************************/
/*** EntityHierarchyCtrlT ***/
/****************************/

BEGIN_EVENT_TABLE(EntityHierarchyCtrlT, wxDataViewCtrl)
//    EVT_KEY_DOWN             (EntityHierarchyCtrlT::OnKeyDown)    // not needed, as wxDataViewCtrl handles F2 already by itself
//    EVT_LEFT_DOWN            (EntityHierarchyCtrlT::OnTreeLeftClick)
//    EVT_LEFT_DCLICK          (EntityHierarchyCtrlT::OnTreeLeftClick)   // Handle double clicks like normal left clicks when it comes to clicks on tree item icons (otherwise double clicks are handled normally).
    EVT_DATAVIEW_ITEM_ACTIVATED(wxID_ANY, EntityHierarchyCtrlT::OnItemActivated)

//  EVT_TREE_SEL_CHANGED          (wxID_ANY, EntityHierarchyCtrlT::OnSelectionChanged)  // done, in method below
    EVT_DATAVIEW_SELECTION_CHANGED(wxID_ANY, EntityHierarchyCtrlT::OnSelectionChanged)

//    EVT_TREE_BEGIN_LABEL_EDIT(wxID_ANY, EntityHierarchyCtrlT::OnBeginLabelEdit)   // not needed with wxDataViewCtrl
//    EVT_TREE_END_LABEL_EDIT  (wxID_ANY, EntityHierarchyCtrlT::OnEndLabelEdit)     // done, in method below
    EVT_DATAVIEW_ITEM_EDITING_DONE(wxID_ANY, EntityHierarchyCtrlT::OnEndItemEdit)

//    EVT_TREE_ITEM_RIGHT_CLICK(wxID_ANY, EntityHierarchyCtrlT::OnTreeItemRightClick)
    EVT_DATAVIEW_ITEM_CONTEXT_MENU(wxID_ANY, EntityHierarchyCtrlT::OnTreeItemContextMenu)

//    EVT_TREE_BEGIN_DRAG      (wxID_ANY, EntityHierarchyCtrlT::OnBeginDrag)
    EVT_DATAVIEW_ITEM_BEGIN_DRAG(wxID_ANY, EntityHierarchyCtrlT::OnBeginDrag)
    EVT_DATAVIEW_ITEM_DROP_POSSIBLE(wxID_ANY, EntityHierarchyCtrlT::OnCheckDrag)
//    EVT_TREE_END_DRAG        (wxID_ANY, EntityHierarchyCtrlT::OnEndDrag)
    EVT_DATAVIEW_ITEM_DROP(wxID_ANY, EntityHierarchyCtrlT::OnEndDrag)
END_EVENT_TABLE()


EntityHierarchyCtrlT::EntityHierarchyCtrlT(ChildFrameT* MainFrame, wxWindow* Parent)
    : wxDataViewCtrl(Parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_MULTIPLE | wxBORDER_NONE),
      m_MainFrame(MainFrame),
      m_MapDoc(MainFrame->GetDoc()),
      m_IsRecursiveSelfNotify(false),
      m_DraggedEntity(NULL)
{
    // Use wxObjectDataPtr<> as per http://docs.wxwidgets.org/trunk/classwx_data_view_model.html documentation.
    wxObjectDataPtr<EntityHierarchyModelT> EntHierModel(new EntityHierarchyModelT(m_MapDoc->GetRootMapEntity()->GetEntity()));

    AssociateModel(EntHierModel.get());

    AppendTextColumn("Name",  EntityHierarchyModelT::COLUMN_ENTITY_NAME,    wxDATAVIEW_CELL_EDITABLE, 180);
    AppendTextColumn("#P",    EntityHierarchyModelT::COLUMN_NUM_PRIMITIVES, wxDATAVIEW_CELL_INERT, wxDVC_DEFAULT_MINWIDTH, wxALIGN_RIGHT,  0);
    AppendBitmapColumn("Vis", EntityHierarchyModelT::COLUMN_VISIBILITY,     wxDATAVIEW_CELL_INERT, wxDVC_DEFAULT_MINWIDTH, wxALIGN_CENTER, 0);
    AppendBitmapColumn("SM",  EntityHierarchyModelT::COLUMN_SELECTION_MODE, wxDATAVIEW_CELL_INERT, wxDVC_DEFAULT_MINWIDTH, wxALIGN_CENTER, 0);

    Expand(wxDataViewItem(m_MapDoc->GetRootMapEntity()->GetEntity()));

    EnableDragSource(wxDF_UNICODETEXT);
    EnableDropTarget(wxDF_UNICODETEXT);

    m_MapDoc->RegisterObserver(this);
}


EntityHierarchyCtrlT::~EntityHierarchyCtrlT()
{
    if (m_MapDoc) m_MapDoc->UnregisterObserver(this);
}


void EntityHierarchyCtrlT::NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection)
{
    if (m_IsRecursiveSelfNotify) return;

    // The following consideration was noted for wxTreeCtrl. It probably does not apply to wxDataViewCtrl,
    // but keeping the code doesn't hurt:
    //     Both UnselectAll() and SelectItem() result in EVT_TREE_SEL_CHANGED event,
    //     see <http://thread.gmane.org/gmane.comp.lib.wxwidgets.general/72754> for details.
    //     m_IsRecursiveSelfNotify makes sure that the tree isn't unintentionally updated recursively.
    m_IsRecursiveSelfNotify = true;

    wxDataViewItemArray SelectedItems;

    for (unsigned long NewSelNr = 0; NewSelNr < NewSelection.Size(); NewSelNr++)
    {
        // If the selected element is something other than an entity representation
        // (that is, a map primitive), don't consider it.
        if (NewSelection[NewSelNr]->GetParent()->GetRepres() != NewSelection[NewSelNr]) continue;

        SelectedItems.Add(wxDataViewItem(NewSelection[NewSelNr]->GetParent()->GetEntity()));
    }

    SetSelections(SelectedItems);

    if (SelectedItems.GetCount() > 0)
        EnsureVisible(SelectedItems.Last());

    m_IsRecursiveSelfNotify = false;
}


void EntityHierarchyCtrlT::NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities)
{
    if (m_IsRecursiveSelfNotify) return;

    GetModel()->Cleared();  // The control will re-read the data from the model.
}


void EntityHierarchyCtrlT::NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT<MapPrimitiveT*>& Primitives)
{
    if (m_IsRecursiveSelfNotify) return;

    for (unsigned int PrimNr = 0; PrimNr < Primitives.Size(); PrimNr++)
    {
        wxASSERT(Primitives[PrimNr]->GetParent() != NULL);
        GetModel()->ValueChanged(wxDataViewItem(Primitives[PrimNr]->GetParent()->GetEntity()), EntityHierarchyModelT::COLUMN_NUM_PRIMITIVES);
    }
}


void EntityHierarchyCtrlT::NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities)
{
    if (m_IsRecursiveSelfNotify) return;

    GetModel()->Cleared();  // The control will re-read the data from the model.
}


void EntityHierarchyCtrlT::NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT<MapPrimitiveT*>& Primitives)
{
    if (m_IsRecursiveSelfNotify) return;

    // Unfortunately, the deleted primitives are no longer attached to their parent entity, that is,
    // `Primitives[PrimNr]->GetParent() == NULL`, so (at this time) we have to update the entire hierarchy.
    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > AllEnts;

    m_MapDoc->GetRootMapEntity()->GetEntity()->GetAll(AllEnts);

    for (unsigned int EntNr = 0; EntNr < AllEnts.Size(); EntNr++)
        GetModel()->ValueChanged(wxDataViewItem(AllEnts[EntNr].get()), EntityHierarchyModelT::COLUMN_NUM_PRIMITIVES);
}


void EntityHierarchyCtrlT::NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail)
{
    if (m_IsRecursiveSelfNotify) return;

    if (Detail == MEMD_ASSIGN_PRIM_TO_ENTITY)
    {
        for (unsigned int ElemNr = 0; ElemNr < MapElements.Size(); ElemNr++)
        {
            wxASSERT(MapElements[ElemNr]->GetParent() != NULL);
            GetModel()->ValueChanged(wxDataViewItem(MapElements[ElemNr]->GetParent()->GetEntity()), EntityHierarchyModelT::COLUMN_NUM_PRIMITIVES);
        }
    }
}


void EntityHierarchyCtrlT::Notify_EntChanged(SubjectT* Subject, const ArrayT< IntrusivePtrT<MapEditor::CompMapEntityT> >& Entities, EntityModDetailE Detail)
{
    if (m_IsRecursiveSelfNotify) return;
    if (Detail != EMD_HIERARCHY) return;

    GetModel()->Cleared();  // The control will re-read the data from the model.

    // After a completed drag-and-drop event (see OnEndDrag() below), the wxDataViewCtrl does not
    // realize that the parent of the dragged entity should be expanded and the dragged entity is
    // still selected. Thus, update the selection status here explicitly.
    NotifySubjectChanged_Selection(Subject, ArrayT<MapElementT*>(), m_MapDoc->GetSelection());
}


void EntityHierarchyCtrlT::Notify_VarChanged(SubjectT* Subject, const cf::TypeSys::VarBaseT& Var)
{
    if (m_IsRecursiveSelfNotify) return;

    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > AllEnts;

    m_MapDoc->GetRootMapEntity()->GetEntity()->GetAll(AllEnts);

    for (unsigned int EntNr = 0; EntNr < AllEnts.Size(); EntNr++)
    {
        IntrusivePtrT<cf::GameSys::ComponentBasicsT> Basics = AllEnts[EntNr]->GetBasics();

        if (&Var == Basics->GetMemberVars().Find("Name"))
        {
            GetModel()->ValueChanged(wxDataViewItem(AllEnts[EntNr].get()), EntityHierarchyModelT::COLUMN_ENTITY_NAME);
            return;
        }

        if (&Var == Basics->GetMemberVars().Find("Show"))
        {
            GetModel()->ValueChanged(wxDataViewItem(AllEnts[EntNr].get()), EntityHierarchyModelT::COLUMN_VISIBILITY);
            return;
        }

        if (&Var == Basics->GetMemberVars().Find("SelMode"))
        {
            GetModel()->ValueChanged(wxDataViewItem(AllEnts[EntNr].get()), EntityHierarchyModelT::COLUMN_SELECTION_MODE);
            return;
        }
    }
}


void EntityHierarchyCtrlT::NotifySubjectDies(SubjectT* dyingSubject)
{
    wxASSERT(dyingSubject == m_MapDoc);

    AssociateModel(NULL);
    m_MapDoc = NULL;
}


void EntityHierarchyCtrlT::OnItemActivated(wxDataViewEvent& Event)
{
    // wxMessageBox(wxString::Format("Item activated, col %i", Event.GetColumn()));
    cf::GameSys::EntityT* Entity = static_cast<cf::GameSys::EntityT*>(Event.GetItem().GetID());

    if (!Entity) return;

    switch (Event.GetColumn())
    {
        case EntityHierarchyModelT::COLUMN_VISIBILITY:
        {
            cf::TypeSys::VarT<bool>* Show = dynamic_cast<cf::TypeSys::VarT<bool>*>(Entity->GetBasics()->GetMemberVars().Find("Show"));

            if (Show)
            {
                m_MainFrame->SubmitCommand(new CommandSetCompVarT<bool>(m_MapDoc->GetAdapter(), *Show, !Show->Get()));
            }

            break;
        }

        case EntityHierarchyModelT::COLUMN_SELECTION_MODE:
        {
            cf::TypeSys::VarT<int>* SelMode = dynamic_cast<cf::TypeSys::VarT<int>*>(Entity->GetBasics()->GetMemberVars().Find("SelMode"));

            if (SelMode)
            {
                using namespace cf::GameSys;

                m_MainFrame->SubmitCommand(new CommandSetCompVarT<int>(m_MapDoc->GetAdapter(), *SelMode,
                    SelMode->Get() == ComponentBasicsT::SINGLE ? ComponentBasicsT::GROUP :
                    SelMode->Get() == ComponentBasicsT::GROUP  ? ComponentBasicsT::LOCKED :
                                                                 ComponentBasicsT::SINGLE));
            }

            break;
        }
    }
}


void EntityHierarchyCtrlT::OnSelectionChanged(wxDataViewEvent& Event)
{
    if (m_MapDoc == NULL || m_IsRecursiveSelfNotify) return;

    m_IsRecursiveSelfNotify = true;

    // Get the currently selected tree items and update the document selection accordingly.
    wxDataViewItemArray SelectedItems;
    GetSelections(SelectedItems);

    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > NewSelection;

    for (size_t SelNr = 0; SelNr < SelectedItems.GetCount(); SelNr++)
    {
        cf::GameSys::EntityT* Entity = static_cast<cf::GameSys::EntityT*>(SelectedItems[SelNr].GetID());

        wxASSERT(Entity);
        NewSelection.PushBack(Entity);
    }

    m_MainFrame->SubmitCommand(CommandSelectT::Set(m_MapDoc, NewSelection, false /*WithEntPrims*/));

    m_IsRecursiveSelfNotify = false;
}


void EntityHierarchyCtrlT::OnEndItemEdit(wxDataViewEvent& Event)
{
    if (Event.GetColumn() != EntityHierarchyModelT::COLUMN_ENTITY_NAME) return;
    if (Event.IsEditCancelled()) return;

    cf::GameSys::EntityT* Entity = static_cast<cf::GameSys::EntityT*>(Event.GetItem().GetID());

    if (!Entity) return;

    cf::TypeSys::VarT<std::string>* Name = dynamic_cast<cf::TypeSys::VarT<std::string>*>(Entity->GetBasics()->GetMemberVars().Find("Name"));

    m_IsRecursiveSelfNotify = true;

    if (Name)
    {
        m_MainFrame->SubmitCommand(new CommandSetCompVarT<std::string>(m_MapDoc->GetAdapter(), *Name, std::string(Event.GetValue().GetString())));
    }

    m_IsRecursiveSelfNotify = false;

    // The command may well have set a name different from Event.GetValue().
    Event.Veto();
    GetModel()->ValueChanged(Event.GetItem(), EntityHierarchyModelT::COLUMN_ENTITY_NAME);
}


void EntityHierarchyCtrlT::OnTreeItemContextMenu(wxDataViewEvent& Event)
{
    cf::GameSys::EntityT* Entity = static_cast<cf::GameSys::EntityT*>(Event.GetItem().GetID());

    if (!Entity) return;

    // Note that GetPopupMenuSelectionFromUser() temporarily disables UI updates for the entity,
    // so our menu IDs used below should be doubly clash-free.
    enum
    {
        ID_MENU_CREATE_ENTITY = wxID_HIGHEST + 1,
        ID_MENU_RENAME
    };

    wxMenu Menu;

    // Create context menus.
    AppendMI(Menu, ID_MENU_CREATE_ENTITY, "Create Entity", "MapEditor/tool-new-entity");
    AppendMI(Menu, ID_MENU_RENAME,        "Rename\tF2", "textfield_rename");
    Menu.AppendSeparator();
    AppendMI(Menu, wxID_CUT,                                  "Cut", wxART_CUT);
    AppendMI(Menu, wxID_COPY,                                 "Copy", wxART_COPY);
    AppendMI(Menu, wxID_PASTE,                                "Paste", wxART_PASTE);
    AppendMI(Menu, ChildFrameT::ID_MENU_EDIT_PASTE_SPECIAL,   "Paste Special...", "");
    AppendMI(Menu, ChildFrameT::ID_MENU_EDIT_DELETE,          "Delete", wxART_DELETE);
    Menu.AppendSeparator();
    AppendMI(Menu, ChildFrameT::ID_MENU_VIEW_CENTER_2D_VIEWS, "Center 2D Views on Selection", "");
    AppendMI(Menu, ChildFrameT::ID_MENU_VIEW_CENTER_3D_VIEWS, "Center 3D Views on Selection", "");

    Menu.UpdateUI(m_MainFrame);

    const int MenuSelID = GetPopupMenuSelectionFromUser(Menu);

    switch (MenuSelID)
    {
        case ID_MENU_CREATE_ENTITY:
        {
            IntrusivePtrT<cf::GameSys::EntityT> Parent = Entity;
            IntrusivePtrT<cf::GameSys::EntityT> NewEnt = new cf::GameSys::EntityT(cf::GameSys::EntityCreateParamsT(m_MapDoc->GetScriptWorld()));
            IntrusivePtrT<CompMapEntityT>       MapEnt = new CompMapEntityT(*m_MapDoc);

            if (Parent->GetParent() == NULL)
            {
                NewEnt->GetTransform()->SetOriginWS(m_MapDoc->SnapToGrid(m_MapDoc->GetMostRecentSelBB().GetCenter(), false /*Toggle*/, -1 /*AxisNoSnap*/));
            }
            else
            {
                // NewEnt->GetBasics()->SetMember("Static", true);
                NewEnt->GetTransform()->SetOriginPS(Vector3fT(8, 8, 8));
            }

            NewEnt->SetApp(MapEnt);

            m_MainFrame->SubmitCommand(new CommandNewEntityT(
                *m_MapDoc,
                NewEnt,
                Parent,
                true /*select the new entity*/));
            break;
        }

        case ID_MENU_RENAME:
            EditItem(Event.GetItem(), GetColumn(EntityHierarchyModelT::COLUMN_ENTITY_NAME));
            break;

        default:
        {
            // This is neither elegant nor entirely right.
            //   - It is not elegant because of the awkward event forwarding to the ChildFrameT.
            //   - It is not entirely right because the context menu should (primarily) act on the tree event's item,
            //     i.e. `((EntityTreeItemT*)GetItemData(TE.GetItem()))->GetEntity()`, not on the general mapdoc's
            //     selection, as the ChildFrameT's menus do.
            //   - I'm not sure if duplicating functionality that is already in the ChildFrameT's main toolbars and
            //     menus makes sense.
            //     It seems better to only have functionality that is *specific* to the entity hierarchy here!
            //   - Well, this *was* done because the "Entity Hierarchy" dialog is intended as a substitute for the old
            //     (pre-component-system) "Entity Report", which had, beside several filter options, also buttons
            //     "Go to" and "Delete", which we reproduce here in an attempt to preserve the old dialog's set of
            //     features.
            wxCommandEvent CE(wxEVT_COMMAND_MENU_SELECTED, MenuSelID);

            m_MainFrame->GetEventHandler()->ProcessEvent(CE);
            break;
        }
    }
}


void EntityHierarchyCtrlT::OnBeginDrag(wxDataViewEvent& Event)
{
    wxASSERT(m_DraggedEntity == NULL);

    if (GetSelectedItemsCount() > 1)
    {
        wxMessageBox("Sorry, you can only drag one entity at a time.");
        Event.Veto();
        return;
    }

    cf::GameSys::EntityT* Entity = static_cast<cf::GameSys::EntityT*>(Event.GetItem().GetID());

    if (!Entity)   // Should never happen.
    {
        wxMessageBox("This entity cannot be dragged.");
        Event.Veto();
        return;
    }

    if (Entity->GetParent() == NULL)
    {
        wxMessageBox("The root entity cannot be dragged.");
        Event.Veto();
        return;
    }

    Event.SetDataObject(new wxTextDataObject);
    Event.SetDragFlags(wxDrag_DefaultMove);

    m_DraggedEntity = Entity;
}


void EntityHierarchyCtrlT::OnCheckDrag(wxDataViewEvent& Event)
{
    if (Event.GetDataFormat() != wxDF_UNICODETEXT)
        Event.Veto();
}


void EntityHierarchyCtrlT::OnEndDrag(wxDataViewEvent& Event)
{
    wxASSERT(!m_DraggedEntity.IsNull());
    if (m_DraggedEntity.IsNull()) return;

    IntrusivePtrT<cf::GameSys::EntityT> SourceEntity = m_DraggedEntity;
    m_DraggedEntity = NULL;

    IntrusivePtrT<cf::GameSys::EntityT> TargetEntity = static_cast<cf::GameSys::EntityT*>(Event.GetItem().GetID());
    if (TargetEntity.IsNull()) return;

    // If SourceEntity is already an immediate child of TargetEntity, do nothing.
    if (SourceEntity->GetParent() == TargetEntity) return;

    // If SourceEntity is dragged into its own subtree, do nothing.
    // That is, make sure that TargetEntity is not in the subtree of SourceEntity (or else the reparenting
    // would create invalid cycles).
    // Note that the TargetEntity can still be a child in a different subtree of SourceEntity->Parent.
    {
        ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > SubTree;

        SourceEntity->GetAll(SubTree);

        if (SubTree.Find(TargetEntity) >= 0) return;
    }

    // Make SourceEntity a child of TargetEntity.
    const unsigned long NewPos = TargetEntity->GetChildren().Size();

    m_MainFrame->SubmitCommand(new CommandChangeEntityHierarchyT(m_MapDoc, SourceEntity, TargetEntity, NewPos));
}


/*****************************/
/*** EntityHierarchyPanelT ***/
/*****************************/

BEGIN_EVENT_TABLE(EntityHierarchyPanelT, wxPanel)
//     EVT_BUTTON(ID_BUTTON_ADD,    EntityHierarchyPanelT::OnButton)
//     EVT_BUTTON(ID_BUTTON_UP,     EntityHierarchyPanelT::OnButton)
//     EVT_BUTTON(ID_BUTTON_DOWN,   EntityHierarchyPanelT::OnButton)
//     EVT_BUTTON(ID_BUTTON_DELETE, EntityHierarchyPanelT::OnButton)
//     EVT_UPDATE_UI_RANGE(ID_BUTTON_ADD, ID_BUTTON_DELETE, EntityHierarchyPanelT::OnButtonUpdate)
END_EVENT_TABLE()


EntityHierarchyPanelT::EntityHierarchyPanelT(ChildFrameT* MainFrame, const wxSize& Size)
    : wxPanel(MainFrame, -1, wxDefaultPosition, Size),
      m_MainFrame(MainFrame),
      m_OldTreeCtrl(NULL),
      m_TreeCtrl(NULL)
{
    // As we are a wxAUI pane rather than a wxDialog, explicitly set that events are not propagated to our parent.
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);

    wxBoxSizer* item0 = new wxBoxSizer( wxVERTICAL );

    // m_OldTreeCtrl = new EntityHierarchyDialogT(m_MainFrame, this);
    // item0->Add(m_OldTreeCtrl, 1, wxEXPAND | wxALL, 5 );

    m_TreeCtrl = new EntityHierarchyCtrlT(m_MainFrame, this);
    item0->Add(m_TreeCtrl, 1, wxEXPAND | wxALL, 0 );

    this->SetSizer( item0 );
    item0->SetSizeHints(this);
}
