/////////////////////////////////////////////////////////////////////////////
// Name:        wx/gtk/slider.h
// Purpose:
// Author:      Robert Roebling
// Id:          $Id: slider.h 49355 2007-10-23 18:16:06Z VZ $
// Copyright:   (c) 1998 Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GTK_SLIDER_H_
#define _WX_GTK_SLIDER_H_

// ----------------------------------------------------------------------------
// wxSlider
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxSlider : public wxSliderBase
{
public:
    wxSlider();
    wxSlider(wxWindow *parent,
             wxWindowID id,
             int value, int minValue, int maxValue,
             const wxPoint& pos = wxDefaultPosition,
             const wxSize& size = wxDefaultSize,
             long style = wxSL_HORIZONTAL,
             const wxValidator& validator = wxDefaultValidator,
             const wxString& name = wxSliderNameStr)
    {
        Create( parent, id, value, minValue, maxValue,
                pos, size, style, validator, name );
    }

    bool Create(wxWindow *parent,
                wxWindowID id,
                int value, int minValue, int maxValue,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxSL_HORIZONTAL,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxSliderNameStr);

    // implement the base class pure virtuals
    virtual int GetValue() const;
    virtual void SetValue(int value);

    virtual void SetRange(int minValue, int maxValue);
    virtual int GetMin() const;
    virtual int GetMax() const;

    virtual void SetLineSize(int lineSize);
    virtual void SetPageSize(int pageSize);
    virtual int GetLineSize() const;
    virtual int GetPageSize() const;

    virtual void SetThumbLength(int lenPixels);
    virtual int GetThumbLength() const;

    static wxVisualAttributes
    GetClassDefaultAttributes(wxWindowVariant variant = wxWINDOW_VARIANT_NORMAL);

    // implementation
    double m_pos;
    int m_scrollEventType;
    bool m_needThumbRelease;
    bool m_blockScrollEvent;

protected:
    virtual GdkWindow *GTKGetWindow(wxArrayGdkWindows& windows) const;

    // set the slider value unconditionally
    void GTKSetValue(int value);

    DECLARE_DYNAMIC_CLASS(wxSlider)
};

#endif // _WX_GTK_SLIDER_H_
