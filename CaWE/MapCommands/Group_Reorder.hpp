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

#ifndef _COMMAND_REORDER_GROUPS_HPP_
#define _COMMAND_REORDER_GROUPS_HPP_

#include "../CommandPattern.hpp"


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
