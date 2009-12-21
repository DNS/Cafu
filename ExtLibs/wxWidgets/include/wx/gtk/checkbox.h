/////////////////////////////////////////////////////////////////////////////
// Name:        wx/gtk/checkbox.h
// Purpose:
// Author:      Robert Roebling
// Id:          $Id: checkbox.h 55404 2008-09-01 08:27:56Z RR $
// Copyright:   (c) 1998 Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef __GTKCHECKBOXH__
#define __GTKCHECKBOXH__

// ----------------------------------------------------------------------------
// wxCheckBox
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxCheckBox : public wxCheckBoxBase
{
public:
    wxCheckBox();
    wxCheckBox( wxWindow *parent, wxWindowID id, const wxString& label,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize, long style = 0,
            const wxValidator& validator = wxDefaultValidator,
            const wxString& name = wxCheckBoxNameStr)
    {
        Create(parent, id, label, pos, size, style, validator, name);
    }
    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxString& label,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxCheckBoxNameStr );

    void SetValue( bool state );
    bool GetValue() const;

    virtual void SetLabel( const wxString& label );
    virtual bool Enable( bool enable = TRUE );

    static wxVisualAttributes
    GetClassDefaultAttributes(wxWindowVariant variant = wxWINDOW_VARIANT_NORMAL);

protected:
    virtual wxSize DoGetBestSize() const;
    virtual void DoApplyWidgetStyle(GtkRcStyle *style);
    virtual GdkWindow *GTKGetWindow(wxArrayGdkWindows& windows) const;

    void DoSet3StateValue(wxCheckBoxState state);
    wxCheckBoxState DoGet3StateValue() const;

public:
    // implementation
    void GTKDisableEvents();
    void GTKEnableEvents();

    GtkWidget *m_widgetCheckbox;
    GtkWidget *m_widgetLabel;

private:
    DECLARE_DYNAMIC_CLASS(wxCheckBox)
};

#endif // __GTKCHECKBOXH__
