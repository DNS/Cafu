/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "DialogEntityHierarchy.hpp"
#include "ChildFrame.hpp"
#include "CompMapEntity.hpp"
#include "Group.hpp"
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
    EVT_TREE_SEL_CHANGING    (wxID_ANY, EntityHierarchyDialogT::OnSelectionChanging)
    EVT_TREE_SEL_CHANGED     (wxID_ANY, EntityHierarchyDialogT::OnSelectionChanged)
    EVT_TREE_BEGIN_LABEL_EDIT(wxID_ANY, EntityHierarchyDialogT::OnBeginLabelEdit)
    EVT_TREE_END_LABEL_EDIT  (wxID_ANY, EntityHierarchyDialogT::OnEndLabelEdit)
    EVT_TREE_ITEM_GETTOOLTIP (wxID_ANY, EntityHierarchyDialogT::OnGetTooltip)
    EVT_TREE_ITEM_RIGHT_CLICK(wxID_ANY, EntityHierarchyDialogT::OnTreeItemRightClick)
    EVT_TREE_BEGIN_DRAG      (wxID_ANY, EntityHierarchyDialogT::OnBeginDrag)
    EVT_TREE_END_DRAG        (wxID_ANY, EntityHierarchyDialogT::OnEndDrag)
END_EVENT_TABLE()


EntityHierarchyDialogT::EntityHierarchyDialogT(ChildFrameT* Parent, wxWindow* WinParent)
    : wxTreeCtrl(WinParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS|wxTR_LINES_AT_ROOT|wxTR_MULTIPLE|wxBORDER_NONE|wxTR_EDIT_LABELS),
      m_MapDoc(Parent->GetDoc()),
      m_Parent(Parent),
      m_IsRecursiveSelfNotify(false),
      m_DraggedEntities()
{
    // // Build list of entity hierarchy icons.
    // wxImageList* TreeIcons = new wxImageList(16, 16);
    //
    // TreeIcons->Add(wxBitmap("CaWE/res/checked.png", wxBITMAP_TYPE_PNG));
    // TreeIcons->Add(wxBitmap("CaWE/res/unchecked.png", wxBITMAP_TYPE_PNG));
    //
    // AssignImageList(TreeIcons);   // Note: wxTreeCtrl takes ownership of this list and deletes it on window destroy.

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

        // TODO: This should rather be an entity-specific icon, e.g. one that represents the entity's first/topmost component:
        // SetItemImage(ID, Children[i]->GetBasics()->IsShown() ? 0 : 1);  // Set the "is visible" or "is invisible" icon.

        if (Recursive) AddChildren(ID, true);
    }
}


const wxTreeItemId EntityHierarchyDialogT::FindTreeItem(const wxTreeItemId& StartingItem, IntrusivePtrT<cf::GameSys::EntityT> Entity) const
{
    // If the item to start with is invalid, return it so the result of this function call is invalid, too.
    if (!StartingItem.IsOk()) return StartingItem;

    // Starting item is valid, so check if it is related to the Entity.
    EntityTreeItemT* EntityItem = (EntityTreeItemT*)GetItemData(StartingItem);

    if (EntityItem->GetEntity() == Entity) return StartingItem;

    // No match, so recursively check all the children.
    wxTreeItemIdValue Cookie;
    wxTreeItemId      ChildItem;

    for (ChildItem = GetFirstChild(StartingItem, Cookie); ChildItem.IsOk(); ChildItem = GetNextChild(StartingItem, Cookie))
    {
        const wxTreeItemId Result = FindTreeItem(ChildItem, Entity);

        if (Result.IsOk()) return Result;
    }

    wxASSERT(!ChildItem.IsOk());
    return ChildItem;
}


void EntityHierarchyDialogT::GetTreeItems(const wxTreeItemId& StartingItem, ArrayT<wxTreeItemId>& Items)
{
    if (!StartingItem.IsOk()) return;

    Items.PushBack(StartingItem);

    wxTreeItemIdValue Cookie;
    wxTreeItemId      ChildItem;

    for (ChildItem = GetFirstChild(StartingItem, Cookie); ChildItem.IsOk(); ChildItem = GetNextChild(StartingItem, Cookie))
    {
        GetTreeItems(ChildItem, Items);
    }
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


void EntityHierarchyDialogT::UpdateAllGroupColors()
{
    ArrayT<wxTreeItemId> TreeItems;

    GetTreeItems(GetRootItem(), TreeItems);

 // const wxColour DefaultBack = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX);
    const wxColour DefaultText = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT);
    const wxColour GrayText    = wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT);

    for (unsigned long ItemNr = 0; ItemNr < TreeItems.Size(); ItemNr++)
    {
        IntrusivePtrT<cf::GameSys::EntityT> Entity = ((EntityTreeItemT*)GetItemData(TreeItems[ItemNr]))->GetEntity();
        MapEntRepresT*                      Repres = GetMapEnt(Entity)->GetRepres();
        GroupT*                             Group  = Repres->GetGroup();

        if (!Repres->IsVisible() || !Repres->CanSelect())
        {
            SetItemTextColour(TreeItems[ItemNr], GrayText);
        }
        else
        {
            SetItemTextColour(TreeItems[ItemNr], Group ? Group->Color : DefaultText);
        }
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


void EntityHierarchyDialogT::NotifySubjectChanged_Groups(SubjectT* Subject)
{
    UpdateAllGroupColors();
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

        // if (&Var == Basics->GetMemberVars().Find("Show"))
        // {
        //     SetItemImage(TreeItems[ItemNr], Basics->IsShown() ? 0 : 1);   // Set the "is visible" or "is invisible" icon.
        //     return;
        // }
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

    // TODO: This should rather be an entity-specific icon, e.g. one that represents the entity's first/topmost component:
    // SetItemImage(ID, RootEntity->GetBasics()->IsShown() ? 0 : 1);   // Set the "is visible" or "is invisible" icon.

    // Add all children of root recursively to the tree.
    AddChildren(ID, true);

    // And properly initialize their group colors.
    UpdateAllGroupColors();

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

    // Make sure that the selected elements are visible.
    for (unsigned long SelNr = 0; SelNr < Selection.Size(); SelNr++)
    {
        wxTreeItemId Result = FindTreeItem(ID, Selection[SelNr]);

        if (Result.IsOk())
            EnsureVisible(Result);
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

    // If an icon was hit, toggle visibility of the associated entity.
    if (HitFlag & wxTREE_HITTEST_ONITEMICON)
    {
        IntrusivePtrT<cf::GameSys::EntityT> ClickedEntity = ((EntityTreeItemT*)GetItemData(ClickedItem))->GetEntity();
        cf::TypeSys::VarT<bool>* Show = dynamic_cast<cf::TypeSys::VarT<bool>*>(ClickedEntity->GetBasics()->GetMemberVars().Find("Show"));

        m_IsRecursiveSelfNotify = true;

        if (Show)
        {
            m_Parent->SubmitCommand(new CommandSetCompVarT<bool>(m_MapDoc->GetAdapter(), *Show, !Show->Get()));
            // SetItemImage(ClickedItem, Show->Get() ? 0 : 1);   // Set the "is visible" or "is invisible" icon.
        }

        m_IsRecursiveSelfNotify = false;
    }
    else
    {
        // Skip event if no icon was hit to preserve wxTreeCtrl functionality.
        ME.Skip();
    }
}


void EntityHierarchyDialogT::OnSelectionChanging(wxTreeEvent& TE)
{
    if (!TE.GetItem().IsOk()) return;

    IntrusivePtrT<cf::GameSys::EntityT> Entity = ((EntityTreeItemT*)GetItemData(TE.GetItem()))->GetEntity();
    MapEntRepresT* Repres = GetMapEnt(Entity)->GetRepres();

    if (!Repres->IsVisible() || !Repres->CanSelect())
    {
        TE.Veto();
    }
}


void EntityHierarchyDialogT::OnSelectionChanged(wxTreeEvent& TE)
{
    if (m_MapDoc == NULL || m_IsRecursiveSelfNotify) return;

    // Note that the code below may well come up with a selection that is *different* from the
    // given selection in this control. For example,
    //   - if "Auto-Group Entities" is enabled, more entities, or
    //   - if a range of entities has been selected with a Shift-click and some of
    //     them are hidden or locked, fewer entities
    // may actually be selected than have been clicked and are currently highlighted by this
    // control.
    // Therefore, we *want* to have the CommandSelectTs below cause calls back to
    // NotifySubjectChanged_Selection() in order to have the control's selection updated
    // according to the true selection in the m_MapDoc.
    // m_IsRecursiveSelfNotify = true;      // Intentionally not used here.

    // Get the currently selected tree items and update the document selection accordingly.
    wxArrayTreeItemIds SelectedItems;
    GetSelections(SelectedItems);

    ArrayT<MapElementT*> RemoveFromSel;
    ArrayT<MapElementT*> AddToSel;

    // What we do here is quite good, but still not perfect. This is mostly due to the Shift key,
    // which enables "range selections" in the tree control, which are difficult to handle:
    //   - it can select hidden or locked entities despite our check in OnSelectionChanging(),
    //   - it creates a complex situation if Control is pressed as well.
    //
    // Possible solutions:
    //   - tell the user to not use the Shift key,
    //   - try to consume or veto the Shift key in OnKeyDown(),
    //   - re-implement the entire selection handling "manually" in OnTreeLeftClick(),
    //   - don't take the control's selection as authoritative, but have it updated to reflect
    //     the "true" selection by a subsequent call to NotifySubjectChanged_Selection().
    //
    // As it happens, the last item is already implemented, also for other reasons as explained
    // above, and seems to solve the problem quite thoroughly and well.
    if (!wxGetKeyState(WXK_CONTROL))    // TE.GetKeyEvent() seems not to be available.
    {
        // Control is not down, so we're free to re-set the selection from scratch.
        // This is roughly analogous to the Selection tool's "dragged rectangle" selection.
        for (size_t SelNr = 0; SelNr < SelectedItems.GetCount(); SelNr++)
        {
            IntrusivePtrT<cf::GameSys::EntityT> Entity = ((EntityTreeItemT*)GetItemData(SelectedItems[SelNr]))->GetEntity();
            MapEntRepresT*                      Repres = GetMapEnt(Entity)->GetRepres();

            if (!Repres->IsVisible() || !Repres->CanSelect())
            {
                // This is already checked for in OnSelectionChanging(), but as it doesn't cover multiple selections
                // as are possible with the Shift key, we have to repeat the check here. The subsequent call to
                // NotifySubjectChanged_Selection() will update our wxTreeCtrl to reflect the true selection.
                continue;
            }

            Repres->GetToggleEffects(RemoveFromSel, AddToSel, m_MapDoc->GetAutoGroupEntities());
        }

        // Set a new selection from scratch (Control is not down).
        AddToSel.PushBack(RemoveFromSel);

        m_Parent->SubmitCommand(CommandSelectT::Set(m_MapDoc, AddToSel));
    }
    else
    {
        // Only toggle the item returned by TE.GetItem().
        // Handling the general case, the toggling of multiple items, might be possible by analyzing the difference
        // between the previous selection (m_MapDoc->GetSelectedEntities()) and the new selection (SelectedItems),
        // and toggling only the entities that are found to have changed. Alas, this seems to be very complicated and
        // cumbersome for a case that probably never occurs in practice...
        IntrusivePtrT<cf::GameSys::EntityT> Entity = ((EntityTreeItemT*)GetItemData(TE.GetItem()))->GetEntity();
        MapEntRepresT*                      Repres = GetMapEnt(Entity)->GetRepres();

        wxASSERT(IsSelected(TE.GetItem()) != Repres->IsSelected());

        Repres->GetToggleEffects(RemoveFromSel, AddToSel, m_MapDoc->GetAutoGroupEntities());

        if (RemoveFromSel.Size() > 0) m_Parent->SubmitCommand(CommandSelectT::Remove(m_MapDoc, RemoveFromSel));
        if (AddToSel.Size()      > 0) m_Parent->SubmitCommand(CommandSelectT::Add   (m_MapDoc, AddToSel));

        wxASSERT(IsSelected(TE.GetItem()) == Repres->IsSelected());
    }

    // m_IsRecursiveSelfNotify = false;     // See above.
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


void EntityHierarchyDialogT::OnGetTooltip(wxTreeEvent& TE)
{
    if (!TE.GetItem().IsOk()) return;

    IntrusivePtrT<cf::GameSys::EntityT> Entity = ((EntityTreeItemT*)GetItemData(TE.GetItem()))->GetEntity();
    MapEntRepresT* Repres = GetMapEnt(Entity)->GetRepres();

    if (!Repres->IsVisible() || !Repres->CanSelect())
    {
        GroupT*  Group   = Repres->GetGroup();
        wxString Tooltip = wxString::Format("This entity is in group \"%s\", which is currently", Group ? Group->Name : "???");

        if (!Repres->IsVisible()) Tooltip += " hidden";
        if (!Repres->IsVisible() && !Repres->CanSelect()) Tooltip += " and";
        if (!Repres->CanSelect()) Tooltip += " locked";

        Tooltip += ".\n";
        Tooltip += "Update the group settings in order to select the entity.";

        TE.SetToolTip(Tooltip);
    }
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
        ID_MENU_RENAME,
        ID_MENU_MOVE_ENTITY_UP,
        ID_MENU_MOVE_ENTITY_DOWN
    };

    wxMenu Menu;

    // Create context menus.
    AppendMI(Menu, ID_MENU_CREATE_ENTITY, "Create Entity", "MapEditor/tool-new-entity");
    AppendMI(Menu, ID_MENU_RENAME,        "Rename\tF2", "textfield_rename");
    Menu.AppendSeparator();
    AppendMI(Menu, ID_MENU_MOVE_ENTITY_UP,   "Move Entity Up",   "list-selection-up");
    AppendMI(Menu, ID_MENU_MOVE_ENTITY_DOWN, "Move Entity Down", "list-selection-down");
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

        case ID_MENU_MOVE_ENTITY_UP:
        case ID_MENU_MOVE_ENTITY_DOWN:
        {
            ArrayT<MapElementT*> Selection = m_MapDoc->GetSelection();

            // Remove all entities (and primitives) whose parents are in the selection as well.
            MapDocumentT::Reduce(Selection);

            // Remove all remaining primitives from the Selection, keep only the entities.
            for (unsigned int SelNr = 0; SelNr < Selection.Size(); SelNr++)
            {
                if (Selection[SelNr]->GetType() != &MapEntRepresT::TypeInfo)
                {
                    Selection.RemoveAt(SelNr);
                    SelNr--;
                }
            }

            if (Selection.Size() == 0)
            {
                wxMessageBox("Select an entity first.\n\nIn order to move an entity, you must select it first.");
                break;
            }

            if (Selection.Size() > 1)
            {
                wxMessageBox("Only one entity can be moved at a time.");
                break;
            }

            const IntrusivePtrT<cf::GameSys::EntityT> SelEnt = Selection[0]->GetParent()->GetEntity();

            if (SelEnt->GetParent().IsNull())
            {
                wxMessageBox("The map's topmost (root) entity cannot be moved.");
                break;
            }

            const int NewPos = SelEnt->GetParent()->GetChildren().Find(SelEnt) + (MenuSelID == ID_MENU_MOVE_ENTITY_UP ? -1 : 1);

            if (NewPos < 0)
            {
                wxMessageBox("This entity is already the first child of its parent.\n\nUse drag-and-drop or cut-and-paste if you would like to assign the entity to another parent.");
                break;
            }

            if (NewPos >= int(SelEnt->GetParent()->GetChildren().Size()))
            {
                wxMessageBox("This entity is already the last child of its parent.\n\nUse drag-and-drop or cut-and-paste if you would like to assign the entity to another parent.");
                break;
            }

            m_Parent->SubmitCommand(new CommandChangeEntityHierarchyT(m_MapDoc, SelEnt, SelEnt->GetParent(), NewPos));
            break;
        }

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
    wxASSERT(m_DraggedEntities.Size() == 0);

    ArrayT<MapElementT*> Selection = m_MapDoc->GetSelection();

    // Remove all entities (and primitives) whose parents are in the selection as well.
    MapDocumentT::Reduce(Selection);

    // Copy all remaining entities into m_DraggedEntities (skipping any primitives).
    for (unsigned int SelNr = 0; SelNr < Selection.Size(); SelNr++)
    {
        if (Selection[SelNr]->GetType() == &MapEntRepresT::TypeInfo)
        {
            // Double-check that this is really a MapEntRepresT.
            wxASSERT(Selection[SelNr]->GetParent()->GetRepres() == Selection[SelNr]);

            m_DraggedEntities.PushBack(Selection[SelNr]->GetParent()->GetEntity());
        }
    }

    if (m_DraggedEntities.Size() == 0)
    {
        return;
    }

    // The entity that is related to TE is somewhat unrelated to our current selection.
    // However, if we start a drag at a previously unselected entity, the wxTreeCtrl
    // preceeds the begin of the drag with an update of the selection. As a result, the
    // entity [ ((EntityTreeItemT*)GetItemData(TE.GetItem()))->GetEntity() ] *should* be
    // in (the hierarchies of) the m_DraggedEntities, but we don't bother checking it.
    if (!TE.GetItem().IsOk())   // Should never happen.
    {
        wxMessageBox("Sorry, this entity cannot be dragged.");
        m_DraggedEntities.Clear();
        return;
    }

    // There is no need for this check, because entities can generally not be dragged "into" their children.
    // if (TE.GetItem() == GetRootItem())
    // {
    //     wxMessageBox("Sorry, the root entity cannot be dragged.");
    //     m_DraggedEntities.Clear();
    //     return;
    // }

    TE.Allow();
}


void EntityHierarchyDialogT::OnEndDrag(wxTreeEvent& TE)
{
    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > SourceEntities = m_DraggedEntities;

    m_DraggedEntities.Clear();

    if (!TE.GetItem().IsOk()) return;

    IntrusivePtrT<cf::GameSys::EntityT> TargetEntity = ((EntityTreeItemT*)GetItemData(TE.GetItem()))->GetEntity();
    MapEntRepresT*                      TargetRepres = GetMapEnt(TargetEntity)->GetRepres();

    // If the target entity is locked, don't drag anything into it.
    if (!TargetRepres->CanSelect()) return;

    // "Validate" the source entities.
    for (unsigned int EntNr = 0; EntNr < SourceEntities.Size(); EntNr++)
    {
        // If a source entity is already an immediate child of TargetEntity,
        // there is nothing to do with it -- let's just remove it from the list.
        if (SourceEntities[EntNr]->GetParent() == TargetEntity)
        {
            SourceEntities.RemoveAtAndKeepOrder(EntNr);
            EntNr--;
            continue;
        }

        // Make sure that TargetEntity is not in the subtree of a source entity (or else the
        // reparenting would create invalid cycles). Although the command below does the same
        // check redundantly again, we also want to have it here for clarity. Note that the
        // TargetEntity can still be a child in a different subtree of SourceEntities[EntNr]->Parent.
        if (SourceEntities[EntNr]->Has(TargetEntity))
        {
            wxMessageBox(wxString::Format(
                "An entity cannot be dragged into one of its children.\n\n"
                "You cannot drag a parent entity (\"%s\") into one of its children (\"%s\").",
                    SourceEntities[EntNr]->GetBasics()->GetEntityName().c_str(),
                    TargetEntity->GetBasics()->GetEntityName().c_str()));
            return;
        }
    }

    // Make all remaining source entities children of TargetEntity.
    ArrayT<CommandT*>   Commands;
    const unsigned long NewPos = TargetEntity->GetChildren().Size();

    for (unsigned int EntNr = 0; EntNr < SourceEntities.Size(); EntNr++)
    {
        Commands.PushBack(new CommandChangeEntityHierarchyT(m_MapDoc, SourceEntities[EntNr], TargetEntity, NewPos + EntNr));
    }

    if (Commands.Size() == 1)
    {
        m_Parent->SubmitCommand(new CommandMacroT(Commands,
            wxString::Format("Drag entity \"%s\"", SourceEntities[0]->GetBasics()->GetEntityName().c_str())));
    }
    else if (Commands.Size() > 1)
    {
        m_Parent->SubmitCommand(new CommandMacroT(Commands,
            wxString::Format("Drag %lu entities", Commands.Size())));
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
    }
}


void EntityHierarchyCtrlT::NotifySubjectDies(SubjectT* dyingSubject)
{
    wxASSERT(dyingSubject == m_MapDoc);

    AssociateModel(NULL);
    m_MapDoc = NULL;
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

    // TODO: See EntityHierarchyDialogT::OnSelectionChanged() for a proper implementation!
    // m_MainFrame->SubmitCommand(CommandSelectT::Set(m_MapDoc, NewSelection, false /*WithEntPrims*/));

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

    m_OldTreeCtrl = new EntityHierarchyDialogT(m_MainFrame, this);
    item0->Add(m_OldTreeCtrl, 1, wxEXPAND | wxALL, 0 );

    // m_TreeCtrl = new EntityHierarchyCtrlT(m_MainFrame, this);
    // item0->Add(m_TreeCtrl, 1, wxEXPAND | wxALL, 5 );

    this->SetSizer( item0 );
    item0->SetSizeHints(this);
}
