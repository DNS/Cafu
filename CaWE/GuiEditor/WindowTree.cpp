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

#include "WindowTree.hpp"
#include "ChildFrame.hpp"
#include "GuiDocument.hpp"

#include "EditorData/Window.hpp"

#include "Commands/Select.hpp"
#include "Commands/Create.hpp"
#include "Commands/ModifyWindow.hpp"
#include "Commands/ModifyGui.hpp"
#include "Commands/ChangeWindowHierarchy.hpp"

#include "GuiSys/Window.hpp"

#include "Templates/Array.hpp"

#include "wx/wx.h"
#include "wx/imaglist.h"


class TreeContextMenuT : public wxMenu
{
    public:

    enum
    {
        ID_MENU_CREATE_WINDOW_BASE=wxID_HIGHEST+1,
        ID_MENU_CREATE_WINDOW_EDIT,
        ID_MENU_CREATE_WINDOW_CHOICE,
        ID_MENU_CREATE_WINDOW_LISTBOX,
        ID_MENU_CREATE_WINDOW_MODEL,
        ID_MENU_DEFAULTFOCUS
    };

    TreeContextMenuT()
        : wxMenu(),
          ID(-1)
    {
        wxMenu* SubMenuCreate=new wxMenu();
        SubMenuCreate->Append(ID_MENU_CREATE_WINDOW_BASE,    "Window");
        SubMenuCreate->Append(ID_MENU_CREATE_WINDOW_EDIT,    "Text Editor");
        SubMenuCreate->Append(ID_MENU_CREATE_WINDOW_CHOICE,  "Choice Box");
        SubMenuCreate->Append(ID_MENU_CREATE_WINDOW_LISTBOX, "List Box");
        SubMenuCreate->Append(ID_MENU_CREATE_WINDOW_MODEL,   "Model Window");

        // Create context menus.
        this->AppendSubMenu(SubMenuCreate, "Create");
        this->Append(ID_MENU_DEFAULTFOCUS, "Set as default focus");
    }

    int GetClickedMenuItem() { return ID; }


    protected:

    void OnMenuClick(wxCommandEvent& CE) { ID=CE.GetId(); }


    private:

    int ID;

    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(TreeContextMenuT, wxMenu)
    EVT_MENU(wxID_ANY, TreeContextMenuT::OnMenuClick)
END_EVENT_TABLE()


class WindowTreeItemT : public wxTreeItemData
{
    public:

    WindowTreeItemT(cf::GuiSys::WindowT* Window) : m_Window(Window) {};

    cf::GuiSys::WindowT* GetWindow() { return m_Window; }


    private:

    cf::GuiSys::WindowT* m_Window;
};


using namespace GuiEditor;


BEGIN_EVENT_TABLE(WindowTreeT, wxTreeCtrl)
    EVT_LEFT_DOWN            (WindowTreeT::OnTreeLeftClick)
    EVT_LEFT_DCLICK          (WindowTreeT::OnTreeLeftClick) // Handle double clicks like normal left clicks when it comes to clicks on tree item icons (otherwise double clicks are handled normally).
    EVT_TREE_SEL_CHANGED     (wxID_ANY, WindowTreeT::OnSelectionChanged)
    EVT_TREE_END_LABEL_EDIT  (wxID_ANY, WindowTreeT::OnLabelChanged)
    EVT_TREE_ITEM_RIGHT_CLICK(wxID_ANY, WindowTreeT::OnTreeItemRightClick)
    EVT_TREE_BEGIN_DRAG      (wxID_ANY, WindowTreeT::OnBeginDrag)
    EVT_TREE_END_DRAG        (wxID_ANY, WindowTreeT::OnEndDrag)
END_EVENT_TABLE()


WindowTreeT::WindowTreeT(ChildFrameT* Parent, const wxSize& Size)
    : wxTreeCtrl(Parent, wxID_ANY, wxDefaultPosition, Size, wxTR_HAS_BUTTONS|wxTR_LINES_AT_ROOT|wxTR_MULTIPLE|wxSUNKEN_BORDER|wxTR_EDIT_LABELS),
      m_GuiDocument(Parent->GetGuiDoc()),
      m_Parent(Parent),
      m_IsRecursiveSelfNotify(false),
      m_DraggedWindow(NULL)
{
    // Build list of window tree icons.
    wxImageList* TreeIcons=new wxImageList(16, 16);

    TreeIcons->Add(wxBitmap("CaWE/res/checked.png", wxBITMAP_TYPE_PNG));
    TreeIcons->Add(wxBitmap("CaWE/res/unchecked.png", wxBITMAP_TYPE_PNG));

    AssignImageList(TreeIcons); // Note: wxTreeCtrl takes ownership of this list and deletes it on window destroy.
}


WindowTreeT::~WindowTreeT()
{
}


// Automatically fills children of tree item using the items client data.
void WindowTreeT::AddChildren(const wxTreeItemId& Item, bool Recursive)
{
    ArrayT<cf::GuiSys::WindowT*> Children;

    // Read children of this object.
    // We can safely cast this since we have set the data ourselves.
    WindowTreeItemT* WindowItem=(WindowTreeItemT*)GetItemData(Item);

    WindowItem->GetWindow()->GetChildren(Children);

    for (unsigned long i=0; i<Children.Size(); i++)
    {
        wxTreeItemId ID=AppendItem(Item, Children[i]->Name, -1, -1, new WindowTreeItemT(Children[i]));

        if (Children[i]->ShowWindow)
            SetItemImage(ID, 0); // Visible icon.
        else
            SetItemImage(ID, 1); // Invisible icon.

        if (Recursive) AddChildren(ID, true);
    }
}


const wxTreeItemId WindowTreeT::FindTreeItem(const wxTreeItemId& StartingItem, cf::GuiSys::WindowT* Window) const
{
    // If the item to start with is invalid, return it so the result of this function call is invalid too.
    if (!StartingItem.IsOk()) return StartingItem;

    // Starting item is valid, so check if it is related to the Window.
    WindowTreeItemT* WindowItem=(WindowTreeItemT*)GetItemData(StartingItem);

    if (WindowItem->GetWindow()==Window) return StartingItem;

    // No match, so recursively check all children of the starting item.
    wxTreeItemIdValue Cookie; // This cookie is needed for iteration over a trees children.

    wxTreeItemId Result=FindTreeItem(GetFirstChild(StartingItem, Cookie), Window); // If there is no first child (the TreeItemId passed to FindTreeItem is invalid).

    if (Result.IsOk()) return Result; // If we found a match, return it.

    for (unsigned long i=1; i<GetChildrenCount(StartingItem); i++)
    {
        Result=FindTreeItem(GetNextChild(StartingItem, Cookie), Window);

        if (Result.IsOk()) return Result; // If we found a match, return it.
    }

    wxASSERT(!Result.IsOk());

    return Result; // No match for this item and its children, so return the invalid result.
}


void WindowTreeT::GetTreeItems(const wxTreeItemId& StartingItem, ArrayT<wxTreeItemId>& Items)
{
    if (!StartingItem.IsOk()) return;

    Items.PushBack(StartingItem);

    if (!ItemHasChildren(StartingItem)) return;

    wxTreeItemIdValue Cookie; // This cookie is needed for iteration over a trees children.

    GetTreeItems(GetFirstChild(StartingItem, Cookie), Items);

    for (unsigned long i=1; i<GetChildrenCount(StartingItem); i++)
        GetTreeItems(GetNextChild(StartingItem, Cookie), Items);
}


void WindowTreeT::NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<cf::GuiSys::WindowT*>& OldSelection, const ArrayT<cf::GuiSys::WindowT*>& NewSelection)
{
    if (m_IsRecursiveSelfNotify) return;

    // Reset tree selection and update it according to new selection.
    UnselectAll();

    for (unsigned long NewSelNr=0; NewSelNr<NewSelection.Size(); NewSelNr++)
    {
        wxTreeItemId Result=FindTreeItem(GetRootItem(), NewSelection[NewSelNr]);

        if (Result.IsOk())
        {
            m_IsRecursiveSelfNotify=true;
            SelectItem(Result);            // Note: SelectItem results in SelectionChanged event. m_IsRecursiveSelfNotify makes sure the tree isn't updated recursively.
            m_IsRecursiveSelfNotify=false;
            Expand(Result);

            // Make sure parents are also expanded.
            while(GetItemParent(Result).IsOk())
            {
                Result=GetItemParent(Result);
                Expand(Result);
            }
        }
    }
}


void WindowTreeT::NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT<cf::GuiSys::WindowT*>& Windows)
{
    if (m_IsRecursiveSelfNotify) return;

    RefreshTree();
}


void WindowTreeT::NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT<cf::GuiSys::WindowT*>& Windows)
{
    if (m_IsRecursiveSelfNotify) return;

    RefreshTree();
}


void WindowTreeT::NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<cf::GuiSys::WindowT*>& Windows, WindowModDetailE Detail)
{
    if (m_IsRecursiveSelfNotify) return;

    if (Detail!=WMD_GENERIC && Detail!=WMD_HIERARCHY) return;

    // TODO can we handle order changes more precisely (e.g. move the tree item to its new order position)?

    RefreshTree();
}


void WindowTreeT::NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<cf::GuiSys::WindowT*>& Windows, WindowModDetailE Detail, const wxString& PropertyName)
{
    if (m_IsRecursiveSelfNotify) return;

    if (PropertyName=="Name")
    {
        wxASSERT(Windows.Size()==1); // Can't set the name property for more windows since it must always be unique.

        SetItemText(FindTreeItem(GetRootItem(), Windows[0]), Windows[0]->Name);
    }

    if (PropertyName=="Visible")
    {
        for (unsigned long WindowNr=0; WindowNr<Windows.Size(); WindowNr++)
        {
            SetItemImage(FindTreeItem(GetRootItem(), Windows[WindowNr]), Windows[WindowNr]->ShowWindow ? 0 : 1);
        }
    }
}


void WindowTreeT::RefreshTree()
{
    if (m_GuiDocument==NULL) return;

    // Get all currently opened tree items and reopen them after the refresh.
    ArrayT<wxTreeItemId>         TreeItems;
    ArrayT<cf::GuiSys::WindowT*> ExpandedWindows;

    // Note that this may well produce TreeItems whose windows have been deleted from the document already.
    GetTreeItems(GetRootItem(), TreeItems);

    for (unsigned long ItemNr=0; ItemNr<TreeItems.Size(); ItemNr++)
        if (IsExpanded(TreeItems[ItemNr]))
            ExpandedWindows.PushBack(((WindowTreeItemT*)GetItemData(TreeItems[ItemNr]))->GetWindow());

    Freeze();

    // For now we just update everything if the gui doc changes.
    DeleteAllItems();

    // First add root window to tree.
    wxTreeItemId ID=AddRoot(m_GuiDocument->GetRootWindow()->Name, -1, -1, new WindowTreeItemT(m_GuiDocument->GetRootWindow()));

    if (m_GuiDocument->GetRootWindow()->ShowWindow)
        SetItemImage(ID, 0); // Visible icon.
    else
        SetItemImage(ID, 1); // Invisible icon.

    // Add all children of root recusively to the tree.
    AddChildren(ID, true);

    // Select first selected window in the tree.
    const ArrayT<cf::GuiSys::WindowT*>& Selection=m_GuiDocument->GetSelection();

    for (unsigned long SelNr=0; SelNr<Selection.Size(); SelNr++)
    {
        wxTreeItemId Result=FindTreeItem(ID, Selection[SelNr]);

        if (Result.IsOk())
        {
            m_IsRecursiveSelfNotify=true;
            SelectItem(Result);            // Note: SelectItem results in SelectionChanged event. m_IsRecursiveSelfNotify makes sure the tree isn't updated recursively.
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

    Thaw();
}


void WindowTreeT::NotifySubjectDies(SubjectT* dyingSubject)
{
    wxASSERT(dyingSubject==m_GuiDocument);

    m_GuiDocument=NULL;

    DeleteAllItems();
}


void WindowTreeT::OnTreeLeftClick(wxMouseEvent& ME)
{
    // Check if we hit an tree item icon.
    int HitFlag=0;

    wxTreeItemId ClickedItem=HitTest(ME.GetPosition(), HitFlag);

    // If a icon was hit, toggle visibility of the associated gui window.
    if (HitFlag & wxTREE_HITTEST_ONITEMICON)
    {
        cf::GuiSys::WindowT* ClickedWindow=((WindowTreeItemT*)GetItemData(ClickedItem))->GetWindow();

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
}


void WindowTreeT::OnSelectionChanged(wxTreeEvent& TE)
{
    if (m_GuiDocument==NULL || m_IsRecursiveSelfNotify) return;

    m_IsRecursiveSelfNotify=true;

    // Compare the new tree selection with the current document selection and update
    // the document selection accordingly.
    wxArrayTreeItemIds SelectedItems;
    GetSelections(SelectedItems);
    ArrayT<cf::GuiSys::WindowT*> NewSelection;

    for (size_t SelNr=0; SelNr<SelectedItems.GetCount(); SelNr++)
        NewSelection.PushBack(((WindowTreeItemT*)GetItemData(SelectedItems[SelNr]))->GetWindow());

    m_Parent->SubmitCommand(CommandSelectT::Set(m_GuiDocument, NewSelection));

    m_IsRecursiveSelfNotify=false;
}


void WindowTreeT::OnLabelChanged(wxTreeEvent& TE)
{
    // Emtpy string means the user has either not changed the label at all or
    // deleted the whole label string.
    if (TE.GetLabel()=="")
    {
        TE.Veto(); // Reset value.
        return;
    }

    cf::GuiSys::WindowT* Window=((WindowTreeItemT*)GetItemData(TE.GetItem()))->GetWindow();

    m_IsRecursiveSelfNotify=true;

    if (!m_Parent->SubmitCommand(new CommandModifyWindowT(m_GuiDocument, Window, "Name", Window->GetMemberVar("name"), TE.GetLabel())))
    {
        TE.Veto(); // Reset value if not valid.
    }

    m_IsRecursiveSelfNotify=false;
}


void WindowTreeT::OnTreeItemRightClick(wxTreeEvent& TE)
{
    TreeContextMenuT ContextMenu;

    PopupMenu(&ContextMenu);

    switch (ContextMenu.GetClickedMenuItem())
    {
        case TreeContextMenuT::ID_MENU_CREATE_WINDOW_BASE:
            m_Parent->SubmitCommand(new CommandCreateT(m_GuiDocument, ((WindowTreeItemT*)GetItemData(TE.GetItem()))->GetWindow()));
            break;

        case TreeContextMenuT::ID_MENU_CREATE_WINDOW_EDIT:
            m_Parent->SubmitCommand(new CommandCreateT(m_GuiDocument, ((WindowTreeItemT*)GetItemData(TE.GetItem()))->GetWindow(), CommandCreateT::WINDOW_TEXTEDITOR));
            break;

        case TreeContextMenuT::ID_MENU_CREATE_WINDOW_CHOICE:
            m_Parent->SubmitCommand(new CommandCreateT(m_GuiDocument, ((WindowTreeItemT*)GetItemData(TE.GetItem()))->GetWindow(), CommandCreateT::WINDOW_CHOICE));
            break;

        case TreeContextMenuT::ID_MENU_CREATE_WINDOW_LISTBOX:
            m_Parent->SubmitCommand(new CommandCreateT(m_GuiDocument, ((WindowTreeItemT*)GetItemData(TE.GetItem()))->GetWindow(), CommandCreateT::WINDOW_LISTBOX));
            break;

        case TreeContextMenuT::ID_MENU_CREATE_WINDOW_MODEL:
            m_Parent->SubmitCommand(new CommandCreateT(m_GuiDocument, ((WindowTreeItemT*)GetItemData(TE.GetItem()))->GetWindow(), CommandCreateT::WINDOW_MODEL));
            break;

        case TreeContextMenuT::ID_MENU_DEFAULTFOCUS:
            m_Parent->SubmitCommand(CommandModifyGuiT::Create(m_GuiDocument, "DefaultFocus", ((WindowTreeItemT*)GetItemData(TE.GetItem()))->GetWindow()->Name));
            break;
    }
}


void WindowTreeT::OnBeginDrag(wxTreeEvent& TE)
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


void WindowTreeT::OnEndDrag(wxTreeEvent& TE)
{
    wxASSERT(m_DraggedWindow);
    if (!m_DraggedWindow) return;
    cf::GuiSys::WindowT* SourceWindow=m_DraggedWindow;
    m_DraggedWindow=NULL;

    if (!TE.GetItem().IsOk()) return;
    cf::GuiSys::WindowT* TargetWindow=((WindowTreeItemT*)GetItemData(TE.GetItem()))->GetWindow();


    // If SourceWindow is already an immediate child of TargetWindow, do nothing.
    if (SourceWindow->Parent==TargetWindow) return;

    // Make sure that TargetWindow is not in the subtree of SourceWindow (or else the reparenting would create invalid cycles).
    // Although the command below does the same check redundantly again, we also want to have it here for clarity.
    // Note that the TargetWindow can still be a child in a different subtree of SourceWindow->Parent.
    {
        ArrayT<cf::GuiSys::WindowT*> SubTree;

        SubTree.PushBack(SourceWindow);
        SourceWindow->GetChildren(SubTree, true /*recurse*/);

        if (SubTree.Find(TargetWindow)>=0) return;
    }


    // Note that the "|| TargetWindow->Parent==NULL" half of the if-condition is actually only for safety,
    // because TargetWindow->Parent==NULL only if TargetWindow is the root, but the root window always has children.
    // If it hadn't, then SourceWindow==TargetWindow, and we had not gotten here.
    if (TargetWindow->Children.Size()>0 || TargetWindow->Parent==NULL)
    {
        // Make SourceWindow a child of TargetWindow.
        const unsigned long NewPos=TargetWindow->Children.Size();

        m_Parent->SubmitCommand(new CommandChangeWindowHierarchyT(m_GuiDocument, SourceWindow, TargetWindow, NewPos));
    }
    else
    {
        wxASSERT(TargetWindow->Parent);     // This condition has been established in the if-branch above.

        // Make SourceWindow a sibling of TargetWindow.
        const unsigned long NewPos=TargetWindow->Parent->Children.Find(TargetWindow);

        m_Parent->SubmitCommand(new CommandChangeWindowHierarchyT(m_GuiDocument, SourceWindow, TargetWindow->Parent, NewPos));
    }
}
