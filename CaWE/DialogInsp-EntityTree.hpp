/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2013 Carsten Fuchs Software.

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

#ifndef CAFU_DIALOG_INSP_ENTITY_TREE_HPP_INCLUDED
#define CAFU_DIALOG_INSP_ENTITY_TREE_HPP_INCLUDED

#include "ObserverPattern.hpp"
#include "wx/panel.h"


class wxCheckBox;
class wxComboBox;
class wxNotebook;
class wxListBox;
class wxTextCtrl;
class MapDocumentT;


class InspDlgEntityTreeT : public wxPanel, public ObserverT
{
    public:

    InspDlgEntityTreeT(wxNotebook* Parent_, MapDocumentT* MapDoc_);
    ~InspDlgEntityTreeT();

    // Implementation of the ObserverT interface.
    void NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection);
    void NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities);
    void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities);
    void Notify_EntChanged(SubjectT* Subject, const ArrayT< IntrusivePtrT<MapEditor::CompMapEntityT> >& Entities, EntityModDetailE Detail);
    void NotifySubjectDies(SubjectT* dyingSubject);


    private:

    MapDocumentT* MapDoc;
    bool          IsRecursiveSelfNotify;    ///< Did we trigger the current call to NotifySubjectChanged() ourselves?

    wxListBox*    EntityListBox;
    wxCheckBox*   ShowHiddenEntitiesCheckBox;

    // Helper functions.
    wxSizer* EntityReportInit(wxWindow* parent, bool call_fit, bool set_sizer);
    void     UpdateEntityListBox();

    // Event handlers.
    void OnCheckbox_ShowHiddenEntities(wxCommandEvent& Event);
    void OnListBox_SelectionChanged(wxCommandEvent& Event);

    // IDs for the controls in whose events we are interested.
    enum
    {
        ID_CHECKBOX_ShowHiddenEntities = wxID_HIGHEST + 1,
        ID_LISTBOX_Entities
    };

    DECLARE_EVENT_TABLE()
};

#endif
