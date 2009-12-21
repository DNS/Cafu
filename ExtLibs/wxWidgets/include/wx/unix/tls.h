///////////////////////////////////////////////////////////////////////////////
// Name:        wx/unix/tls.h
// Purpose:     Pthreads implementation of wxTlsValue<>
// Author:      Vadim Zeitlin
// Created:     2008-08-08
// RCS-ID:      $Id: tls.h 58757 2009-02-08 11:45:59Z VZ $
// Copyright:   (c) 2008 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_UNIX_TLS_H_
#define _WX_UNIX_TLS_H_

#include <pthread.h>

// ----------------------------------------------------------------------------
// wxTlsKey is a helper class encapsulating the TLS value index
// ----------------------------------------------------------------------------

class wxTlsKey
{
public:
    // ctor allocates a new key and possibly registering a destructor function
    // for it (notice that using destructor function is Pthreads-specific and
    // not supported in Win32 implementation)
    wxTlsKey(void (*destructor)(void *) = NULL)
    {
        if ( pthread_key_create(&m_key, destructor) != 0 )
            m_key = 0;
    }

    // return true if the key was successfully allocated
    bool IsOk() const { return m_key != 0; }

    // get the key value, there is no error return
    void *Get() const
    {
        return pthread_getspecific(m_key);
    }

    // change the key value, return true if ok
    bool Set(void *value)
    {
        return pthread_setspecific(m_key, value) == 0;
    }

    // free the key
    ~wxTlsKey()
    {
        if ( IsOk() )
            pthread_key_delete(m_key);
    }

private:
    pthread_key_t m_key;

    wxDECLARE_NO_COPY_CLASS(wxTlsKey);
};

#endif // _WX_UNIX_TLS_H_

