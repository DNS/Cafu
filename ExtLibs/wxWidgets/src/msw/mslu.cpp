/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/mslu.cpp
// Purpose:     Fixes for bugs in MSLU
// Author:      Vaclav Slavik
// Modified by:
// Created:     2002/02/17
// RCS-ID:      $Id: mslu.cpp 46390 2007-06-10 17:14:14Z VS $
// Copyright:   (c) 2002 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
    #include <dir.h>
#endif

#ifndef WX_PRECOMP
    #include "wx/utils.h"
#endif

//------------------------------------------------------------------------
// Check for use of MSLU
//------------------------------------------------------------------------

#if wxUSE_BASE

bool WXDLLIMPEXP_BASE wxUsingUnicowsDll()
{
#if wxUSE_UNICODE_MSLU
    return (wxGetOsVersion() == wxOS_WINDOWS_9X);
#else
    return false;
#endif
}

#endif // wxUSE_BASE


#if wxUSE_UNICODE_MSLU

//------------------------------------------------------------------------
//
// NB: MSLU only covers Win32 API, it doesn't provide Unicode implementation of
//     libc functions. Unfortunately, some of MSVCRT wchar_t functions
//     (e.g. _wopen) don't work on Windows 9x, so we have to workaround it
//     by calling the char version. We still want to use wchar_t version on
//     NT/2000/XP, though, because they allow for Unicode file names.
//
//     Moreover, there are bugs in unicows.dll, of course. We have to
//     workaround them, too.
//
//------------------------------------------------------------------------

#include "wx/msw/private.h"
#include "wx/msw/mslu.h"

#include <stdio.h>
#include <io.h>
#include <sys/stat.h>

#ifdef __VISUALC__
    #include <direct.h>
#endif

// Undef redirection macros defined in wx/msw/wrapwin.h:
#undef DrawStateW
#undef GetOpenFileNameW
#undef GetSaveFileNameW

//------------------------------------------------------------------------
// Wrongly implemented functions from unicows.dll
//------------------------------------------------------------------------

#if wxUSE_GUI

WXDLLEXPORT int  wxMSLU_DrawStateW(WXHDC dc, WXHBRUSH br, WXFARPROC outputFunc,
                                   WXLPARAM lData, WXWPARAM wData,
                                   int x, int y, int cx, int cy,
                                   unsigned int flags)
{
    // VS: There's yet another bug in MSLU: DrawStateW behaves like if it was
    //     expecting char*, not wchar_t* input. We have to use DrawStateA
    //     explicitly.

    if ( wxUsingUnicowsDll() )
    {
        return DrawStateA((HDC)dc, (HBRUSH)br, (DRAWSTATEPROC)outputFunc,
                          (LPARAM)(const char*)
                                wxConvLocal.cWX2MB((const wxChar*)lData),
                          wData, x, y, cx, cy, flags);
    }
    else
    {
        return DrawStateW((HDC)dc, (HBRUSH)br, (DRAWSTATEPROC)outputFunc,
                          lData, wData, x, y, cx, cy, flags);
    }
}

static void wxFixOPENFILENAME(LPOPENFILENAME ofn)
{
#ifdef OFN_EXPLORER
    // VS: there's a bug in unicows.dll - when multiple files are selected,
    //     of.nFileOffset doesn't point to the first filename but rather to
    //     the last component of directory name. This bug is known to MSLU
    //     developers, but they are not going to fix it: "this is a true
    //     limitation, that we have decided to live with" and "working
    //     harder on this case just did not seem worth the effort"...
    //
    //     Our only option is to try to fix it ourselves:

    if ( (ofn->Flags & OFN_ALLOWMULTISELECT) &&
         ofn->lpstrFile[ofn->nFileOffset-1] != wxT('\0') )
    {
        if ( wxDirExists(ofn->lpstrFile) )
        {
            // 1st component is dir => multiple files selected
            ofn->nFileOffset = wxStrlen(ofn->lpstrFile)+1;
        }
    }
#endif // OFN_EXPLORER
}

WXDLLEXPORT int wxMSLU_GetOpenFileNameW(void *ofn)
{
    int ret = GetOpenFileName((LPOPENFILENAME)ofn);
    if ( wxUsingUnicowsDll() && ret != 0 )
        wxFixOPENFILENAME((LPOPENFILENAME)ofn);
    return ret;
}

WXDLLEXPORT int wxMSLU_GetSaveFileNameW(void *ofn)
{
    int ret = GetSaveFileName((LPOPENFILENAME)ofn);
    if ( wxUsingUnicowsDll() && ret != 0 )
        wxFixOPENFILENAME((LPOPENFILENAME)ofn);
    return ret;
}

#endif // wxUSE_GUI

//------------------------------------------------------------------------
// Missing libc file manipulation functions in Win9x
//------------------------------------------------------------------------

#if wxUSE_BASE

WXDLLIMPEXP_BASE int wxMSLU__wrename(const wchar_t *oldname,
                                     const wchar_t *newname)
{
    if ( wxUsingUnicowsDll() )
        return rename(wxConvFile.cWX2MB(oldname), wxConvFile.cWX2MB(newname));
    else
        return _wrename(oldname, newname);
}

WXDLLIMPEXP_BASE int wxMSLU__wremove(const wchar_t *name)
{
    if ( wxUsingUnicowsDll() )
        return remove(wxConvFile.cWX2MB(name));
    else
        return _wremove(name);
}

WXDLLIMPEXP_BASE FILE* wxMSLU__wfopen(const wchar_t *name,const wchar_t* mode)
{
    if ( wxUsingUnicowsDll() )
        return fopen(wxConvFile.cWX2MB(name),wxConvFile.cWX2MB(mode));
    else
        return _wfopen(name,mode);
}

WXDLLIMPEXP_BASE FILE* wxMSLU__wfreopen(const wchar_t *name,
                                        const wchar_t* mode,
                                        FILE *stream)
{
    if ( wxUsingUnicowsDll() )
        return freopen(wxConvFile.cWX2MB(name), wxConvFile.cWX2MB(mode), stream);
    else
        return _wfreopen(name, mode, stream);
}

#if defined( __VISUALC__ ) \
    || ( defined(__MINGW32__) && wxCHECK_W32API_VERSION( 0, 5 ) ) \
    || ( defined(__MWERKS__) && defined(__WXMSW__) ) \
    || ( defined(__BORLANDC__) && (__BORLANDC__ > 0x460) )

WXDLLIMPEXP_BASE int wxMSLU__wopen(const wchar_t *name, int flags, int mode)
{
    if ( wxUsingUnicowsDll() )
#ifdef __BORLANDC__
        return open(wxConvFile.cWX2MB(name), flags, mode);
#else
        return _open(wxConvFile.cWX2MB(name), flags, mode);
#endif
    else
        return _wopen(name, flags, mode);
}

WXDLLIMPEXP_BASE int wxMSLU__waccess(const wchar_t *name, int mode)
{
    if ( wxUsingUnicowsDll() )
        return _access(wxConvFile.cWX2MB(name), mode);
    else
        return _waccess(name, mode);
}

WXDLLIMPEXP_BASE int wxMSLU__wmkdir(const wchar_t *name)
{
    if ( wxUsingUnicowsDll() )
        return _mkdir(wxConvFile.cWX2MB(name));
    else
        return _wmkdir(name);
}

WXDLLIMPEXP_BASE int wxMSLU__wrmdir(const wchar_t *name)
{
    if ( wxUsingUnicowsDll() )
        return _rmdir(wxConvFile.cWX2MB(name));
    else
        return _wrmdir(name);
}

WXDLLIMPEXP_BASE int wxMSLU__wstat(const wchar_t *name, struct _stat *buffer)
{
    if ( wxUsingUnicowsDll() )
        return _stat((const char*)wxConvFile.cWX2MB(name), buffer);
    else
        return _wstat(name, buffer);
}

#ifdef __BORLANDC__
//here _stati64 is defined as stati64, see wx/filefn.h
#undef _stati64
WXDLLIMPEXP_BASE int wxMSLU__wstati64(const wchar_t *name, struct _stati64 *buffer)
 {
     if ( wxUsingUnicowsDll() )
        return _stati64((const char*)wxConvFile.cWX2MB(name), (stati64 *) buffer);
    else
        return _wstati64(name, (stati64 *) buffer);
}
#else
WXDLLIMPEXP_BASE int wxMSLU__wstati64(const wchar_t *name, struct _stati64 *buffer)
{
    if ( wxUsingUnicowsDll() )
        return _stati64((const char*)wxConvFile.cWX2MB(name), buffer);
    else
        return _wstati64(name, buffer);
}
#endif //__BORLANDC__

#endif // compilers having wopen() &c

#endif // wxUSE_BASE

#endif // wxUSE_UNICODE_MSLU
