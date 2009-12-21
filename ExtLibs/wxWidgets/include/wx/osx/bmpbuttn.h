/////////////////////////////////////////////////////////////////////////////
// Name:        bmpbuttn.h
// Purpose:     wxBitmapButton class
// Author:      Stefan Csomor
// Modified by:
// Created:     1998-01-01
// RCS-ID:      $Id: bmpbuttn.h 58168 2009-01-17 10:43:43Z SC $
// Copyright:   (c) Stefan Csomor
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_BMPBUTTN_H_
#define _WX_BMPBUTTN_H_

#include "wx/button.h"

WXDLLIMPEXP_DATA_CORE(extern const char) wxButtonNameStr[];

#define wxDEFAULT_BUTTON_MARGIN 4

class WXDLLIMPEXP_CORE wxBitmapButton: public wxBitmapButtonBase
{
    DECLARE_DYNAMIC_CLASS(wxBitmapButton)

public:
    wxBitmapButton()
        {
            SetMargins(wxDEFAULT_BUTTON_MARGIN, wxDEFAULT_BUTTON_MARGIN);
        }
    
    wxBitmapButton(wxWindow *parent, wxWindowID id, const wxBitmap& bitmap,
                   const wxPoint& pos = wxDefaultPosition,
                   const wxSize& size = wxDefaultSize, long style = wxBU_AUTODRAW,
                   const wxValidator& validator = wxDefaultValidator,
                   const wxString& name = wxButtonNameStr)
        {
            Create(parent, id, bitmap, pos, size, style, validator, name);
        }

    bool Create(wxWindow *parent, wxWindowID id, const wxBitmap& bitmap,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize, long style = wxBU_AUTODRAW,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxButtonNameStr);

    virtual void SetBitmapLabel(const wxBitmap& bitmap);

protected:
    virtual wxSize DoGetBestSize() const;    
};

#endif
    // _WX_BMPBUTTN_H_
