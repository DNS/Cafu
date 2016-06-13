/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_WX_CAWE_TESTVAL_HPP_INCLUDED
#define CAFU_WX_CAWE_TESTVAL_HPP_INCLUDED

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
