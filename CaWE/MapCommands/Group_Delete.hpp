/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#ifndef _COMMAND_DELETE_GROUP_HPP_
#define _COMMAND_DELETE_GROUP_HPP_

#include "../CommandPattern.hpp"


class CommandAssignGroupT;
class GroupT;
class MapDocumentT;


/// This class implements a command for deleting one or more groups from the map document.
/// Any members of the deleted group(s) are assigned the NULL (no) group.
/// The counterpart to this class is CommandNewGroupT.
class CommandDeleteGroupT : public CommandT
{
    public:

    /// The constructor for deleting a single group.
    /// @param MapDoc   The map document to delete the group from.
    /// @param Group    The group to be deleted from the map document.
    CommandDeleteGroupT(MapDocumentT& MapDoc, GroupT* Group);

    /// The constructor for deleting multiple groups at once.
    /// @param MapDoc   The map document to delete the groups from.
    /// @param Groups   The groups to be deleted from the map document.
    CommandDeleteGroupT(MapDocumentT& MapDoc, const ArrayT<GroupT*>& Groups);

    /// The destructor.
    ~CommandDeleteGroupT();

    // Implementation of the CommandT interface.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    MapDocumentT&         m_MapDoc;           ///< The map document to delete the groups from.
    const ArrayT<GroupT*> m_DelGroups;        ///< The list of groups to delete.
    const ArrayT<GroupT*> m_OldGroups;        ///< The original list of groups before the delete.
    CommandAssignGroupT*  m_AssignNullGroup;
};

#endif
