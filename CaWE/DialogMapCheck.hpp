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

#ifndef _DIALOG_MAPCHECK_HPP_
#define _DIALOG_MAPCHECK_HPP_


class MapCheckerT;


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
