/////////////////////////////////////////////////////////////////////////////
// Name:        wx/motif/dcprint.h
// Purpose:     wxPrinterDC class
// Author:      Julian Smart
// Modified by:
// Created:     17/09/98
// RCS-ID:      $Id: dcprint.h 56783 2008-11-15 11:10:34Z FM $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_DCPRINT_H_
#define _WX_DCPRINT_H_

#include "wx/motif/dc.h"

class WXDLLIMPEXP_CORE wxPrinterDC : public wxMotifDCImpl
{
public:
    // Create a printer DC
    wxPrinterDCImpl(const wxString& driver, const wxString& device,
                    const wxString& output,
                    bool interactive = true,
                    int orientation = wxPORTRAIT);
    virtual ~wxPrinterDC();

    wxRect GetPaperRect() const;

    DECLARE_CLASS(wxPrinterDCImpl)
};

#endif // _WX_DCPRINT_H_
