/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/private/timer.h
// Purpose:     wxTimer class
// Author:      Julian Smart
// Created:     01/02/97
// RCS-ID:      $Id: timer.h 47608 2007-07-21 02:05:03Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MSW_PRIVATE_TIMER_H_
#define _WX_MSW_PRIVATE_TIMER_H_

#if wxUSE_TIMER

#include "wx/private/timer.h"

class WXDLLIMPEXP_BASE wxMSWTimerImpl : public wxTimerImpl
{
public:
    wxMSWTimerImpl(wxTimer *timer) : wxTimerImpl(timer) { m_id = 0; }

    virtual bool Start(int milliseconds = -1, bool oneShot = false);
    virtual void Stop();

    virtual bool IsRunning() const { return m_id != 0; }

protected:
    unsigned long m_id;
};

#endif // wxUSE_TIMER

#endif // _WX_TIMERH_
