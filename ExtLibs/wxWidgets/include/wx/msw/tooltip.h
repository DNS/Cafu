///////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/tooltip.h
// Purpose:     wxToolTip class - tooltip control
// Author:      Vadim Zeitlin
// Modified by:
// Created:     31.01.99
// RCS-ID:      $Id: tooltip.h 58757 2009-02-08 11:45:59Z VZ $
// Copyright:   (c) 1999 Robert Roebling, Vadim Zeitlin
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MSW_TOOLTIP_H_
#define _WX_MSW_TOOLTIP_H_

#include "wx/object.h"

class WXDLLIMPEXP_FWD_CORE wxWindow;

class WXDLLIMPEXP_CORE wxToolTip : public wxObject
{
public:
    // ctor & dtor
    wxToolTip(const wxString &tip);
    virtual ~wxToolTip();

    // accessors
        // tip text
    void SetTip(const wxString& tip);
    const wxString& GetTip() const { return m_text; }

        // the window we're associated with
    void SetWindow(wxWindow *win);
    wxWindow *GetWindow() const { return m_window; }

    // controlling tooltip behaviour: globally change tooltip parameters
        // enable or disable the tooltips globally
    static void Enable(bool flag);
        // set the delay after which the tooltip appears
    static void SetDelay(long milliseconds);
        // set the delay after which the tooltip disappears or how long the
        // tooltip remains visible
    static void SetAutoPop(long milliseconds);
        // set the delay between subsequent tooltips to appear
    static void SetReshow(long milliseconds);
        // set maximum width for the new tooltips: -1 disables wrapping
        // entirely, 0 restores the default behaviour
    static void SetMaxWidth(int width);

    // implementation only from now on
    // -------------------------------

    // should be called in responde to WM_MOUSEMOVE
    static void RelayEvent(WXMSG *msg);

    // add a window to the tooltip control
    void Add(WXHWND hwnd);

    // remove any tooltip from the window
    static void Remove(WXHWND hwnd);

private:
    // the one and only one tooltip control we use - never access it directly
    // but use GetToolTipCtrl() which will create it when needed
    static WXHWND ms_hwndTT;

    // create the tooltip ctrl if it doesn't exist yet and return its HWND
    static WXHWND GetToolTipCtrl();

    // new tooltip maximum width, defaults to min(display width, 400)
    static int ms_maxWidth;

    // remove this tooltip from the tooltip control
    void Remove();

    wxString  m_text;           // tooltip text
    wxWindow *m_window;         // window we're associated with

    DECLARE_ABSTRACT_CLASS(wxToolTip)
    wxDECLARE_NO_COPY_CLASS(wxToolTip);
};

#endif // _WX_MSW_TOOLTIP_H_
