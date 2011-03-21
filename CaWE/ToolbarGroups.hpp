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

#ifndef _TOOLBAR_GROUPS_HPP_
#define _TOOLBAR_GROUPS_HPP_

#include "ObserverPattern.hpp"
#include "MapCommands/Group_SetProp.hpp"
#include "Templates/Array.hpp"
#include "wx/panel.h"


class MapDocumentT;
class GroupsListViewT;


/// The groups toolbar (actually, a dialog).
class GroupsToolbarT : public wxPanel, public ObserverT
{
    public:

    /// The constructor.
    GroupsToolbarT(wxWindow* Parent, MapDocumentT* MapDoc);

    /// The destructor.
    ~GroupsToolbarT();

    // Implementation of the ObserverT interface.
    void NotifySubjectChanged_Groups(SubjectT* Subject);
    void NotifySubjectDies(SubjectT* Subject);


    private:

    friend class GroupsListViewT;

    MapDocumentT*    m_MapDoc;
    GroupsListViewT* m_ListView;
    bool             m_IsRecursiveSelfNotify;

    // The event handlers.
    void OnToggleVisibility(unsigned long GroupNr);                                 ///< Called by the m_ListView when the visibility of the group with the given number was toggled.
    void OnToggleProperty(unsigned long GroupNr, CommandGroupSetPropT::PropT Prop); ///< Called by the m_ListView when a property of the group with the given number was toggled.
    void OnMenu(wxCommandEvent& CE);                                                ///< Event handler for popup (context) menu events.
    void OnMenuUpdate(wxUpdateUIEvent& UE);                                         ///< Event handler for popup menu update events.

    /// IDs for the controls in whose events we are interested.
    enum
    {
        ID_LISTVIEW_GROUPS=wxID_HIGHEST+1,
        ID_MENU_SELECT,
        ID_MENU_EDIT_RENAME,
        ID_MENU_EDIT_SETCOLOR,
        ID_MENU_EDIT_SHOW,
        ID_MENU_EDIT_HIDE,
        ID_MENU_EDIT_CANSELECT,
        ID_MENU_EDIT_LOCK,
        ID_MENU_EDIT_SELASGROUP,
        ID_MENU_EDIT_SELASINDIV,
     // ID_MENU_NEW_GROUP,      // Create a new, empty group. Doesn't buy us any new functionality at the cost of confusing the user.
     // ID_MENU_AUGMENT_GROUP,  // Add/Assign the selected elements as members of this group. Same problem as with ID_MENU_NEW_GROUP.
        ID_MENU_DELETE,
        ID_MENU_MERGE,
        ID_MENU_MOVEUP,
        ID_MENU_MOVEDOWN
    };

    DECLARE_EVENT_TABLE()
};

#endif
