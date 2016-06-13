/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
