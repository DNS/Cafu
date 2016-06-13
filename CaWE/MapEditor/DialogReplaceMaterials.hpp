/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_DIALOG_REPLACE_MATERIALS_HPP_INCLUDED
#define CAFU_DIALOG_REPLACE_MATERIALS_HPP_INCLUDED

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
