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

#ifndef _COMMAND_GROUP_SET_VISIBILITY_HPP_
#define _COMMAND_GROUP_SET_VISIBILITY_HPP_

#include "../CommandPattern.hpp"


class CommandSelectT;
class GroupT;
class MapDocumentT;


/// This class implements a command for setting the visibility status of a group.
/// The current selection is automatically reduced to visible elements only, that is,
/// selected map elements that are hidden become automatically unselected.
class CommandGroupSetVisibilityT : public CommandT
{
    public:

    /// The constructor.
    /// @param MapDoc   The map document the group is in.
    /// @param Group    The group whose visibility is set.
    /// @param NewVis   The new visibility status for the group. If Group->IsVisible is already NewVis, Do() will fail.
    CommandGroupSetVisibilityT(MapDocumentT& MapDoc, GroupT* Group, bool NewVis);

    /// The destructor.
    ~CommandGroupSetVisibilityT();

    /// Returns the group whose visibility is set.
    const GroupT* GetGroup() const { return m_Group; }

    // Implementation of the CommandT interface.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    MapDocumentT&   m_MapDoc;
    GroupT*         m_Group;
    const bool      m_NewVis;
    CommandSelectT* m_CommandReduceSel;
};

#endif
