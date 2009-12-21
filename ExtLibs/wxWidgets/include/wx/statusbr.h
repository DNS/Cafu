/////////////////////////////////////////////////////////////////////////////
// Name:        wx/statusbr.h
// Purpose:     wxStatusBar class interface
// Author:      Vadim Zeitlin
// Modified by:
// Created:     05.02.00
// RCS-ID:      $Id: statusbr.h 59568 2009-03-15 19:45:34Z FM $
// Copyright:   (c) Vadim Zeitlin
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_STATUSBR_H_BASE_
#define _WX_STATUSBR_H_BASE_

#include "wx/defs.h"

#if wxUSE_STATUSBAR

#include "wx/window.h"
#include "wx/list.h"
#include "wx/dynarray.h"

extern WXDLLIMPEXP_DATA_CORE(const char) wxStatusBarNameStr[];

// ----------------------------------------------------------------------------
// wxStatusBar constants
// ----------------------------------------------------------------------------

// style flags for fields
#define wxSB_NORMAL    0x0000
#define wxSB_FLAT      0x0001
#define wxSB_RAISED    0x0002

// ----------------------------------------------------------------------------
// wxStatusBarPane: an helper for wxStatusBar
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxStatusBarPane
{
    // only wxStatusBarBase can access our internal members and modify them:
    friend class WXDLLIMPEXP_FWD_CORE wxStatusBarBase;

public:
    wxStatusBarPane(int style = wxSB_NORMAL, size_t width = 0)
        : m_nStyle(style), m_nWidth(width) { m_arrStack.Add(wxEmptyString); }
        
    int GetWidth() const
        { return m_nWidth; }
    int GetStyle() const
        { return m_nStyle; }
        
    const wxArrayString& GetStack() const
        { return m_arrStack; }

    // use wxStatusBar setter functions to modify a wxStatusBarPane

protected:
    int m_nStyle;
    int m_nWidth;     // the width maybe negative, indicating a variable-width field

    // this is the array of the stacked strings of this pane; note that this
    // stack does include also the string currently displayed in this pane
    // as the version stored in the native status bar control is possibly
    // ellipsized; note that arrStack.Last() is the top of the stack
    // (i.e. the string shown in the status bar)
    wxArrayString m_arrStack;
};

WX_DECLARE_OBJARRAY(wxStatusBarPane, wxStatusBarPaneArray);

// ----------------------------------------------------------------------------
// wxStatusBar: a window near the bottom of the frame used for status info
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxStatusBarBase : public wxWindow
{
public:
    wxStatusBarBase();

    virtual ~wxStatusBarBase();

    // field count
    // -----------

    // set the number of fields and call SetStatusWidths(widths) if widths are
    // given
    virtual void SetFieldsCount(int number = 1, const int *widths = NULL);
    int GetFieldsCount() const { return m_panes.GetCount(); }

    // field text
    // ----------

    virtual void SetStatusText(const wxString& text, int number = 0)
        { m_panes[number].GetStack().Last() = text; }
    virtual wxString GetStatusText(int number = 0) const
        { return m_panes[number].GetStack().Last(); }
    const wxArrayString& GetStatusStack(int n) const
        { return m_panes[n].GetStack(); }

    void PushStatusText(const wxString& text, int number = 0);
    void PopStatusText(int number = 0);

    // fields widths
    // -------------

    // set status field widths as absolute numbers: positive widths mean that
    // the field has the specified absolute width, negative widths are
    // interpreted as the sizer options, i.e. the extra space (total space
    // minus the sum of fixed width fields) is divided between the fields with
    // negative width according to the abs value of the width (field with width
    // -2 grows twice as much as one with width -1 &c)
    virtual void SetStatusWidths(int n, const int widths[]);
    
    int GetStatusWidth(int n) const
        { return m_panes[n].GetWidth(); }

    // field styles
    // ------------

    // Set the field style. Use either wxSB_NORMAL (default) for a standard 3D
    // border around a field, wxSB_FLAT for no border around a field, so that it
    // appears flat or wxSB_POPOUT to make the field appear raised.
    // Setting field styles only works on wxMSW
    virtual void SetStatusStyles(int n, const int styles[]);
    
    int GetStatusStyle(int n) const
        { return m_panes[n].GetStyle(); }

    // geometry
    // --------

    // Get the position and size of the field's internal bounding rectangle
    virtual bool GetFieldRect(int i, wxRect& rect) const = 0;

    // sets the minimal vertical size of the status bar
    virtual void SetMinHeight(int height) = 0;

    // get the dimensions of the horizontal and vertical borders
    virtual int GetBorderX() const = 0;
    virtual int GetBorderY() const = 0;

    // miscellaneous
    // -------------
    
    const wxStatusBarPane& GetField(int n) const
        { return m_panes[n]; }
    
    // wxWindow overrides:
    
    // don't want status bars to accept the focus at all
    virtual bool AcceptsFocus() const { return false; }

    // the client size of a toplevel window doesn't include the status bar
    virtual bool CanBeOutsideClientArea() const { return true; }

protected:
    virtual wxBorder GetDefaultBorder() const { return wxBORDER_NONE; }

    // calculate the real field widths for the given total available size
    wxArrayInt CalculateAbsWidths(wxCoord widthTotal) const;

    // the array with the pane infos:
    wxStatusBarPaneArray m_panes;

    // if true overrides the width info of the wxStatusBarPanes
    bool m_bSameWidthForAllPanes;

    wxDECLARE_NO_COPY_CLASS(wxStatusBarBase);
};

// ----------------------------------------------------------------------------
// include the actual wxStatusBar class declaration
// ----------------------------------------------------------------------------

#if defined(__WXUNIVERSAL__)
    #define wxStatusBarUniv wxStatusBar
    #include "wx/univ/statusbr.h"
#elif defined(__WXPALMOS__)
    #define wxStatusBarPalm wxStatusBar
    #include "wx/palmos/statusbr.h"
#elif defined(__WIN32__) && wxUSE_NATIVE_STATUSBAR
    #include "wx/msw/statusbar.h"
#elif defined(__WXMAC__)
    #define wxStatusBarMac wxStatusBar
    #include "wx/generic/statusbr.h"
    #include "wx/osx/statusbr.h"
#else
    #define wxStatusBarGeneric wxStatusBar
    #include "wx/generic/statusbr.h"
#endif

#endif // wxUSE_STATUSBAR

#endif
    // _WX_STATUSBR_H_BASE_
