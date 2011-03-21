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

#ifndef _COMMAND_NEW_GROUP_HPP_
#define _COMMAND_NEW_GROUP_HPP_

#include "../CommandPattern.hpp"


class GroupT;
class MapDocumentT;
class MapElementT;


/// This class implements a command for adding a new group to the map document.
/// The newly added group initially has no members: it's up to the caller to put map elements into the new group.
/// Having no members means that the new group initially doesn't affect (the visibility of) any map elements,
/// but also that it is possibly subject to auto-pruning if no members are added to it soon.
/// The counterpart to this class is CommandDeleteGroupT.
class CommandNewGroupT : public CommandT
{
    public:

    /// The constructor.
    /// @param MapDoc   The map document to add the new group to.
    /// @param Name     The name for the new group.
    CommandNewGroupT(MapDocumentT& MapDoc, const wxString& Name);

    /// The destructor.
    ~CommandNewGroupT();

    /// Returns the new group.
    GroupT* GetGroup() { return m_Group; }

    // Implementation of the CommandT interface.
    bool     Do();
    void     Undo();
    wxString GetName() const;


    private:

    MapDocumentT& m_MapDoc;
    GroupT*       m_Group;
};

#endif
