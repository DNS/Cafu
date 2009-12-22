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

#include "ToolbarGroups.hpp"
#include "MapDocument.hpp"
#include "MapElement.hpp"
#include "Group.hpp"
#include "MapCommands/Select.hpp"
#include "MapCommands/Group_Assign.hpp"
#include "MapCommands/Group_Delete.hpp"
#include "MapCommands/Group_Reorder.hpp"
#include "MapCommands/Group_SetVisibility.hpp"

#include "wx/checklst.h"
#include "wx/colordlg.h"
#include "wx/imaglist.h"
#include "wx/listctrl.h"


// Define some constants.
enum { ICON_EYE, ICON_EYE_GREY, ICON_LOCK, ICON_EDIT, ICON_SELECT_INDIV, ICON_SELECT_ASGROUP,
       ICON_MOUSE_CURSOR, ICON_RENAME, ICON_COLORWHEEL, ICON_DELETE, ICON_MERGE, ICON_ARROW_UP, ICON_ARROW_DOWN };


/// This class implements the list view of the map editor groups.
///
/// It derives from wxListView (rather than using a wxListView instance in the user code directly),
/// so that we can (more easily) deal with mouse events (which are reported in window space) and size events.
/// (wxDataViewListCtrl might have been a better choice than wxListView, but can only be done post-wx2.8.x.)
///
/// It also extends the wxListView base class by "client data" for each item, special-cased to our needs as
/// pointers to GroupT. Nearly all GroupsToolbarT code could have been implemented without keeping this
/// explicit client data (the item numbers into m_MapDoc.GetGroups() would have been sufficient), except
/// for *one* place in GroupsToolbarT::NotifySubjectChanged_Groups(), where the m_MapDoc.GetGroups()
/// have already been changed but we must still learn which groups were selected *before* the change.
class GroupsListViewT : public wxListView
{
    public:

    GroupsListViewT(GroupsToolbarT* Parent, wxWindowID ID);

    // Overrides of parent class methods.
    bool DeleteAllItems();
    long InsertItem(long index, const wxString& label, int imageIndex);

    // Extensions of the parent class interface.
    void            SetClientData(unsigned long item, GroupT* CD);
 // GroupT*         GetClientData(unsigned long item) const;
    ArrayT<GroupT*> GetSelectedGroups() const;


    private:

    GroupsToolbarT* m_Parent;
    ArrayT<GroupT*> m_ClientData;    ///< Our custom client data for each list item.

    void OnKeyDown      (wxKeyEvent&         KE);
    void OnMouseLeftDown(wxMouseEvent&       ME);   ///< We also handle "double-click" events in this method (use ME.ButtonDClick() for distinction).
    void OnContextMenu  (wxContextMenuEvent& CE);
    void OnSize         (wxSizeEvent&        SE);
    void OnEndLabelEdit (wxListEvent&        LE);

    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(GroupsListViewT, wxListView)
    EVT_KEY_DOWN    (GroupsListViewT::OnKeyDown)
    EVT_LEFT_DOWN   (GroupsListViewT::OnMouseLeftDown)
    EVT_LEFT_DCLICK (GroupsListViewT::OnMouseLeftDown)
    EVT_CONTEXT_MENU(GroupsListViewT::OnContextMenu)
    EVT_SIZE        (GroupsListViewT::OnSize)
    EVT_LIST_END_LABEL_EDIT(GroupsToolbarT::ID_LISTVIEW_GROUPS, GroupsListViewT::OnEndLabelEdit)
END_EVENT_TABLE()


GroupsListViewT::GroupsListViewT(GroupsToolbarT* Parent, wxWindowID ID)
    : wxListView(Parent, ID, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_NO_HEADER | wxLC_EDIT_LABELS),
      m_Parent(Parent)
{
    // Build list of list view icons.
    wxImageList* ListIcons=new wxImageList(16, 16);

    // The order in which the bitmaps are added to the list should (must) match the ICON_* enum!!
    ListIcons->Add(wxBitmap("CaWE/res/eye.png", wxBITMAP_TYPE_PNG));
#ifdef __WXMSW__
    ListIcons->Add(wxBitmap("CaWE/res/eye_grey_win.png", wxBITMAP_TYPE_PNG));
#else
    // This is the correct image (with alpha channel), but it doesn't work correctly under (wx)MSW,
    // see http://trac.wxwidgets.org/ticket/9050 for more information.
    ListIcons->Add(wxBitmap("CaWE/res/eye_grey.png", wxBITMAP_TYPE_PNG));
#endif
    ListIcons->Add(wxBitmap("CaWE/res/lock.png", wxBITMAP_TYPE_PNG));
    ListIcons->Add(wxBitmap("CaWE/res/wrench.png", wxBITMAP_TYPE_PNG));
    ListIcons->Add(wxBitmap("CaWE/res/GroupSelect-Indiv.png", wxBITMAP_TYPE_PNG));
    ListIcons->Add(wxBitmap("CaWE/res/GroupSelect-AsOne.png", wxBITMAP_TYPE_PNG));

    // And a few more for the RMB context menu.
    ListIcons->Add(wxBitmap("CaWE/res/GuiEditor/cursor.png", wxBITMAP_TYPE_PNG));
    ListIcons->Add(wxBitmap("CaWE/res/textfield_rename.png", wxBITMAP_TYPE_PNG));
    ListIcons->Add(wxBitmap("CaWE/res/color_wheel.png", wxBITMAP_TYPE_PNG));
    ListIcons->Add(wxBitmap("CaWE/res/GuiEditor/delete.png", wxBITMAP_TYPE_PNG));
    ListIcons->Add(wxBitmap("CaWE/res/merge.png", wxBITMAP_TYPE_PNG));
    ListIcons->Add(wxBitmap("CaWE/res/GuiEditor/arrow_up.png", wxBITMAP_TYPE_PNG));
    ListIcons->Add(wxBitmap("CaWE/res/GuiEditor/arrow_down.png", wxBITMAP_TYPE_PNG));

    // The wxListView takes ownership of the icons list and deletes it later.
    AssignImageList(ListIcons, wxIMAGE_LIST_SMALL);


    // According to the wx listctrl sample:
    // "Note that under MSW for SetColumnWidth() to work we need to create the
    //  items with images initially even if we specify dummy image id."
    wxListItem itemCol;
    itemCol.SetImage(-1);

    itemCol.SetText(wxT("Vis and Name"));
    InsertColumn(0, itemCol);

    itemCol.SetText(wxT("Lock"));
    InsertColumn(1, itemCol);

    itemCol.SetText(wxT("As Group"));
    InsertColumn(2, itemCol);


    // wxSizeEvent SE;
    // OnSize(SE);
}


bool GroupsListViewT::DeleteAllItems()
{
    m_ClientData.Overwrite();
    return wxListView::DeleteAllItems();
}


long GroupsListViewT::InsertItem(long index, const wxString& label, int imageIndex)
{
    m_ClientData.InsertAt(index, NULL);
    return wxListView::InsertItem(index, label, imageIndex);
}


void GroupsListViewT::SetClientData(unsigned long item, GroupT* CD)
{
    m_ClientData[item]=CD;
}


/* GroupT* GroupsListViewT::GetClientData(unsigned long item) const
{
    return m_ClientData[item];
} */


ArrayT<GroupT*> GroupsListViewT::GetSelectedGroups() const
{
    ArrayT<GroupT*> SelGroups;

    for (long SelNr=GetFirstSelected(); SelNr!=-1; SelNr=GetNextSelected(SelNr))
        SelGroups.PushBack(m_ClientData[SelNr]);

    return SelGroups;
}


void GroupsListViewT::OnKeyDown(wxKeyEvent& KE)
{
    switch (KE.GetKeyCode())
    {
        case WXK_F2:
        {
            const long SelNr=GetFirstSelected();

            if (SelNr!=-1) EditLabel(SelNr);
            break;
        }

        default:
            KE.Skip();
            break;
    }
}


void GroupsListViewT::OnMouseLeftDown(wxMouseEvent& ME)
{
    int           Flags  =0;
    int           ItemNr =HitTest(ME.GetPosition(), Flags);
    unsigned long ItemNrU=(unsigned long)ItemNr;
    MapDocumentT* MapDoc =m_Parent->m_MapDoc;

    if (ItemNr!=wxNOT_FOUND && MapDoc!=NULL && ItemNrU<MapDoc->GetGroups().Size())
    {
        GroupT* Group=MapDoc->GetGroups()[ItemNrU];
        wxRect  Rect;

        if (GetSubItemRect(ItemNr, 0, Rect, wxLIST_RECT_ICON) && Rect.Contains(ME.GetPosition()))
        {
            // The new visibility will be the opposite from Group->IsVisible, so set the icon accordingly.
            SetItemColumnImage(ItemNr, 0, !Group->IsVisible ? ICON_EYE : ICON_EYE_GREY);

            m_Parent->OnToggleVisibility(ItemNr);
            return;
        }

        if (GetSubItemRect(ItemNr, 1, Rect, wxLIST_RECT_ICON) && Rect.Contains(ME.GetPosition()))
        {
            // The new visibility will be the opposite from Group->IsVisible, so set the icon accordingly.
            SetItemColumnImage(ItemNr, 1, !Group->CanSelect ? ICON_EDIT : ICON_LOCK);

            m_Parent->OnToggleProperty(ItemNr, CommandGroupSetPropT::PROP_CANSELECT);
            return;
        }

        if (GetSubItemRect(ItemNr, 2, Rect, wxLIST_RECT_ICON) && Rect.Contains(ME.GetPosition()))
        {
            // The new visibility will be the opposite from Group->IsVisible, so set the icon accordingly.
            SetItemColumnImage(ItemNr, 2, !Group->SelectAsGroup ? ICON_SELECT_ASGROUP : ICON_SELECT_INDIV);

            m_Parent->OnToggleProperty(ItemNr, CommandGroupSetPropT::PROP_SELECTASGROUP);
            return;
        }

        // It's not a click on one of the icons, so it's likely on the label (the group name), or at least somewhere in the same row.
        // If the click was a double-click, treat it like a "Select" popup menu event.
        // Note that this double-click processing does NOT get into the way of label editing (the built-in feature of the wxListView),
        // because label editing is initiated with a single-click on an already selected item.
        if (ME.ButtonDClick())
        {
            wxCommandEvent CE(wxEVT_NULL, GroupsToolbarT::ID_MENU_SELECT);
            m_Parent->OnMenu(CE);
            return;
        }
    }

    // The mouse event was not processed above.
    // Skip it to continue default processing.
    ME.Skip();
}


static wxMenuItem* GetMI(wxMenu* Menu, int MenuID, const wxString& Text, const wxString& Help, const wxBitmap& Bitmap)
{
    wxMenuItem* MI=new wxMenuItem(Menu, MenuID, Text, Help);

#ifndef __WXMSW__
    // The images are correct (with alpha channel), but they don't work correctly under (wx)MSW,
    // see http://trac.wxwidgets.org/ticket/9050 for more information.
    MI->SetBitmap(Bitmap);
#endif

    return MI;
}


void GroupsListViewT::OnContextMenu(wxContextMenuEvent& CE)
{
    wxMenu  GroupsPopupMenu;
    wxMenu* EditMenu=new wxMenu();

    const wxImageList* ImgList=GetImageList(wxIMAGE_LIST_SMALL);

 // GroupsPopupMenu.SetTitle("Groups menu");    // This confusingly looks just like a menu item in bold font.
    GroupsPopupMenu.Append(GetMI(&GroupsPopupMenu, GroupsToolbarT::ID_MENU_SELECT, "&Select", "Selects all member elements of the group.", ImgList->GetBitmap(ICON_MOUSE_CURSOR)));
    EditMenu->Append(GetMI(EditMenu, GroupsToolbarT::ID_MENU_EDIT_RENAME,     "Rename\tF2", "", ImgList->GetBitmap(ICON_RENAME)));
    EditMenu->Append(GetMI(EditMenu, GroupsToolbarT::ID_MENU_EDIT_SETCOLOR,   "Set color", "", ImgList->GetBitmap(ICON_COLORWHEEL)));
    EditMenu->AppendSeparator();
    EditMenu->Append(GetMI(EditMenu, GroupsToolbarT::ID_MENU_EDIT_SHOW,       "&Show", "Show the members of the group.", ImgList->GetBitmap(ICON_EYE)));
    EditMenu->Append(GetMI(EditMenu, GroupsToolbarT::ID_MENU_EDIT_HIDE,       "&Hide", "Hide the members of the group.", ImgList->GetBitmap(ICON_EYE_GREY)));
    EditMenu->Append(GetMI(EditMenu, GroupsToolbarT::ID_MENU_EDIT_CANSELECT,  "&Can select", "The members of the group can be selected normally.", ImgList->GetBitmap(ICON_EDIT)));
    EditMenu->Append(GetMI(EditMenu, GroupsToolbarT::ID_MENU_EDIT_LOCK,       "Ca&n't select (lock)", "Lock the members of the group (exclude them from getting selected).", ImgList->GetBitmap(ICON_LOCK)));
    EditMenu->Append(GetMI(EditMenu, GroupsToolbarT::ID_MENU_EDIT_SELASGROUP, "Select as &group", "All members of the group are selected 'as one'. Clicking one member selects the entire group.", ImgList->GetBitmap(ICON_SELECT_ASGROUP)));
    EditMenu->Append(GetMI(EditMenu, GroupsToolbarT::ID_MENU_EDIT_SELASINDIV, "Select &individually", "The members of the group are selected individually, their common group membership is ignored.", ImgList->GetBitmap(ICON_SELECT_INDIV)));
    GroupsPopupMenu.AppendSubMenu(EditMenu, "&Edit", "Edits the properties of the group.");
    GroupsPopupMenu.Append(GetMI(&GroupsPopupMenu, GroupsToolbarT::ID_MENU_DELETE, "&Dissolve", "Dissolves (breaks and deletes) the group. Its member elements become ungrouped again.", ImgList->GetBitmap(ICON_DELETE)));
    GroupsPopupMenu.Append(GetMI(&GroupsPopupMenu, GroupsToolbarT::ID_MENU_MERGE,  "&Merge groups", "Merges all selected groups into one.", ImgList->GetBitmap(ICON_MERGE)));
    GroupsPopupMenu.AppendSeparator();
    GroupsPopupMenu.Append(GetMI(&GroupsPopupMenu, GroupsToolbarT::ID_MENU_MOVEUP,   "Move &up", "Moves the group up in the list.", ImgList->GetBitmap(ICON_ARROW_UP)));
    GroupsPopupMenu.Append(GetMI(&GroupsPopupMenu, GroupsToolbarT::ID_MENU_MOVEDOWN, "Move &down", "Moves the group down in the list.", ImgList->GetBitmap(ICON_ARROW_DOWN)));

    PopupMenu(&GroupsPopupMenu);
}


void GroupsListViewT::OnSize(wxSizeEvent& SE)
{
    const wxSize Size=GetClientSize();

    SetColumnWidth(0, Size.x-16-16);
    SetColumnWidth(1, 16);
    SetColumnWidth(2, 16);
}


void GroupsListViewT::OnEndLabelEdit(wxListEvent& LE)
{
    MapDocumentT* MapDoc=m_Parent->m_MapDoc;
    unsigned long Index =LE.GetIndex();

    if (LE.IsEditCancelled()) return;
    if (Index>=MapDoc->GetGroups().Size()) return;

    m_Parent->m_IsRecursiveSelfNotify=true;
    MapDoc->GetHistory().SubmitCommand(new CommandGroupSetPropT(*MapDoc, MapDoc->GetGroups()[Index], LE.GetLabel()));
    m_Parent->m_IsRecursiveSelfNotify=false;
}


BEGIN_EVENT_TABLE(GroupsToolbarT, wxPanel)
    EVT_MENU_RANGE     (GroupsToolbarT::ID_MENU_SELECT, GroupsToolbarT::ID_MENU_MOVEDOWN, GroupsToolbarT::OnMenu)
    EVT_UPDATE_UI_RANGE(GroupsToolbarT::ID_MENU_SELECT, GroupsToolbarT::ID_MENU_MOVEDOWN, GroupsToolbarT::OnMenuUpdate)
END_EVENT_TABLE()


GroupsToolbarT::GroupsToolbarT(wxWindow* Parent, MapDocumentT* MapDoc)
    : wxPanel(Parent, -1, wxDefaultPosition, wxDefaultSize),
      m_MapDoc(MapDoc),
      m_ListView(NULL),
      m_IsRecursiveSelfNotify(false)
{
    // As we are now a wxAUI pane rather than a wxDialog, explicitly set that events are not propagated to our parent.
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);

    wxBoxSizer *item0 = new wxBoxSizer(wxVERTICAL);

    // wxStaticText *item1 = new wxStaticText(this, -1, wxT("Groups:"), wxDefaultPosition, wxDefaultSize, 0 );
    // item0->Add( item1, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );

    m_ListView=new GroupsListViewT(this, ID_LISTVIEW_GROUPS);
    item0->Add(m_ListView, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP|wxBOTTOM, 5 );

    this->SetSizer(item0);
    item0->SetSizeHints(this);


    m_MapDoc->RegisterObserver(this);
    NotifySubjectChanged_Groups(m_MapDoc);
}


GroupsToolbarT::~GroupsToolbarT()
{
    if (m_MapDoc)
        m_MapDoc->UnregisterObserver(this);
}


void GroupsToolbarT::NotifySubjectChanged_Groups(SubjectT* Subject)
{
    wxASSERT(Subject==m_MapDoc);
    if (m_IsRecursiveSelfNotify) return;

    ArrayT<GroupT*> SelGroups=m_ListView->GetSelectedGroups();

    m_ListView->DeleteAllItems();

    for (unsigned long GroupNr=0; GroupNr<m_MapDoc->GetGroups().Size(); GroupNr++)
    {
        GroupT* Group=m_MapDoc->GetGroups()[GroupNr];

        m_ListView->InsertItem(GroupNr, Group->Name, Group->IsVisible ? ICON_EYE : ICON_EYE_GREY);
        m_ListView->SetItem(GroupNr, 1, "", Group->CanSelect ? ICON_EDIT : ICON_LOCK);
        m_ListView->SetItem(GroupNr, 2, "", Group->SelectAsGroup ? ICON_SELECT_ASGROUP : ICON_SELECT_INDIV);

        m_ListView->SetClientData(GroupNr, Group);
        m_ListView->SetItemBackgroundColour(GroupNr, Group->Color); // The groups color is also the related items background color.
        if (SelGroups.Find(Group)!=-1) m_ListView->Select(GroupNr); // Restore the previous group selection state.
    }
}


void GroupsToolbarT::NotifySubjectDies(SubjectT* Subject)
{
    wxASSERT(Subject==m_MapDoc);

    m_MapDoc=NULL;
}


void GroupsToolbarT::OnToggleVisibility(unsigned long GroupNr)
{
    if (!m_MapDoc) return;
    if (GroupNr>=m_MapDoc->GetGroups().Size()) return;

    GroupT* Group=m_MapDoc->GetGroups()[GroupNr];

    // The overall intention of this code is to deal with cases where the user toggles the visibility of a group
    // multiple times in a row, without submitting another command (that is shown in the history) in between.
    // A trivial implementation would add a new command to the history for each mouse click, possibly growing it enormously.
    // Alternatively, flagging the "set group visibility" command as "not shown in history" would only conceal the problem
    // while at the same time removing the users ability to explicitly see and undo the command in the menu.
    // Therefore, we try to re-use a readily available command (at the current position in the history) whenever possible,
    // and submit a new "set group visibility" command only when necessary.

    // First, try to achieve the desired action by undoing a previous "set group visibility" command.
    {
        const CommandGroupSetVisibilityT* UndoVisCmd=dynamic_cast<const CommandGroupSetVisibilityT*>(m_MapDoc->GetHistory().GetUndoCommand());

        if (UndoVisCmd && UndoVisCmd->GetGroup()==Group)
        {
            m_IsRecursiveSelfNotify=true;
            m_MapDoc->GetHistory().Undo();
            m_IsRecursiveSelfNotify=false;
            return;
        }
    }

    // Second, try to achieve the desired action by redoing a following "set group visibility" command.
    {
        const CommandGroupSetVisibilityT* RedoVisCmd=dynamic_cast<const CommandGroupSetVisibilityT*>(m_MapDoc->GetHistory().GetRedoCommand());

        if (RedoVisCmd && RedoVisCmd->GetGroup()==Group)
        {
            m_IsRecursiveSelfNotify=true;
            m_MapDoc->GetHistory().Redo();
            m_IsRecursiveSelfNotify=false;
            return;
        }
    }

    // Third and finally, the normal case: Submit a new command for toggling the visibility of the group.
    m_IsRecursiveSelfNotify=true;
    m_MapDoc->GetHistory().SubmitCommand(new CommandGroupSetVisibilityT(*m_MapDoc, Group, !Group->IsVisible));
    m_IsRecursiveSelfNotify=false;
}


void GroupsToolbarT::OnToggleProperty(unsigned long GroupNr, CommandGroupSetPropT::PropT Prop)
{
    if (!m_MapDoc) return;
    if (GroupNr>=m_MapDoc->GetGroups().Size()) return;

    // The overall intention of this code is the same as in OnToggleVisibility().
    GroupT* Group=m_MapDoc->GetGroups()[GroupNr];

    // First, try to achieve the desired action by undoing a previous "set group property" command.
    {
        const CommandGroupSetPropT* UndoPropCmd=dynamic_cast<const CommandGroupSetPropT*>(m_MapDoc->GetHistory().GetUndoCommand());

        if (UndoPropCmd && UndoPropCmd->GetGroup()==Group && UndoPropCmd->GetProp()==Prop)
        {
            m_IsRecursiveSelfNotify=true;
            m_MapDoc->GetHistory().Undo();
            m_IsRecursiveSelfNotify=false;
            return;
        }
    }

    // Second, try to achieve the desired action by redoing a following "set group property" command.
    {
        const CommandGroupSetPropT* RedoPropCmd=dynamic_cast<const CommandGroupSetPropT*>(m_MapDoc->GetHistory().GetRedoCommand());

        if (RedoPropCmd && RedoPropCmd->GetGroup()==Group && RedoPropCmd->GetProp()==Prop)
        {
            m_IsRecursiveSelfNotify=true;
            m_MapDoc->GetHistory().Redo();
            m_IsRecursiveSelfNotify=false;
            return;
        }
    }

    // Third and finally, the normal case: Submit a new command for toggling the property of the group.
    m_IsRecursiveSelfNotify=true;
    m_MapDoc->GetHistory().SubmitCommand(new CommandGroupSetPropT(*m_MapDoc, Group, Prop,
        !(Prop==CommandGroupSetPropT::PROP_CANSELECT ? Group->CanSelect : Group->SelectAsGroup)));
    m_IsRecursiveSelfNotify=false;
}


void GroupsToolbarT::OnMenu(wxCommandEvent& CE)
{
    const ArrayT<GroupT*> SelGroups=m_ListView->GetSelectedGroups();

    if (!m_MapDoc) return;
    if (SelGroups.Size()==0) return;


    switch (CE.GetId())
    {
        case ID_MENU_SELECT:
        {
            // Note that SelGroups contains both visible and invisible groups. That's perfectly fine with us, there is
            // no need to filter out the invisible groups: if only invisible groups are selected, the selection gets cleared.
            ArrayT<MapElementT*> Elems;
            m_MapDoc->GetAllElems(Elems);

            // Only keep elements in the list that are in one of the SelGroups and visible, remove all others.
            for (unsigned long ElemNr=0; ElemNr<Elems.Size(); ElemNr++)
            {
                MapElementT* Elem=Elems[ElemNr];

                if (SelGroups.Find(Elem->GetGroup())==-1 || !Elem->IsVisible())
                {
                    Elems.RemoveAt(ElemNr);
                    ElemNr--;
                }
            }

            // Select the elements in Elems (which may be empty).
            m_MapDoc->GetHistory().SubmitCommand(CommandSelectT::Set(m_MapDoc, Elems));
            break;
        }

        case ID_MENU_EDIT_RENAME:
        {
            const wxString NewName=wxGetTextFromUser("Please enter the new name for the group.", "Rename group", SelGroups[0]->Name, this);
            if (NewName=="") break;

            ArrayT<CommandT*> SubCommands;
            for (unsigned long GroupNr=0; GroupNr<SelGroups.Size(); GroupNr++)
                SubCommands.PushBack(new CommandGroupSetPropT(*m_MapDoc, SelGroups[GroupNr], NewName));

            m_MapDoc->GetHistory().SubmitCommand(new CommandMacroT(SubCommands, SubCommands[0]->GetName()));
            break;
        }

        case ID_MENU_EDIT_SETCOLOR:
        {
            const wxColour NewColor=wxGetColourFromUser(this, SelGroups[0]->Color, "Choose new group color");
            if (!NewColor.Ok()) break;

            ArrayT<CommandT*> SubCommands;
            for (unsigned long GroupNr=0; GroupNr<SelGroups.Size(); GroupNr++)
                SubCommands.PushBack(new CommandGroupSetPropT(*m_MapDoc, SelGroups[GroupNr], NewColor));

            m_MapDoc->GetHistory().SubmitCommand(new CommandMacroT(SubCommands, SubCommands[0]->GetName()));
            break;
        }

        case ID_MENU_EDIT_SHOW:
        case ID_MENU_EDIT_HIDE:
        {
            const bool        WantVisible=(CE.GetId()==ID_MENU_EDIT_SHOW);
            ArrayT<CommandT*> SubCommands;

            for (unsigned long GroupNr=0; GroupNr<SelGroups.Size(); GroupNr++)
                if (SelGroups[GroupNr]->IsVisible!=WantVisible)
                    SubCommands.PushBack(new CommandGroupSetVisibilityT(*m_MapDoc, SelGroups[GroupNr], WantVisible));

            if (SubCommands.Size()>0)
                m_MapDoc->GetHistory().SubmitCommand(new CommandMacroT(SubCommands, wxString(WantVisible ? "Show " : "Hide ") + wxString(SubCommands.Size()==1 ? "group" : "groups")));
            break;
        }

        case ID_MENU_EDIT_CANSELECT:
        case ID_MENU_EDIT_LOCK:
        {
            const bool        WantCanSelect=(CE.GetId()==ID_MENU_EDIT_CANSELECT);
            ArrayT<CommandT*> SubCommands;

            for (unsigned long GroupNr=0; GroupNr<SelGroups.Size(); GroupNr++)
                if (SelGroups[GroupNr]->CanSelect!=WantCanSelect)
                    SubCommands.PushBack(new CommandGroupSetPropT(*m_MapDoc, SelGroups[GroupNr],
                        CommandGroupSetPropT::PROP_CANSELECT, WantCanSelect));

            if (SubCommands.Size()>0)
                m_MapDoc->GetHistory().SubmitCommand(new CommandMacroT(SubCommands, SubCommands[0]->GetName()));
            break;
        }

        case ID_MENU_EDIT_SELASGROUP:
        case ID_MENU_EDIT_SELASINDIV:
        {
            const bool        WantSelAsGroup=(CE.GetId()==ID_MENU_EDIT_SELASGROUP);
            ArrayT<CommandT*> SubCommands;

            for (unsigned long GroupNr=0; GroupNr<SelGroups.Size(); GroupNr++)
                if (SelGroups[GroupNr]->SelectAsGroup!=WantSelAsGroup)
                    SubCommands.PushBack(new CommandGroupSetPropT(*m_MapDoc, SelGroups[GroupNr],
                        CommandGroupSetPropT::PROP_SELECTASGROUP, WantSelAsGroup));

            if (SubCommands.Size()>0)
                m_MapDoc->GetHistory().SubmitCommand(new CommandMacroT(SubCommands, SubCommands[0]->GetName()));
            break;
        }

        case ID_MENU_DELETE:
        {
            m_MapDoc->GetHistory().SubmitCommand(new CommandDeleteGroupT(*m_MapDoc, SelGroups));
            break;
        }

        case ID_MENU_MERGE:
        {
            if (SelGroups.Size()<2) break;

            wxString NewName=SelGroups[0]->Name;

            for (unsigned long GroupNr=1; GroupNr<SelGroups.Size(); GroupNr++)
                NewName+=" + "+SelGroups[GroupNr]->Name;

            NewName=wxGetTextFromUser("Please enter the name for the merged group.", "Merge groups", NewName, this);
            if (NewName=="") break;


            ArrayT<CommandT*> SubCommands;

            // 1. Set the new name for the resulting merged group.
            CommandGroupSetPropT* CmdRename=new CommandGroupSetPropT(*m_MapDoc, SelGroups[0], NewName);

            CmdRename->Do();
            SubCommands.PushBack(CmdRename);

            // 2. Assign all members from the other groups to the first group.
            for (unsigned long GroupNr=1; GroupNr<SelGroups.Size(); GroupNr++)
            {
                CommandAssignGroupT* CmdAssign=new CommandAssignGroupT(*m_MapDoc, SelGroups[GroupNr]->GetMembers(*m_MapDoc), SelGroups[0]);

                CmdAssign->Do();
                SubCommands.PushBack(CmdAssign);
            }

            // 3. Delete all groups but the first.
            ArrayT<GroupT*> CollectedGroups=SelGroups;
            CollectedGroups.RemoveAt(0);
            CommandDeleteGroupT* CmdDelete=new CommandDeleteGroupT(*m_MapDoc, CollectedGroups);

            CmdDelete->Do();
            SubCommands.PushBack(CmdDelete);

            // 4. Submit the composite macro command.
            m_MapDoc->GetHistory().SubmitCommand(new CommandMacroT(SubCommands, "Merge groups"));
            break;
        }

        case ID_MENU_MOVEUP:
        {
            // Move all selected groups up one step.
            ArrayT<GroupT*> NewOrder=m_MapDoc->GetGroups();
            unsigned long   PosNr=1;

            for (unsigned long GroupNr=0; GroupNr<SelGroups.Size(); GroupNr++)
            {
                while (PosNr<NewOrder.Size() && NewOrder[PosNr]!=SelGroups[GroupNr]) PosNr++;
                if (PosNr>=NewOrder.Size()) break;
                std::swap(NewOrder[PosNr-1], NewOrder[PosNr]);
            }

            if (PosNr<NewOrder.Size())
                m_MapDoc->GetHistory().SubmitCommand(new CommandReorderGroupsT(*m_MapDoc, NewOrder));
            break;
        }

        case ID_MENU_MOVEDOWN:
        {
            // Move all selected groups down one step.
            // In order to avoid having to use signed integers for the loop variables,
            // we simply "imagine" that the arrays were indexed/numbered "the other way round".
            ArrayT<GroupT*> NewOrder=m_MapDoc->GetGroups();
            unsigned long   PosNr=1;

            for (unsigned long GroupNr=0; GroupNr<SelGroups.Size(); GroupNr++)
            {
                while (PosNr<NewOrder.Size() && NewOrder[NewOrder.Size()-1-PosNr]!=SelGroups[SelGroups.Size()-1-GroupNr]) PosNr++;
                if (PosNr>=NewOrder.Size()) break;
                std::swap(NewOrder[NewOrder.Size()-1-(PosNr-1)], NewOrder[NewOrder.Size()-1-PosNr]);
            }

            if (PosNr<NewOrder.Size())
                m_MapDoc->GetHistory().SubmitCommand(new CommandReorderGroupsT(*m_MapDoc, NewOrder));
            break;
        }
    }
}


void GroupsToolbarT::OnMenuUpdate(wxUpdateUIEvent& UE)
{
    const int SelCount=m_ListView->GetSelectedItemCount();

    switch (UE.GetId())
    {
        case ID_MENU_MERGE:
            UE.Enable(SelCount>1);
            break;

        default:
            UE.Enable(SelCount>0);
            break;
    }
}
