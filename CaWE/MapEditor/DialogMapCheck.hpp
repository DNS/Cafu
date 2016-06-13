/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_DIALOG_MAPCHECK_HPP_INCLUDED
#define CAFU_DIALOG_MAPCHECK_HPP_INCLUDED

#include "Templates/Array.hpp"
#include "wx/wx.h"


class MapCheckerT;
class MapDocumentT;


class MapCheckDialogT : public wxDialog
{
    public:

    /// The constructor.
    MapCheckDialogT(wxWindow* Parent, MapDocumentT& MapDoc);

    /// The destructor.
    ~MapCheckDialogT();


    private:

    void UpdateProblems();

    MapDocumentT&        m_MapDoc;          ///< The map document that this dialog is for.
    ArrayT<MapCheckerT*> m_Problems;        ///< The list of current problems.
    wxListBox*           ListBoxProblems;
    wxStaticText*        StaticTextProblemDescription;
    wxButton*            ButtonGoToError;
    wxButton*            ButtonFix;
    wxButton*            ButtonFixAll;

    // Event handlers.
    void OnListBoxProblemsSelChange(wxCommandEvent& Event);
    void OnButtonGoToError(wxCommandEvent& Event);
    void OnButtonFix(wxCommandEvent& Event);
    void OnButtonFixAll(wxCommandEvent& Event);

    // IDs for the controls in whose events we are interested.
    enum
    {
        ID_LISTBOX_PROBLEMS=wxID_HIGHEST+1,
        ID_BUTTON_GOTO_ERROR,
        ID_BUTTON_FIX,
        ID_BUTTON_FIXALL
    };

    DECLARE_EVENT_TABLE()
};

#endif
