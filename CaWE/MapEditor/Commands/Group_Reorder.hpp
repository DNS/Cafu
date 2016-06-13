/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMMAND_REORDER_GROUPS_HPP_INCLUDED
#define CAFU_COMMAND_REORDER_GROUPS_HPP_INCLUDED

#include "../../CommandPattern.hpp"


class GroupT;
class MapDocumentT;


/// This class implements a command for changing the order of the groups in the map document.
class CommandReorderGroupsT : public CommandT
{
    public:

    /// The constructor for reordering the groups.
    /// @param MapDoc     The map document to reorder the groups in.
    /// @param NewOrder   The new order for the groups (a permutation of the MapDoc.GetGroups() array).
    CommandReorderGroupsT(MapDocumentT& MapDoc, const ArrayT<GroupT*>& NewOrder);

    // Implementation of the CommandT interface.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    MapDocumentT&         m_MapDoc;     ///< The map document to reorder the groups in.
    const ArrayT<GroupT*> m_OldOrder;   ///< The list of groups in previous order.
    const ArrayT<GroupT*> m_NewOrder;   ///< The list of groups in new order.
};

#endif
