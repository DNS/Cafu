/////////////////////////////////////////////////////////////////////////////
// Name:        wx/osx/cocoa/private.h
// Purpose:     Private declarations: as this header is only included by
//              wxWidgets itself, it may contain identifiers which don't start
//              with "wx".
// Author:      Stefan Csomor
// Modified by:
// Created:     1998-01-01
// RCS-ID:      $Id: private.h 53819 2008-05-29 14:11:45Z SC $
// Copyright:   (c) Stefan Csomor
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_PRIVATE_COCOA_H_
#define _WX_PRIVATE_COCOA_H_

#include "wx/osx/core/private.h"

#include "wx/defs.h"

#include <ApplicationServices/ApplicationServices.h>

#if wxOSX_USE_ATSU_TEXT
    // we need theming and atsu
    #include <Carbon/Carbon.h>
#endif

#ifdef __OBJC__
    #import <Cocoa/Cocoa.h>
#endif

//
// shared between Cocoa and Carbon
//

// bring in themeing types without pulling in the headers

#if wxUSE_GUI
typedef SInt16 ThemeBrush;
CGColorRef WXDLLIMPEXP_CORE wxMacCreateCGColorFromHITheme( ThemeBrush brush ) ;
OSStatus WXDLLIMPEXP_CORE wxMacDrawCGImage(
                               CGContextRef    inContext,
                               const CGRect *  inBounds,
                               CGImageRef      inImage) ;
WX_NSImage WXDLLIMPEXP_CORE wxOSXCreateNSImageFromCGImage( CGImageRef image );
#endif

long UMAGetSystemVersion() ;
WXDLLIMPEXP_BASE void wxMacStringToPascal( const wxString&from , StringPtr to );
WXDLLIMPEXP_BASE wxString wxMacFSRefToPath( const FSRef *fsRef , CFStringRef additionalPathComponent = NULL );
WXDLLIMPEXP_BASE OSStatus wxMacPathToFSRef( const wxString&path , FSRef *fsRef );
WXDLLIMPEXP_BASE wxString wxMacHFSUniStrToString( ConstHFSUniStr255Param uniname );

//
//
//

#if wxUSE_GUI

class WXDLLIMPEXP_CORE wxWidgetCocoaImpl : public wxWidgetImpl
{
public :
    wxWidgetCocoaImpl( wxWindowMac* peer , WXWidget w, bool isRootControl = false ) ;    
    wxWidgetCocoaImpl() ;    
    ~wxWidgetCocoaImpl();    
    
    void Init();
    
    virtual bool        IsVisible() const ;
    virtual void        SetVisibility(bool);

    virtual void        Raise();
    
    virtual void        Lower();

    virtual void        ScrollRect( const wxRect *rect, int dx, int dy );

    virtual WXWidget    GetWXWidget() const { return m_osxView; }

    virtual void        SetBackgroundColour(const wxColour&);

    virtual void        GetContentArea( int &left , int &top , int &width , int &height ) const;
    virtual void        Move(int x, int y, int width, int height);
    virtual void        GetPosition( int &x, int &y ) const;
    virtual void        GetSize( int &width, int &height ) const;
    virtual void        SetControlSize( wxWindowVariant variant );

    virtual void        SetNeedsDisplay( const wxRect* where = NULL );
    virtual bool        GetNeedsDisplay() const;

    virtual bool        CanFocus() const;
    // return true if successful
    virtual bool        SetFocus();
    virtual bool        HasFocus() const;

    void                RemoveFromParent();
    void                Embed( wxWidgetImpl *parent );

    void                SetDefaultButton( bool isDefault );
    void                PerformClick();
    virtual void        SetLabel(const wxString& title, wxFontEncoding encoding);

    void                SetCursor( const wxCursor & cursor );
    void                CaptureMouse();
    void                ReleaseMouse();

    wxInt32             GetValue() const;
    void                SetValue( wxInt32 v );
    void                SetBitmap( const wxBitmap& bitmap );
    void                SetupTabs( const wxNotebook &notebook );
    void                GetBestRect( wxRect *r ) const;
    bool                IsEnabled() const;
    void                Enable( bool enable );
    bool                ButtonClickDidStateChange() { return true ;}
    void                SetMinimum( wxInt32 v );
    void                SetMaximum( wxInt32 v );
    wxInt32             GetMinimum() const;
    wxInt32             GetMaximum() const;
    void                PulseGauge();
    void                SetScrollThumb( wxInt32 value, wxInt32 thumbSize );

    void                SetFont( const wxFont & font , const wxColour& foreground , long windowStyle, bool ignoreBlack = true );

    void                InstallEventHandler( WXWidget control = NULL );
    
    virtual bool        DoHandleMouseEvent(NSEvent *event); 
    virtual bool        DoHandleKeyEvent(NSEvent *event);
    virtual bool        DoHandleCharEvent(NSEvent *event, NSString *text);
    virtual void        DoNotifyFocusEvent(bool receivedFocus, wxWidgetImpl* otherWindow); 

    void                SetFlipped(bool flipped);
    virtual bool        IsFlipped() const { return m_isFlipped; }

    // cocoa thunk connected calls

    virtual unsigned int        draggingEntered(void* sender, WXWidget slf, void* _cmd);
    virtual void                draggingExited(void* sender, WXWidget slf, void* _cmd);
    virtual unsigned int        draggingUpdated(void* sender, WXWidget slf, void* _cmd);
    virtual bool                performDragOperation(void* sender, WXWidget slf, void* _cmd);
    virtual void                mouseEvent(WX_NSEvent event, WXWidget slf, void* _cmd);
    virtual void                keyEvent(WX_NSEvent event, WXWidget slf, void* _cmd);
    virtual void                insertText(NSString* text, WXWidget slf, void* _cmd);
    virtual bool                performKeyEquivalent(WX_NSEvent event, WXWidget slf, void* _cmd);
    virtual bool                acceptsFirstResponder(WXWidget slf, void* _cmd);
    virtual bool                becomeFirstResponder(WXWidget slf, void* _cmd);
    virtual bool                resignFirstResponder(WXWidget slf, void* _cmd);
    virtual void                resetCursorRects(WXWidget slf, void* _cmd);
    virtual bool                isFlipped(WXWidget slf, void* _cmd);
    virtual void                drawRect(void* rect, WXWidget slf, void* _cmd);
    
    virtual void                controlAction(WXWidget slf, void* _cmd, void* sender);
    virtual void                controlDoubleAction(WXWidget slf, void* _cmd, void *sender);

protected:
    WXWidget m_osxView;
    NSEvent* m_lastKeyDownEvent;
    bool m_isFlipped;
    // if it the control has an editor, that editor will already send some
    // events, don't resend them 
    bool m_hasEditor;
    
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxWidgetCocoaImpl)
};

DECLARE_WXCOCOA_OBJC_CLASS( wxNSWindow );

class wxNonOwnedWindowCocoaImpl : public wxNonOwnedWindowImpl
{
public :
    wxNonOwnedWindowCocoaImpl( wxNonOwnedWindow* nonownedwnd) ;
    wxNonOwnedWindowCocoaImpl();
    
    virtual ~wxNonOwnedWindowCocoaImpl();
    
    virtual void Destroy() ;
    void Create( wxWindow* parent, const wxPoint& pos, const wxSize& size,
    long style, long extraStyle, const wxString& name ) ;
    
    WXWindow GetWXWindow() const;
    void Raise();    
    void Lower();
    bool Show(bool show);    
    bool ShowWithEffect(bool show, wxShowEffect effect, unsigned timeout);
        
    void Update();
    bool SetTransparent(wxByte alpha);
    bool SetBackgroundColour(const wxColour& col );
    void SetExtraStyle( long exStyle );    
    bool SetBackgroundStyle(wxBackgroundStyle style);    
    bool CanSetTransparent();

    void MoveWindow(int x, int y, int width, int height);
    void GetPosition( int &x, int &y ) const;
    void GetSize( int &width, int &height ) const;

    void GetContentArea( int &left , int &top , int &width , int &height ) const;    
    bool SetShape(const wxRegion& region);
    
    virtual void SetTitle( const wxString& title, wxFontEncoding encoding ) ;
    
    virtual bool IsMaximized() const;
    
    virtual bool IsIconized() const;
    
    virtual void Iconize( bool iconize );
    
    virtual void Maximize(bool maximize);
    
    virtual bool IsFullScreen() const;
    
    virtual bool ShowFullScreen(bool show, long style);

    virtual void RequestUserAttention(int flags);
    
    virtual void ScreenToWindow( int *x, int *y );
    
    virtual void WindowToScreen( int *x, int *y );
    
    wxNonOwnedWindow*   GetWXPeer() { return m_wxPeer; }
protected :
    WX_wxNSWindow         m_macWindow;
    void *              m_macFullScreenData ;
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxNonOwnedWindowCocoaImpl)
};    

#ifdef __OBJC__

    extern NSRect wxToNSRect( NSView* parent, const wxRect& r );
    extern wxRect wxFromNSRect( NSView* parent, const NSRect& rect );
    extern NSPoint wxToNSPoint( NSView* parent, const wxPoint& p );
    extern wxPoint wxFromNSPoint( NSView* parent, const NSPoint& p );
    
    NSRect WXDLLIMPEXP_CORE wxOSXGetFrameForControl( wxWindowMac* window , const wxPoint& pos , const wxSize &size , 
        bool adjustForOrigin = true );
        
    // used for many wxControls
    
    @interface wxNSButton : NSButton
    {
    }
    
    @end

    @interface wxNSBox : NSBox
    {
    }

    @end

    @interface wxNSTextFieldEditor : NSTextView
    {
        NSEvent* lastKeyDownEvent;
    }

    @end

    @interface wxNSTextField : NSTextField
    {
        wxNSTextFieldEditor* fieldEditor;
    }

    - (wxNSTextFieldEditor*) fieldEditor;
    - (void) setFieldEditor:(wxNSTextFieldEditor*) fieldEditor;

    @end

    @interface wxNSSecureTextField : NSSecureTextField
    {
    }

    @end


    @interface wxNSTextView : NSTextView
    {
    }
    
    @end

    @interface wxNSMenu : NSMenu
    {
       wxMenuImpl* impl;
    }

    - (void) setImplementation:(wxMenuImpl*) item;
    - (wxMenuImpl*) implementation;

    @end 

    @interface wxNSMenuItem : NSMenuItem
    {
       wxMenuItemImpl* impl;
    }

    - (void) setImplementation:(wxMenuItemImpl*) item;
    - (wxMenuItemImpl*) implementation;

    - (void)clickedAction:(id)sender;
    - (BOOL)validateMenuItem:(NSMenuItem *)menuItem;

    @end 
    
    void WXDLLIMPEXP_CORE wxOSXCocoaClassAddWXMethods(Class c);

    /*
    We need this for ShowModal, as the sheet just disables the parent window and
    returns control to the app, whereas we don't want to return from ShowModal
    until the sheet has been dismissed.
    */
    @interface ModalDialogDelegate : NSObject
    {
        BOOL sheetFinished;
        int resultCode;
    }

    - (BOOL)finished;
    - (int)code;
    - (void)waitForSheetToFinish;
    - (void)sheetDidEnd:(NSWindow *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo;
    @end


#endif // __OBJC__

// NSCursor

WX_NSCursor wxMacCocoaCreateStockCursor( int cursor_type );
WX_NSCursor  wxMacCocoaCreateCursorFromCGImage( CGImageRef cgImageRef, float hotSpotX, float hotSpotY );
void  wxMacCocoaSetCursor( WX_NSCursor cursor );
void  wxMacCocoaHideCursor();
void  wxMacCocoaShowCursor();

typedef struct tagClassicCursor
{
    wxUint16 bits[16];
    wxUint16 mask[16];
    wxInt16 hotspot[2];
}ClassicCursor;

const short kwxCursorBullseye = 0;
const short kwxCursorBlank = 1;
const short kwxCursorPencil = 2;
const short kwxCursorMagnifier = 3;
const short kwxCursorNoEntry = 4;
const short kwxCursorPaintBrush = 5;
const short kwxCursorPointRight = 6;
const short kwxCursorPointLeft = 7;
const short kwxCursorQuestionArrow = 8;
const short kwxCursorRightArrow = 9;
const short kwxCursorSizeNS = 10;
const short kwxCursorSize = 11;
const short kwxCursorSizeNESW = 12;
const short kwxCursorSizeNWSE = 13;
const short kwxCursorRoller = 14;
const short kwxCursorLast = kwxCursorRoller;

// exposing our fallback cursor map

extern ClassicCursor gMacCursors[];

#endif

#endif
    // _WX_PRIVATE_COCOA_H_

