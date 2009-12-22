/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

Cafu is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

Cafu is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Cafu. If not, see <http://www.gnu.org/licenses/>.

For support and more information about Cafu, visit us at <http://www.cafu.de>.
=================================================================================
*/

#include "cfSpinControl.hpp"

#if defined(_WIN32) && defined(_MSC_VER)
// Turn off warning 4355: "'this' : wird in Initialisierungslisten fuer Basisklasse verwendet".
#pragma warning(disable:4355)
#endif


class MouseRepeatTimerT : public wxTimer
{
    public:

    MouseRepeatTimerT(wxButton& Button_)
        : Button(Button_),
          FireCount(0)
    {
    }

    bool Start(int milliseconds=-1, bool oneShot=false)
    {
        FireCount=0;

        return wxTimer::Start(milliseconds, oneShot);
    }

    void Notify()
    {
        // Simulate a button down event from our button.
        wxCommandEvent CE(wxEVT_COMMAND_BUTTON_CLICKED, Button.GetId());

        // Command() is a method of wxControl, from which wxButton is derived.
        Button.Command(CE);

        // Count how often we've been fired.
        FireCount++;

        // This is the emergency stop for our timer, just in case the Button misses the left-MB-up event for some reason.
        // Normally, we should call CaptureMouse() in SpinButtonT::OnMouseLeftDown(), call "if (HasCapture()) ReleaseMouse();"
        // in SpinButtonT::OnMouseLeftUp(), and have "if (!Button.HasCapture()) { Stop(); return; }" here, but for some reason,
        // this seems to confuse the wxButton base class of Button... (therefore I revert to this simpler method).
        if (FireCount>=200) Stop();
    }

    unsigned long GetFireCount() const
    {
        return FireCount;
    }


    private:

    wxButton&     Button;
    unsigned long FireCount;
};


// Can unfortunately not also derive from wxTimer, as both wxButton and wxTimer are derived from wxEvtHandler,
// which in turn seems to cause problems within the event table macros...
class SpinButtonT : public wxButton
{
    public:

    SpinButtonT(wxWindow* Parent, wxWindowID ID, const wxString& Label)
        : wxButton(Parent, ID, Label, wxDefaultPosition, wxSize(16, 10), wxWANTS_CHARS),
          MouseRepeatTimer(*this),
          SuppressNextButtonEvent(false)
    {
        Connect(ID, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(SpinButtonT::OnButton));
    }


    private:

    void OnMouseLeftDown(wxMouseEvent&   ME);
    void OnMouseLeftUp  (wxMouseEvent&   ME);
    void OnLoseFocus    (wxFocusEvent&   FE);
    void OnButton       (wxCommandEvent& CE);

    MouseRepeatTimerT MouseRepeatTimer;
    bool              SuppressNextButtonEvent;

    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(SpinButtonT, wxButton)
    EVT_LEFT_DOWN (SpinButtonT::OnMouseLeftDown)
    EVT_LEFT_UP   (SpinButtonT::OnMouseLeftUp  )
    EVT_KILL_FOCUS(SpinButtonT::OnLoseFocus    )
END_EVENT_TABLE()


void SpinButtonT::OnMouseLeftDown(wxMouseEvent& ME)
{
    // Start the timer repeatedly in a 250ms interval.
    MouseRepeatTimer.Start(250);

    // Let the base class have the event, too.
    ME.Skip();
}


void SpinButtonT::OnMouseLeftUp(wxMouseEvent& ME)
{
    // Stop the timer.
    MouseRepeatTimer.Stop();

    if (MouseRepeatTimer.GetFireCount()>0)
    {
        // The timer was fired at least once.
        // That means that it would be desireable to suppress the button event that comes with this release of the mouse button...
        SuppressNextButtonEvent=true;
    }

    // Let the base class have the event, too.
    ME.Skip();
}


void SpinButtonT::OnLoseFocus(wxFocusEvent& FE)
{
    // Stop the timer.
    MouseRepeatTimer.Stop();
}


void SpinButtonT::OnButton(wxCommandEvent& CE)
{
    if (!SuppressNextButtonEvent) CE.Skip();    // Call CE.Skip() so that the event reaches our parent window (the SpinControlT)!

    SuppressNextButtonEvent=false;
}


DEFINE_EVENT_TYPE(EVT_CF_SPINCONTROL)


BEGIN_EVENT_TABLE(cfSpinControlT, wxWindow)
    EVT_SIZE      (                                   cfSpinControlT::OnSize)
    EVT_KEY_DOWN  (                                   cfSpinControlT::OnKeyDown)
    EVT_TEXT      (cfSpinControlT::ID_TEXTCTRL_VALUE, cfSpinControlT::OnTextChanged)
    EVT_TEXT_ENTER(cfSpinControlT::ID_TEXTCTRL_VALUE, cfSpinControlT::OnEnter)
    EVT_BUTTON    (cfSpinControlT::ID_BUTTON_INC,     cfSpinControlT::OnButton)
    EVT_BUTTON    (cfSpinControlT::ID_BUTTON_DEC,     cfSpinControlT::OnButton)
END_EVENT_TABLE()


cfSpinControlT::cfSpinControlT(wxWindow* Parent, wxWindowID ID, const wxPoint& Pos, const wxSize& Size, double DefaultValue_, double MinVal_, double MaxVal_, const char* FormatString_, double Increment_, bool Wrap_, EventTriggerE Trigger_)
    : wxWindow(Parent, ID, Pos, Size, wxWANTS_CHARS),
      MinVal(MinVal_),
      MaxVal(MaxVal_),
      FormatString(FormatString_),
      Increment(Increment_),
      Wrap(Wrap_),
      OldValue(DefaultValue_),
      Trigger(Trigger_),
      TextCtrl(NULL),
      IncreaseButton(NULL),
      DecreaseButton(NULL)
{
    wxBoxSizer *item0 = new wxBoxSizer( wxHORIZONTAL );

    TextCtrl=new wxTextCtrl(this, ID_TEXTCTRL_VALUE, wxT(""), wxDefaultPosition, wxSize(70-16, -1), wxWANTS_CHARS | wxTE_PROCESS_ENTER);
    item0->Add(TextCtrl, 1, wxGROW|wxALIGN_CENTER /*|wxLEFT|wxTOP|wxBOTTOM*/, 5 );

    wxBoxSizer *item2 = new wxBoxSizer( wxVERTICAL );

    IncreaseButton=new SpinButtonT(this, ID_BUTTON_INC, wxT("+"));
    item2->Add(IncreaseButton, 0, wxALIGN_CENTER|wxLEFT, 2);

    DecreaseButton=new SpinButtonT(this, ID_BUTTON_DEC, wxT("-"));
    item2->Add(DecreaseButton, 0, wxALIGN_CENTER|wxLEFT, 2);

    item0->Add( item2, 0, wxALIGN_CENTER, 5 );

    this->SetSizer(item0);
    item0->SetSizeHints(this);
    Layout();
    SetAutoLayout(true);

    // Set the default value, and don't trigger a EVT_CF_SPINCONTROL event.
    SetValue(DefaultValue_, false);
}


void cfSpinControlT::SetValue(double d, bool CheckEvent)
{
    if (Wrap && MaxVal>MinVal)
    {
        const double Range=MaxVal-MinVal;

        while (d<MinVal) d+=Range;
        while (d>MaxVal) d-=Range;
    }

    wxString ValueString=wxString::Format(FormatString, d);

    // Set OldValue to the new value in advance, so that no event will be generated.
    if (!CheckEvent) ValueString.ToDouble(&OldValue);

    // This triggers a wxEVT_COMMAND_TEXT_UPDATED event, and thus OnTextChanged() is called.
    TextCtrl->SetValue(ValueString);
}


void cfSpinControlT::SetValue(const wxString& s, bool CheckEvent)
{
    double d;

    if (s.ToDouble(&d))
    {
        SetValue(d, CheckEvent);
        return;
    }


    // This triggers a wxEVT_COMMAND_TEXT_UPDATED event, thus calls OnTextChanged(),
    // but as "s", the new contents of the control, is invalid, no EVT_CF_SPINCONTROL is generated.
    TextCtrl->SetValue(s);

    // Intentionally set the OldValue to something odd, so that the next input of a good number
    // will trigger an EVT_CF_SPINCONTROL event again.
    OldValue=-1234.5678912345;
}


void cfSpinControlT::SetBackgroundColor(const wxColour& color)
{
    TextCtrl->SetBackgroundColour(color);
}


bool cfSpinControlT::IsValid() const
{
    double d=0.0;

    if (!TextCtrl->GetValue().ToDouble(&d)) return false;

    if (d<MinVal) return false;
    if (d>MaxVal) return false;

    return true;
}


double cfSpinControlT::GetValue() const
{
    double d=0.0;

    if (!IsValid()) return 0.0;
    if (!TextCtrl->GetValue().ToDouble(&d)) return 0.0;

    return d;
}


void cfSpinControlT::OnSize(wxSizeEvent& Event)
{
    Layout();
}


void cfSpinControlT::OnKeyDown(wxKeyEvent& Event)
{
    if (!IsValid())
    {
        Event.Skip();
        return;
    }

    switch (Event.GetKeyCode())
    {
        case WXK_UP:              SetValue(GetValue()+    Increment, true); break;
        case WXK_DOWN:            SetValue(GetValue()-    Increment, true); break;
        case WXK_NUMPAD_ADD:      SetValue(GetValue()+2.0*Increment, true); break;
        case WXK_NUMPAD_SUBTRACT: SetValue(GetValue()-2.0*Increment, true); break;
        default: Event.Skip(); break;
    }
}


void cfSpinControlT::OnTextChanged(wxCommandEvent& Event)
{
    const bool valid=IsValid();

    TextCtrl->SetBackgroundColour(valid ? wxNullColour : wxColour(255, 100, 100));
    TextCtrl->Refresh();

    if (!valid) return;

    const double NewValue=GetValue();

    // Only trigger event if value changed and trigger state is TEXT.
    if (OldValue!=NewValue && (Trigger & TEXT))
    {
        // Trigger an EVT_CF_SPINCONTROL event.
        wxCommandEvent ValueChangedEvent(EVT_CF_SPINCONTROL, GetId());

        ValueChangedEvent.SetEventObject(this);
        GetEventHandler()->ProcessEvent(ValueChangedEvent);

        OldValue=NewValue;
    }
}


void cfSpinControlT::OnEnter(wxCommandEvent& Event)
{
    if (!IsValid()) return;

    // Only trigger event if trigger state is ENTER.
    // Note that we deliberately don't check if the value has changed. This allows for explicit
    // event creation on ENTER, disregarding the state of the value.
    if (Trigger & ENTER)
    {
        // Trigger an EVT_CF_SPINCONTROL event.
        wxCommandEvent ValueChangedEvent(EVT_CF_SPINCONTROL, GetId());

        ValueChangedEvent.SetEventObject(this);
        GetEventHandler()->ProcessEvent(ValueChangedEvent);

        OldValue=GetValue();
    }
}


void cfSpinControlT::OnButton(wxCommandEvent& Event)
{
    if (!IsValid()) return;

    // Set the value, but don't trigger an event.
    SetValue(GetValue()+(Event.GetId()==ID_BUTTON_INC ? Increment : -Increment), false);

    // Only trigger event if trigger state is BUTTON.
    if (Trigger & BUTTON)
    {
        wxCommandEvent ValueChangedEvent(EVT_CF_SPINCONTROL, GetId());

        ValueChangedEvent.SetEventObject(this);
        GetEventHandler()->ProcessEvent(ValueChangedEvent);
    }
}
