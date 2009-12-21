///////////////////////////////////////////////////////////////////////////////
// Name:        wx/mac/carbon/evtloop.h
// Purpose:     declaration of wxEventLoop for wxMac
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2006-01-12
// RCS-ID:      $Id: evtloop.h 58911 2009-02-15 14:25:08Z FM $
// Copyright:   (c) 2006 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MAC_CARBON_EVTLOOP_H_
#define _WX_MAC_CARBON_EVTLOOP_H_

class OpaqueEventRef;
typedef OpaqueEventRef *EventRef;

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
    // dispatch an event and release it
    void DispatchAndReleaseEvent(EventRef event);

    double      m_sleepTime;
};

#endif // _WX_MAC_CARBON_EVTLOOP_H_

