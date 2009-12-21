/////////////////////////////////////////////////////////////////////////////
// Name:        config.h
// Purpose:     wxConfig base header
// Author:      Julian Smart
// Modified by:
// Created:
// Copyright:   (c) Julian Smart
// RCS-ID:      $Id: config.h 59405 2009-03-07 14:03:45Z VZ $
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_CONFIG_H_BASE_
#define _WX_CONFIG_H_BASE_

#include "wx/defs.h"

#if wxUSE_CONFIG

#include "wx/confbase.h"

// ----------------------------------------------------------------------------
// define the native wxConfigBase implementation
// ----------------------------------------------------------------------------

// under Windows we prefer to use the native implementation but can be forced
// to use the file-based one
#if defined(__WXMSW__) && wxUSE_CONFIG_NATIVE
    #include "wx/msw/regconf.h"
    #define wxConfig  wxRegConfig
#elif defined(__WXOS2__) && wxUSE_CONFIG_NATIVE
    #include "wx/os2/iniconf.h"
    #define wxConfig wxIniConfig
#elif defined(__WXPALMOS__) && wxUSE_CONFIG_NATIVE
    #include "wx/palmos/prefconf.h"
    #define wxConfig wxPrefConfig
#else // either we're under Unix or wish to always use config files
    #include "wx/fileconf.h"
    #define wxConfig wxFileConfig
#endif

#endif // wxUSE_CONFIG

#endif // _WX_CONFIG_H_BASE_
