/////////////////////////////////////////////////////////////////////////////
// Name:        dialog.h
// Purpose:
// Author:      Robert Roebling
// Created:
// Id:          $Id: dialog.h 55115 2008-08-18 11:51:53Z VZ $
// Copyright:   (c) 1998 Robert Roebling
// Licence:           wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef __GTKDIALOGH__
#define __GTKDIALOGH__

class WXDLLIMPEXP_FWD_CORE wxGUIEventLoop;

//-----------------------------------------------------------------------------
// wxDialog
//-----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxDialog: public wxDialogBase
{
public:
    wxDialog() { Init(); }
    wxDialog( wxWindow *parent, wxWindowID id,
            const wxString &title,
            const wxPoint &pos = wxDefaultPosition,
            const wxSize &size = wxDefaultSize,
            long style = wxDEFAULT_DIALOG_STYLE,
            const wxString &name = wxDialogNameStr );
    bool Create( wxWindow *parent, wxWindowID id,
            const wxString &title,
            const wxPoint &pos = wxDefaultPosition,
            const wxSize &size = wxDefaultSize,
            long style = wxDEFAULT_DIALOG_STYLE,
            const wxString &name = wxDialogNameStr );
    virtual ~wxDialog();

    virtual bool Show( bool show = TRUE );
    virtual int ShowModal();
    virtual void EndModal( int retCode );
    virtual bool IsModal() const;
    void SetModal( bool modal );

    // implementation
    // --------------

    bool       m_modalShowing;

private:
    // common part of all ctors
    void Init();
    wxGUIEventLoop *m_modalLoop;
    DECLARE_DYNAMIC_CLASS(wxDialog)
};

#endif // __GTKDIALOGH__
