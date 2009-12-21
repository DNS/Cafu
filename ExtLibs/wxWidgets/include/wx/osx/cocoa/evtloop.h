///////////////////////////////////////////////////////////////////////////////
// Name:        wx/osx/cocoa/evtloop.h
// Purpose:     declaration of wxGUIEventLoop for wxOSX/Cocoa
// Author:      Vadim Zeitlin
// Created:     2008-12-28
// RCS-ID:      $Id: evtloop.h 58911 2009-02-15 14:25:08Z FM $
// Copyright:   (c) 2006 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_OSX_COCOA_EVTLOOP_H_
#define _WX_OSX_COCOA_EVTLOOP_H_

class WXDLLIMPEXP_CORE wxGUIEventLoop : public wxEventLoopManual
{
public:
    wxGUIEventLoop();

    // implement/override base class pure virtual
    virtual bool Pending() const;
    virtual bool Dispatch();
    virtual int DispatchTimeout(unsigned long timeout);

    virtual void WakeUp();
    virtual bool YieldFor(long eventsToProcess);

private:
    double m_sleepTime;
};

#endif // _WX_OSX_COCOA_EVTLOOP_H_

