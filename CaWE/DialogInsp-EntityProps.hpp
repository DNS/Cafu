/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#ifndef _DIALOG_INSP_ENTITY_PROPS_HPP_
#define _DIALOG_INSP_ENTITY_PROPS_HPP_

#include "ObserverPattern.hpp"
#include "wx/panel.h"

#include <map>


class wxPropertyGridManager;
class wxPropertyGridEvent;
class wxPropertyCategory;
class wxPGProperty;
class wxStaticText;
class MapDocumentT;
class MapEntityBaseT;
struct PropInfoT;


/// This class displays the properties of all currently selected entities.
/// It is implemented as an observer of the map document, and therefore acts just like the 2D and 3D views as another "view" of the "model".
/// As the user can also change the values of the properties (and thus modify the document), this class also acts as a "controller".
/// While the presentation of the properties of exactly one entity is straightforward, presenting the properties of multiple entities
/// all at the same time in a manner that is clear both for the user as well as in the implementation is not.
/// This has been solved by cleverly "overlaying" properties that occur in multiple entities.
/// (See the notes in the implementation for more details.)
class InspDlgEntityPropsT : public wxPanel, public ObserverT
{
    public:

    InspDlgEntityPropsT(wxWindow* Parent_, MapDocumentT* MapDoc_);
    ~InspDlgEntityPropsT();

    // Implementation of the ObserverT interface.
    void NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection);
    void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements);
    void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail, const wxString& Key);
    void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail);
    void NotifySubjectDies(SubjectT* dyingSubject);


    private:

    wxSizer* InspectorEntityPropsInit(wxWindow* parent, bool call_fit=true, bool set_sizer=true);

    // Helper.
    void UpdatePropertyGrid();

    // Event handler methods.
    void OnPropertyGridChanged(wxPropertyGridEvent& event);
    void OnPropertyGridItemRightClick(wxPropertyGridEvent& event);
    void OnContextMenuItemAdd(wxCommandEvent& event);
    void OnContextMenuItemDelete(wxCommandEvent& event);

    MapDocumentT*          MapDoc;
    wxPropertyGridManager* PropMan;
    wxPropertyCategory*    ClassKeys;       ///< PropertyGrid category for keys of an entity class.
    wxPropertyCategory*    MixedKeys;       ///< PropertyGrid category for keys that belong to more than one entity class.
    wxPropertyCategory*    UnDefKeys;       ///< PropertyGrid category for keys that are not defined by any entity class.
    wxMenu*                PopUpMenu;       ///< Context menu used to add or delete properties.
    wxStaticText*          SelectionText;   ///< Text that is displayed above the property grid. It shows the number of selected entities.

    std::map<wxString, PropInfoT> CombinedPropInfos;        ///< The property infos of all selected entities combined.
    wxPGProperty*                 LastRightClickedProperty; ///< The last property on which the user made a right click. Needed to process context menu events to the right property.
    ArrayT<MapEntityBaseT*>       SelectedEntities;         ///< All currently selected entities in the map.

    bool IsRecursiveSelfNotify;


    // IDs for the controls whose events we are interested in.
    enum
    {
        ID_PROPERTY_GRID_MAN=wxID_HIGHEST+1,
        ID_MENU_CONTEXT_ADD,
        ID_MENU_CONTEXT_DELETE
    };

    DECLARE_EVENT_TABLE()
};

#endif
