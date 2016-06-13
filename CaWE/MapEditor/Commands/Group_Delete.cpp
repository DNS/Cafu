/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
    m_AssignNullGroup=new CommandAssignGroupT(m_MapDoc, Group->GetMembers(), NULL);
}


CommandDeleteGroupT::CommandDeleteGroupT(MapDocumentT& MapDoc, const ArrayT<GroupT*>& Groups)
    : m_MapDoc(MapDoc),
      m_DelGroups(Groups),
      m_OldGroups(MapDoc.GetGroups()),
      m_AssignNullGroup(NULL)
{
    ArrayT<MapElementT*> AllMembers;

    for (unsigned long GroupNr=0; GroupNr<m_DelGroups.Size(); GroupNr++)
        AllMembers.PushBack(m_DelGroups[GroupNr]->GetMembers());

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
