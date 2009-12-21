/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/ctrlcmn.cpp
// Purpose:     wxControl common interface
// Author:      Vadim Zeitlin
// Modified by:
// Created:     26.07.99
// RCS-ID:      $Id: ctrlcmn.cpp 58811 2009-02-09 13:16:42Z VZ $
// Copyright:   (c) wxWidgets team
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

#if wxUSE_CONTROLS

#include "wx/control.h"

#ifndef WX_PRECOMP
    #include "wx/dc.h"
    #include "wx/log.h"
    #include "wx/radiobut.h"
    #include "wx/statbmp.h"
    #include "wx/bitmap.h"
    #include "wx/utils.h"       // for wxStripMenuCodes()
#endif

const char wxControlNameStr[] = "control";

// ============================================================================
// implementation
// ============================================================================

wxControlBase::~wxControlBase()
{
    // this destructor is required for Darwin
}

bool wxControlBase::Create(wxWindow *parent,
                           wxWindowID id,
                           const wxPoint &pos,
                           const wxSize &size,
                           long style,
                           const wxValidator& wxVALIDATOR_PARAM(validator),
                           const wxString &name)
{
    bool ret = wxWindow::Create(parent, id, pos, size, style, name);

#if wxUSE_VALIDATORS
    if ( ret )
        SetValidator(validator);
#endif // wxUSE_VALIDATORS

    return ret;
}

bool wxControlBase::CreateControl(wxWindowBase *parent,
                                  wxWindowID id,
                                  const wxPoint& pos,
                                  const wxSize& size,
                                  long style,
                                  const wxValidator& validator,
                                  const wxString& name)
{
    // even if it's possible to create controls without parents in some port,
    // it should surely be discouraged because it doesn't work at all under
    // Windows
    wxCHECK_MSG( parent, false, wxT("all controls must have parents") );

    if ( !CreateBase(parent, id, pos, size, style, validator, name) )
        return false;

    parent->AddChild(this);

    return true;
}

/* static */
wxString wxControlBase::GetLabelText(const wxString& label)
{
    // we don't want strip the TABs here, just the mnemonics
    return wxStripMenuCodes(label, wxStrip_Mnemonics);
}

void wxControlBase::Command(wxCommandEvent& event)
{
    (void)GetEventHandler()->ProcessEvent(event);
}

void wxControlBase::InitCommandEvent(wxCommandEvent& event) const
{
    event.SetEventObject((wxControlBase *)this);    // const_cast

    // event.SetId(GetId()); -- this is usuall done in the event ctor

    switch ( m_clientDataType )
    {
        case wxClientData_Void:
            event.SetClientData(GetClientData());
            break;

        case wxClientData_Object:
            event.SetClientObject(GetClientObject());
            break;

        case wxClientData_None:
            // nothing to do
            ;
    }
}

bool wxControlBase::SetFont(const wxFont& font)
{
    InvalidateBestSize();
    return wxWindow::SetFont(font);
}

// wxControl-specific processing after processing the update event
void wxControlBase::DoUpdateWindowUI(wxUpdateUIEvent& event)
{
    // call inherited
    wxWindowBase::DoUpdateWindowUI(event);

    // update label
    if ( event.GetSetText() )
    {
        if ( event.GetText() != GetLabel() )
            SetLabel(event.GetText());
    }

    // Unfortunately we don't yet have common base class for
    // wxRadioButton, so we handle updates of radiobuttons here.
    // TODO: If once wxRadioButtonBase will exist, move this code there.
#if wxUSE_RADIOBTN
    if ( event.GetSetChecked() )
    {
        wxRadioButton *radiobtn = wxDynamicCastThis(wxRadioButton);
        if ( radiobtn )
            radiobtn->SetValue(event.GetChecked());
    }
#endif // wxUSE_RADIOBTN
}

/* static */
wxString wxControlBase::RemoveMnemonics(const wxString& str)
{
    return wxStripMenuCodes(str, wxStrip_Mnemonics);
}

/* static */
wxString wxControlBase::EscapeMnemonics(const wxString& text)
{
    wxString label(text);
    label.Replace("&", "&&");
    return label;
}

/* static */
int wxControlBase::FindAccelIndex(const wxString& label, wxString *labelOnly)
{
    // the character following MNEMONIC_PREFIX is the accelerator for this
    // control unless it is MNEMONIC_PREFIX too - this allows to insert
    // literal MNEMONIC_PREFIX chars into the label
    static const wxChar MNEMONIC_PREFIX = _T('&');

    if ( labelOnly )
    {
        labelOnly->Empty();
        labelOnly->Alloc(label.length());
    }

    int indexAccel = -1;
    for ( wxString::const_iterator pc = label.begin(); pc != label.end(); ++pc )
    {
        if ( *pc == MNEMONIC_PREFIX )
        {
            ++pc; // skip it
            if ( pc == label.end() )
                break;
            else if ( *pc != MNEMONIC_PREFIX )
            {
                if ( indexAccel == -1 )
                {
                    // remember it (-1 is for MNEMONIC_PREFIX itself
                    indexAccel = pc - label.begin() - 1;
                }
                else
                {
                    wxFAIL_MSG(_T("duplicate accel char in control label"));
                }
            }
        }

        if ( labelOnly )
        {
            *labelOnly += *pc;
        }
    }

    return indexAccel;
}

wxBorder wxControlBase::GetDefaultBorder() const
{
    return wxBORDER_THEME;
}

// ----------------------------------------------------------------------------
// wxControlBase - ellipsization code
// ----------------------------------------------------------------------------

#define wxELLIPSE_REPLACEMENT       wxT("...")

/* static and protected */
wxString wxControlBase::DoEllipsizeSingleLine(const wxString& curLine, const wxDC& dc,
                                              wxEllipsizeMode mode, int maxFinalWidth,
                                              int replacementWidth, int marginWidth)
{
    wxASSERT_MSG(replacementWidth > 0 && marginWidth > 0,
                 "Invalid parameters");
    wxASSERT_MSG(!curLine.Contains('\n'),
                 "Use Ellipsize() instead!");

    // NOTE: this function assumes that any mnemonic/tab character has already
    //       been handled if it was necessary to handle them (see Ellipsize())

    if (maxFinalWidth <= 0)
        return wxEmptyString;

    wxArrayInt charOffsets;
    size_t len = curLine.length();
    if (len == 0 ||
        !dc.GetPartialTextExtents(curLine, charOffsets))
        return curLine;

    wxASSERT(charOffsets.GetCount() == len);

    size_t totalWidth = charOffsets.Last();
    if ( totalWidth <= (size_t)maxFinalWidth )
        return curLine;     // we don't need to do any ellipsization!

    int excessPixels = totalWidth - maxFinalWidth +
                       replacementWidth +
                       marginWidth;     // security margin (NEEDED!)
    wxASSERT(excessPixels>0);

    // remove characters in excess
    size_t initialChar,     // index of first char to erase
           nChars;          // how many chars do we need to erase?

    switch (mode)
    {
        case wxELLIPSIZE_START:
            initialChar = 0;
            for ( nChars=0;
                  nChars < len && charOffsets[nChars] < excessPixels;
                  nChars++ )
                ;
            break;

        case wxELLIPSIZE_MIDDLE:
            {
                // the start & end of the removed span of chars
                initialChar = len/2;
                size_t endChar = len/2;

                int removed = 0;
                for ( ; removed < excessPixels; )
                {
                    if (initialChar > 0)
                    {
                        // width of the initialChar-th character
                        int width = charOffsets[initialChar] -
                                    charOffsets[initialChar-1];

                        // remove the initialChar-th character
                        removed += width;
                        initialChar--;
                    }

                    if (endChar < len - 1 &&
                        removed < excessPixels)
                    {
                        // width of the (endChar+1)-th character
                        int width = charOffsets[endChar+1] -
                                    charOffsets[endChar];

                        // remove the endChar-th character
                        removed += width;
                        endChar++;
                    }

                    if (initialChar == 0 && endChar == len-1)
                    {
                        nChars = len+1;
                        break;
                    }
                }

                initialChar++;
                nChars = endChar - initialChar + 1;
            }
            break;

        case wxELLIPSIZE_END:
            {
                wxASSERT(len > 0);

                int maxWidth = totalWidth - excessPixels;
                for ( initialChar = 0;
                      initialChar < len && charOffsets[initialChar] < maxWidth;
                      initialChar++ )
                    ;

                if (initialChar == 0)
                {
                    nChars = len;
                }
                else
                {
                    //initialChar--;      // go back one character
                    nChars = len - initialChar;
                }
            }
            break;

        default:
            wxFAIL_MSG("invalid ellipsize mode");
            return curLine;
    }

    wxString ret(curLine);
    if (nChars >= len)
    {
        // need to remove the entire row!
        ret.clear();
    }
    else
    {
        // erase nChars characters after initialChar (included):
        ret.erase(initialChar, nChars+1);

        // if there is space for the replacement dots, add them
        if (maxFinalWidth > replacementWidth)
            ret.insert(initialChar, wxELLIPSE_REPLACEMENT);
    }

    // if everything was ok, we should have shortened this line
    // enough to make it fit in maxFinalWidth:
    wxASSERT(dc.GetTextExtent(ret).GetWidth() < maxFinalWidth);

    return ret;
}

/* static */
wxString wxControlBase::Ellipsize(const wxString& label, const wxDC& dc,
                                  wxEllipsizeMode mode, int maxFinalWidth,
                                  int flags)
{
    wxString ret;

    // these cannot be cached between different Ellipsize() calls as they can
    // change because of e.g. a font change; however we calculate them only once
    // when ellipsizing multiline labels:
    int replacementWidth = dc.GetTextExtent(wxELLIPSE_REPLACEMENT).GetWidth();
    int marginWidth = dc.GetCharWidth();

    // NB: we must handle correctly labels with newlines:
    wxString curLine;
    for ( wxString::const_iterator pc = label.begin(); ; ++pc )
    {
        if ( pc == label.end() || *pc == wxS('\n') )
        {
            curLine = DoEllipsizeSingleLine(curLine, dc, mode, maxFinalWidth,
                                            replacementWidth, marginWidth);

            // add this (ellipsized) row to the rest of the label
            ret << curLine;
            if ( pc == label.end() )
            {
                // NOTE: this is the return which always exits the function
                return ret;
            }
            else
            {
                ret << *pc;
                curLine.clear();
            }
        }
        // we need to remove mnemonics from the label for correct calculations
        else if ( *pc == wxS('&') && (flags & wxELLIPSIZE_PROCESS_MNEMONICS) != 0 )
        {
            // pc+1 is safe: at worst we'll be at end()
            wxString::const_iterator next = pc + 1;
            if ( next != label.end() && *next == wxS('&') )
                curLine += wxS('&');          // && becomes &
            //else: remove this ampersand
        }
        // we need also to expand tabs to properly calc their size
        else if ( *pc == wxS('\t') && (flags & wxELLIPSIZE_EXPAND_TAB) != 0 )
        {
            // Windows natively expands the TABs to 6 spaces. Do the same:
            curLine += wxS("      ");
        }
        else
        {
            curLine += *pc;
        }
    }

    // this return would generate a
    //  warning C4702: unreachable code
    // with MSVC since the function always exits from inside the loop
    //return ret;
}



// ----------------------------------------------------------------------------
// wxStaticBitmap
// ----------------------------------------------------------------------------

#if wxUSE_STATBMP

wxStaticBitmapBase::~wxStaticBitmapBase()
{
    // this destructor is required for Darwin
}

wxSize wxStaticBitmapBase::DoGetBestSize() const
{
    wxSize best;
    wxBitmap bmp = GetBitmap();
    if ( bmp.Ok() )
        best = wxSize(bmp.GetWidth(), bmp.GetHeight());
    else
        // this is completely arbitrary
        best = wxSize(16, 16);
    CacheBestSize(best);
    return best;
}

#endif // wxUSE_STATBMP

#endif // wxUSE_CONTROLS
