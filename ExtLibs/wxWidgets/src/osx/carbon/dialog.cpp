/////////////////////////////////////////////////////////////////////////////
// Name:        src/osx/carbon/dialog.cpp
// Purpose:     wxDialog class
// Author:      Stefan Csomor
// Modified by:
// Created:     1998-01-01
// RCS-ID:      $Id: dialog.cpp 61420 2009-07-13 05:12:22Z SC $
// Copyright:   (c) Stefan Csomor
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#include "wx/dialog.h"

#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/utils.h"
    #include "wx/frame.h"
    #include "wx/settings.h"
#endif // WX_PRECOMP

#include "wx/osx/private.h"
#include "wx/evtloop.h"

extern wxList wxModalDialogs;

void wxDialog::DoShowModal()
{
    wxCHECK_RET( !IsModal(), wxT("DoShowModal() called twice") );

    wxModalDialogs.Append(this);

    SetFocus() ;

    WindowRef windowRef = (WindowRef) GetWXWindow();
    WindowGroupRef windowGroup;
    WindowGroupRef formerParentGroup;
    bool resetGroupParent = false;

    if ( GetParent() == NULL )
    {
        windowGroup = GetWindowGroup(windowRef) ;
        formerParentGroup = GetWindowGroupParent( windowGroup );
        SetWindowGroupParent( windowGroup, GetWindowGroupOfClass( kMovableModalWindowClass ) );
        resetGroupParent = true;
    }
    BeginAppModalStateForWindow(windowRef) ;

    wxEventLoopGuarantor ensureHasLoop;
    wxEventLoopBase * const loop = wxEventLoop::GetActive();
    while ( IsModal() )
        loop->Dispatch();

    EndAppModalStateForWindow(windowRef) ;
    if ( resetGroupParent )
    {
        SetWindowGroupParent( windowGroup , formerParentGroup );
    }
}