/////////////////////////////////////////////////////////////////////////////
// Name:        src/unix/evtloopunix.cpp
// Purpose:     wxEventLoop implementation
// Author:      Lukasz Michalski (lm@zork.pl)
// Created:     2007-05-07
// RCS-ID:      $Id: evtloopunix.cpp 58951 2009-02-16 17:19:17Z PC $
// Copyright:   (c) 2006 Zork Lukasz Michalski
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ===========================================================================
// declarations
// ===========================================================================

// ---------------------------------------------------------------------------
// headers
// ---------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#if wxUSE_CONSOLE_EVENTLOOP

#include "wx/evtloop.h"

#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/log.h"
#endif

#include <errno.h>
#include "wx/apptrait.h"
#include "wx/thread.h"
#include "wx/module.h"
#include "wx/unix/pipe.h"
#include "wx/unix/private/timer.h"
#include "wx/unix/private/epolldispatcher.h"
#include "wx/private/selectdispatcher.h"

#define TRACE_EVENTS _T("events")

// ===========================================================================
// wxEventLoop::PipeIOHandler implementation
// ===========================================================================

namespace wxPrivate
{

// pipe used for wake up messages: when a child thread wants to wake up
// the event loop in the main thread it writes to this pipe
class PipeIOHandler : public wxFDIOHandler
{
public:
    // default ctor does nothing, call Create() to really initialize the
    // object
    PipeIOHandler() { }

    bool Create();

    // this method can be, and normally is, called from another thread
    void WakeUp();

    int GetReadFd() { return m_pipe[wxPipe::Read]; }

    // implement wxFDIOHandler pure virtual methods
    virtual void OnReadWaiting();
    virtual void OnWriteWaiting() { }
    virtual void OnExceptionWaiting() { }

private:
    wxPipe m_pipe;
};

// ----------------------------------------------------------------------------
// initialization
// ----------------------------------------------------------------------------

bool PipeIOHandler::Create()
{
    if ( !m_pipe.Create() )
    {
        wxLogError(_("Failed to create wake up pipe used by event loop."));
        return false;
    }

    const int fdRead = GetReadFd();

    int flags = fcntl(fdRead, F_GETFL, 0);
    if ( flags == -1 || fcntl(fdRead, F_SETFL, flags | O_NONBLOCK) == -1 )
    {
        wxLogSysError(_("Failed to switch wake up pipe to non-blocking mode"));
        return false;
    }

    wxLogTrace(TRACE_EVENTS, wxT("Wake up pipe (%d, %d) created"),
               fdRead, m_pipe[wxPipe::Write]);

    return true;
}

// ----------------------------------------------------------------------------
// wakeup handling
// ----------------------------------------------------------------------------

void PipeIOHandler::WakeUp()
{
    if ( write(m_pipe[wxPipe::Write], "s", 1) != 1 )
    {
        // don't use wxLog here, we can be in another thread and this could
        // result in dead locks
        perror("write(wake up pipe)");
    }
}

void PipeIOHandler::OnReadWaiting()
{
    // got wakeup from child thread: read all data available in pipe just to
    // make it empty (even though we write one byte at a time from WakeUp(),
    // it could have been called several times)
    char buf[4];
    for ( ;; )
    {
        const int size = read(GetReadFd(), buf, WXSIZEOF(buf));

        if ( size == 0 || (size == -1 && (errno == EAGAIN || errno == EINTR)) )
        {
            // nothing left in the pipe (EAGAIN is expected for an FD with
            // O_NONBLOCK)
            break;
        }

        if ( size == -1 )
        {
            wxLogSysError(_("Failed to read from wake-up pipe"));

            break;
        }
    }

    // writing to the wake up pipe will make wxConsoleEventLoop return from
    // wxFDIODispatcher::Dispatch() it might be currently blocking in, nothing
    // else needs to be done
}

} // namespace wxPrivate

// ===========================================================================
// wxEventLoop implementation
// ===========================================================================

//-----------------------------------------------------------------------------
// initialization
//-----------------------------------------------------------------------------

wxConsoleEventLoop::wxConsoleEventLoop()
{
    m_wakeupPipe = new wxPrivate::PipeIOHandler();
    if ( !m_wakeupPipe->Create() )
    {
        wxDELETE(m_wakeupPipe);
        m_dispatcher = NULL;
        return;
    }

    m_dispatcher = wxFDIODispatcher::Get();
    if ( !m_dispatcher )
        return;

    m_dispatcher->RegisterFD
                  (
                    m_wakeupPipe->GetReadFd(),
                    m_wakeupPipe,
                    wxFDIO_INPUT
                  );
}

wxConsoleEventLoop::~wxConsoleEventLoop()
{
    delete m_wakeupPipe;
}

//-----------------------------------------------------------------------------
// events dispatch and loop handling
//-----------------------------------------------------------------------------

bool wxConsoleEventLoop::Pending() const
{
    if ( m_dispatcher->HasPending() )
        return true;

#if wxUSE_TIMER
    wxUsecClock_t nextTimer;
    if ( wxTimerScheduler::Get().GetNext(&nextTimer) &&
            !wxMilliClockToLong(nextTimer) )
        return true;
#endif // wxUSE_TIMER

    return false;
}

bool wxConsoleEventLoop::Dispatch()
{
    DispatchTimeout(wxFDIODispatcher::TIMEOUT_INFINITE);

    return true;
}

int wxConsoleEventLoop::DispatchTimeout(unsigned long timeout)
{
#if wxUSE_TIMER
    // check if we need to decrease the timeout to account for a timer
    wxUsecClock_t nextTimer;
    if ( wxTimerScheduler::Get().GetNext(&nextTimer) )
    {
        unsigned long timeUntilNextTimer = wxMilliClockToLong(nextTimer / 1000);
        if ( timeUntilNextTimer < timeout )
            timeout = timeUntilNextTimer;
    }
#endif // wxUSE_TIMER

    bool hadEvent = m_dispatcher->Dispatch(timeout) > 0;

#if wxUSE_TIMER
    if ( wxTimerScheduler::Get().NotifyExpired() )
        hadEvent = true;
#endif // wxUSE_TIMER

    return hadEvent ? 1 : -1;
}

void wxConsoleEventLoop::WakeUp()
{
    m_wakeupPipe->WakeUp();
}

void wxConsoleEventLoop::OnNextIteration()
{
    // call the signal handlers for any signals we caught recently
    wxTheApp->CheckSignal();
}


wxEventLoopBase *wxConsoleAppTraits::CreateEventLoop()
{
    return new wxEventLoop();
}

#endif // wxUSE_CONSOLE_EVENTLOOP
