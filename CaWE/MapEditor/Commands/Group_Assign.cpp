/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Group_Assign.hpp"
#include "Select.hpp"
#include "../Group.hpp"
#include "../MapDocument.hpp"
#include "../MapElement.hpp"


CommandAssignGroupT::CommandAssignGroupT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& MapElems, GroupT* Group)
    : m_MapDoc(MapDoc),
      m_MapElems(MapElems),
      m_Group(Group),
      m_PrevGroups(),
      m_VisChanged(),
      m_CommandReduceSel(NULL)
{
    ArrayT<MapElementT*> Unselect;

    for (unsigned long ElemNr=0; ElemNr<MapElems.Size(); ElemNr++)
    {
        MapElementT* Elem=m_MapElems[ElemNr];

        // Record the previous group for Elem.
        m_PrevGroups.PushBack(Elem->GetGroup());

        // If the visibility of Elem will be affected by the new group, record it.
        const bool OldVis=Elem->IsVisible(); Elem->SetGroup(m_Group);               // Temporarily put Elem into its new group.
        const bool NewVis=Elem->IsVisible(); Elem->SetGroup(m_PrevGroups[ElemNr]);  // Restore original group.

        if (NewVis!=OldVis) m_VisChanged.PushBack(Elem);
        if (!NewVis && Elem->IsSelected()) Unselect.PushBack(Elem);
    }

    if (Unselect.Size()>0)
        m_CommandReduceSel=CommandSelectT::Remove(&m_MapDoc, Unselect);
}


/*
/// Returns the groups in the map document that have been abandoned (became empty) by the re-assignment of map elements.
/// Call this method only when the command is "done" (IsDone() returns true), or else it will return wrong results!
ArrayT<GroupT*> CommandAssignGroupT::GetAbandonedGroups() const
{
    // Calling this when not done is a mistake - the groups obviously are not (yet) abandoned then.
    wxASSERT(m_Done);

    ArrayT<GroupT*> EmptyGroups;

    // Copy only unique groups from m_PrevGroups to EmptyGroups.
    for (unsigned long GroupNr=0; GroupNr<m_PrevGroups.Size(); GroupNr++)
        if (m_PrevGroups[GroupNr]!=NULL && EmptyGroups.Find(m_PrevGroups[GroupNr])==-1)
            EmptyGroups.PushBack(m_PrevGroups[GroupNr]);

    // Remove all groups from EmptyGroups that still have members.
    for (unsigned long GroupNr=0; GroupNr<EmptyGroups.Size(); GroupNr++)
    {
        if (EmptyGroups[GroupNr]->HasMembers(m_MapDoc))
        {
            EmptyGroups.RemoveAt(GroupNr);
            GroupNr--;
        }
    }

    return EmptyGroups;
}
*/


bool CommandAssignGroupT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Verify that when m_Group is non-NULL, it is actually in the m_MapDoc.GetGroups().
    if (m_Group)
    {
        const int Index=m_MapDoc.GetGroups().Find(m_Group);
        wxASSERT(Index>=0);
        if (Index<0) return false;
    }

    // Remove elements that become invisible from the selection.
    if (m_CommandReduceSel) m_CommandReduceSel->Do();

    // Put the map elements into the new group.
    for (unsigned long ElemNr=0; ElemNr<m_MapElems.Size(); ElemNr++)
        m_MapElems[ElemNr]->SetGroup(m_Group);

    // Notify all observers about all map elements whose visibility changed.
    if (m_VisChanged.Size()>0)
        m_MapDoc.UpdateAllObservers_Modified(m_VisChanged, MEMD_VISIBILITY);

    m_Done=true;
    return true;
}


void CommandAssignGroupT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Place the map elements into the previous group.
    for (unsigned long ElemNr=0; ElemNr<m_MapElems.Size(); ElemNr++)
    {
        wxASSERT(m_MapElems[ElemNr]->GetGroup()==m_Group);
        m_MapElems[ElemNr]->SetGroup(m_PrevGroups[ElemNr]);
    }

    // Notify all observers about all map elements whose visibility changed.
    if (m_VisChanged.Size()>0)
        m_MapDoc.UpdateAllObservers_Modified(m_VisChanged, MEMD_VISIBILITY);

    // Restore the selection.
    if (m_CommandReduceSel) m_CommandReduceSel->Undo();

    m_Done=false;
}


wxString CommandAssignGroupT::GetName() const
{
    return m_Group ? "Put elements into group" : "Ungroup elements";
}
