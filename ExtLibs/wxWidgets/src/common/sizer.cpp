/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/sizer.cpp
// Purpose:     provide new wxSizer class for layout
// Author:      Robert Roebling and Robin Dunn, contributions by
//              Dirk Holtwick, Ron Lee
// Modified by: Ron Lee
// Created:
// RCS-ID:      $Id: sizer.cpp 59725 2009-03-22 12:53:48Z VZ $
// Copyright:   (c) Robin Dunn, Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/sizer.h"
#include "wx/private/flagscheck.h"

#ifndef WX_PRECOMP
    #include "wx/string.h"
    #include "wx/intl.h"
    #include "wx/math.h"
    #include "wx/utils.h"
    #include "wx/settings.h"
    #include "wx/button.h"
    #include "wx/statbox.h"
    #include "wx/toplevel.h"
#endif // WX_PRECOMP

#include "wx/display.h"
#include "wx/listimpl.cpp"


//---------------------------------------------------------------------------

IMPLEMENT_CLASS(wxSizerItem, wxObject)
IMPLEMENT_CLASS(wxSizer, wxObject)
IMPLEMENT_CLASS(wxGridSizer, wxSizer)
IMPLEMENT_CLASS(wxFlexGridSizer, wxGridSizer)
IMPLEMENT_CLASS(wxBoxSizer, wxSizer)
#if wxUSE_STATBOX
IMPLEMENT_CLASS(wxStaticBoxSizer, wxBoxSizer)
#endif
#if wxUSE_BUTTON
IMPLEMENT_CLASS(wxStdDialogButtonSizer, wxBoxSizer)
#endif

WX_DEFINE_EXPORTED_LIST( wxSizerItemList )

/*
    TODO PROPERTIES
      sizeritem
        object
        object_ref
          minsize
          option
          flag
          border
     spacer
        option
        flag
        borfder
    boxsizer
       orient
    staticboxsizer
       orient
       label
    gridsizer
       rows
       cols
       vgap
       hgap
    flexgridsizer
       rows
       cols
       vgap
       hgap
       growablerows
       growablecols
    minsize
*/

// ----------------------------------------------------------------------------
// wxSizerItem
// ----------------------------------------------------------------------------

// check for flags conflicts
static const int SIZER_FLAGS_MASK =
    wxADD_FLAG(wxCENTRE,
    wxADD_FLAG(wxHORIZONTAL,
    wxADD_FLAG(wxVERTICAL,
    wxADD_FLAG(wxLEFT,
    wxADD_FLAG(wxRIGHT,
    wxADD_FLAG(wxUP,
    wxADD_FLAG(wxDOWN,
    wxADD_FLAG(wxALIGN_NOT,
    wxADD_FLAG(wxALIGN_CENTER_HORIZONTAL,
    wxADD_FLAG(wxALIGN_RIGHT,
    wxADD_FLAG(wxALIGN_BOTTOM,
    wxADD_FLAG(wxALIGN_CENTER_VERTICAL,
    wxADD_FLAG(wxFIXED_MINSIZE,
    wxADD_FLAG(wxRESERVE_SPACE_EVEN_IF_HIDDEN,
    wxADD_FLAG(wxSTRETCH_NOT,
    wxADD_FLAG(wxSHRINK,
    wxADD_FLAG(wxGROW,
    wxADD_FLAG(wxSHAPED,
    0))))))))))))))))));

#define ASSERT_VALID_SIZER_FLAGS(f)  wxASSERT_VALID_FLAGS(f, SIZER_FLAGS_MASK)


void wxSizerItem::Init(const wxSizerFlags& flags)
{
    Init();

    m_proportion = flags.GetProportion();
    m_flag = flags.GetFlags();
    m_border = flags.GetBorderInPixels();

    ASSERT_VALID_SIZER_FLAGS( m_flag );
}

wxSizerItem::wxSizerItem()
{
    Init();

    m_proportion = 0;
    m_border = 0;
    m_flag = 0;
    m_id = wxID_NONE;
}

// window item
void wxSizerItem::DoSetWindow(wxWindow *window)
{
    wxCHECK_RET( window, _T("NULL window in wxSizerItem::SetWindow()") );

    m_kind = Item_Window;
    m_window = window;

    // window doesn't become smaller than its initial size, whatever happens
    m_minSize = window->GetSize();

    if ( m_flag & wxFIXED_MINSIZE )
        window->SetMinSize(m_minSize);

    // aspect ratio calculated from initial size
    SetRatio(m_minSize);
}

wxSizerItem::wxSizerItem(wxWindow *window,
                         int proportion,
                         int flag,
                         int border,
                         wxObject* userData)
           : m_kind(Item_None),
             m_proportion(proportion),
             m_border(border),
             m_flag(flag),
             m_id(wxID_NONE),
             m_userData(userData)
{
    ASSERT_VALID_SIZER_FLAGS( m_flag );

    DoSetWindow(window);
}

// sizer item
void wxSizerItem::DoSetSizer(wxSizer *sizer)
{
    m_kind = Item_Sizer;
    m_sizer = sizer;
}

wxSizerItem::wxSizerItem(wxSizer *sizer,
                         int proportion,
                         int flag,
                         int border,
                         wxObject* userData)
           : m_kind(Item_None),
             m_sizer(NULL),
             m_proportion(proportion),
             m_border(border),
             m_flag(flag),
             m_id(wxID_NONE),
             m_ratio(0.0),
             m_userData(userData)
{
    ASSERT_VALID_SIZER_FLAGS( m_flag );

    DoSetSizer(sizer);

    // m_minSize is set later
}

// spacer item
void wxSizerItem::DoSetSpacer(const wxSize& size)
{
    m_kind = Item_Spacer;
    m_spacer = new wxSizerSpacer(size);
    m_minSize = size;
    SetRatio(size);
}

wxSizerItem::wxSizerItem(int width,
                         int height,
                         int proportion,
                         int flag,
                         int border,
                         wxObject* userData)
           : m_kind(Item_None),
             m_sizer(NULL),
             m_minSize(width, height), // minimal size is the initial size
             m_proportion(proportion),
             m_border(border),
             m_flag(flag),
             m_id(wxID_NONE),
             m_userData(userData)
{
    ASSERT_VALID_SIZER_FLAGS( m_flag );

    DoSetSpacer(wxSize(width, height));
}

wxSizerItem::~wxSizerItem()
{
    delete m_userData;
    Free();
}

void wxSizerItem::Free()
{
    switch ( m_kind )
    {
        case Item_None:
            break;

        case Item_Window:
            m_window->SetContainingSizer(NULL);
            break;

        case Item_Sizer:
            delete m_sizer;
            break;

        case Item_Spacer:
            delete m_spacer;
            break;

        case Item_Max:
        default:
            wxFAIL_MSG( _T("unexpected wxSizerItem::m_kind") );
    }

    m_kind = Item_None;
}

wxSize wxSizerItem::GetSpacer() const
{
    wxSize size;
    if ( m_kind == Item_Spacer )
        size = m_spacer->GetSize();

    return size;
}


wxSize wxSizerItem::GetSize() const
{
    wxSize ret;
    switch ( m_kind )
    {
        case Item_None:
            break;

        case Item_Window:
            ret = m_window->GetSize();
            break;

        case Item_Sizer:
            ret = m_sizer->GetSize();
            break;

        case Item_Spacer:
            ret = m_spacer->GetSize();
            break;

        case Item_Max:
        default:
            wxFAIL_MSG( _T("unexpected wxSizerItem::m_kind") );
    }

    if (m_flag & wxWEST)
        ret.x += m_border;
    if (m_flag & wxEAST)
        ret.x += m_border;
    if (m_flag & wxNORTH)
        ret.y += m_border;
    if (m_flag & wxSOUTH)
        ret.y += m_border;

    return ret;
}

bool wxSizerItem::InformFirstDirection(int direction, int size, int availableOtherDir)
{
    // The size that come here will be including borders. Child items should get it
    // without borders.
    if( size>0 )
    {
        if( direction==wxHORIZONTAL )
        {
            if (m_flag & wxWEST)
                size -= m_border;
            if (m_flag & wxEAST)
                size -= m_border;
        }
        else if( direction==wxVERTICAL )
        {
            if (m_flag & wxNORTH)
                size -= m_border;
            if (m_flag & wxSOUTH)
                size -= m_border;
        }
    }

    bool didUse = false;
    // Pass the information along to the held object
    if (IsSizer())
    {
        didUse = GetSizer()->InformFirstDirection(direction,size,availableOtherDir);
        if (didUse)
            m_minSize = GetSizer()->CalcMin();
    }
    else if (IsWindow())
    {
        didUse =  GetWindow()->InformFirstDirection(direction,size,availableOtherDir);
        if (didUse)
            m_minSize = m_window->GetEffectiveMinSize();

        // This information is useful for items with wxSHAPED flag, since
        // we can request an optimal min size for such an item. Even if
        // we overwrite the m_minSize member here, we can read it back from
        // the owned window (happens automatically).
        if( (m_flag & wxSHAPED) && (m_flag & wxEXPAND) && direction )
        {
            if( !wxIsNullDouble(m_ratio) )
            {
                wxCHECK_MSG( (m_proportion==0), false, _T("Shaped item, non-zero proportion in wxSizerItem::InformFirstDirection()") );
                if( direction==wxHORIZONTAL && !wxIsNullDouble(m_ratio) )
                {
                    // Clip size so that we don't take too much
                    if( availableOtherDir>=0 && int(size/m_ratio)-m_minSize.y>availableOtherDir )
                        size = int((availableOtherDir+m_minSize.y)*m_ratio);
                    m_minSize = wxSize(size,int(size/m_ratio));
                }
                else if( direction==wxVERTICAL )
                {
                    // Clip size so that we don't take too much
                    if( availableOtherDir>=0 && int(size*m_ratio)-m_minSize.x>availableOtherDir )
                        size = int((availableOtherDir+m_minSize.x)/m_ratio);
                    m_minSize = wxSize(int(size*m_ratio),size);
                }
                didUse = true;
            }
        }
    }

    return didUse;
}

wxSize wxSizerItem::CalcMin()
{
    if (IsSizer())
    {
        m_minSize = m_sizer->GetMinSize();

        // if we have to preserve aspect ratio _AND_ this is
        // the first-time calculation, consider ret to be initial size
        if ( (m_flag & wxSHAPED) && wxIsNullDouble(m_ratio) )
            SetRatio(m_minSize);
    }
    else if ( IsWindow() )
    {
        // Since the size of the window may change during runtime, we
        // should use the current minimal/best size.
        m_minSize = m_window->GetEffectiveMinSize();
    }

    return GetMinSizeWithBorder();
}

wxSize wxSizerItem::GetMinSizeWithBorder() const
{
    wxSize ret = m_minSize;

    if (m_flag & wxWEST)
        ret.x += m_border;
    if (m_flag & wxEAST)
        ret.x += m_border;
    if (m_flag & wxNORTH)
        ret.y += m_border;
    if (m_flag & wxSOUTH)
        ret.y += m_border;

    return ret;
}


void wxSizerItem::SetDimension( const wxPoint& pos_, const wxSize& size_ )
{
    wxPoint pos = pos_;
    wxSize size = size_;
    if (m_flag & wxSHAPED)
    {
        // adjust aspect ratio
        int rwidth = (int) (size.y * m_ratio);
        if (rwidth > size.x)
        {
            // fit horizontally
            int rheight = (int) (size.x / m_ratio);
            // add vertical space
            if (m_flag & wxALIGN_CENTER_VERTICAL)
                pos.y += (size.y - rheight) / 2;
            else if (m_flag & wxALIGN_BOTTOM)
                pos.y += (size.y - rheight);
            // use reduced dimensions
            size.y =rheight;
        }
        else if (rwidth < size.x)
        {
            // add horizontal space
            if (m_flag & wxALIGN_CENTER_HORIZONTAL)
                pos.x += (size.x - rwidth) / 2;
            else if (m_flag & wxALIGN_RIGHT)
                pos.x += (size.x - rwidth);
            size.x = rwidth;
        }
    }

    // This is what GetPosition() returns. Since we calculate
    // borders afterwards, GetPosition() will be the left/top
    // corner of the surrounding border.
    m_pos = pos;

    if (m_flag & wxWEST)
    {
        pos.x += m_border;
        size.x -= m_border;
    }
    if (m_flag & wxEAST)
    {
        size.x -= m_border;
    }
    if (m_flag & wxNORTH)
    {
        pos.y += m_border;
        size.y -= m_border;
    }
    if (m_flag & wxSOUTH)
    {
        size.y -= m_border;
    }

    if (size.x < 0)
        size.x = 0;
    if (size.y < 0)
        size.y = 0;

    m_rect = wxRect(pos, size);

    switch ( m_kind )
    {
        case Item_None:
            wxFAIL_MSG( _T("can't set size of uninitialized sizer item") );
            break;

        case Item_Window:
        {
            // Use wxSIZE_FORCE_EVENT here since a sizer item might
            // have changed alignment or some other property which would
            // not change the size of the window. In such a case, no
            // wxSizeEvent would normally be generated and thus the
            // control wouldn't get layed out correctly here.
#if 1
            m_window->SetSize(pos.x, pos.y, size.x, size.y,
                              wxSIZE_ALLOW_MINUS_ONE|wxSIZE_FORCE_EVENT );
#else
            m_window->SetSize(pos.x, pos.y, size.x, size.y,
                              wxSIZE_ALLOW_MINUS_ONE );
#endif
            break;
        }
        case Item_Sizer:
            m_sizer->SetDimension(pos, size);
            break;

        case Item_Spacer:
            m_spacer->SetSize(size);
            break;

        case Item_Max:
        default:
            wxFAIL_MSG( _T("unexpected wxSizerItem::m_kind") );
    }
}

void wxSizerItem::DeleteWindows()
{
    switch ( m_kind )
    {
        case Item_None:
        case Item_Spacer:
            break;

        case Item_Window:
            //We are deleting the window from this sizer - normally
            //the window destroys the sizer associated with it,
            //which might destroy this, which we don't want
            m_window->SetContainingSizer(NULL);
            m_window->Destroy();
            //Putting this after the switch will result in a spacer
            //not being deleted properly on destruction
            m_kind = Item_None;
            break;

        case Item_Sizer:
            m_sizer->DeleteWindows();
            break;

        case Item_Max:
        default:
            wxFAIL_MSG( _T("unexpected wxSizerItem::m_kind") );
    }

}

void wxSizerItem::Show( bool show )
{
    switch ( m_kind )
    {
        case Item_None:
            wxFAIL_MSG( _T("can't show uninitialized sizer item") );
            break;

        case Item_Window:
            m_window->Show(show);
            break;

        case Item_Sizer:
            m_sizer->Show(show);
            break;

        case Item_Spacer:
            m_spacer->Show(show);
            break;

        case Item_Max:
        default:
            wxFAIL_MSG( _T("unexpected wxSizerItem::m_kind") );
    }
}

bool wxSizerItem::IsShown() const
{
    if ( m_flag & wxRESERVE_SPACE_EVEN_IF_HIDDEN )
        return true;

    switch ( m_kind )
    {
        case Item_None:
            // we may be called from CalcMin(), just return false so that we're
            // not used
            break;

        case Item_Window:
            return m_window->IsShown();

        case Item_Sizer:
            // arbitrarily decide that if at least one of our elements is
            // shown, so are we (this arbitrariness is the reason for
            // deprecating this function)
            {
                // Some apps (such as dialog editors) depend on an empty sizer still
                // being laid out correctly and reporting the correct size and position.
                if (m_sizer->GetChildren().GetCount() == 0)
                    return true;

                for ( wxSizerItemList::compatibility_iterator
                        node = m_sizer->GetChildren().GetFirst();
                      node;
                      node = node->GetNext() )
                {
                    if ( node->GetData()->IsShown() )
                        return true;
                }
            }
            return false;

        case Item_Spacer:
            return m_spacer->IsShown();

        case Item_Max:
        default:
            wxFAIL_MSG( _T("unexpected wxSizerItem::m_kind") );
    }

    return false;
}

#if WXWIN_COMPATIBILITY_2_6
void wxSizerItem::SetOption( int option )
{
    SetProportion( option );
}

int wxSizerItem::GetOption() const
{
    return GetProportion();
}
#endif // WXWIN_COMPATIBILITY_2_6


//---------------------------------------------------------------------------
// wxSizer
//---------------------------------------------------------------------------

wxSizer::~wxSizer()
{
    WX_CLEAR_LIST(wxSizerItemList, m_children);
}

wxSizerItem* wxSizer::Insert( size_t index, wxSizerItem *item )
{
    m_children.Insert( index, item );

    if ( item->GetWindow() )
        item->GetWindow()->SetContainingSizer( this );

    if ( item->GetSizer() )
        item->GetSizer()->SetContainingWindow( m_containingWindow );

    return item;
}

void wxSizer::SetContainingWindow(wxWindow *win)
{
    if ( win == m_containingWindow )
        return;

    m_containingWindow = win;

    // set the same window for all nested sizers as well, they also are in the
    // same window
    for ( wxSizerItemList::compatibility_iterator node = m_children.GetFirst();
          node;
          node = node->GetNext() )
    {
        wxSizerItem *const item = node->GetData();
        wxSizer *const sizer = item->GetSizer();

        if ( sizer )
        {
            sizer->SetContainingWindow(win);
        }
    }
}

#if WXWIN_COMPATIBILITY_2_6
bool wxSizer::Remove( wxWindow *window )
{
    return Detach( window );
}
#endif // WXWIN_COMPATIBILITY_2_6

bool wxSizer::Remove( wxSizer *sizer )
{
    wxASSERT_MSG( sizer, _T("Removing NULL sizer") );

    wxSizerItemList::compatibility_iterator node = m_children.GetFirst();
    while (node)
    {
        wxSizerItem     *item = node->GetData();

        if (item->GetSizer() == sizer)
        {
            delete item;
            m_children.Erase( node );
            return true;
        }

        node = node->GetNext();
    }

    return false;
}

bool wxSizer::Remove( int index )
{
    wxCHECK_MSG( index >= 0 && (size_t)index < m_children.GetCount(),
                 false,
                 _T("Remove index is out of range") );

    wxSizerItemList::compatibility_iterator node = m_children.Item( index );

    wxCHECK_MSG( node, false, _T("Failed to find child node") );

    delete node->GetData();
    m_children.Erase( node );

    return true;
}

bool wxSizer::Detach( wxSizer *sizer )
{
    wxASSERT_MSG( sizer, _T("Detaching NULL sizer") );

    wxSizerItemList::compatibility_iterator node = m_children.GetFirst();
    while (node)
    {
        wxSizerItem     *item = node->GetData();

        if (item->GetSizer() == sizer)
        {
            item->DetachSizer();
            delete item;
            m_children.Erase( node );
            return true;
        }
        node = node->GetNext();
    }

    return false;
}

bool wxSizer::Detach( wxWindow *window )
{
    wxASSERT_MSG( window, _T("Detaching NULL window") );

    wxSizerItemList::compatibility_iterator node = m_children.GetFirst();
    while (node)
    {
        wxSizerItem     *item = node->GetData();

        if (item->GetWindow() == window)
        {
            delete item;
            m_children.Erase( node );
            return true;
        }
        node = node->GetNext();
    }

    return false;
}

bool wxSizer::Detach( int index )
{
    wxCHECK_MSG( index >= 0 && (size_t)index < m_children.GetCount(),
                 false,
                 _T("Detach index is out of range") );

    wxSizerItemList::compatibility_iterator node = m_children.Item( index );

    wxCHECK_MSG( node, false, _T("Failed to find child node") );

    wxSizerItem *item = node->GetData();

    if ( item->IsSizer() )
        item->DetachSizer();

    delete item;
    m_children.Erase( node );
    return true;
}

bool wxSizer::Replace( wxWindow *oldwin, wxWindow *newwin, bool recursive )
{
    wxASSERT_MSG( oldwin, _T("Replacing NULL window") );
    wxASSERT_MSG( newwin, _T("Replacing with NULL window") );

    wxSizerItemList::compatibility_iterator node = m_children.GetFirst();
    while (node)
    {
        wxSizerItem     *item = node->GetData();

        if (item->GetWindow() == oldwin)
        {
            item->AssignWindow(newwin);
            newwin->SetContainingSizer( this );
            return true;
        }
        else if (recursive && item->IsSizer())
        {
            if (item->GetSizer()->Replace( oldwin, newwin, true ))
                return true;
        }

        node = node->GetNext();
    }

    return false;
}

bool wxSizer::Replace( wxSizer *oldsz, wxSizer *newsz, bool recursive )
{
    wxASSERT_MSG( oldsz, _T("Replacing NULL sizer") );
    wxASSERT_MSG( newsz, _T("Replacing with NULL sizer") );

    wxSizerItemList::compatibility_iterator node = m_children.GetFirst();
    while (node)
    {
        wxSizerItem     *item = node->GetData();

        if (item->GetSizer() == oldsz)
        {
            item->AssignSizer(newsz);
            return true;
        }
        else if (recursive && item->IsSizer())
        {
            if (item->GetSizer()->Replace( oldsz, newsz, true ))
                return true;
        }

        node = node->GetNext();
    }

    return false;
}

bool wxSizer::Replace( size_t old, wxSizerItem *newitem )
{
    wxCHECK_MSG( old < m_children.GetCount(), false, _T("Replace index is out of range") );
    wxASSERT_MSG( newitem, _T("Replacing with NULL item") );

    wxSizerItemList::compatibility_iterator node = m_children.Item( old );

    wxCHECK_MSG( node, false, _T("Failed to find child node") );

    wxSizerItem *item = node->GetData();
    node->SetData(newitem);
    delete item;

    return true;
}

void wxSizer::Clear( bool delete_windows )
{
    // First clear the ContainingSizer pointers
    wxSizerItemList::compatibility_iterator node = m_children.GetFirst();
    while (node)
    {
        wxSizerItem     *item = node->GetData();

        if (item->IsWindow())
            item->GetWindow()->SetContainingSizer( NULL );
        node = node->GetNext();
    }

    // Destroy the windows if needed
    if (delete_windows)
        DeleteWindows();

    // Now empty the list
    WX_CLEAR_LIST(wxSizerItemList, m_children);
}

void wxSizer::DeleteWindows()
{
    wxSizerItemList::compatibility_iterator node = m_children.GetFirst();
    while (node)
    {
        wxSizerItem     *item = node->GetData();

        item->DeleteWindows();
        node = node->GetNext();
    }
}

wxSize wxSizer::ComputeFittingClientSize(wxWindow *window)
{
    wxCHECK_MSG( window, wxDefaultSize, "window can't be NULL" );

    // take the min size by default and limit it by max size
    wxSize size = GetMinClientSize(window);
    wxSize sizeMax;

    wxTopLevelWindow *tlw = wxDynamicCast(window, wxTopLevelWindow);
    if ( tlw )
    {
        // hack for small screen devices where TLWs are always full screen
        if ( tlw->IsAlwaysMaximized() )
        {
            return tlw->GetClientSize();
        }

        // limit the window to the size of the display it is on
        int disp = wxDisplay::GetFromWindow(window);
        if ( disp == wxNOT_FOUND )
        {
            // or, if we don't know which one it is, of the main one
            disp = 0;
        }

        sizeMax = wxDisplay(disp).GetClientArea().GetSize();

        // space for decorations and toolbars etc.
        sizeMax = tlw->WindowToClientSize(sizeMax);
    }
    else
    {
        sizeMax = GetMaxClientSize(window);
    }

    if ( sizeMax.x != wxDefaultCoord && size.x > sizeMax.x )
            size.x = sizeMax.x;
    if ( sizeMax.y != wxDefaultCoord && size.y > sizeMax.y )
            size.y = sizeMax.y;

    return size;
}

wxSize wxSizer::ComputeFittingWindowSize(wxWindow *window)
{
    wxCHECK_MSG( window, wxDefaultSize, "window can't be NULL" );

    return window->ClientToWindowSize(ComputeFittingClientSize(window));
}

wxSize wxSizer::Fit( wxWindow *window )
{
    wxCHECK_MSG( window, wxDefaultSize, "window can't be NULL" );

    // set client size
    window->SetClientSize(ComputeFittingClientSize(window));

    // return entire size
    return window->GetSize();
}

void wxSizer::FitInside( wxWindow *window )
{
    wxSize size;
    if (window->IsTopLevel())
        size = VirtualFitSize( window );
    else
        size = GetMinClientSize( window );

    window->SetVirtualSize( size );
}

void wxSizer::Layout()
{
    // (re)calculates minimums needed for each item and other preparations
    // for layout
    CalcMin();

    // Applies the layout and repositions/resizes the items
    RecalcSizes();
}

void wxSizer::SetSizeHints( wxWindow *window )
{
    // Preserve the window's max size hints, but set the
    // lower bound according to the sizer calculations.

    // This is equivalent to calling Fit(), except that we need to set
    // the size hints _in between_ the two steps performed by Fit
    // (1. ComputeFittingClientSize, 2. SetClientSize). That's because
    // otherwise SetClientSize() could have no effect if there already are
    // size hints in effect that forbid requested client size.

    const wxSize clientSize = ComputeFittingClientSize(window);

    window->SetMinClientSize(clientSize);
    window->SetClientSize(clientSize);
}

#if WXWIN_COMPATIBILITY_2_8
void wxSizer::SetVirtualSizeHints( wxWindow *window )
{
    FitInside( window );
}
#endif // WXWIN_COMPATIBILITY_2_8

// TODO on mac we need a function that determines how much free space this
// min size contains, in order to make sure that we have 20 pixels of free
// space around the controls
wxSize wxSizer::GetMaxClientSize( wxWindow *window ) const
{
    return window->WindowToClientSize(window->GetMaxSize());
}

wxSize wxSizer::GetMinClientSize( wxWindow *WXUNUSED(window) )
{
    return GetMinSize();  // Already returns client size.
}

wxSize wxSizer::VirtualFitSize( wxWindow *window )
{
    wxSize size     = GetMinClientSize( window );
    wxSize sizeMax  = GetMaxClientSize( window );

    // Limit the size if sizeMax != wxDefaultSize

    if ( size.x > sizeMax.x && sizeMax.x != wxDefaultCoord )
        size.x = sizeMax.x;
    if ( size.y > sizeMax.y && sizeMax.y != wxDefaultCoord )
        size.y = sizeMax.y;

    return size;
}

wxSize wxSizer::GetMinSize()
{
    wxSize ret( CalcMin() );
    if (ret.x < m_minSize.x) ret.x = m_minSize.x;
    if (ret.y < m_minSize.y) ret.y = m_minSize.y;
    return ret;
}

void wxSizer::DoSetMinSize( int width, int height )
{
    m_minSize.x = width;
    m_minSize.y = height;
}

bool wxSizer::DoSetItemMinSize( wxWindow *window, int width, int height )
{
    wxASSERT_MSG( window, _T("SetMinSize for NULL window") );

    // Is it our immediate child?

    wxSizerItemList::compatibility_iterator node = m_children.GetFirst();
    while (node)
    {
        wxSizerItem     *item = node->GetData();

        if (item->GetWindow() == window)
        {
            item->SetMinSize( width, height );
            return true;
        }
        node = node->GetNext();
    }

    // No?  Search any subsizers we own then

    node = m_children.GetFirst();
    while (node)
    {
        wxSizerItem     *item = node->GetData();

        if ( item->GetSizer() &&
             item->GetSizer()->DoSetItemMinSize( window, width, height ) )
        {
            // A child sizer found the requested windw, exit.
            return true;
        }
        node = node->GetNext();
    }

    return false;
}

bool wxSizer::DoSetItemMinSize( wxSizer *sizer, int width, int height )
{
    wxASSERT_MSG( sizer, _T("SetMinSize for NULL sizer") );

    // Is it our immediate child?

    wxSizerItemList::compatibility_iterator node = m_children.GetFirst();
    while (node)
    {
        wxSizerItem     *item = node->GetData();

        if (item->GetSizer() == sizer)
        {
            item->GetSizer()->DoSetMinSize( width, height );
            return true;
        }
        node = node->GetNext();
    }

    // No?  Search any subsizers we own then

    node = m_children.GetFirst();
    while (node)
    {
        wxSizerItem     *item = node->GetData();

        if ( item->GetSizer() &&
             item->GetSizer()->DoSetItemMinSize( sizer, width, height ) )
        {
            // A child found the requested sizer, exit.
            return true;
        }
        node = node->GetNext();
    }

    return false;
}

bool wxSizer::DoSetItemMinSize( size_t index, int width, int height )
{
    wxSizerItemList::compatibility_iterator node = m_children.Item( index );

    wxCHECK_MSG( node, false, _T("Failed to find child node") );

    wxSizerItem     *item = node->GetData();

    if (item->GetSizer())
    {
        // Sizers contains the minimal size in them, if not calculated ...
        item->GetSizer()->DoSetMinSize( width, height );
    }
    else
    {
        // ... but the minimal size of spacers and windows is stored via the item
        item->SetMinSize( width, height );
    }

    return true;
}

wxSizerItem* wxSizer::GetItem( wxWindow *window, bool recursive )
{
    wxASSERT_MSG( window, _T("GetItem for NULL window") );

    wxSizerItemList::compatibility_iterator node = m_children.GetFirst();
    while (node)
    {
        wxSizerItem     *item = node->GetData();

        if (item->GetWindow() == window)
        {
            return item;
        }
        else if (recursive && item->IsSizer())
        {
            wxSizerItem *subitem = item->GetSizer()->GetItem( window, true );
            if (subitem)
                return subitem;
        }

        node = node->GetNext();
    }

    return NULL;
}

wxSizerItem* wxSizer::GetItem( wxSizer *sizer, bool recursive )
{
    wxASSERT_MSG( sizer, _T("GetItem for NULL sizer") );

    wxSizerItemList::compatibility_iterator node = m_children.GetFirst();
    while (node)
    {
        wxSizerItem *item = node->GetData();

        if (item->GetSizer() == sizer)
        {
            return item;
        }
        else if (recursive && item->IsSizer())
        {
            wxSizerItem *subitem = item->GetSizer()->GetItem( sizer, true );
            if (subitem)
                return subitem;
        }

        node = node->GetNext();
    }

    return NULL;
}

wxSizerItem* wxSizer::GetItem( size_t index )
{
    wxCHECK_MSG( index < m_children.GetCount(),
                 NULL,
                 _T("GetItem index is out of range") );

    return m_children.Item( index )->GetData();
}

wxSizerItem* wxSizer::GetItemById( int id, bool recursive )
{
    // This gets a sizer item by the id of the sizer item
    // and NOT the id of a window if the item is a window.

    wxSizerItemList::compatibility_iterator node = m_children.GetFirst();
    while (node)
    {
        wxSizerItem     *item = node->GetData();

        if (item->GetId() == id)
        {
            return item;
        }
        else if (recursive && item->IsSizer())
        {
            wxSizerItem *subitem = item->GetSizer()->GetItemById( id, true );
            if (subitem)
                return subitem;
        }

        node = node->GetNext();
    }

    return NULL;
}

bool wxSizer::Show( wxWindow *window, bool show, bool recursive )
{
    wxSizerItem *item = GetItem( window, recursive );

    if ( item )
    {
         item->Show( show );
         return true;
    }

    return false;
}

bool wxSizer::Show( wxSizer *sizer, bool show, bool recursive )
{
    wxSizerItem *item = GetItem( sizer, recursive );

    if ( item )
    {
         item->Show( show );
         return true;
    }

    return false;
}

bool wxSizer::Show( size_t index, bool show)
{
    wxSizerItem *item = GetItem( index );

    if ( item )
    {
         item->Show( show );
         return true;
    }

    return false;
}

void wxSizer::ShowItems( bool show )
{
    wxSizerItemList::compatibility_iterator node = m_children.GetFirst();
    while (node)
    {
        node->GetData()->Show( show );
        node = node->GetNext();
    }
}

bool wxSizer::IsShown( wxWindow *window ) const
{
    wxSizerItemList::compatibility_iterator node = m_children.GetFirst();
    while (node)
    {
        wxSizerItem     *item = node->GetData();

        if (item->GetWindow() == window)
        {
            return item->IsShown();
        }
        node = node->GetNext();
    }

    wxFAIL_MSG( _T("IsShown failed to find sizer item") );

    return false;
}

bool wxSizer::IsShown( wxSizer *sizer ) const
{
    wxSizerItemList::compatibility_iterator node = m_children.GetFirst();
    while (node)
    {
        wxSizerItem     *item = node->GetData();

        if (item->GetSizer() == sizer)
        {
            return item->IsShown();
        }
        node = node->GetNext();
    }

    wxFAIL_MSG( _T("IsShown failed to find sizer item") );

    return false;
}

bool wxSizer::IsShown( size_t index ) const
{
    wxCHECK_MSG( index < m_children.GetCount(),
                 false,
                 _T("IsShown index is out of range") );

    return m_children.Item( index )->GetData()->IsShown();
}


//---------------------------------------------------------------------------
// wxGridSizer
//---------------------------------------------------------------------------

wxGridSizer::wxGridSizer( int rows, int cols, int vgap, int hgap )
    : m_rows( ( cols == 0 && rows == 0 ) ? 1 : rows )
    , m_cols( cols )
    , m_vgap( vgap )
    , m_hgap( hgap )
{
}

wxGridSizer::wxGridSizer( int cols, int vgap, int hgap )
    : m_rows( cols == 0 ? 1 : 0 )
    , m_cols( cols )
    , m_vgap( vgap )
    , m_hgap( hgap )
{
}

wxSizerItem *wxGridSizer::Insert(size_t index, wxSizerItem *item)
{
    // if only the number of columns or the number of rows is specified for a
    // sizer, arbitrarily many items can be added to it but if both of them are
    // fixed, then the sizer can't have more than that many items -- check for
    // this here to ensure that we detect errors as soon as possible
    if ( m_cols && m_rows )
    {
        const int nitems = m_children.GetCount();
        if ( nitems == m_cols*m_rows )
        {
            wxFAIL_MSG(
                wxString::Format(
                    "too many items (%d > %d*%d) in grid sizer (maybe you "
                    "should omit the number of either rows or columns?)",
                nitems + 1, m_cols, m_rows)
            );

            // additionally, continuing to use the specified number of columns
            // and rows is not a good idea as callers of CalcRowsCols() expect
            // that all sizer items can fit into m_cols/m_rows-sized arrays
            // which is not the case if there are too many items and results in
            // crashes, so let it compute the number of rows automatically by
            // forgetting the (wrong) number of rows specified (this also has a
            // nice side effect of giving only one assert even if there are
            // many more items than allowed in this sizer)
            m_rows = 0;
        }
    }

    return wxSizer::Insert(index, item);
}

int wxGridSizer::CalcRowsCols(int& nrows, int& ncols) const
{
    const int nitems = m_children.GetCount();
    if ( m_cols && m_rows )
    {
        ncols = m_cols;
        nrows = m_rows;

        // this should be impossible because the too high number of items
        // should have been detected by Insert() above
        wxASSERT_MSG( nitems <= ncols*nrows, "logic error in wxGridSizer" );
    }
    else if ( m_cols )
    {
        ncols = m_cols;
        nrows = (nitems + m_cols - 1) / m_cols;
    }
    else if ( m_rows )
    {
        ncols = (nitems + m_rows - 1) / m_rows;
        nrows = m_rows;
    }
    else // 0 columns, 0 rows?
    {
        wxFAIL_MSG( _T("grid sizer must have either rows or columns fixed") );

        nrows =
        ncols = 0;
    }

    return nitems;
}

void wxGridSizer::RecalcSizes()
{
    int nitems, nrows, ncols;
    if ( (nitems = CalcRowsCols(nrows, ncols)) == 0 )
        return;

    wxSize sz( GetSize() );
    wxPoint pt( GetPosition() );

    int w = (sz.x - (ncols - 1) * m_hgap) / ncols;
    int h = (sz.y - (nrows - 1) * m_vgap) / nrows;

    int x = pt.x;
    for (int c = 0; c < ncols; c++)
    {
        int y = pt.y;
        for (int r = 0; r < nrows; r++)
        {
            int i = r * ncols + c;
            if (i < nitems)
            {
                wxSizerItemList::compatibility_iterator node = m_children.Item( i );

                wxASSERT_MSG( node, _T("Failed to find SizerItemList node") );

                SetItemBounds( node->GetData(), x, y, w, h);
            }
            y = y + h + m_vgap;
        }
        x = x + w + m_hgap;
    }
}

wxSize wxGridSizer::CalcMin()
{
    int nrows, ncols;
    if ( CalcRowsCols(nrows, ncols) == 0 )
        return wxSize();

    // Find the max width and height for any component
    int w = 0;
    int h = 0;

    wxSizerItemList::compatibility_iterator node = m_children.GetFirst();
    while (node)
    {
        wxSizerItem     *item = node->GetData();
        wxSize           sz( item->CalcMin() );

        w = wxMax( w, sz.x );
        h = wxMax( h, sz.y );

        node = node->GetNext();
    }

    // In case we have a nested sizer with a two step algo , give it
    // a chance to adjust to that (we give it width component)
    node = m_children.GetFirst();
    bool didChangeMinSize = false;
    while (node)
    {
        wxSizerItem     *item = node->GetData();
        didChangeMinSize |= item->InformFirstDirection( wxHORIZONTAL, w, -1 );

        node = node->GetNext();
    }

    // And redo iteration in case min size changed
    if( didChangeMinSize )
    {
        node = m_children.GetFirst();
        w = h = 0;
        while (node)
        {
            wxSizerItem     *item = node->GetData();
            wxSize           sz( item->GetMinSizeWithBorder() );

            w = wxMax( w, sz.x );
            h = wxMax( h, sz.y );

            node = node->GetNext();
        }
    }

    return wxSize( ncols * w + (ncols-1) * m_hgap,
                   nrows * h + (nrows-1) * m_vgap );
}

void wxGridSizer::SetItemBounds( wxSizerItem *item, int x, int y, int w, int h )
{
    wxPoint pt( x,y );
    wxSize sz( item->GetMinSizeWithBorder() );
    int flag = item->GetFlag();

    if ((flag & wxEXPAND) || (flag & wxSHAPED))
    {
       sz = wxSize(w, h);
    }
    else
    {
        if (flag & wxALIGN_CENTER_HORIZONTAL)
        {
            pt.x = x + (w - sz.x) / 2;
        }
        else if (flag & wxALIGN_RIGHT)
        {
            pt.x = x + (w - sz.x);
        }

        if (flag & wxALIGN_CENTER_VERTICAL)
        {
            pt.y = y + (h - sz.y) / 2;
        }
        else if (flag & wxALIGN_BOTTOM)
        {
            pt.y = y + (h - sz.y);
        }
    }

    item->SetDimension(pt, sz);
}

//---------------------------------------------------------------------------
// wxFlexGridSizer
//---------------------------------------------------------------------------

wxFlexGridSizer::wxFlexGridSizer( int rows, int cols, int vgap, int hgap )
               : wxGridSizer( rows, cols, vgap, hgap ),
                 m_flexDirection(wxBOTH),
                 m_growMode(wxFLEX_GROWMODE_SPECIFIED)
{
}

wxFlexGridSizer::wxFlexGridSizer( int cols, int vgap, int hgap )
               : wxGridSizer( cols, vgap, hgap ),
                 m_flexDirection(wxBOTH),
                 m_growMode(wxFLEX_GROWMODE_SPECIFIED)
{
}

wxFlexGridSizer::~wxFlexGridSizer()
{
}

void wxFlexGridSizer::RecalcSizes()
{
    int nrows, ncols;
    if ( !CalcRowsCols(nrows, ncols) )
        return;

    const wxPoint pt(GetPosition());
    const wxSize sz(GetSize());

    AdjustForGrowables(sz);

    wxSizerItemList::const_iterator i = m_children.begin();
    const wxSizerItemList::const_iterator end = m_children.end();

    int y = 0;
    for ( int r = 0; r < nrows; r++ )
    {
        if ( m_rowHeights[r] == -1 )
        {
            // this row is entirely hidden, skip it
            for ( int c = 0; c < ncols; c++ )
            {
                if ( i == end )
                    return;

                ++i;
            }

            continue;
        }

        const int hrow = m_rowHeights[r];
        int h = sz.y - y; // max remaining height, don't overflow it
        if ( hrow < h )
            h = hrow;

        int x = 0;
        for ( int c = 0; c < ncols && i != end; c++, ++i )
        {
            const int wcol = m_colWidths[c];

            if ( wcol == -1 )
                continue;

            int w = sz.x - x; // max possible value, ensure we don't overflow
            if ( wcol < w )
                w = wcol;

            SetItemBounds(*i, pt.x + x, pt.y + y, w, h);

            x += wcol + m_hgap;
        }

        if ( i == end )
            return;

        y += hrow + m_vgap;
    }
}

// helper function used in CalcMin() to sum up the sizes of non-hidden items
static int SumArraySizes(const wxArrayInt& sizes, int gap)
{
    // Sum total minimum size, including gaps between rows/columns.
    // -1 is used as a magic number meaning empty row/column.
    int total = 0;

    const size_t count = sizes.size();
    for ( size_t n = 0; n < count; n++ )
    {
        if ( sizes[n] != -1 )
        {
            if ( total )
                total += gap; // separate from the previous column

            total += sizes[n];
        }
    }

    return total;
}

void wxFlexGridSizer::FindWidthsAndHeights(int nrows, int ncols)
{
    // We have to recalculate the sizes in case the item minimum size has
    // changed since the previous layout, or the item has been hidden using
    // wxSizer::Show(). If all the items in a row/column are hidden, the final
    // dimension of the row/column will be -1, indicating that the column
    // itself is hidden.
    m_rowHeights.assign(nrows, -1);
    m_colWidths.assign(ncols, -1);

    // n is the index of the item in left-to-right top-to-bottom order
    size_t n = 0;
    for ( wxSizerItemList::iterator i = m_children.begin();
          i != m_children.end();
          ++i, ++n )
    {
        wxSizerItem * const item = *i;
        if ( item->IsShown() )
        {
            // NOTE: Not doing the calculation here, this is just
            // for finding max values.
            const wxSize sz(item->GetMinSizeWithBorder());

            const int row = n / ncols;
            const int col = n % ncols;

            if ( sz.y > m_rowHeights[row] )
                m_rowHeights[row] = sz.y;
            if ( sz.x > m_colWidths[col] )
                m_colWidths[col] = sz.x;
        }
    }

    AdjustForFlexDirection();

    m_calculatedMinSize = wxSize(SumArraySizes(m_colWidths, m_hgap),
                                 SumArraySizes(m_rowHeights, m_vgap));
}

wxSize wxFlexGridSizer::CalcMin()
{
    int nrows,
        ncols;

    // Number of rows/columns can change as items are added or removed.
    if ( !CalcRowsCols(nrows, ncols) )
        return wxSize();


    // We have to recalculate the sizes in case the item minimum size has
    // changed since the previous layout, or the item has been hidden using
    // wxSizer::Show(). If all the items in a row/column are hidden, the final
    // dimension of the row/column will be -1, indicating that the column
    // itself is hidden.
    m_rowHeights.assign(nrows, -1);
    m_colWidths.assign(ncols, -1);

    for ( wxSizerItemList::iterator i = m_children.begin();
          i != m_children.end();
          ++i)
    {
        wxSizerItem * const item = *i;
        if ( item->IsShown() )
        {
            item->CalcMin();
        }
    }

    // The stage of looking for max values in each row/column has been
    // made a separate function, since it's reused in AdjustForGrowables.
    FindWidthsAndHeights(nrows,ncols);

    return m_calculatedMinSize;
}

void wxFlexGridSizer::AdjustForFlexDirection()
{
    // the logic in CalcMin works when we resize flexibly in both directions
    // but maybe this is not the case
    if ( m_flexDirection != wxBOTH )
    {
        // select the array corresponding to the direction in which we do *not*
        // resize flexibly
        wxArrayInt& array = m_flexDirection == wxVERTICAL ? m_colWidths
                                                          : m_rowHeights;

        const size_t count = array.GetCount();

        // find the largest value in this array
        size_t n;
        int largest = 0;

        for ( n = 0; n < count; ++n )
        {
            if ( array[n] > largest )
                largest = array[n];
        }

        // and now fill it with the largest value
        for ( n = 0; n < count; ++n )
        {
            // don't touch hidden rows
            if ( array[n] != -1 )
                array[n] = largest;
        }
    }
}

// helper of AdjustForGrowables() which is called for rows/columns separately
//
// parameters:
//      delta: the extra space, we do nothing unless it's positive
//      growable: indices or growable rows/cols in sizes array
//      sizes: the height/widths of rows/cols to adjust
//      proportions: proportions of the growable rows/cols or NULL if they all
//                   should be assumed to have proportion of 1
static void
DoAdjustForGrowables(int delta,
                     const wxArrayInt& growable,
                     wxArrayInt& sizes,
                     const wxArrayInt *proportions)
{
    if ( delta <= 0 )
        return;

    // total sum of proportions of all non-hidden rows
    int sum_proportions = 0;

    // number of currently shown growable rows
    int num = 0;

    const int max_idx = sizes.size();

    const size_t count = growable.size();
    size_t idx;
    for ( idx = 0; idx < count; idx++ )
    {
        // Since the number of rows/columns can change as items are
        // inserted/deleted, we need to verify at runtime that the
        // requested growable rows/columns are still valid.
        if ( growable[idx] >= max_idx )
            continue;

        // If all items in a row/column are hidden, that row/column will
        // have a dimension of -1.  This causes the row/column to be
        // hidden completely.
        if ( sizes[growable[idx]] == -1 )
            continue;

        if ( proportions )
            sum_proportions += (*proportions)[idx];

        num++;
    }

    if ( !num )
        return;

    // the remaining extra free space, adjusted during each iteration
    for ( idx = 0; idx < count; idx++ )
    {
        if ( growable[idx] >= max_idx )
            continue;

        if ( sizes[ growable[idx] ] == -1 )
            continue;

        int cur_delta;
        if ( sum_proportions == 0 )
        {
            // no growable rows -- divide extra space evenly among all
            cur_delta = delta/num;
            num--;
        }
        else // allocate extra space proportionally
        {
            const int cur_prop = (*proportions)[idx];
            cur_delta = (delta*cur_prop)/sum_proportions;
            sum_proportions -= cur_prop;
        }

        sizes[growable[idx]] += cur_delta;
        delta -= cur_delta;
    }
}

void wxFlexGridSizer::AdjustForGrowables(const wxSize& sz)
{
#if wxDEBUG_LEVEL
    // by the time this function is called, the sizer should be already fully
    // initialized and hence the number of its columns and rows is known and we
    // can check that all indices in m_growableCols/Rows are valid (see also
    // comments in AddGrowableCol/Row())
    if ( !m_rows || !m_cols )
    {
        int nrows, ncols;
        CalcRowsCols(nrows, ncols);

        if ( !m_rows )
        {
            for ( size_t n = 0; n < m_growableRows.size(); n++ )
            {
                wxASSERT_MSG( m_growableRows[n] < nrows,
                              "invalid growable row index" );
            }
        }

        if ( !m_cols )
        {
            for ( size_t n = 0; n < m_growableCols.size(); n++ )
            {
                wxASSERT_MSG( m_growableCols[n] < ncols,
                              "invalid growable column index" );
            }
        }
    }
#endif // wxDEBUG_LEVEL


    if ( (m_flexDirection & wxHORIZONTAL) || (m_growMode != wxFLEX_GROWMODE_NONE) )
    {
        DoAdjustForGrowables
        (
            sz.x - m_calculatedMinSize.x,
            m_growableCols,
            m_colWidths,
            m_growMode == wxFLEX_GROWMODE_SPECIFIED ? &m_growableColsProportions
                                                    : NULL
        );

        // This gives nested objects that benefit from knowing one size
        // component in advance the chance to use that.
        bool didAdjustMinSize = false;
        int nrows, ncols;
        CalcRowsCols(nrows, ncols);

        // Iterate over all items and inform about column width
        size_t n = 0;
        for ( wxSizerItemList::iterator i = m_children.begin();
              i != m_children.end();
              ++i, ++n )
        {
            const int col = n % ncols;
            didAdjustMinSize |= (*i)->InformFirstDirection(wxHORIZONTAL, m_colWidths[col], sz.y - m_calculatedMinSize.y);
        }

        // Only redo if info was actually used
        if( didAdjustMinSize )
        {
            DoAdjustForGrowables
            (
                sz.x - m_calculatedMinSize.x,
                m_growableCols,
                m_colWidths,
                m_growMode == wxFLEX_GROWMODE_SPECIFIED ? &m_growableColsProportions
                                                        : NULL
            );
        }
    }

    if ( (m_flexDirection & wxVERTICAL) || (m_growMode != wxFLEX_GROWMODE_NONE) )
    {
        // pass NULL instead of proportions if the grow mode is ALL as we
        // should treat all rows as having proportion of 1 then
        DoAdjustForGrowables
        (
            sz.y - m_calculatedMinSize.y,
            m_growableRows,
            m_rowHeights,
            m_growMode == wxFLEX_GROWMODE_SPECIFIED ? &m_growableRowsProportions
                                                    : NULL
        );
    }
}

bool wxFlexGridSizer::IsRowGrowable( size_t idx )
{
    return m_growableRows.Index( idx ) != wxNOT_FOUND;
}

bool wxFlexGridSizer::IsColGrowable( size_t idx )
{
    return m_growableCols.Index( idx ) != wxNOT_FOUND;
}

void wxFlexGridSizer::AddGrowableRow( size_t idx, int proportion )
{
    int nrows, ncols;
    CalcRowsCols(nrows, ncols);

    wxASSERT_MSG( !IsRowGrowable( idx ),
                  "AddGrowableRow() called for growable row" );

    // notice that we intentionally don't check the index validity here in (the
    // common) case when the number of rows was not specified in the ctor -- in
    // this case it will be computed only later, when all items are added to
    // the sizer, and the check will be done in AdjustForGrowables()
    wxCHECK_RET( !m_rows || idx < (size_t)m_rows, "invalid row index" );

    m_growableRows.Add( idx );
    m_growableRowsProportions.Add( proportion );
}

void wxFlexGridSizer::AddGrowableCol( size_t idx, int proportion )
{
    int nrows, ncols;
    CalcRowsCols(nrows, ncols);

    wxASSERT_MSG( !IsColGrowable( idx ),
                  "AddGrowableCol() called for growable column" );

    // see comment in AddGrowableRow(): although it's less common to omit the
    // specification of the number of columns, it still can also happen
    wxCHECK_RET( !m_cols || idx < (size_t)ncols, "invalid column index" );

    m_growableCols.Add( idx );
    m_growableColsProportions.Add( proportion );
}

// helper function for RemoveGrowableCol/Row()
static void
DoRemoveFromArrays(size_t idx, wxArrayInt& items, wxArrayInt& proportions)
{
    const size_t count = items.size();
    for ( size_t n = 0; n < count; n++ )
    {
        if ( (size_t)items[n] == idx )
        {
            items.RemoveAt(n);
            proportions.RemoveAt(n);
            return;
        }
    }

    wxFAIL_MSG( _T("column/row is already not growable") );
}

void wxFlexGridSizer::RemoveGrowableCol( size_t idx )
{
    DoRemoveFromArrays(idx, m_growableCols, m_growableColsProportions);
}

void wxFlexGridSizer::RemoveGrowableRow( size_t idx )
{
    DoRemoveFromArrays(idx, m_growableRows, m_growableRowsProportions);
}

//---------------------------------------------------------------------------
// wxBoxSizer
//---------------------------------------------------------------------------

void wxBoxSizer::RecalcSizes()
{
    if ( m_children.empty() )
        return;

    const wxCoord totalMinorSize = GetSizeInMinorDir(m_size);

    // the amount of free space which we should redistribute among the
    // stretchable items (i.e. those with non zero proportion)
    int delta = GetSizeInMajorDir(m_size) - GetSizeInMajorDir(m_minSize);


    // Inform child items about the size in minor direction, that can
    // change how much free space we have in major dir and how to distribute it.
    int majorMinSum = 0;
    wxSizerItemList::const_iterator i ;
    for ( i = m_children.begin();
          i != m_children.end();
          ++i )
    {
        wxSizerItem * const item = *i;

        if ( !item->IsShown() )
            continue;

        wxSize szMinPrev = item->GetMinSizeWithBorder();
        item->InformFirstDirection(m_orient^wxBOTH,totalMinorSize,delta);
        wxSize szMin = item->GetMinSizeWithBorder();
        int deltaChange = GetSizeInMajorDir(szMin-szMinPrev);
        if( deltaChange )
        {
            // Since we passed available space along to the item, it should not
            // take too much, so delta should not become negative.
            delta -= deltaChange;
        }
        majorMinSum += GetSizeInMajorDir(item->GetMinSizeWithBorder());
    }
    // And update our min size
    SizeInMajorDir(m_minSize) = majorMinSum;


    // might have a new delta now
    delta = GetSizeInMajorDir(m_size) - GetSizeInMajorDir(m_minSize);

    // the position at which we put the next child
    wxPoint pt(m_position);

    int totalProportion = m_totalProportion;
    for ( i = m_children.begin();
          i != m_children.end();
          ++i )
    {
        wxSizerItem * const item = *i;

        if ( !item->IsShown() )
            continue;

        const wxSize sizeThis(item->GetMinSizeWithBorder());

        // adjust the size in the major direction using the proportion
        wxCoord majorSize = GetSizeInMajorDir(sizeThis);

        // if there is not enough space, don't try to distribute negative space
        // among the children, this would result in overlapping windows which
        // we don't want
        if ( delta > 0 )
        {
            const int propItem = item->GetProportion();
            if ( propItem )
            {
                const int deltaItem = (delta * propItem) / totalProportion;

                majorSize += deltaItem;

                delta -= deltaItem;
                totalProportion -= propItem;
            }
        }


        // apply the alignment in the minor direction
        wxPoint posChild(pt);

        wxCoord minorSize = GetSizeInMinorDir(sizeThis);
        const int flag = item->GetFlag();
        if ( flag & (wxEXPAND | wxSHAPED) )
        {
            minorSize = totalMinorSize;
        }
        else if ( flag & (IsVertical() ? wxALIGN_RIGHT : wxALIGN_BOTTOM) )
        {
            PosInMinorDir(posChild) += totalMinorSize - minorSize;
        }
        // NB: wxCENTRE is used here only for backwards compatibility,
        //     wxALIGN_CENTRE should be used in new code
        else if ( flag & (wxCENTER | (IsVertical() ? wxALIGN_CENTRE_HORIZONTAL : wxALIGN_CENTRE_VERTICAL)))
        {
            PosInMinorDir(posChild) += (totalMinorSize - minorSize) / 2;
        }


        // apply RTL adjustment for horizontal sizers:
        if ( !IsVertical() && m_containingWindow )
        {
            posChild.x = m_containingWindow->AdjustForLayoutDirection
                                             (
                                                posChild.x,
                                                majorSize,
                                                m_size.x
                                             );
        }

        // finally set size of this child and advance to the next one
        item->SetDimension(posChild, SizeFromMajorMinor(majorSize, minorSize));

        PosInMajorDir(pt) += majorSize;
    }
}

wxSize wxBoxSizer::CalcMin()
{
    m_totalProportion = 0;
    m_minSize = wxSize(0, 0);

    // calculate the minimal sizes for all items and count sum of proportions
    for ( wxSizerItemList::const_iterator i = m_children.begin();
          i != m_children.end();
          ++i )
    {
        wxSizerItem * const item = *i;

        if ( !item->IsShown() )
            continue;

        const wxSize sizeMinThis = item->CalcMin();
        SizeInMajorDir(m_minSize) += GetSizeInMajorDir(sizeMinThis);
        if ( GetSizeInMinorDir(sizeMinThis) > GetSizeInMinorDir(m_minSize) )
            SizeInMinorDir(m_minSize) = GetSizeInMinorDir(sizeMinThis);

        m_totalProportion += item->GetProportion();
    }

    return m_minSize;
}

//---------------------------------------------------------------------------
// wxStaticBoxSizer
//---------------------------------------------------------------------------

#if wxUSE_STATBOX

wxStaticBoxSizer::wxStaticBoxSizer( wxStaticBox *box, int orient )
    : wxBoxSizer( orient ),
      m_staticBox( box )
{
    wxASSERT_MSG( box, wxT("wxStaticBoxSizer needs a static box") );

    // do this so that our Detach() is called if the static box is destroyed
    // before we are
    m_staticBox->SetContainingSizer(this);
}

wxStaticBoxSizer::wxStaticBoxSizer(int orient, wxWindow *win, const wxString& s)
                : wxBoxSizer(orient),
                  m_staticBox(new wxStaticBox(win, wxID_ANY, s))
{
    // same as above
    m_staticBox->SetContainingSizer(this);
}

wxStaticBoxSizer::~wxStaticBoxSizer()
{
    delete m_staticBox;
}

void wxStaticBoxSizer::RecalcSizes()
{
    int top_border, other_border;
    m_staticBox->GetBordersForSizer(&top_border, &other_border);

    m_staticBox->SetSize( m_position.x, m_position.y, m_size.x, m_size.y );

    wxPoint old_pos( m_position );
    m_position.x += other_border;
    m_position.y += top_border;
    wxSize old_size( m_size );
    m_size.x -= 2*other_border;
    m_size.y -= top_border + other_border;

    wxBoxSizer::RecalcSizes();

    m_position = old_pos;
    m_size = old_size;
}

wxSize wxStaticBoxSizer::CalcMin()
{
    int top_border, other_border;
    m_staticBox->GetBordersForSizer(&top_border, &other_border);

    wxSize ret( wxBoxSizer::CalcMin() );
    ret.x += 2*other_border;

    // ensure that we're wide enough to show the static box label (there is no
    // need to check for the static box best size in vertical direction though)
    const int boxWidth = m_staticBox->GetBestSize().x;
    if ( ret.x < boxWidth )
        ret.x = boxWidth;

    ret.y += other_border + top_border;

    return ret;
}

void wxStaticBoxSizer::ShowItems( bool show )
{
    m_staticBox->Show( show );
    wxBoxSizer::ShowItems( show );
}

bool wxStaticBoxSizer::Detach( wxWindow *window )
{
    // avoid deleting m_staticBox in our dtor if it's being detached from the
    // sizer (which can happen because it's being already destroyed for
    // example)
    if ( window == m_staticBox )
    {
        m_staticBox = NULL;
        return true;
    }

    return wxSizer::Detach( window );
}

#endif // wxUSE_STATBOX

#if wxUSE_BUTTON

wxStdDialogButtonSizer::wxStdDialogButtonSizer()
    : wxBoxSizer(wxHORIZONTAL)
{
    // Vertical buttons with lots of space on either side
    // looks rubbish on WinCE, so let's not do this for now.
    // If we are going to use vertical buttons, we should
    // put the sizer to the right of other controls in the dialog,
    // and that's beyond the scope of this sizer.
#ifndef __WXWINCE__
    bool is_pda = (wxSystemSettings::GetScreenType() <= wxSYS_SCREEN_PDA);
    // If we have a PDA screen, put yes/no button over
    // all other buttons, otherwise on the left side.
    if (is_pda)
        m_orient = wxVERTICAL;
#endif

    m_buttonAffirmative = NULL;
    m_buttonApply = NULL;
    m_buttonNegative = NULL;
    m_buttonCancel = NULL;
    m_buttonHelp = NULL;
}

void wxStdDialogButtonSizer::AddButton(wxButton *mybutton)
{
    switch (mybutton->GetId())
    {
        case wxID_OK:
        case wxID_YES:
        case wxID_SAVE:
            m_buttonAffirmative = mybutton;
            break;
        case wxID_APPLY:
            m_buttonApply = mybutton;
            break;
        case wxID_NO:
            m_buttonNegative = mybutton;
            break;
        case wxID_CANCEL:
        case wxID_CLOSE:
            m_buttonCancel = mybutton;
            break;
        case wxID_HELP:
        case wxID_CONTEXT_HELP:
            m_buttonHelp = mybutton;
            break;
        default:
            break;
    }
}

void wxStdDialogButtonSizer::SetAffirmativeButton( wxButton *button )
{
    m_buttonAffirmative = button;
}

void wxStdDialogButtonSizer::SetNegativeButton( wxButton *button )
{
    m_buttonNegative = button;
}

void wxStdDialogButtonSizer::SetCancelButton( wxButton *button )
{
    m_buttonCancel = button;
}

void wxStdDialogButtonSizer::Realize()
{
#ifdef __WXMAC__
        Add(0, 0, 0, wxLEFT, 6);
        if (m_buttonHelp)
            Add((wxWindow*)m_buttonHelp, 0, wxALIGN_CENTRE | wxLEFT | wxRIGHT, 6);

        if (m_buttonNegative){
            // HIG POLICE BULLETIN - destructive buttons need extra padding
            // 24 pixels on either side
            Add((wxWindow*)m_buttonNegative, 0, wxALIGN_CENTRE | wxLEFT | wxRIGHT, 12);
        }

        // extra whitespace between help/negative and cancel/ok buttons
        Add(0, 0, 1, wxEXPAND, 0);

        if (m_buttonCancel){
            Add((wxWindow*)m_buttonCancel, 0, wxALIGN_CENTRE | wxLEFT | wxRIGHT, 6);
            // Cancel or help should be default
            // m_buttonCancel->SetDefaultButton();
        }

        // Ugh, Mac doesn't really have apply dialogs, so I'll just
        // figure the best place is between Cancel and OK
        if (m_buttonApply)
            Add((wxWindow*)m_buttonApply, 0, wxALIGN_CENTRE | wxLEFT | wxRIGHT, 6);

        if (m_buttonAffirmative){
            Add((wxWindow*)m_buttonAffirmative, 0, wxALIGN_CENTRE | wxLEFT, 6);

            if (m_buttonAffirmative->GetId() == wxID_SAVE){
                // these buttons have set labels under Mac so we should use them
                m_buttonAffirmative->SetLabel(_("Save"));
                if (m_buttonNegative)
                    m_buttonNegative->SetLabel(_("Don't Save"));
            }
        }

        // Extra space around and at the right
        Add(12, 40);
#elif defined(__WXGTK20__)
        Add(0, 0, 0, wxLEFT, 9);
        if (m_buttonHelp)
            Add((wxWindow*)m_buttonHelp, 0, wxALIGN_CENTRE | wxLEFT | wxRIGHT, 3);

        // extra whitespace between help and cancel/ok buttons
        Add(0, 0, 1, wxEXPAND, 0);

        if (m_buttonNegative){
            Add((wxWindow*)m_buttonNegative, 0, wxALIGN_CENTRE | wxLEFT | wxRIGHT, 3);
        }

        // according to HIG, in explicit apply windows the order is:
        // [ Help                     Apply   Cancel   OK ]
        if (m_buttonApply)
            Add((wxWindow*)m_buttonApply, 0, wxALIGN_CENTRE | wxLEFT | wxRIGHT, 3);

        if (m_buttonCancel){
            Add((wxWindow*)m_buttonCancel, 0, wxALIGN_CENTRE | wxLEFT | wxRIGHT, 3);
            // Cancel or help should be default
            // m_buttonCancel->SetDefaultButton();
        }

        if (m_buttonAffirmative)
            Add((wxWindow*)m_buttonAffirmative, 0, wxALIGN_CENTRE | wxLEFT, 6);
#elif defined(__WXMSW__)
        // Windows

        // right-justify buttons
        Add(0, 0, 1, wxEXPAND, 0);

        if (m_buttonAffirmative){
            Add((wxWindow*)m_buttonAffirmative, 0, wxALIGN_CENTRE | wxLEFT | wxRIGHT, m_buttonAffirmative->ConvertDialogToPixels(wxSize(2, 0)).x);
        }

        if (m_buttonNegative){
            Add((wxWindow*)m_buttonNegative, 0, wxALIGN_CENTRE | wxLEFT | wxRIGHT, m_buttonNegative->ConvertDialogToPixels(wxSize(2, 0)).x);
        }

        if (m_buttonCancel){
            Add((wxWindow*)m_buttonCancel, 0, wxALIGN_CENTRE | wxLEFT | wxRIGHT, m_buttonCancel->ConvertDialogToPixels(wxSize(2, 0)).x);
        }
        if (m_buttonApply)
            Add((wxWindow*)m_buttonApply, 0, wxALIGN_CENTRE | wxLEFT | wxRIGHT, m_buttonApply->ConvertDialogToPixels(wxSize(2, 0)).x);

        if (m_buttonHelp)
            Add((wxWindow*)m_buttonHelp, 0, wxALIGN_CENTRE | wxLEFT | wxRIGHT, m_buttonHelp->ConvertDialogToPixels(wxSize(2, 0)).x);
#else
        // GTK+1 and any other platform

        // Add(0, 0, 0, wxLEFT, 5); // Not sure what this was for but it unbalances the dialog
        if (m_buttonHelp)
            Add((wxWindow*)m_buttonHelp, 0, wxALIGN_CENTRE | wxLEFT | wxRIGHT, m_buttonHelp->ConvertDialogToPixels(wxSize(4, 0)).x);

        // extra whitespace between help and cancel/ok buttons
        Add(0, 0, 1, wxEXPAND, 0);

        if (m_buttonApply)
            Add((wxWindow*)m_buttonApply, 0, wxALIGN_CENTRE | wxLEFT | wxRIGHT, m_buttonApply->ConvertDialogToPixels(wxSize(4, 0)).x);

        if (m_buttonAffirmative){
            Add((wxWindow*)m_buttonAffirmative, 0, wxALIGN_CENTRE | wxLEFT | wxRIGHT, m_buttonAffirmative->ConvertDialogToPixels(wxSize(4, 0)).x);
        }

        if (m_buttonNegative){
            Add((wxWindow*)m_buttonNegative, 0, wxALIGN_CENTRE | wxLEFT | wxRIGHT, m_buttonNegative->ConvertDialogToPixels(wxSize(4, 0)).x);
        }

        if (m_buttonCancel){
            Add((wxWindow*)m_buttonCancel, 0, wxALIGN_CENTRE | wxLEFT | wxRIGHT, m_buttonCancel->ConvertDialogToPixels(wxSize(4, 0)).x);
            // Cancel or help should be default
            // m_buttonCancel->SetDefaultButton();
        }

#endif
}

#endif // wxUSE_BUTTON
