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

#include "Group_Delete.hpp"
#include "Group_Assign.hpp"
#include "../Group.hpp"
#include "../MapDocument.hpp"


static ArrayT<GroupT*> OneElemArray(GroupT* Group)
{
    ArrayT<GroupT*> Array;

    Array.PushBack(Group);
    return Array;
}


CommandDeleteGroupT::CommandDeleteGroupT(MapDocumentT& MapDoc, GroupT* Group)
    : m_MapDoc(MapDoc),
      m_DelGroups(OneElemArray(Group)),
      m_OldGroups(MapDoc.GetGroups()),
      m_AssignNullGroup(NULL)
{
    m_AssignNullGroup=new CommandAssignGroupT(m_MapDoc, Group->GetMembers(m_MapDoc), NULL);
}


CommandDeleteGroupT::CommandDeleteGroupT(MapDocumentT& MapDoc, const ArrayT<GroupT*>& Groups)
    : m_MapDoc(MapDoc),
      m_DelGroups(Groups),
      m_OldGroups(MapDoc.GetGroups()),
      m_AssignNullGroup(NULL)
{
    ArrayT<MapElementT*> AllMembers;

    for (unsigned long GroupNr=0; GroupNr<m_DelGroups.Size(); GroupNr++)
        AllMembers.PushBack(m_DelGroups[GroupNr]->GetMembers(m_MapDoc));

    m_AssignNullGroup=new CommandAssignGroupT(m_MapDoc, AllMembers, NULL);
}


CommandDeleteGroupT::~CommandDeleteGroupT()
{
    delete m_AssignNullGroup;

    if (m_Done)
    {
        for (unsigned long GroupNr=0; GroupNr<m_DelGroups.Size(); GroupNr++)
        {
            delete m_DelGroups[GroupNr];
        }
    }
}


bool CommandDeleteGroupT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Assign all members of all deleted groups the NULL (no) group.
    m_AssignNullGroup->Do();

    // Subtract the m_DelGroups from the groups array in the map document
    // and notify all observers that our groups inventory changed.
    for (unsigned long GroupNr=0; GroupNr<m_DelGroups.Size(); GroupNr++)
    {
        const int Index=m_MapDoc.GetGroups().Find(m_DelGroups[GroupNr]);

        wxASSERT(Index>=0);
        if (Index<0) continue;

        m_MapDoc.GetGroups().RemoveAtAndKeepOrder(Index);
    }

    m_MapDoc.UpdateAllObservers_GroupsChanged();

    m_Done=true;
    return true;
}


void CommandDeleteGroupT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Restore the old groups array in the map document
    // and notify all observers that our groups inventory changed.
    m_MapDoc.GetGroups()=m_OldGroups;
    m_MapDoc.UpdateAllObservers_GroupsChanged();

    // Assign all former members their original group again.
    m_AssignNullGroup->Undo();

    m_Done=false;
}


wxString CommandDeleteGroupT::GetName() const
{
    return m_DelGroups.Size()==1 ? "Delete group" : "Delete groups";
}
