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

#ifndef _DIALOG_REPLACE_MATERIALS_HPP_
#define _DIALOG_REPLACE_MATERIALS_HPP_

#include "wx/wx.h"


class MapDocumentT;


class ReplaceMaterialsDialogT : public wxDialog
{
    public:

    /// Constructor.
    ReplaceMaterialsDialogT(bool IsSomethingSelected, MapDocumentT& MapDoc, const wxString& InitialFindMatName="");


    private:

    MapDocumentT&   m_MapDoc;

    wxTextCtrl*     TextCtrlFindMatName;
    wxTextCtrl*     TextCtrlReplaceMatName;
    wxRadioButton*  RadioButtonSearchInSelection;
    wxRadioButton*  RadioButtonSearchInWholeWorld;
    wxCheckBox*     CheckBoxInclusiveBrushes;
    wxCheckBox*     CheckBoxInclusiveBPs;
    wxCheckBox*     CheckBoxInclusiveHidden;
    wxRadioBox*     RadioBoxSearchFor;
    wxRadioBox*     RadioBoxReplaceRescaleMode;
    wxCheckBox*     CheckBoxFindOnly;
    wxStaticBitmap* m_BitmapFindMat;
    wxStaticBitmap* m_BitmapReplaceMat;
    wxStaticBox*    StaticBoxReplace;
    wxButton*       ButtonBrowseReplace;

    // Event handlers.
    void OnOK(wxCommandEvent& Event);
    void OnButtonBrowseFind(wxCommandEvent& Event);
    void OnButtonBrowseReplace(wxCommandEvent& Event);
    void OnCheckboxFindOnly(wxCommandEvent& Event);
    void OnRadioButtonSearchIn(wxCommandEvent& Event);
    void OnTextUpdateFindMatName(wxCommandEvent& Event);
    void OnTextUpdateReplaceMatName(wxCommandEvent& Event);

    // IDs for the controls in whose events we are interested.
    enum
    {
        ID_BUTTON_BROWSE_FIND=wxID_HIGHEST+1,
        ID_BUTTON_BROWSE_REPLACE,
        ID_CHECKBOX_FINDONLY,
        ID_RADIOBUTTON_SEARCH_IN_SELECTION,
        ID_RADIOBUTTON_SEARCH_IN_WHOLEWORLD,
        ID_TEXTCTRL_FINDMATNAME,
        ID_TEXTCTRL_REPLACEMATNAME
    };

    DECLARE_EVENT_TABLE()
};

#endif
