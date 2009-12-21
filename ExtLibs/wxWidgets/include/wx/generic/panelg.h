/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/panelg.h
// Purpose:     wxPanel: a container for child controls
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: panelg.h 52834 2008-03-26 15:06:00Z FM $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GENERIC_PANEL_H_
#define _WX_GENERIC_PANEL_H_

// ----------------------------------------------------------------------------
// headers and forward declarations
// ----------------------------------------------------------------------------

#include "wx/window.h"
#include "wx/containr.h"

class WXDLLIMPEXP_FWD_CORE wxControlContainer;

extern WXDLLIMPEXP_DATA_CORE(const char) wxPanelNameStr[];

// ----------------------------------------------------------------------------
// wxPanel contains other controls and implements TAB traversal between them
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxPanel : public wxWindow
{
public:
    wxPanel() { Init(); }

    // Old-style constructor (no default values for coordinates to avoid
    // ambiguity with the new one)
    wxPanel(wxWindow *parent,
            int x, int y, int width, int height,
            long style = wxTAB_TRAVERSAL | wxNO_BORDER,
            const wxString& name = wxPanelNameStr)
    {
        Init();

        Create(parent, wxID_ANY, wxPoint(x, y), wxSize(width, height), style, name);
    }

    // Constructor
    wxPanel(wxWindow *parent,
            wxWindowID winid = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxTAB_TRAVERSAL | wxNO_BORDER,
            const wxString& name = wxPanelNameStr)
    {
        Init();

        Create(parent, winid, pos, size, style, name);
    }

    // Pseudo ctor
    bool Create(wxWindow *parent,
                wxWindowID winid = wxID_ANY,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxTAB_TRAVERSAL | wxNO_BORDER,
                const wxString& name = wxPanelNameStr);

    virtual ~wxPanel();

    // implementation from now on
    // --------------------------

        // calls layout for layout constraints and sizers
    void OnSize(wxSizeEvent& event);

    virtual void InitDialog();

#ifdef __WXUNIVERSAL__
    virtual bool IsCanvasWindow() const { return true; }
#endif

    WX_DECLARE_CONTROL_CONTAINER();

protected:
    // common part of all ctors
    void Init();

    // choose the default border for this window
    virtual wxBorder GetDefaultBorder() const { return wxWindowBase::GetDefaultBorder(); }

    DECLARE_DYNAMIC_CLASS_NO_COPY(wxPanel)
    DECLARE_EVENT_TABLE()
};

#endif
    // _WX_GENERIC_PANEL_H_
