/////////////////////////////////////////////////////////////////////////////
// Name:        wx/log.h
// Purpose:     Assorted wxLogXXX functions, and wxLog (sink for logs)
// Author:      Vadim Zeitlin
// Modified by:
// Created:     29/01/98
// RCS-ID:      $Id: log.h 59759 2009-03-23 10:42:17Z VZ $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_LOG_H_
#define _WX_LOG_H_

#include "wx/defs.h"

// ----------------------------------------------------------------------------
// types
// ----------------------------------------------------------------------------

// NB: this is needed even if wxUSE_LOG == 0
typedef unsigned long wxLogLevel;

// the trace masks have been superseded by symbolic trace constants, they're
// for compatibility only and will be removed soon - do NOT use them
#if WXWIN_COMPATIBILITY_2_8
    #define wxTraceMemAlloc 0x0001  // trace memory allocation (new/delete)
    #define wxTraceMessages 0x0002  // trace window messages/X callbacks
    #define wxTraceResAlloc 0x0004  // trace GDI resource allocation
    #define wxTraceRefCount 0x0008  // trace various ref counting operations

    #ifdef  __WXMSW__
        #define wxTraceOleCalls 0x0100  // OLE interface calls
    #endif

    typedef unsigned long wxTraceMask;
#endif // WXWIN_COMPATIBILITY_2_8

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/string.h"
#include "wx/strvararg.h"

#if wxUSE_LOG

#include "wx/arrstr.h"

#ifndef __WXPALMOS5__
#ifndef __WXWINCE__
    #include <time.h>   // for time_t
#endif
#endif // ! __WXPALMOS5__

#include "wx/dynarray.h"

// wxUSE_LOG_DEBUG enables the debug log messages
#ifndef wxUSE_LOG_DEBUG
    #if wxDEBUG_LEVEL
        #define wxUSE_LOG_DEBUG 1
    #else // !wxDEBUG_LEVEL
        #define wxUSE_LOG_DEBUG 0
    #endif
#endif

// wxUSE_LOG_TRACE enables the trace messages, they are disabled by default
#ifndef wxUSE_LOG_TRACE
    #if wxDEBUG_LEVEL
        #define wxUSE_LOG_TRACE 1
    #else // !wxDEBUG_LEVEL
        #define wxUSE_LOG_TRACE 0
    #endif
#endif // wxUSE_LOG_TRACE

// ----------------------------------------------------------------------------
// forward declarations
// ----------------------------------------------------------------------------

#if wxUSE_GUI
    class WXDLLIMPEXP_FWD_CORE wxFrame;
#endif // wxUSE_GUI

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// different standard log levels (you may also define your own)
enum wxLogLevelValues
{
    wxLOG_FatalError, // program can't continue, abort immediately
    wxLOG_Error,      // a serious error, user must be informed about it
    wxLOG_Warning,    // user is normally informed about it but may be ignored
    wxLOG_Message,    // normal message (i.e. normal output of a non GUI app)
    wxLOG_Status,     // informational: might go to the status line of GUI app
    wxLOG_Info,       // informational message (a.k.a. 'Verbose')
    wxLOG_Debug,      // never shown to the user, disabled in release mode
    wxLOG_Trace,      // trace messages are also only enabled in debug mode
    wxLOG_Progress,   // used for progress indicator (not yet)
    wxLOG_User = 100, // user defined levels start here
    wxLOG_Max = 10000
};

// symbolic trace masks - wxLogTrace("foo", "some trace message...") will be
// discarded unless the string "foo" has been added to the list of allowed
// ones with AddTraceMask()

#define wxTRACE_MemAlloc wxT("memalloc") // trace memory allocation (new/delete)
#define wxTRACE_Messages wxT("messages") // trace window messages/X callbacks
#define wxTRACE_ResAlloc wxT("resalloc") // trace GDI resource allocation
#define wxTRACE_RefCount wxT("refcount") // trace various ref counting operations

#ifdef  __WXMSW__
    #define wxTRACE_OleCalls wxT("ole")  // OLE interface calls
#endif

#include "wx/iosfwrap.h"

// ----------------------------------------------------------------------------
// derive from this class to redirect (or suppress, or ...) log messages
// normally, only a single instance of this class exists but it's not enforced
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxLog
{
public:
    // ctor
    wxLog(){}

    // these functions allow to completely disable all log messages

    // is logging disabled now?
    static bool IsEnabled() { return ms_doLog; }

    // change the flag state, return the previous one
    static bool EnableLogging(bool doIt = true)
        { bool doLogOld = ms_doLog; ms_doLog = doIt; return doLogOld; }

    // static sink function - see DoLog() for function to overload in the
    // derived classes
    static void OnLog(wxLogLevel level, const wxString& szString, time_t t);

    // message buffering

    // flush shows all messages if they're not logged immediately (FILE
    // and iostream logs don't need it, but wxGuiLog does to avoid showing
    // 17 modal dialogs one after another)
    virtual void Flush();

    // flush the active target if any
    static void FlushActive()
    {
        if ( !ms_suspendCount )
        {
            wxLog *log = GetActiveTarget();
            if ( log )
                log->Flush();
        }
    }

    // only one sink is active at each moment
    // get current log target, will call wxApp::CreateLogTarget() to
    // create one if none exists
    static wxLog *GetActiveTarget();

    // change log target, pLogger may be NULL
    static wxLog *SetActiveTarget(wxLog *pLogger);

    // suspend the message flushing of the main target until the next call
    // to Resume() - this is mainly for internal use (to prevent wxYield()
    // from flashing the messages)
    static void Suspend() { ms_suspendCount++; }

    // must be called for each Suspend()!
    static void Resume() { ms_suspendCount--; }

    // functions controlling the default wxLog behaviour
    // verbose mode is activated by standard command-line '-verbose'
    // option
    static void SetVerbose(bool bVerbose = true) { ms_bVerbose = bVerbose; }

    // Set log level.  Log messages with level > logLevel will not be logged.
    static void SetLogLevel(wxLogLevel logLevel) { ms_logLevel = logLevel; }

    // should GetActiveTarget() try to create a new log object if the
    // current is NULL?
    static void DontCreateOnDemand();

    // Make GetActiveTarget() create a new log object again.
    static void DoCreateOnDemand();

    // log the count of repeating messages instead of logging the messages
    // multiple times
    static void SetRepetitionCounting(bool bRepetCounting = true)
        { ms_bRepetCounting = bRepetCounting; }

    // gets duplicate counting status
    static bool GetRepetitionCounting() { return ms_bRepetCounting; }

    // add string trace mask
    static void AddTraceMask(const wxString& str);

    // add string trace mask
    static void RemoveTraceMask(const wxString& str);

    // remove all string trace masks
    static void ClearTraceMasks();

    // get string trace masks: note that this is MT-unsafe if other threads can
    // call AddTraceMask() concurrently
    static const wxArrayString& GetTraceMasks() { return ms_aTraceMasks; }

    // sets the time stamp string format: this is used as strftime() format
    // string for the log targets which add time stamps to the messages; set
    // it to empty string to disable time stamping completely.
    static void SetTimestamp(const wxString& ts) { ms_timestamp = ts; }

    // disable time stamping of log messages
    static void DisableTimestamp() { SetTimestamp(wxEmptyString); }


    // accessors

    // gets the verbose status
    static bool GetVerbose() { return ms_bVerbose; }

    // is this trace mask in the list?
    static bool IsAllowedTraceMask(const wxString& mask);

    // return the current loglevel limit
    static wxLogLevel GetLogLevel() { return ms_logLevel; }

    // get the current timestamp format string (may be NULL)
    static const wxString& GetTimestamp() { return ms_timestamp; }


    // helpers

    // put the time stamp into the string if ms_timestamp != NULL (don't
    // change it otherwise)
    static void TimeStamp(wxString *str);

    // this method should only be called from derived classes DoLog()
    // implementations and shouldn't be called directly, use logging functions
    // instead
    void Log(wxLogLevel level, const wxString& msg, time_t t)
    {
        DoLog(level, msg, t);
    }

    // make dtor virtual for all derived classes
    virtual ~wxLog();


    // this method exists for backwards compatibility only, don't use
    bool HasPendingMessages() const { return true; }

#if WXWIN_COMPATIBILITY_2_6
    // this function doesn't do anything any more, don't call it
    wxDEPRECATED( static wxChar *SetLogBuffer(wxChar *buf, size_t size = 0) );
#endif

    // don't use integer masks any more, use string trace masks instead
#if WXWIN_COMPATIBILITY_2_8
    wxDEPRECATED_INLINE( static void SetTraceMask(wxTraceMask ulMask),
        ms_ulTraceMask = ulMask; )
    wxDEPRECATED_BUT_USED_INTERNALLY_INLINE( static wxTraceMask GetTraceMask(),
        return ms_ulTraceMask; )
#endif // WXWIN_COMPATIBILITY_2_8

protected:
    // the logging functions that can be overridden

    // default DoLog() prepends the time stamp and a prefix corresponding
    // to the message to szString and then passes it to DoLogString()
    virtual void DoLog(wxLogLevel level, const wxString& szString, time_t t);
#if WXWIN_COMPATIBILITY_2_8
    // these shouldn't be used by new code
    wxDEPRECATED_BUT_USED_INTERNALLY(
        virtual void DoLog(wxLogLevel level, const char *szString, time_t t)
    );

    wxDEPRECATED_BUT_USED_INTERNALLY(
        virtual void DoLog(wxLogLevel level, const wchar_t *wzString, time_t t)
    );
#endif // WXWIN_COMPATIBILITY_2_8

    void LogString(const wxString& szString, time_t t)
        { DoLogString(szString, t); }

    // default DoLogString does nothing but is not pure virtual because if
    // you override DoLog() you might not need it at all
    virtual void DoLogString(const wxString& szString, time_t t);
#if WXWIN_COMPATIBILITY_2_8
    // these shouldn't be used by new code
    virtual void DoLogString(const char *WXUNUSED(szString),
                             time_t WXUNUSED(t)) {}
    virtual void DoLogString(const wchar_t *WXUNUSED(szString),
                             time_t WXUNUSED(t)) {}
#endif // WXWIN_COMPATIBILITY_2_8

    // this macro should be used in the derived classes to avoid warnings about
    // hiding the other DoLog() overloads when overriding DoLog(wxString) --
    // but don't use it with MSVC which doesn't give this warning but does give
    // warning when a deprecated function is overridden
#if WXWIN_COMPATIBILITY_2_8 && !defined(__VISUALC__)
    #define wxSUPPRESS_DOLOG_HIDE_WARNING()                                   \
        virtual void DoLog(wxLogLevel, const char *, time_t) { }              \
        virtual void DoLog(wxLogLevel, const wchar_t *, time_t) { }

    #define wxSUPPRESS_DOLOGSTRING_HIDE_WARNING()                             \
        virtual void DoLogString(const char *, time_t) { }                    \
        virtual void DoLogString(const wchar_t *, time_t) { }
#else
    #define wxSUPPRESS_DOLOG_HIDE_WARNING()
    #define wxSUPPRESS_DOLOGSTRING_HIDE_WARNING()
#endif

    // log a message indicating the number of times the previous message was
    // repeated if ms_prevCounter > 0, does nothing otherwise; return the old
    // value of ms_prevCounter
    unsigned LogLastRepeatIfNeeded();

private:
    // implement of LogLastRepeatIfNeeded(): it assumes that the
    // caller had already locked ms_prevCS
    unsigned LogLastRepeatIfNeededUnlocked();

    // static variables
    // ----------------

    // if true, don't log the same message multiple times, only log it once
    // with the number of times it was repeated
    static bool        ms_bRepetCounting;

    static wxString    ms_prevString;   // previous message that was logged
    static unsigned    ms_prevCounter;  // how many times it was repeated
    static time_t      ms_prevTimeStamp;// timestamp of the previous message
    static wxLogLevel  ms_prevLevel;    // level of the previous message

    static wxLog      *ms_pLogger;      // currently active log sink
    static bool        ms_doLog;        // false => all logging disabled
    static bool        ms_bAutoCreate;  // create new log targets on demand?
    static bool        ms_bVerbose;     // false => ignore LogInfo messages

    static wxLogLevel  ms_logLevel;     // limit logging to levels <= ms_logLevel

    static size_t      ms_suspendCount; // if positive, logs are not flushed

    // format string for strftime(), if NULL, time stamping log messages is
    // disabled
    static wxString    ms_timestamp;

#if WXWIN_COMPATIBILITY_2_8
    static wxTraceMask ms_ulTraceMask;   // controls wxLogTrace behaviour
#endif // WXWIN_COMPATIBILITY_2_8

    // currently enabled trace masks
    static wxArrayString ms_aTraceMasks;
};

// ----------------------------------------------------------------------------
// "trivial" derivations of wxLog
// ----------------------------------------------------------------------------

// log everything except for the debug/trace messages (which are passed to
// wxMessageOutputDebug) to a buffer
class WXDLLIMPEXP_BASE wxLogBuffer : public wxLog
{
public:
    wxLogBuffer() { }

    // get the string contents with all messages logged
    const wxString& GetBuffer() const { return m_str; }

    // show the buffer contents to the user in the best possible way (this uses
    // wxMessageOutputMessageBox) and clear it
    virtual void Flush();

protected:
#if wxUSE_LOG_DEBUG || wxUSE_LOG_TRACE
    virtual void DoLog(wxLogLevel level, const wxString& szString, time_t t);

    wxSUPPRESS_DOLOG_HIDE_WARNING()
#endif // wxUSE_LOG_DEBUG || wxUSE_LOG_TRACE

    virtual void DoLogString(const wxString& szString, time_t t);

    wxSUPPRESS_DOLOGSTRING_HIDE_WARNING()

private:
    wxString m_str;

    wxDECLARE_NO_COPY_CLASS(wxLogBuffer);
};


// log everything to a "FILE *", stderr by default
class WXDLLIMPEXP_BASE wxLogStderr : public wxLog
{
public:
    // redirect log output to a FILE
    wxLogStderr(FILE *fp = NULL);

protected:
    // implement sink function
    virtual void DoLogString(const wxString& szString, time_t t);

    wxSUPPRESS_DOLOGSTRING_HIDE_WARNING()

    FILE *m_fp;

    wxDECLARE_NO_COPY_CLASS(wxLogStderr);
};

#if wxUSE_STD_IOSTREAM

// log everything to an "ostream", cerr by default
class WXDLLIMPEXP_BASE wxLogStream : public wxLog
{
public:
    // redirect log output to an ostream
    wxLogStream(wxSTD ostream *ostr = (wxSTD ostream *) NULL);

protected:
    // implement sink function
    virtual void DoLogString(const wxString& szString, time_t t);

    wxSUPPRESS_DOLOGSTRING_HIDE_WARNING()

    // using ptr here to avoid including <iostream.h> from this file
    wxSTD ostream *m_ostr;
};

#endif // wxUSE_STD_IOSTREAM

// ----------------------------------------------------------------------------
// /dev/null log target: suppress logging until this object goes out of scope
// ----------------------------------------------------------------------------

// example of usage:
/*
    void Foo()
    {
        wxFile file;

        // wxFile.Open() normally complains if file can't be opened, we don't
        // want it
        wxLogNull logNo;

        if ( !file.Open("bar") )
            ... process error ourselves ...

        // ~wxLogNull called, old log sink restored
    }
 */
class WXDLLIMPEXP_BASE wxLogNull
{
public:
    wxLogNull() : m_flagOld(wxLog::EnableLogging(false)) { }
    ~wxLogNull() { (void)wxLog::EnableLogging(m_flagOld); }

private:
    bool m_flagOld; // the previous value of the wxLog::ms_doLog
};

// ----------------------------------------------------------------------------
// chaining log target: installs itself as a log target and passes all
// messages to the real log target given to it in the ctor but also forwards
// them to the previously active one
//
// note that you don't have to call SetActiveTarget() with this class, it
// does it itself in its ctor
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxLogChain : public wxLog
{
public:
    wxLogChain(wxLog *logger);
    virtual ~wxLogChain();

    // change the new log target
    void SetLog(wxLog *logger);

    // this can be used to temporarily disable (and then reenable) passing
    // messages to the old logger (by default we do pass them)
    void PassMessages(bool bDoPass) { m_bPassMessages = bDoPass; }

    // are we passing the messages to the previous log target?
    bool IsPassingMessages() const { return m_bPassMessages; }

    // return the previous log target (may be NULL)
    wxLog *GetOldLog() const { return m_logOld; }

    // override base class version to flush the old logger as well
    virtual void Flush();

    // call to avoid destroying the old log target
    void DetachOldLog() { m_logOld = NULL; }

protected:
    // pass the chain to the old logger if needed
    virtual void DoLog(wxLogLevel level, const wxString& szString, time_t t);

    wxSUPPRESS_DOLOG_HIDE_WARNING()

private:
    // the current log target
    wxLog *m_logNew;

    // the previous log target
    wxLog *m_logOld;

    // do we pass the messages to the old logger?
    bool m_bPassMessages;

    wxDECLARE_NO_COPY_CLASS(wxLogChain);
};

// a chain log target which uses itself as the new logger

#define wxLogPassThrough wxLogInterposer

class WXDLLIMPEXP_BASE wxLogInterposer : public wxLogChain
{
public:
    wxLogInterposer();

private:
    wxDECLARE_NO_COPY_CLASS(wxLogInterposer);
};

// a temporary interposer which doesn't destroy the old log target
// (calls DetachOldLog)

class WXDLLIMPEXP_BASE wxLogInterposerTemp : public wxLogChain
{
public:
    wxLogInterposerTemp();

private:
    wxDECLARE_NO_COPY_CLASS(wxLogInterposerTemp);
};

#if wxUSE_GUI
    // include GUI log targets:
    #include "wx/generic/logg.h"
#endif // wxUSE_GUI

// ============================================================================
// global functions
// ============================================================================

// ----------------------------------------------------------------------------
// Log functions should be used by application instead of stdio, iostream &c
// for log messages for easy redirection
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// get error code/error message from system in a portable way
// ----------------------------------------------------------------------------

// return the last system error code
WXDLLIMPEXP_BASE unsigned long wxSysErrorCode();

// return the error message for given (or last if 0) error code
WXDLLIMPEXP_BASE const wxChar* wxSysErrorMsg(unsigned long nErrCode = 0);

// ----------------------------------------------------------------------------
// define wxLog<level>
// ----------------------------------------------------------------------------

#define DECLARE_LOG_FUNCTION(level)                                         \
    extern void WXDLLIMPEXP_BASE                                            \
    wxDoLog##level##Wchar(const wxChar *format, ...);                       \
    extern void WXDLLIMPEXP_BASE                                            \
    wxDoLog##level##Utf8(const char *format, ...);                          \
    WX_DEFINE_VARARG_FUNC_VOID(wxLog##level,                                \
                               1, (const wxFormatString&),                  \
                               wxDoLog##level##Wchar, wxDoLog##level##Utf8) \
    DECLARE_LOG_FUNCTION_WATCOM(level)                                      \
    extern void WXDLLIMPEXP_BASE wxVLog##level(const wxString& format,      \
                                               va_list argptr)

#ifdef __WATCOMC__
    // workaround for http://bugzilla.openwatcom.org/show_bug.cgi?id=351;
    // can't use WX_WATCOM_ONLY_CODE here because the macro would expand to
    // something too big for Borland C++ to handle
    #define DECLARE_LOG_FUNCTION_WATCOM(level)                              \
        WX_VARARG_WATCOM_WORKAROUND(void, wxLog##level,                     \
                                   1, (const wxString&),                    \
                                   (wxFormatString(f1)))                    \
        WX_VARARG_WATCOM_WORKAROUND(void, wxLog##level,                     \
                                   1, (const wxCStrData&),                  \
                                   (wxFormatString(f1)))                    \
        WX_VARARG_WATCOM_WORKAROUND(void, wxLog##level,                     \
                                   1, (const char*),                        \
                                   (wxFormatString(f1)))                    \
        WX_VARARG_WATCOM_WORKAROUND(void, wxLog##level,                     \
                                   1, (const wchar_t*),                     \
                                   (wxFormatString(f1)))
#else
    #define DECLARE_LOG_FUNCTION_WATCOM(level)
#endif


#define DECLARE_LOG_FUNCTION2_EXP(level, argclass, arg, expdecl)            \
    extern void expdecl wxDoLog##level##Wchar(argclass arg,                 \
                                              const wxChar *format, ...);   \
    extern void expdecl wxDoLog##level##Utf8(argclass arg,                  \
                                             const char *format, ...);      \
    WX_DEFINE_VARARG_FUNC_VOID(wxLog##level,                                \
                               2, (argclass, const wxFormatString&),        \
                               wxDoLog##level##Wchar, wxDoLog##level##Utf8) \
    DECLARE_LOG_FUNCTION2_EXP_WATCOM(level, argclass, arg, expdecl)         \
    extern void expdecl wxVLog##level(argclass arg,                         \
                                      const wxString& format,               \
                                      va_list argptr)

#ifdef __WATCOMC__
    // workaround for http://bugzilla.openwatcom.org/show_bug.cgi?id=351;
    // can't use WX_WATCOM_ONLY_CODE here because the macro would expand to
    // something too big for Borland C++ to handle
    #define DECLARE_LOG_FUNCTION2_EXP_WATCOM(level, argclass, arg, expdecl) \
        WX_VARARG_WATCOM_WORKAROUND(void, wxLog##level,                     \
                                   2, (argclass, const wxString&),          \
                                   (f1, wxFormatString(f2)))                \
        WX_VARARG_WATCOM_WORKAROUND(void, wxLog##level,                     \
                                   2, (argclass, const wxCStrData&),        \
                                   (f1, wxFormatString(f2)))                \
        WX_VARARG_WATCOM_WORKAROUND(void, wxLog##level,                     \
                                   2, (argclass, const char*),              \
                                   (f1, wxFormatString(f2)))                \
        WX_VARARG_WATCOM_WORKAROUND(void, wxLog##level,                     \
                                   2, (argclass, const wchar_t*),           \
                                   (f1, wxFormatString(f2)))
#else
    #define DECLARE_LOG_FUNCTION2_EXP_WATCOM(level, argclass, arg, expdecl)
#endif


#else // !wxUSE_LOG

#undef wxUSE_LOG_DEBUG
#define wxUSE_LOG_DEBUG 0

#undef wxUSE_LOG_TRACE
#define wxUSE_LOG_TRACE 0

#ifdef __WATCOMC__
    // workaround for http://bugzilla.openwatcom.org/show_bug.cgi?id=351
    #define WX_WATCOM_ONLY_CODE( x )  x
#else
    #define WX_WATCOM_ONLY_CODE( x )
#endif

#if defined(__WATCOMC__) || defined(__MINGW32__)
    // Mingw has similar problem with wxLogSysError:
    #define WX_WATCOM_OR_MINGW_ONLY_CODE( x )  x
#else
    #define WX_WATCOM_OR_MINGW_ONLY_CODE( x )
#endif

// log functions do nothing at all
#define DECLARE_LOG_FUNCTION(level)                                         \
    WX_DEFINE_VARARG_FUNC_NOP(wxLog##level, 1, (const wxString&))           \
    WX_WATCOM_ONLY_CODE(                                                    \
        WX_DEFINE_VARARG_FUNC_NOP(wxLog##level, 1, (const char*))           \
        WX_DEFINE_VARARG_FUNC_NOP(wxLog##level, 1, (const wchar_t*))        \
        WX_DEFINE_VARARG_FUNC_NOP(wxLog##level, 1, (const wxCStrData&))     \
    )                                                                       \
    inline void wxVLog##level(const wxString& WXUNUSED(format),             \
                              va_list WXUNUSED(argptr)) { }                 \

#define DECLARE_LOG_FUNCTION2_EXP(level, argclass, arg, expdecl)            \
    WX_DEFINE_VARARG_FUNC_NOP(wxLog##level, 2, (argclass, const wxString&)) \
    WX_WATCOM_OR_MINGW_ONLY_CODE(                                           \
        WX_DEFINE_VARARG_FUNC_NOP(wxLog##level, 2, (argclass, const char*)) \
        WX_DEFINE_VARARG_FUNC_NOP(wxLog##level, 2, (argclass, const wchar_t*)) \
        WX_DEFINE_VARARG_FUNC_NOP(wxLog##level, 2, (argclass, const wxCStrData&)) \
    )                                                                       \
    inline void wxVLog##level(argclass WXUNUSED(arg),                       \
                              const wxString& WXUNUSED(format),             \
                              va_list WXUNUSED(argptr)) {}

// Empty Class to fake wxLogNull
class WXDLLIMPEXP_BASE wxLogNull
{
public:
    wxLogNull() { }
};

// Dummy macros to replace some functions.
#define wxSysErrorCode() (unsigned long)0
#define wxSysErrorMsg( X ) (const wxChar*)NULL

// Fake symbolic trace masks... for those that are used frequently
#define wxTRACE_OleCalls wxEmptyString // OLE interface calls

#endif // wxUSE_LOG/!wxUSE_LOG

#define DECLARE_LOG_FUNCTION2(level, argclass, arg)                         \
    DECLARE_LOG_FUNCTION2_EXP(level, argclass, arg, WXDLLIMPEXP_BASE)

// VC6 produces a warning if we a macro expanding to nothing to
// DECLARE_LOG_FUNCTION2:
#if defined(__VISUALC__) && __VISUALC__ < 1300
    // "not enough actual parameters for macro 'DECLARE_LOG_FUNCTION2_EXP'"
    #pragma warning(disable:4003)
#endif

// a generic function for all levels (level is passes as parameter)
DECLARE_LOG_FUNCTION2(Generic, wxLogLevel, level);

// one function per each level
DECLARE_LOG_FUNCTION(FatalError);
DECLARE_LOG_FUNCTION(Error);
DECLARE_LOG_FUNCTION(Warning);
DECLARE_LOG_FUNCTION(Message);
DECLARE_LOG_FUNCTION(Info);
DECLARE_LOG_FUNCTION(Verbose);

// this function sends the log message to the status line of the top level
// application frame, if any
DECLARE_LOG_FUNCTION(Status);

#if wxUSE_GUI
    // this one is the same as previous except that it allows to explicitly
    class WXDLLIMPEXP_FWD_CORE wxFrame;
    // specify the frame to which the output should go
    DECLARE_LOG_FUNCTION2_EXP(Status, wxFrame *, pFrame, WXDLLIMPEXP_CORE);
#endif // wxUSE_GUI

// additional one: as wxLogError, but also logs last system call error code
// and the corresponding error message if available
DECLARE_LOG_FUNCTION(SysError);

// and another one which also takes the error code (for those broken APIs
// that don't set the errno (like registry APIs in Win32))
DECLARE_LOG_FUNCTION2(SysError, long, lErrCode);
#ifdef __WATCOMC__
// workaround for http://bugzilla.openwatcom.org/show_bug.cgi?id=351
DECLARE_LOG_FUNCTION2(SysError, unsigned long, lErrCode);
#endif


// debug functions can be completely disabled in optimized builds

// if these log functions are disabled, we prefer to define them as (empty)
// variadic macros as this completely removes them and their argument
// evaluation from the object code but if this is not supported by compiler we
// use empty inline functions instead (defining them as nothing would result in
// compiler warnings)
//
// note that making wxVLogDebug/Trace() themselves (empty inline) functions is
// a bad idea as some compilers are stupid enough to not inline even empty
// functions if their parameters are complicated enough, but by defining them
// as an empty inline function we ensure that even dumbest compilers optimise
// them away
#ifdef __BORLANDC__
    // but Borland gives "W8019: Code has no effect" for wxLogNop() so we need
    // to define it differently for it to avoid these warnings (same problem as
    // with wxUnusedVar())
    #define wxLogNop() { }
#else
    inline void wxLogNop() { }
#endif

#if wxUSE_LOG_DEBUG
    DECLARE_LOG_FUNCTION(Debug);
#else // !wxUSE_LOG_DEBUG
    #define wxVLogDebug(fmt, valist) wxLogNop()

    #ifdef HAVE_VARIADIC_MACROS
        #define wxLogDebug(fmt, ...) wxLogNop()
    #else // !HAVE_VARIADIC_MACROS
        WX_DEFINE_VARARG_FUNC_NOP(wxLogDebug, 1, (const wxString&))
    #endif
#endif // wxUSE_LOG_DEBUG/!wxUSE_LOG_DEBUG

#if wxUSE_LOG_TRACE
    // this version only logs the message if the mask had been added to the
    // list of masks with AddTraceMask()
    DECLARE_LOG_FUNCTION2(Trace, const wxString&, mask);
#ifdef __WATCOMC__
    // workaround for http://bugzilla.openwatcom.org/show_bug.cgi?id=351
    DECLARE_LOG_FUNCTION2(Trace, const char*, mask);
    DECLARE_LOG_FUNCTION2(Trace, const wchar_t*, mask);
#endif

    // and this one does nothing if all of level bits are not set in
    // wxLog::GetActive()->GetTraceMask() -- it's deprecated in favour of
    // string identifiers
#if WXWIN_COMPATIBILITY_2_8
    DECLARE_LOG_FUNCTION2(Trace, wxTraceMask, mask);
#ifdef __WATCOMC__
    // workaround for http://bugzilla.openwatcom.org/show_bug.cgi?id=351
    DECLARE_LOG_FUNCTION2(Trace, int, mask);
#endif
#endif // WXWIN_COMPATIBILITY_2_8

#else  // !wxUSE_LOG_TRACE
    #define wxVLogTrace(mask, fmt, valist) wxLogNop()

    #ifdef HAVE_VARIADIC_MACROS
        #define wxLogTrace(mask, fmt, ...) wxLogNop()
    #else // !HAVE_VARIADIC_MACROS
        #if WXWIN_COMPATIBILITY_2_8
        WX_DEFINE_VARARG_FUNC_NOP(wxLogTrace, 2, (wxTraceMask, const wxString&))
        #endif
        WX_DEFINE_VARARG_FUNC_NOP(wxLogTrace, 2, (const wxString&, const wxString&))
        #ifdef __WATCOMC__
        // workaround for http://bugzilla.openwatcom.org/show_bug.cgi?id=351
        WX_DEFINE_VARARG_FUNC_NOP(wxLogTrace, 2, (const char*, const char*))
        WX_DEFINE_VARARG_FUNC_NOP(wxLogTrace, 2, (const wchar_t*, const wchar_t*))
        #endif
    #endif // HAVE_VARIADIC_MACROS/!HAVE_VARIADIC_MACROS
#endif // wxUSE_LOG_TRACE/!wxUSE_LOG_TRACE

#if defined(__VISUALC__) && __VISUALC__ < 1300
    #pragma warning(default:4003)
#endif

// wxLogFatalError helper: show the (fatal) error to the user in a safe way,
// i.e. without using wxMessageBox() for example because it could crash
void WXDLLIMPEXP_BASE
wxSafeShowMessage(const wxString& title, const wxString& text);

// ----------------------------------------------------------------------------
// debug only logging functions: use them with API name and error code
// ----------------------------------------------------------------------------

#if wxUSE_LOG_DEBUG
    // make life easier for people using VC++ IDE: clicking on the message
    // will take us immediately to the place of the failed API
#ifdef __VISUALC__
    #define wxLogApiError(api, rc)                                            \
        wxLogDebug(wxT("%s(%d): '%s' failed with error 0x%08lx (%s)."),       \
                   __FILE__, __LINE__, api,                                   \
                   (long)rc, wxSysErrorMsg(rc))
#else // !VC++
    #define wxLogApiError(api, rc)                                            \
        wxLogDebug(wxT("In file %s at line %d: '%s' failed with ")            \
                   wxT("error 0x%08lx (%s)."),                                \
                   __FILE__, __LINE__, api,                                   \
                   (long)rc, wxSysErrorMsg(rc))
#endif // VC++/!VC++

    #define wxLogLastError(api) wxLogApiError(api, wxSysErrorCode())

#else // !wxUSE_LOG_DEBUG
    #define wxLogApiError(api, err) wxLogNop()
    #define wxLogLastError(api) wxLogNop()
#endif // wxUSE_LOG_DEBUG/!wxUSE_LOG_DEBUG

// wxCocoa has additiional trace masks
#if defined(__WXCOCOA__)
#include "wx/cocoa/log.h"
#endif

#ifdef WX_WATCOM_ONLY_CODE
    #undef WX_WATCOM_ONLY_CODE
#endif

#endif  // _WX_LOG_H_

