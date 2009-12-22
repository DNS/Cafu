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

#ifndef _CF_SPIN_CONTROL_HPP_
#define _CF_SPIN_CONTROL_HPP_

#include "wx/wx.h"


class SpinButtonT;


/// This class implements a spin control for doubles.
/// TODO: Must derive from wxButton and wxTextCtrl in order to make the arrow and +/- keys work.
class cfSpinControlT : public wxWindow      // Not derived from wxControl because its methods don't seem fit.
{
    public:

    // Actions that trigger an event to tell the parent, that the SpinCtrl value has changed.
    enum EventTriggerE
    {
        BUTTON=0x01, // SpinCtrl button clicked.
        ENTER =0x02, // Enter pressed.
        TEXT  =0x04, // Text changed.
        ALL   =0x07, // All above.
    };

    cfSpinControlT(wxWindow* Parent, wxWindowID ID, const wxPoint& Pos, const wxSize& Size, double DefaultValue_, double MinVal_, double MaxVal_, const char* FormatString_, double Increment_, bool Wrap_=false, EventTriggerE Trigger_=ALL);

    /// Sets the value of the control.
    /// @param d            The value to set the control to.
    /// @param CheckEvent   If true, a EVT_CF_SPINCONTROL event is triggered if d is different from the old value.
    ///                     Otherwise, no EVT_CF_SPINCONTROL event is ever triggered, even if d is different from the old value.
    void SetValue(double d, bool CheckEvent);

    /// Sets the value of the control as a string.
    /// You can use this to intentionally set "invalid" strings to the control.
    /// If s is a valid double number d, this just calls SetValue(d, CheckEvent).
    /// Otherwise, the control text is set to (the invalid string) s, and it is made sure that when the user
    /// next overwrites this with a valid number, an EVT_CF_SPINCONTROL event is generated.
    void SetValue(const wxString& s, bool CheckEvent);

    /// Sets the background color in the spin control text field.
    void SetBackgroundColor(const wxColour& color);

    /// Returns whether the value in the control is valid, that is,
    /// if the text control has a valid number string and the numerical value is in the valid min/max range.
    bool IsValid() const;

    /// Returns the numeric value of the control.
    /// If IsValid() returns false, the return value is always 0.
    double GetValue() const;


    private:

    const double MinVal;
    const double MaxVal;
    const char*  FormatString;
    const double Increment;
    const bool   Wrap;
    double       OldValue;      ///< Used to determine when an EVT_CF_SPINCONTROL event should be triggered.

    const EventTriggerE Trigger;

    wxTextCtrl*  TextCtrl;
    SpinButtonT* IncreaseButton;
    SpinButtonT* DecreaseButton;

    // Event handlers.
    void OnSize       (wxSizeEvent&    Event);
    void OnKeyDown    (wxKeyEvent&     Event);
    void OnTextChanged(wxCommandEvent& Event);
    void OnEnter      (wxCommandEvent& Event);
    void OnButton     (wxCommandEvent& Event);

    // IDs for the controls whose events we are interested in.
    enum
    {
        ID_TEXTCTRL_VALUE=wxID_HIGHEST+1,
        ID_BUTTON_INC,
        ID_BUTTON_DEC
    };

    DECLARE_EVENT_TABLE()
};


DECLARE_EVENT_TYPE(EVT_CF_SPINCONTROL, -1)

#endif
