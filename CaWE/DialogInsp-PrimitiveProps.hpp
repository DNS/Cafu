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

#ifndef _DIALOG_INSP_PRIMITIVE_PROPS_HPP_
#define _DIALOG_INSP_PRIMITIVE_PROPS_HPP_

#include "ObserverPattern.hpp"
#include "wx/panel.h"


class wxPropertyGridManager;
class wxPropertyGridEvent;
class MapDocumentT;
class MapElementT;
class wxStaticText;


class InspDlgPrimitivePropsT : public wxPanel, public ObserverT
{
    public:

    InspDlgPrimitivePropsT(wxWindow* Parent_, MapDocumentT* MapDoc_);
    ~InspDlgPrimitivePropsT();

    // Implementation of the ObserverT interface.
    void NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection);
    void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements);
    void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail);
    void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail, const ArrayT<BoundingBox3fT>& OldBounds);
    void NotifySubjectDies(SubjectT* dyingSubject);


    private:

    wxSizer* InspectorPrimitivePropsInit(wxWindow* parent, bool call_fit=true, bool set_sizer=true);

    void UpdateGrid();

    // Event handler methods.
    void OnPropertyGridChanged(wxPropertyGridEvent& Event);

    MapDocumentT*          MapDoc;
    wxPropertyGridManager* PropMan;
    wxStaticText*          SelectionText;  ///< Text that is displayed above the property grid. It shows the number of selected primitives.

    bool IsRecursiveSelfNotify;


    // IDs for the controls whose events we are interested in.
    enum
    {
        ID_PROPERTY_GRID_MAN=wxID_HIGHEST+1
    };

    DECLARE_EVENT_TABLE()
};

#endif
