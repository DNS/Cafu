/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#ifndef __WX_CAWE_TESTVAL_HPP__
#define __WX_CAWE_TESTVAL_HPP__

#include "wx/defs.h"
#include "wx/textctrl.h"
#include "wx/validate.h"


class textNumberValidator : public wxValidator
{
    DECLARE_CLASS(textNumberValidator)

    public:

	textNumberValidator(double* val, double minvalue = 0.0, double maxvalue = 0.0);
	textNumberValidator(int* val, int minvalue = 0, int maxvalue = 0);

	textNumberValidator(const textNumberValidator& val);

    ~textNumberValidator();

    virtual wxObject *Clone() const;
    bool Copy(const textNumberValidator& val);

	virtual bool Validate(wxWindow *parent);

    virtual bool TransferToWindow();
    virtual bool TransferFromWindow();

    // Filter keystrokes
    void OnChar(wxKeyEvent& event);

    DECLARE_EVENT_TABLE()


    protected:

	double*			m_doubleValue;
	double			m_doubleMin;
	double			m_doubleMax;

	int*			m_intValue;
	int				m_intMin;
	int				m_intMax;

	bool			m_isDouble;

    bool CheckValidator() const
    {
        wxCHECK_MSG( m_validatorWindow, false,
                     _T("No window associated with validator") );
        wxCHECK_MSG( m_validatorWindow->IsKindOf(CLASSINFO(wxTextCtrl)), false,
                     _T("wxTextValidator is only for wxTextCtrl's") );
//        wxCHECK_MSG( m_stringValue, false,
  //                   _T("No variable storage for validator") );

        return true;
    }
};

#endif
