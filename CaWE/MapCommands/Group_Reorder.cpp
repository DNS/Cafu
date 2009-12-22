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

#include "Group_Reorder.hpp"
#include "../MapDocument.hpp"


CommandReorderGroupsT::CommandReorderGroupsT(MapDocumentT& MapDoc, const ArrayT<GroupT*>& NewOrder)
    : m_MapDoc(MapDoc),
      m_OldOrder(MapDoc.GetGroups()),
      m_NewOrder(NewOrder)
{
}


bool CommandReorderGroupsT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Set the new groups order in the map document
    // and notify all observers that our groups inventory changed.
    m_MapDoc.GetGroups()=m_NewOrder;
    m_MapDoc.UpdateAllObservers_GroupsChanged();

    m_Done=true;
    return true;
}


void CommandReorderGroupsT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Restore the old groups order in the map document
    // and notify all observers that our groups inventory changed.
    m_MapDoc.GetGroups()=m_OldOrder;
    m_MapDoc.UpdateAllObservers_GroupsChanged();

    m_Done=false;
}


wxString CommandReorderGroupsT::GetName() const
{
    return "Reorder groups";
}
