/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "WindowHierarchy.hpp"
#include "ChildFrame.hpp"
#include "GuiDocument.hpp"

#include "Commands/Select.hpp"
#include "Commands/Create.hpp"
#include "Commands/ModifyGui.hpp"
#include "Commands/ChangeWindowHierarchy.hpp"

#include "../SetCompVar.hpp"

#include "GuiSys/Window.hpp"
#include "Templates/Array.hpp"

#include "wx/wx.h"
#include "wx/artprov.h"
#include "wx/imaglist.h"


namespace
{
    class WindowTreeItemT : public wxTreeItemData
    {
        public:

        WindowTreeItemT(IntrusivePtrT<cf::GuiSys::WindowT> Window) : m_Window(Window) {};

        IntrusivePtrT<cf::GuiSys::WindowT> GetWindow() { return m_Window; }


        private:

        IntrusivePtrT<cf::GuiSys::WindowT> m_Window;
    };
}


using namespace GuiEditor;


BEGIN_EVENT_TABLE(WindowHierarchyT, wxTreeCtrl)
    EVT_KEY_DOWN             (WindowHierarchyT::OnKeyDown)
    EVT_LEFT_DOWN            (WindowHierarchyT::OnTreeLeftClick)
    EVT_LEFT_DCLICK          (WindowHierarchyT::OnTreeLeftClick) // Handle double clicks like normal left clicks when it comes to clicks on tree item icons (otherwise double clicks are handled normally).
    EVT_TREE_SEL_CHANGED     (wxID_ANY, WindowHierarchyT::OnSelectionChanged)
    EVT_TREE_END_LABEL_EDIT  (wxID_ANY, WindowHierarchyT::OnEndLabelEdit)
    EVT_TREE_ITEM_RIGHT_CLICK(wxID_ANY, WindowHierarchyT::OnTreeItemRightClick)
    EVT_TREE_BEGIN_DRAG      (wxID_ANY, WindowHierarchyT::OnBeginDrag)
    EVT_TREE_END_DRAG        (wxID_ANY, WindowHierarchyT::OnEndDrag)
END_EVENT_TABLE()


WindowHierarchyT::WindowHierarchyT(ChildFrameT* Parent, const wxSize& Size)
    : wxTreeCtrl(Parent, wxID_ANY, wxDefaultPosition, Size, wxTR_HAS_BUTTONS|wxTR_LINES_AT_ROOT|wxTR_MULTIPLE|wxBORDER_NONE|wxTR_EDIT_LABELS),
      m_GuiDocument(Parent->GetGuiDoc()),
      m_Parent(Parent),
      m_IsRecursiveSelfNotify(false),
      m_DraggedWindow(NULL)
{
    // Build list of window hierarchy icons.
    wxImageList* TreeIcons=new wxImageList(16, 16);

    TreeIcons->Add(wxBitmap("CaWE/res/checked.png", wxBITMAP_TYPE_PNG));
    TreeIcons->Add(wxBitmap("CaWE/res/unchecked.png", wxBITMAP_TYPE_PNG));

    AssignImageList(TreeIcons); // Note: wxTreeCtrl takes ownership of this list and deletes it on window destroy.
}


WindowHierarchyT::~WindowHierarchyT()
{
}


// Automatically fills children of tree item using the items client data.
void WindowHierarchyT::AddChildren(const wxTreeItemId& Item, bool Recursive)
{
    ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> > Children;

    // Read children of this object.
    // We can safely cast this since we have set the data ourselves.
    WindowTreeItemT* WindowItem=(WindowTreeItemT*)GetItemData(Item);

    WindowItem->GetWindow()->GetChildren(Children);

    for (unsigned long i=0; i<Children.Size(); i++)
    {
        wxTreeItemId ID = AppendItem(Item, Children[i]->GetBasics()->GetWindowName(), -1, -1, new WindowTreeItemT(Children[i]));

        SetItemImage(ID, Children[i]->GetBasics()->IsShown() ? 0 : 1);  // Set the "is visible" or "is invisible" icon.

        if (Recursive) AddChildren(ID, true);
    }
}


const wxTreeItemId WindowHierarchyT::FindTreeItem(const wxTreeItemId& StartingItem, IntrusivePtrT<cf::GuiSys::WindowT> Window) const
{
    // If the item to start with is invalid, return it so the result of this function call is invalid, too.
    if (!StartingItem.IsOk()) return StartingItem;

    // Starting item is valid, so check if it is related to the Window.
    WindowTreeItemT* WindowItem = (WindowTreeItemT*)GetItemData(StartingItem);

    if (WindowItem->GetWindow() == Window) return StartingItem;

    // No match, so recursively check all the children.
    wxTreeItemIdValue Cookie;
    wxTreeItemId      ChildItem;

    for (ChildItem = GetFirstChild(StartingItem, Cookie); ChildItem.IsOk(); ChildItem = GetNextChild(StartingItem, Cookie))
    {
        const wxTreeItemId Result = FindTreeItem(ChildItem, Window);

        if (Result.IsOk()) return Result;
    }

    wxASSERT(!ChildItem.IsOk());
    return ChildItem;
}


void WindowHierarchyT::GetTreeItems(const wxTreeItemId& StartingItem, ArrayT<wxTreeItemId>& Items)
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


void WindowHierarchyT::NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& OldSelection, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& NewSelection)
{
    if (m_IsRecursiveSelfNotify) return;

    // Both UnselectAll() and SelectItem() result in EVT_TREE_SEL_CHANGED event,
    // see <http://thread.gmane.org/gmane.comp.lib.wxwidgets.general/72754> for details.
    // m_IsRecursiveSelfNotify makes sure that the tree isn't unintentionally updated recursively.
    m_IsRecursiveSelfNotify=true;

    // Reset tree selection and update it according to new selection.
    UnselectAll();

    for (unsigned long NewSelNr=0; NewSelNr<NewSelection.Size(); NewSelNr++)
    {
        wxTreeItemId Result=FindTreeItem(GetRootItem(), NewSelection[NewSelNr]);

        if (Result.IsOk())
        {
            SelectItem(Result);
            EnsureVisible(Result);
        }
    }

    m_IsRecursiveSelfNotify=false;
}


void WindowHierarchyT::NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows)
{
    if (m_IsRecursiveSelfNotify) return;

    RefreshTree();
}


void WindowHierarchyT::NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows)
{
    if (m_IsRecursiveSelfNotify) return;

    RefreshTree();
}


void WindowHierarchyT::NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows, WindowModDetailE Detail)
{
    if (m_IsRecursiveSelfNotify) return;

    if (Detail!=WMD_GENERIC && Detail!=WMD_HIERARCHY) return;

    // TODO can we handle order changes more precisely (e.g. move the tree item to its new order position)?

    RefreshTree();
}


void WindowHierarchyT::Notify_Changed(SubjectT* Subject, const cf::TypeSys::VarBaseT& Var)
{
    if (m_IsRecursiveSelfNotify) return;

    ArrayT<wxTreeItemId> TreeItems;

    GetTreeItems(GetRootItem(), TreeItems);

    for (unsigned long ItemNr = 0; ItemNr < TreeItems.Size(); ItemNr++)
    {
        IntrusivePtrT<cf::GuiSys::ComponentBasicsT> Basics = ((WindowTreeItemT*)GetItemData(TreeItems[ItemNr]))->GetWindow()->GetBasics();

        if (&Var == Basics->GetMemberVars().Find("Name"))
        {
            SetItemText(TreeItems[ItemNr], Basics->GetWindowName());
            return;
        }

        if (&Var == Basics->GetMemberVars().Find("Show"))
        {
            SetItemImage(TreeItems[ItemNr], Basics->IsShown() ? 0 : 1);   // Set the "is visible" or "is invisible" icon.
            return;
        }
    }
}


void WindowHierarchyT::RefreshTree()
{
    if (m_GuiDocument==NULL) return;

    // Get all currently opened tree items and reopen them after the refresh.
    ArrayT<wxTreeItemId>                         TreeItems;
    ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> > ExpandedWindows;

    // Note that this may well produce TreeItems whose windows have been deleted from the document already.
    GetTreeItems(GetRootItem(), TreeItems);

    for (unsigned long ItemNr=0; ItemNr<TreeItems.Size(); ItemNr++)
        if (IsExpanded(TreeItems[ItemNr]))
            ExpandedWindows.PushBack(((WindowTreeItemT*)GetItemData(TreeItems[ItemNr]))->GetWindow());

    Freeze();

    // For now we just update everything if the gui doc changes.
    DeleteAllItems();

    // First add root window to tree.
    wxTreeItemId ID = AddRoot(m_GuiDocument->GetRootWindow()->GetBasics()->GetWindowName(), -1, -1, new WindowTreeItemT(m_GuiDocument->GetRootWindow()));

    SetItemImage(ID, m_GuiDocument->GetRootWindow()->GetBasics()->IsShown() ? 0 : 1);  // Set the "is visible" or "is invisible" icon.

    // Add all children of root recursively to the tree.
    AddChildren(ID, true);

    // Re-select selected windows in the tree.
    const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Selection=m_GuiDocument->GetSelection();

    for (unsigned long SelNr=0; SelNr<Selection.Size(); SelNr++)
    {
        wxTreeItemId Result=FindTreeItem(ID, Selection[SelNr]);

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

    Expand(ID); // Always expand root window.

    // Expand all previously expanded items.
    for (unsigned long ExWinNr=0; ExWinNr<ExpandedWindows.Size(); ExWinNr++)
    {
        wxTreeItemId Result=FindTreeItem(GetRootItem(), ExpandedWindows[ExWinNr]);

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


void WindowHierarchyT::NotifySubjectDies(SubjectT* dyingSubject)
{
    wxASSERT(dyingSubject==m_GuiDocument);

    m_GuiDocument=NULL;

    DeleteAllItems();
}


void WindowHierarchyT::OnKeyDown(wxKeyEvent& KE)
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


void WindowHierarchyT::OnTreeLeftClick(wxMouseEvent& ME)
{
    // Check if we hit an tree item icon.
    int HitFlag=0;

    wxTreeItemId ClickedItem=HitTest(ME.GetPosition(), HitFlag);

    // If an icon was hit, toggle visibility of the associated gui window.
    if (HitFlag & wxTREE_HITTEST_ONITEMICON)
    {
        IntrusivePtrT<cf::GuiSys::WindowT> ClickedWindow = ((WindowTreeItemT*)GetItemData(ClickedItem))->GetWindow();
        cf::TypeSys::VarT<bool>* Show = dynamic_cast<cf::TypeSys::VarT<bool>*>(ClickedWindow->GetBasics()->GetMemberVars().Find("Show"));

        m_IsRecursiveSelfNotify=true;

        if (Show)
        {
            m_Parent->SubmitCommand(new CommandSetCompVarT<bool>(m_GuiDocument->GetAdapter(), *Show, !Show->Get()));
            SetItemImage(ClickedItem, Show->Get() ? 0 : 1);   // Set the "is visible" or "is invisible" icon.
        }

        m_IsRecursiveSelfNotify=false;
    }
    else
    {
        // Skip event if no icon was hit to preserve wxTreeCtrl functionality.
        ME.Skip();
    }
}


void WindowHierarchyT::OnSelectionChanged(wxTreeEvent& TE)
{
    if (m_GuiDocument==NULL || m_IsRecursiveSelfNotify) return;

    m_IsRecursiveSelfNotify=true;

    // Get the currently selected tree items and update the document selection accordingly.
    wxArrayTreeItemIds SelectedItems;
    GetSelections(SelectedItems);

    ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> > NewSelection;
    for (size_t SelNr=0; SelNr<SelectedItems.GetCount(); SelNr++)
        NewSelection.PushBack(((WindowTreeItemT*)GetItemData(SelectedItems[SelNr]))->GetWindow());

    m_Parent->SubmitCommand(CommandSelectT::Set(m_GuiDocument, NewSelection));

    m_IsRecursiveSelfNotify=false;
}


void WindowHierarchyT::OnEndLabelEdit(wxTreeEvent& TE)
{
    IntrusivePtrT<cf::GuiSys::WindowT> Window = ((WindowTreeItemT*)GetItemData(TE.GetItem()))->GetWindow();
    cf::TypeSys::VarT<std::string>* Name = dynamic_cast<cf::TypeSys::VarT<std::string>*>(Window->GetBasics()->GetMemberVars().Find("Name"));

    if (TE.IsEditCancelled()) return;

    m_IsRecursiveSelfNotify=true;

    if (Name)
    {
        m_Parent->SubmitCommand(new CommandSetCompVarT<std::string>(m_GuiDocument->GetAdapter(), *Name, std::string(TE.GetLabel())));
    }

    m_IsRecursiveSelfNotify=false;

    // The command may well have set a name different from TE.GetLabel().
    TE.Veto();
    SetItemText(TE.GetItem(), Window->GetBasics()->GetWindowName());
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


void WindowHierarchyT::OnTreeItemRightClick(wxTreeEvent& TE)
{
    // Note that GetPopupMenuSelectionFromUser() temporarily disables UI updates for the window,
    // so our menu IDs used below should be doubly clash-free.
    enum
    {
        ID_MENU_CREATE_WINDOW = wxID_HIGHEST + 1,
        ID_MENU_DEFAULTFOCUS,
        ID_MENU_RENAME
    };

    wxMenu Menu;

    // Create context menus.
    AppendMI(Menu, ID_MENU_CREATE_WINDOW, "Create Window", "window-new");
    AppendMI(Menu, ID_MENU_DEFAULTFOCUS,  "Set as default focus", "");
    AppendMI(Menu, ID_MENU_RENAME,        "Rename\tF2", "textfield_rename");

    switch (GetPopupMenuSelectionFromUser(Menu))
    {
        case ID_MENU_CREATE_WINDOW:
            m_Parent->SubmitCommand(new CommandCreateT(m_GuiDocument, ((WindowTreeItemT*)GetItemData(TE.GetItem()))->GetWindow()));
            break;

        case ID_MENU_DEFAULTFOCUS:
            m_Parent->SubmitCommand(CommandModifyGuiT::Create(m_GuiDocument, "DefaultFocus", ((WindowTreeItemT*)GetItemData(TE.GetItem()))->GetWindow()->GetBasics()->GetWindowName()));
            break;

        case ID_MENU_RENAME:
            EditLabel(TE.GetItem());
            break;
    }
}


void WindowHierarchyT::OnBeginDrag(wxTreeEvent& TE)
{
    wxASSERT(m_DraggedWindow==NULL);

    // Unfortunately we need this array in order to call the method below.
    // This is the only way to get the number of selected tree items.
    wxArrayTreeItemIds SelectedItems;

    if (GetSelections(SelectedItems)>1)
    {
        wxMessageBox("Sorry, you can only drag one window at a time.");
        return;
    }

    if (!TE.GetItem().IsOk())   // Should never happen.
    {
        wxMessageBox("Sorry, this window cannot be dragged.");
        return;
    }

    if (TE.GetItem()==GetRootItem())
    {
        wxMessageBox("Sorry, the root window cannot be dragged.");
        return;
    }

    m_DraggedWindow=((WindowTreeItemT*)GetItemData(TE.GetItem()))->GetWindow();
    TE.Allow();
}


void WindowHierarchyT::OnEndDrag(wxTreeEvent& TE)
{
    wxASSERT(!m_DraggedWindow.IsNull());
    if (m_DraggedWindow.IsNull()) return;
    IntrusivePtrT<cf::GuiSys::WindowT> SourceWindow=m_DraggedWindow;
    m_DraggedWindow=NULL;

    if (!TE.GetItem().IsOk()) return;
    IntrusivePtrT<cf::GuiSys::WindowT> TargetWindow=((WindowTreeItemT*)GetItemData(TE.GetItem()))->GetWindow();


    // If SourceWindow is already an immediate child of TargetWindow, do nothing.
    if (SourceWindow->GetParent() == TargetWindow) return;

    // Make sure that TargetWindow is not in the subtree of SourceWindow (or else the reparenting would create invalid cycles).
    // Although the command below does the same check redundantly again, we also want to have it here for clarity.
    // Note that the TargetWindow can still be a child in a different subtree of SourceWindow->Parent.
    {
        ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> > SubTree;

        SubTree.PushBack(SourceWindow);
        SourceWindow->GetChildren(SubTree, true /*recurse*/);

        if (SubTree.Find(TargetWindow)>=0) return;
    }


    // Note that the "|| TargetWindow->Parent==NULL" half of the if-condition is actually only for safety,
    // because TargetWindow->Parent==NULL only if TargetWindow is the root, but the root window always has children.
    // If it hadn't, then SourceWindow==TargetWindow, and we had not gotten here.
    if (TargetWindow->GetChildren().Size() > 0 || TargetWindow->GetParent() == NULL)
    {
        // Make SourceWindow a child of TargetWindow.
        const unsigned long NewPos = TargetWindow->GetChildren().Size();

        m_Parent->SubmitCommand(new CommandChangeWindowHierarchyT(m_GuiDocument, SourceWindow, TargetWindow, NewPos));
    }
    else
    {
        wxASSERT(!TargetWindow->GetParent().IsNull());   // This condition has been established in the if-branch above.

        // Make SourceWindow a sibling of TargetWindow.
        const unsigned long NewPos = TargetWindow->GetParent()->GetChildren().Find(TargetWindow);

        m_Parent->SubmitCommand(new CommandChangeWindowHierarchyT(m_GuiDocument, SourceWindow, TargetWindow->GetParent(), NewPos));
    }
}
