/////////////////////////////////////////////////////////////////////////////
// Name:        src/palmos/dcmemory.cpp
// Purpose:     wxMemoryDC class
// Author:      William Osborne - minimal working wxPalmOS port
// Modified by:
// Created:     10/13/04
// RCS-ID:      $Id: dcmemory.cpp 50704 2007-12-15 01:25:56Z VZ $
// Copyright:   (c) William Osborne
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/dcmemory.h"
#include "wx/palmos/dcmemory.h"

#ifndef WX_PRECOMP
    #include "wx/utils.h"
    #include "wx/log.h"
#endif

// ----------------------------------------------------------------------------
// wxMemoryDCImpl
// ----------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(wxMemoryDCImpl, wxPalmDCImpl)

wxMemoryDCImpl::wxMemoryDCImpl( wxMemoryDC *owner ) 
        : wxPalmDCImpl( owner )
{
    CreateCompatible(NULL); 
    Init(); 
}

wxMemoryDCImpl::wxMemoryDCImpl( wxMemoryDC *owner, wxBitmap& bitmap ) 
        : wxPalmDCImpl( owner ) 
{ 
    CreateCompatible(NULL); 
    Init(); 
    DoSelect(bitmap);
}

wxMemoryDCImpl::wxMemoryDCImpl( wxMemoryDC *owner, wxDC *dc )
        : wxPalmDCImpl( owner ) 
{
    wxCHECK_RET( dc, _T("NULL dc in wxMemoryDC ctor") );

    CreateCompatible(dc);

    Init();
}

void wxMemoryDCImpl::Init()
{
}

bool wxMemoryDCImpl::CreateCompatible(wxDC *dc)
{
    wxDCImpl *impl = dc ? dc->GetImpl() : NULL ;
    wxPalmDCImpl *msw_impl = wxDynamicCast( impl, wxPalmDCImpl );
    if ( dc && !msw_impl)
    {
        m_ok = false;
        return false;
    }

    return false;
}

void wxMemoryDCImpl::DoSelect( const wxBitmap& bitmap )
{
}

void wxMemoryDCImpl::DoGetSize(int *width, int *height) const
{
}

wxBitmap wxMemoryDCImpl::DoGetAsBitmap(const wxRect* subrect) const
{
}
