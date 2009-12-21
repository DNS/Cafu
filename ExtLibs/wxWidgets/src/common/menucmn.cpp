///////////////////////////////////////////////////////////////////////////////
// Name:        src/common/menucmn.cpp
// Purpose:     wxMenu and wxMenuBar methods common to all ports
// Author:      Vadim Zeitlin
// Modified by:
// Created:     26.10.99
// RCS-ID:      $Id: menucmn.cpp 58227 2009-01-19 13:55:27Z VZ $
// Copyright:   (c) wxWidgets team
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_MENUS

#ifndef WX_PRECOMP
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/menu.h"
#endif

#include "wx/stockitem.h"

// ----------------------------------------------------------------------------
// template lists
// ----------------------------------------------------------------------------

#include "wx/listimpl.cpp"

WX_DEFINE_LIST(wxMenuList)
WX_DEFINE_LIST(wxMenuItemList)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxMenuItemBase
// ----------------------------------------------------------------------------

wxMenuItemBase::wxMenuItemBase(wxMenu *parentMenu,
                               int id,
                               const wxString& text,
                               const wxString& help,
                               wxItemKind kind,
                               wxMenu *subMenu)
{
    // notice that parentMenu can be NULL: the item can be attached to the menu
    // later with SetMenu()

    m_parentMenu  = parentMenu;
    m_subMenu     = subMenu;
    m_isEnabled   = true;
    m_isChecked   = false;
    m_id          = id;
    m_kind        = kind;
    if (m_id == wxID_ANY)
        m_id = wxWindow::NewControlId();
    if (m_id == wxID_SEPARATOR)
        m_kind = wxITEM_SEPARATOR;

    SetItemLabel(text);
    SetHelp(help);
}

wxMenuItemBase::~wxMenuItemBase()
{
    delete m_subMenu;
}

#if wxUSE_ACCEL

wxAcceleratorEntry *wxMenuItemBase::GetAccel() const
{
    return wxAcceleratorEntry::Create(GetItemLabel());
}

void wxMenuItemBase::SetAccel(wxAcceleratorEntry *accel)
{
    wxString text = m_text.BeforeFirst(wxT('\t'));
    if ( accel )
    {
        text += wxT('\t');
        text += accel->ToString();
    }

    SetItemLabel(text);
}

#endif // wxUSE_ACCEL

void wxMenuItemBase::SetItemLabel(const wxString& str)
{
    m_text = str;

    if ( m_text.empty() && !IsSeparator() )
    {
        wxASSERT_MSG( wxIsStockID(GetId()),
                      wxT("A non-stock menu item with an empty label?") );
        m_text = wxGetStockLabel(GetId(), wxSTOCK_WITH_ACCELERATOR |
                                          wxSTOCK_WITH_MNEMONIC);
    }
}

void wxMenuItemBase::SetHelp(const wxString& str)
{
    m_help = str;

    if ( m_help.empty() && !IsSeparator() && wxIsStockID(GetId()) )
    {
        // get a stock help string
        m_help = wxGetStockHelpString(GetId());
    }
}

#ifndef __WXPM__
wxString wxMenuItemBase::GetLabelText(const wxString& text)
{
    return wxStripMenuCodes(text);
}
#endif

#if WXWIN_COMPATIBILITY_2_8
wxString wxMenuItemBase::GetLabelFromText(const wxString& text)
{
    return GetLabelText(text);
}
#endif

bool wxMenuBase::ms_locked = true;

// ----------------------------------------------------------------------------
// wxMenu ctor and dtor
// ----------------------------------------------------------------------------

void wxMenuBase::Init(long style)
{
    m_menuBar = NULL;
    m_menuParent = NULL;

    m_invokingWindow = NULL;
    m_style = style;
    m_clientData = NULL;
    m_eventHandler = this;
}

wxMenuBase::~wxMenuBase()
{
    WX_CLEAR_LIST(wxMenuItemList, m_items);
}

// ----------------------------------------------------------------------------
// wxMenu item adding/removing
// ----------------------------------------------------------------------------

void wxMenuBase::AddSubMenu(wxMenu *submenu)
{
    wxCHECK_RET( submenu, _T("can't add a NULL submenu") );

    submenu->SetParent((wxMenu *)this);
}

wxMenuItem* wxMenuBase::DoAppend(wxMenuItem *item)
{
    wxCHECK_MSG( item, NULL, wxT("invalid item in wxMenu::Append()") );

    m_items.Append(item);
    item->SetMenu((wxMenu*)this);
    if ( item->IsSubMenu() )
    {
        AddSubMenu(item->GetSubMenu());
    }

    return item;
}

wxMenuItem* wxMenuBase::Insert(size_t pos, wxMenuItem *item)
{
    wxCHECK_MSG( item, NULL, wxT("invalid item in wxMenu::Insert") );

    if ( pos == GetMenuItemCount() )
    {
        return DoAppend(item);
    }
    else
    {
        wxCHECK_MSG( pos < GetMenuItemCount(), NULL,
                     wxT("invalid index in wxMenu::Insert") );

        return DoInsert(pos, item);
    }
}

wxMenuItem* wxMenuBase::DoInsert(size_t pos, wxMenuItem *item)
{
    wxCHECK_MSG( item, NULL, wxT("invalid item in wxMenu::Insert()") );

    wxMenuItemList::compatibility_iterator node = m_items.Item(pos);
    wxCHECK_MSG( node, NULL, wxT("invalid index in wxMenu::Insert()") );

    m_items.Insert(node, item);
    item->SetMenu((wxMenu*)this);
    if ( item->IsSubMenu() )
    {
        AddSubMenu(item->GetSubMenu());
    }

    return item;
}

wxMenuItem *wxMenuBase::Remove(wxMenuItem *item)
{
    wxCHECK_MSG( item, NULL, wxT("invalid item in wxMenu::Remove") );

    return DoRemove(item);
}

wxMenuItem *wxMenuBase::DoRemove(wxMenuItem *item)
{
    wxMenuItemList::compatibility_iterator node = m_items.Find(item);

    // if we get here, the item is valid or one of Remove() functions is broken
    wxCHECK_MSG( node, NULL, wxT("bug in wxMenu::Remove logic") );

    // we detach the item, but we do delete the list node (i.e. don't call
    // DetachNode() here!)
    m_items.Erase(node);

    // item isn't attached to anything any more
    item->SetMenu(NULL);
    wxMenu *submenu = item->GetSubMenu();
    if ( submenu )
    {
        submenu->SetParent(NULL);
        if ( submenu->IsAttached() )
            submenu->Detach();
    }

    return item;
}

bool wxMenuBase::Delete(wxMenuItem *item)
{
    wxCHECK_MSG( item, false, wxT("invalid item in wxMenu::Delete") );

    return DoDelete(item);
}

bool wxMenuBase::DoDelete(wxMenuItem *item)
{
    wxMenuItem *item2 = DoRemove(item);
    wxCHECK_MSG( item2, false, wxT("failed to delete menu item") );

    // don't delete the submenu
    item2->SetSubMenu(NULL);

    delete item2;

    return true;
}

bool wxMenuBase::Destroy(wxMenuItem *item)
{
    wxCHECK_MSG( item, false, wxT("invalid item in wxMenu::Destroy") );

    return DoDestroy(item);
}

bool wxMenuBase::DoDestroy(wxMenuItem *item)
{
    wxMenuItem *item2 = DoRemove(item);
    wxCHECK_MSG( item2, false, wxT("failed to delete menu item") );

    delete item2;

    return true;
}

// ----------------------------------------------------------------------------
// wxMenu searching for items
// ----------------------------------------------------------------------------

// Finds the item id matching the given string, wxNOT_FOUND if not found.
int wxMenuBase::FindItem(const wxString& text) const
{
    wxString label = wxMenuItem::GetLabelText(text);
    for ( wxMenuItemList::compatibility_iterator node = m_items.GetFirst();
          node;
          node = node->GetNext() )
    {
        wxMenuItem *item = node->GetData();
        if ( item->IsSubMenu() )
        {
            int rc = item->GetSubMenu()->FindItem(label);
            if ( rc != wxNOT_FOUND )
                return rc;
        }

        // we execute this code for submenus as well to alllow finding them by
        // name just like the ordinary items
        if ( !item->IsSeparator() )
        {
            if ( item->GetItemLabelText() == label )
                return item->GetId();
        }
    }

    return wxNOT_FOUND;
}

// recursive search for item by id
wxMenuItem *wxMenuBase::FindItem(int itemId, wxMenu **itemMenu) const
{
    if ( itemMenu )
        *itemMenu = NULL;

    wxMenuItem *item = NULL;
    for ( wxMenuItemList::compatibility_iterator node = m_items.GetFirst();
          node && !item;
          node = node->GetNext() )
    {
        item = node->GetData();

        if ( item->GetId() == itemId )
        {
            if ( itemMenu )
                *itemMenu = (wxMenu *)this;
        }
        else if ( item->IsSubMenu() )
        {
            item = item->GetSubMenu()->FindItem(itemId, itemMenu);
        }
        else
        {
            // don't exit the loop
            item = NULL;
        }
    }

    return item;
}

// non recursive search
wxMenuItem *wxMenuBase::FindChildItem(int id, size_t *ppos) const
{
    wxMenuItem *item = NULL;
    wxMenuItemList::compatibility_iterator node = GetMenuItems().GetFirst();

    size_t pos;
    for ( pos = 0; node; pos++ )
    {
        if ( node->GetData()->GetId() == id )
        {
            item = node->GetData();

            break;
        }

        node = node->GetNext();
    }

    if ( ppos )
    {
        *ppos = item ? pos : (size_t)wxNOT_FOUND;
    }

    return item;
}

// find by position
wxMenuItem* wxMenuBase::FindItemByPosition(size_t position) const
{
    wxCHECK_MSG( position < m_items.GetCount(), NULL,
                 _T("wxMenu::FindItemByPosition(): invalid menu index") );

    return m_items.Item( position )->GetData();
}

// ----------------------------------------------------------------------------
// wxMenu helpers used by derived classes
// ----------------------------------------------------------------------------

// Update a menu and all submenus recursively. source is the object that has
// the update event handlers defined for it. If NULL, the menu or associated
// window will be used.
void wxMenuBase::UpdateUI(wxEvtHandler* source)
{
    if (GetInvokingWindow())
    {
        // Don't update menus if the parent
        // frame is about to get deleted
        wxWindow *tlw = wxGetTopLevelParent( GetInvokingWindow() );
        if (tlw && wxPendingDelete.Member(tlw))
            return;
    }

    if ( !source && GetInvokingWindow() )
        source = GetInvokingWindow()->GetEventHandler();
    if ( !source )
        source = GetEventHandler();
    if ( !source )
        source = this;

    wxMenuItemList::compatibility_iterator  node = GetMenuItems().GetFirst();
    while ( node )
    {
        wxMenuItem* item = node->GetData();
        if ( !item->IsSeparator() )
        {
            wxWindowID id = item->GetId();
            wxUpdateUIEvent event(id);
            event.SetEventObject( source );

            if ( source->ProcessEvent(event) )
            {
                // if anything changed, update the changed attribute
                if (event.GetSetText())
                    SetLabel(id, event.GetText());
                if (event.GetSetChecked())
                    Check(id, event.GetChecked());
                if (event.GetSetEnabled())
                    Enable(id, event.GetEnabled());
            }

            // recurse to the submenus
            if ( item->GetSubMenu() )
                item->GetSubMenu()->UpdateUI(source);
        }
        //else: item is a separator (which doesn't process update UI events)

        node = node->GetNext();
    }
}

bool wxMenuBase::SendEvent(int id, int checked)
{
    wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, id);
    event.SetEventObject(this);
    event.SetInt(checked);

    bool processed = false;

    // Try the menu's event handler
    // if ( !processed )
    {
        wxEvtHandler *handler = GetEventHandler();
        if ( handler )
            processed = handler->SafelyProcessEvent(event);
    }

    // Try the window the menu was popped up from (and up through the
    // hierarchy)
    if ( !processed )
    {
        const wxMenuBase *menu = this;
        while ( menu )
        {
            wxWindow *win = menu->GetInvokingWindow();
            if ( win )
            {
                processed = win->HandleWindowEvent(event);
                break;
            }

            menu = menu->GetParent();
        }
    }

    return processed;
}

// ----------------------------------------------------------------------------
// wxMenu attaching/detaching to/from menu bar
// ----------------------------------------------------------------------------

wxMenuBar* wxMenuBase::GetMenuBar() const
{
    if(GetParent())
        return GetParent()->GetMenuBar();
    return m_menuBar;
}

void wxMenuBase::Attach(wxMenuBarBase *menubar)
{
    // use Detach() instead!
    wxASSERT_MSG( menubar, _T("menu can't be attached to NULL menubar") );

    // use IsAttached() to prevent this from happening
    wxASSERT_MSG( !m_menuBar, _T("attaching menu twice?") );

    m_menuBar = (wxMenuBar *)menubar;
}

void wxMenuBase::Detach()
{
    // use IsAttached() to prevent this from happening
    wxASSERT_MSG( m_menuBar, _T("detaching unattached menu?") );

    m_menuBar = NULL;
}

// ----------------------------------------------------------------------------
// wxMenu functions forwarded to wxMenuItem
// ----------------------------------------------------------------------------

void wxMenuBase::Enable( int id, bool enable )
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_RET( item, wxT("wxMenu::Enable: no such item") );

    item->Enable(enable);
}

bool wxMenuBase::IsEnabled( int id ) const
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_MSG( item, false, wxT("wxMenu::IsEnabled: no such item") );

    return item->IsEnabled();
}

void wxMenuBase::Check( int id, bool enable )
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_RET( item, wxT("wxMenu::Check: no such item") );

    item->Check(enable);
}

bool wxMenuBase::IsChecked( int id ) const
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_MSG( item, false, wxT("wxMenu::IsChecked: no such item") );

    return item->IsChecked();
}

void wxMenuBase::SetLabel( int id, const wxString &label )
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_RET( item, wxT("wxMenu::SetLabel: no such item") );

    item->SetItemLabel(label);
}

wxString wxMenuBase::GetLabel( int id ) const
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_MSG( item, wxEmptyString, wxT("wxMenu::GetLabel: no such item") );

    return item->GetItemLabel();
}

void wxMenuBase::SetHelpString( int id, const wxString& helpString )
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_RET( item, wxT("wxMenu::SetHelpString: no such item") );

    item->SetHelp( helpString );
}

wxString wxMenuBase::GetHelpString( int id ) const
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_MSG( item, wxEmptyString, wxT("wxMenu::GetHelpString: no such item") );

    return item->GetHelp();
}

// ----------------------------------------------------------------------------
// wxMenuBarBase ctor and dtor
// ----------------------------------------------------------------------------

wxMenuBarBase::wxMenuBarBase()
{
    // not attached yet
    m_menuBarFrame = NULL;
}

wxMenuBarBase::~wxMenuBarBase()
{
    WX_CLEAR_LIST(wxMenuList, m_menus);
}

// ----------------------------------------------------------------------------
// wxMenuBar item access: the base class versions manage m_menus list, the
// derived class should reflect the changes in the real menubar
// ----------------------------------------------------------------------------

wxMenu *wxMenuBarBase::GetMenu(size_t pos) const
{
    wxMenuList::compatibility_iterator node = m_menus.Item(pos);
    wxCHECK_MSG( node, NULL, wxT("bad index in wxMenuBar::GetMenu()") );

    return node->GetData();
}

bool wxMenuBarBase::Append(wxMenu *menu, const wxString& WXUNUSED(title))
{
    wxCHECK_MSG( menu, false, wxT("can't append NULL menu") );

    m_menus.Append(menu);
    menu->Attach(this);

    return true;
}

bool wxMenuBarBase::Insert(size_t pos, wxMenu *menu,
                           const wxString& title)
{
    if ( pos == m_menus.GetCount() )
    {
        return wxMenuBarBase::Append(menu, title);
    }
    else // not at the end
    {
        wxCHECK_MSG( menu, false, wxT("can't insert NULL menu") );

        wxMenuList::compatibility_iterator node = m_menus.Item(pos);
        wxCHECK_MSG( node, false, wxT("bad index in wxMenuBar::Insert()") );

        m_menus.Insert(node, menu);
        menu->Attach(this);

        return true;
    }
}

wxMenu *wxMenuBarBase::Replace(size_t pos, wxMenu *menu,
                               const wxString& WXUNUSED(title))
{
    wxCHECK_MSG( menu, NULL, wxT("can't insert NULL menu") );

    wxMenuList::compatibility_iterator node = m_menus.Item(pos);
    wxCHECK_MSG( node, NULL, wxT("bad index in wxMenuBar::Replace()") );

    wxMenu *menuOld = node->GetData();
    node->SetData(menu);

    menu->Attach(this);
    menuOld->Detach();

    return menuOld;
}

wxMenu *wxMenuBarBase::Remove(size_t pos)
{
    wxMenuList::compatibility_iterator node = m_menus.Item(pos);
    wxCHECK_MSG( node, NULL, wxT("bad index in wxMenuBar::Remove()") );

    wxMenu *menu = node->GetData();
    m_menus.Erase(node);
    menu->Detach();

    return menu;
}

int wxMenuBarBase::FindMenu(const wxString& title) const
{
    wxString label = wxMenuItem::GetLabelText(title);

    size_t count = GetMenuCount();
    for ( size_t i = 0; i < count; i++ )
    {
        wxString title2 = GetMenuLabel(i);
        if ( (title2 == title) ||
             (wxMenuItem::GetLabelText(title2) == label) )
        {
            // found
            return (int)i;
        }
    }

    return wxNOT_FOUND;

}

// ----------------------------------------------------------------------------
// wxMenuBar attaching/detaching to/from the frame
// ----------------------------------------------------------------------------

void wxMenuBarBase::Attach(wxFrame *frame)
{
    wxASSERT_MSG( !IsAttached(), wxT("menubar already attached!") );

    m_menuBarFrame = frame;
}

void wxMenuBarBase::Detach()
{
    wxASSERT_MSG( IsAttached(), wxT("detaching unattached menubar") );

    m_menuBarFrame = NULL;
}

// ----------------------------------------------------------------------------
// wxMenuBar searching for items
// ----------------------------------------------------------------------------

wxMenuItem *wxMenuBarBase::FindItem(int id, wxMenu **menu) const
{
    if ( menu )
        *menu = NULL;

    wxMenuItem *item = NULL;
    size_t count = GetMenuCount(), i;
    wxMenuList::const_iterator it;
    for ( i = 0, it = m_menus.begin(); !item && (i < count); i++, it++ )
    {
        item = (*it)->FindItem(id, menu);
    }

    return item;
}

int wxMenuBarBase::FindMenuItem(const wxString& menu, const wxString& item) const
{
    wxString label = wxMenuItem::GetLabelText(menu);

    int i = 0;
    wxMenuList::compatibility_iterator node;
    for ( node = m_menus.GetFirst(); node; node = node->GetNext(), i++ )
    {
        if ( label == wxMenuItem::GetLabelText(GetMenuLabel(i)) )
            return node->GetData()->FindItem(item);
    }

    return wxNOT_FOUND;
}

// ---------------------------------------------------------------------------
// wxMenuBar functions forwarded to wxMenuItem
// ---------------------------------------------------------------------------

void wxMenuBarBase::Enable(int id, bool enable)
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_RET( item, wxT("attempt to enable an item which doesn't exist") );

    item->Enable(enable);
}

void wxMenuBarBase::Check(int id, bool check)
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_RET( item, wxT("attempt to check an item which doesn't exist") );
    wxCHECK_RET( item->IsCheckable(), wxT("attempt to check an uncheckable item") );

    item->Check(check);
}

bool wxMenuBarBase::IsChecked(int id) const
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_MSG( item, false, wxT("wxMenuBar::IsChecked(): no such item") );

    return item->IsChecked();
}

bool wxMenuBarBase::IsEnabled(int id) const
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_MSG( item, false, wxT("wxMenuBar::IsEnabled(): no such item") );

    return item->IsEnabled();
}

void wxMenuBarBase::SetLabel(int id, const wxString& label)
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_RET( item, wxT("wxMenuBar::SetLabel(): no such item") );

    item->SetItemLabel(label);
}

wxString wxMenuBarBase::GetLabel(int id) const
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_MSG( item, wxEmptyString,
                 wxT("wxMenuBar::GetLabel(): no such item") );

    return item->GetItemLabel();
}

void wxMenuBarBase::SetHelpString(int id, const wxString& helpString)
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_RET( item, wxT("wxMenuBar::SetHelpString(): no such item") );

    item->SetHelp(helpString);
}

wxString wxMenuBarBase::GetHelpString(int id) const
{
    wxMenuItem *item = FindItem(id);

    wxCHECK_MSG( item, wxEmptyString,
                 wxT("wxMenuBar::GetHelpString(): no such item") );

    return item->GetHelp();
}

void wxMenuBarBase::UpdateMenus()
{
    wxEvtHandler* source;
    wxMenu* menu;
    int nCount = GetMenuCount();
    for (int n = 0; n < nCount; n++)
    {
        menu = GetMenu( n );
        if (menu != NULL)
        {
            source = menu->GetEventHandler();
            if (source != NULL)
                menu->UpdateUI( source );
        }
    }
}

#if WXWIN_COMPATIBILITY_2_8
// get or change the label of the menu at given position
void wxMenuBarBase::SetLabelTop(size_t pos, const wxString& label)
{
    SetMenuLabel(pos, label);
}

wxString wxMenuBarBase::GetLabelTop(size_t pos) const
{
    return GetMenuLabelText(pos);
}
#endif

#endif // wxUSE_MENUS
