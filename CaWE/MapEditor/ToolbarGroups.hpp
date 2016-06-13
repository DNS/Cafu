/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_TOOLBAR_GROUPS_HPP_INCLUDED
#define CAFU_TOOLBAR_GROUPS_HPP_INCLUDED

#include "ObserverPattern.hpp"
#include "Commands/Group_SetProp.hpp"
#include "Templates/Array.hpp"
#include "wx/panel.h"


class ChildFrameT;
class CommandHistoryT;
class MapDocumentT;
class GroupsListViewT;


/// The groups toolbar (actually, a dialog).
class GroupsToolbarT : public wxPanel, public ObserverT
{
    public:

    /// The constructor.
    GroupsToolbarT(ChildFrameT* ChildFrame, CommandHistoryT* History);

    /// The destructor.
    ~GroupsToolbarT();

    // Implementation of the ObserverT interface.
    void NotifySubjectChanged_Groups(SubjectT* Subject);
    void NotifySubjectDies(SubjectT* Subject);


    private:

    friend class GroupsListViewT;

    ChildFrameT*     m_ChildFrame;
    CommandHistoryT* m_History;
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
        ID_MENU_MOVEDOWN,
        ID_MENU_SHOW_ALL
    };

    DECLARE_EVENT_TABLE()
};

#endif
