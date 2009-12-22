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

#ifndef _COMMAND_ASSIGN_GROUP_HPP_
#define _COMMAND_ASSIGN_GROUP_HPP_

#include "../CommandPattern.hpp"


class CommandSelectT;
class GroupT;
class MapDocumentT;
class MapElementT;


/// This class implements a command for putting a set of given map elements into a given group ("MapElems[i].Group=NewGroup").
/// If the given group is hidden, any selected map elements are automatically unselected.
class CommandAssignGroupT : public CommandT
{
    public:

    /// The constructor.
    /// @param MapDoc     The relevant map document.
    /// @param MapElems   The map elements that are to be put into the given group.
    /// @param Group      The group that the MapElems are put into. Can be NULL for "no group".
    CommandAssignGroupT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& MapElems, GroupT* Group);

    // Implementation of the CommandT interface.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    MapDocumentT&        m_MapDoc;
    ArrayT<MapElementT*> m_MapElems;
    GroupT*              m_Group;
    ArrayT<GroupT*>      m_PrevGroups;  ///< The m_MapElems previous groups: In which group was m_MapElems[i] before our Do() put it into m_Group? Used for Undo().
    ArrayT<MapElementT*> m_VisChanged;  ///< The elements from m_MapElems whose visibility changed due to their being put into m_Group.
    CommandSelectT*      m_CommandReduceSel;
};

#endif
