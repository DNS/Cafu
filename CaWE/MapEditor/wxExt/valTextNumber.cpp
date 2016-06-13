/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef WX_PRECOMP
  #include <stdio.h>
  #include "wx/textctrl.h"
  #include "wx/utils.h"
  #include "wx/msgdlg.h"
  #include "wx/intl.h"
#endif

#include "valTextNumber.hpp"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>


IMPLEMENT_CLASS(textNumberValidator, wxValidator)

BEGIN_EVENT_TABLE(textNumberValidator, wxValidator)
    EVT_CHAR(textNumberValidator::OnChar)
END_EVENT_TABLE()


textNumberValidator::textNumberValidator(double* val, double minvalue, double maxvalue)
{
	m_doubleValue = val;

	if ( minvalue < maxvalue )
	{
		m_doubleMin = minvalue;
		m_doubleMax = maxvalue;
	}
	else
	{
		m_doubleMin = maxvalue;
		m_doubleMax = minvalue;
	}

	m_isDouble = true;
}

textNumberValidator::textNumberValidator(int* val, int minvalue, int maxvalue)
{
	m_intValue = val;

	if ( minvalue < maxvalue )
	{
		m_intMin = minvalue;
		m_intMax = maxvalue;
	}
	else
	{
		m_intMin = maxvalue;
		m_intMax = minvalue;
	}

	m_isDouble = false;
}

textNumberValidator::textNumberValidator(const textNumberValidator& val)
    : wxValidator()
{
    Copy(val);
}

wxObject* textNumberValidator::Clone() const
{
  return new textNumberValidator(*this);
}

bool textNumberValidator::Copy(const textNumberValidator& val)
{
    wxValidator::Copy(val);

	m_doubleValue = val.m_doubleValue;
	m_doubleMin = val.m_doubleMin;
	m_doubleMax = val.m_doubleMax;

	m_intValue = val.m_intValue;
	m_intMin = val.m_intMin;
	m_intMax = val.m_intMax;

	m_isDouble = val.m_isDouble;

    return true;
}

textNumberValidator::~textNumberValidator()
{
}

bool textNumberValidator::Validate(wxWindow* parent)
{
	if ( !CheckValidator() )
		return false;

	wxTextCtrl* control = (wxTextCtrl*)m_validatorWindow;

	if ( !control->IsEnabled() )
		return true;

    wxString errormsg;
	wxString stringValue = control->GetValue();

	bool ok = true;

	if (m_isDouble)
	{
		double tmpVal;

		if (!stringValue.ToDouble(&tmpVal))
		{
			ok = false;
			errormsg = "Please enter a valid number.";
		}
		else if (m_doubleMin != m_doubleMax)
		{
			if ( (tmpVal < m_doubleMin) || (tmpVal > m_doubleMax) )
			{
				ok = false;
				errormsg = wxString::Format("Please enter a number between %.2f and %.2f", m_doubleMin, m_doubleMax);
			}
		}
	}
	else
	{
		long tmpVal;

		if (!stringValue.ToLong(&tmpVal))
		{
			ok = false;
			errormsg = "Please enter a valid number.";
		}
		else if (m_intMin != m_intMax)
		{
			if ( (tmpVal < m_intMin) || (tmpVal > m_intMax) )
			{
				ok = false;
				errormsg = wxString::Format("Please enter a number between %d and %d", m_intMin, m_intMax);
			}
		}
	}

    if ( !ok )
    {
        wxASSERT_MSG( !errormsg.empty(), _T("you forgot to set errormsg") );

        m_validatorWindow->SetFocus();

//        wxString buf;
  //      buf.Printf(errormsg, val.c_str());

//        wxMessageBox(buf, _("Invalid Number"), wxOK | wxICON_EXCLAMATION, parent);
		wxMessageBox(errormsg, wxT("Invalid Number"), wxOK | wxICON_EXCLAMATION, parent);
    }

    return ok;
}

// Called to transfer data to the window
bool textNumberValidator::TransferToWindow(void)
{
    if( !CheckValidator() )
        return false;

    wxTextCtrl *control = (wxTextCtrl *)m_validatorWindow ;

	if (m_isDouble)
	{
		control->SetValue(wxString::Format("%.2f", *m_doubleValue));
	}
	else
	{
		control->SetValue(wxString::Format("%d", *m_intValue));
	}

    return true;
}

// Called to transfer data to the window
bool textNumberValidator::TransferFromWindow(void)
{
    if( !CheckValidator() )
        return false;

    wxTextCtrl *control = (wxTextCtrl *) m_validatorWindow ;
	wxString stringValue = control->GetValue();

	if ( m_isDouble)
	{
		stringValue.ToDouble(m_doubleValue);
	}
	else
	{
		long tmpVal;
		stringValue.ToLong(&tmpVal);
		*m_intValue = (int)tmpVal;
	}

    return true;
}


void textNumberValidator::OnChar(wxKeyEvent& event)
{
	if (m_validatorWindow)
	{
		int keycode = (int)event.GetKeyCode();

		if (!(keycode < WXK_SPACE || keycode == WXK_DELETE || keycode > WXK_START) &&
			!wxIsdigit(keycode) && (keycode != '.') && (keycode != '-'))
		{
			if (!wxValidator::IsSilent())
				wxBell();

			return;
		}
	}

	event.Skip();
}
