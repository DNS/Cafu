/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_DIALOG_GOTO_PRIMITIVE_HPP_INCLUDED
#define CAFU_DIALOG_GOTO_PRIMITIVE_HPP_INCLUDED

#include "wx/wx.h"


/// A dialog for finding and selecting a primitive by number.
class GotoPrimitiveDialogT : public wxDialog
{
    public:

    GotoPrimitiveDialogT(wxWindow* parent=0);

    int m_EntityNumber;
    int m_PrimitiveNumber;
};

#endif
