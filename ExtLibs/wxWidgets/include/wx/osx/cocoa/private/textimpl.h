/////////////////////////////////////////////////////////////////////////////
// Name:        textimpl.h
// Purpose:     textcontrol implementation classes that have to be exposed
// Author:      Stefan Csomor
// Modified by:
// Created:     03/02/99
// RCS-ID:      $Id: textimpl.h 60760 2009-05-27 09:58:37Z SC $
// Copyright:   (c) Stefan Csomor
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_OSX_COCOA_PRIVATE_TEXTIMPL_H_
#define _WX_OSX_COCOA_PRIVATE_TEXTIMPL_H_

#include "wx/osx/private.h"

// implementation exposed, so that search control can pull it

class wxNSTextFieldControl : public wxWidgetCocoaImpl, public wxTextWidgetImpl
{
public :
    wxNSTextFieldControl( wxTextCtrl *wxPeer, WXWidget w );
    virtual ~wxNSTextFieldControl();

    virtual wxString GetStringValue() const ;
    virtual void SetStringValue( const wxString &str) ;
    virtual void Copy() ;
    virtual void Cut() ;
    virtual void Paste() ;
    virtual bool CanPaste() const ;
    virtual void SetEditable(bool editable) ;
    virtual void GetSelection( long* from, long* to) const ;
    virtual void SetSelection( long from , long to );
    virtual void WriteText(const wxString& str) ;
    
    virtual void controlAction(WXWidget slf, void* _cmd, void *sender);
protected :
    NSTextField* m_textField;
    long m_selStart;
    long m_selEnd;
};

class wxNSTextViewControl : public wxWidgetCocoaImpl, public wxTextWidgetImpl
{
public:
    wxNSTextViewControl( wxTextCtrl *wxPeer, WXWidget w );
    virtual ~wxNSTextViewControl();

    virtual wxString GetStringValue() const ;
    virtual void SetStringValue( const wxString &str) ;
    virtual void Copy() ;
    virtual void Cut() ;
    virtual void Paste() ;
    virtual bool CanPaste() const ;
    virtual void SetEditable(bool editable) ;
    virtual void GetSelection( long* from, long* to) const ;
    virtual void SetSelection( long from , long to );
    virtual void WriteText(const wxString& str) ;
    virtual void SetFont( const wxFont & font , const wxColour& foreground , long windowStyle, bool ignoreBlack = true );

protected:
    NSScrollView* m_scrollView;
    NSTextView* m_textView;
};

#endif // _WX_OSX_COCOA_PRIVATE_TEXTIMPL_H_
