/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/dialog.h
// Purpose:     wxDialog class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: dialog.h 58757 2009-02-08 11:45:59Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_DIALOG_H_
#define _WX_DIALOG_H_

#include "wx/panel.h"

extern WXDLLIMPEXP_DATA_CORE(const char) wxDialogNameStr[];

class WXDLLIMPEXP_FWD_CORE wxDialogModalData;

#if wxUSE_TOOLBAR && (defined(__SMARTPHONE__) || defined(__POCKETPC__))
class WXDLLIMPEXP_FWD_CORE wxToolBar;
extern WXDLLIMPEXP_DATA_CORE(const char) wxToolBarNameStr[];
#endif

// Dialog boxes
class WXDLLIMPEXP_CORE wxDialog : public wxDialogBase
{
public:
    wxDialog() { Init(); }

    // full ctor
    wxDialog(wxWindow *parent, wxWindowID id,
             const wxString& title,
             const wxPoint& pos = wxDefaultPosition,
             const wxSize& size = wxDefaultSize,
             long style = wxDEFAULT_DIALOG_STYLE,
             const wxString& name = wxDialogNameStr)
    {
        Init();

        (void)Create(parent, id, title, pos, size, style, name);
    }

    bool Create(wxWindow *parent, wxWindowID id,
                const wxString& title,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxDEFAULT_DIALOG_STYLE,
                const wxString& name = wxDialogNameStr);

    virtual ~wxDialog();

    // return true if we're showing the dialog modally
    virtual bool IsModal() const { return m_modalData != NULL; }

    // show the dialog modally and return the value passed to EndModal()
    virtual int ShowModal();

    // may be called to terminate the dialog with the given return code
    virtual void EndModal(int retCode);


    // we treat dialog toolbars specially under Windows CE
#if wxUSE_TOOLBAR && defined(__POCKETPC__)
    // create main toolbar by calling OnCreateToolBar()
    virtual wxToolBar* CreateToolBar(long style = -1,
                                     wxWindowID winid = wxID_ANY,
                                     const wxString& name = wxToolBarNameStr);
    // return a new toolbar
    virtual wxToolBar *OnCreateToolBar(long style,
                                       wxWindowID winid,
                                       const wxString& name );

    // get the main toolbar
    wxToolBar *GetToolBar() const { return m_dialogToolBar; }
#endif // wxUSE_TOOLBAR && __POCKETPC__


    // implementation only from now on
    // -------------------------------

    // override some base class virtuals
    virtual bool Show(bool show = true);

    virtual void Raise();

    virtual void SetWindowStyleFlag(long style);

#ifdef __POCKETPC__
    // Responds to the OK button in a PocketPC titlebar. This
    // can be overridden, or you can change the id used for
    // sending the event with SetAffirmativeId. Returns false
    // if the event was not processed.
    virtual bool DoOK();
#endif

    // Windows callbacks
    WXLRESULT MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam);

protected:
    // find the window to use as parent for this dialog if none has been
    // specified explicitly by the user
    //
    // may return NULL
    wxWindow *FindSuitableParent() const;

    // common part of all ctors
    void Init();

    // these functions deal with the gripper window shown in the corner of
    // resizeable dialogs
    void CreateGripper();
    void DestroyGripper();
    void ShowGripper(bool show);
    void ResizeGripper();

private:
    // this function is used to adjust Z-order of new children relative to the
    // gripper if we have one
    void OnWindowCreate(wxWindowCreateEvent& event);

#if wxUSE_TOOLBAR && defined(__POCKETPC__)
    wxToolBar*  m_dialogToolBar;
#endif

    // this pointer is non-NULL only while the modal event loop is running
    wxDialogModalData *m_modalData;

    // gripper window for a resizable dialog, NULL if we're not resizable
    WXHWND m_hGripper;

    DECLARE_DYNAMIC_CLASS(wxDialog)
    wxDECLARE_NO_COPY_CLASS(wxDialog);
};

#endif
    // _WX_DIALOG_H_
