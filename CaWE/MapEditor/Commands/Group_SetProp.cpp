/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Group_SetProp.hpp"
#include "../Group.hpp"
#include "../MapDocument.hpp"
#include "../MapElement.hpp"


CommandGroupSetPropT::CommandGroupSetPropT(MapDocumentT& MapDoc, GroupT* Group, const wxString& NewName)
    : CommandT(true /*ShowInHistory*/, false /*SuggestsSave*/),
      m_MapDoc(MapDoc),
      m_Group(Group),
      m_Prop(PROP_NAME),
      m_OldName(Group->Name),
      m_OldColor(),
      m_OldFlag(),
      m_NewName(NewName),
      m_NewColor(),
      m_NewFlag()
{
    // Verify that m_Group is actually in the m_MapDoc.GetGroups().
    wxASSERT(m_MapDoc.GetGroups().Find(m_Group)>=0);
}


CommandGroupSetPropT::CommandGroupSetPropT(MapDocumentT& MapDoc, GroupT* Group, const wxColor& NewColor)
    : CommandT(true /*ShowInHistory*/, false /*SuggestsSave*/),
      m_MapDoc(MapDoc),
      m_Group(Group),
      m_Prop(PROP_COLOR),
      m_OldName(),
      m_OldColor(Group->Color),
      m_OldFlag(),
      m_NewName(),
      m_NewColor(NewColor),
      m_NewFlag()
{
    // Verify that m_Group is actually in the m_MapDoc.GetGroups().
    wxASSERT(m_MapDoc.GetGroups().Find(m_Group)>=0);
}


CommandGroupSetPropT::CommandGroupSetPropT(MapDocumentT& MapDoc, GroupT* Group, PropT Prop, bool NewFlag)
    : CommandT(true /*ShowInHistory*/, false /*SuggestsSave*/),
      m_MapDoc(MapDoc),
      m_Group(Group),
      m_Prop(Prop),
      m_OldName(),
      m_OldColor(),
      m_OldFlag(Prop==PROP_CANSELECT ? Group->CanSelect : Group->SelectAsGroup),
      m_NewName(),
      m_NewColor(),
      m_NewFlag(NewFlag)
{
    // Verify that m_Group is actually in the m_MapDoc.GetGroups().
    wxASSERT(m_MapDoc.GetGroups().Find(m_Group)>=0);

    // Verify that m_Prop is valid.
    wxASSERT(m_Prop==PROP_CANSELECT || m_Prop==PROP_SELECTASGROUP);
}


bool CommandGroupSetPropT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Set the new property
    // and notify all observers that our groups inventory changed.
    switch (m_Prop)
    {
        case PROP_NAME:          m_Group->Name=m_NewName;          break;
        case PROP_COLOR:         m_Group->Color=m_NewColor;        break;
        case PROP_CANSELECT:     m_Group->CanSelect=m_NewFlag;     break;
        case PROP_SELECTASGROUP: m_Group->SelectAsGroup=m_NewFlag; break;
    }

    m_MapDoc.UpdateAllObservers_GroupsChanged();

    // If the elements in the group have changed color, update all observers accordingly.
    if (m_Prop==PROP_COLOR)
    {
        const ArrayT<MapElementT*> GroupElems = m_Group->GetMembers();

        if (GroupElems.Size() > 0)
            m_MapDoc.UpdateAllObservers_Modified(GroupElems, MEMD_GENERIC /*MEMD_VISIBILITY*/);
    }

    m_Done=true;
    return true;
}


void CommandGroupSetPropT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Restore the old property
    // and notify all observers that our groups inventory changed.
    switch (m_Prop)
    {
        case PROP_NAME:          m_Group->Name=m_OldName;          break;
        case PROP_COLOR:         m_Group->Color=m_OldColor;        break;
        case PROP_CANSELECT:     m_Group->CanSelect=m_OldFlag;     break;
        case PROP_SELECTASGROUP: m_Group->SelectAsGroup=m_OldFlag; break;
    }

    m_MapDoc.UpdateAllObservers_GroupsChanged();

    // If the elements in the group have changed color, update all observers accordingly.
    if (m_Prop==PROP_COLOR)
    {
        const ArrayT<MapElementT*> GroupElems = m_Group->GetMembers();

        if (GroupElems.Size() > 0)
            m_MapDoc.UpdateAllObservers_Modified(GroupElems, MEMD_GENERIC /*MEMD_VISIBILITY*/);
    }

    m_Done=false;
}


wxString CommandGroupSetPropT::GetName() const
{
    switch (m_Prop)
    {
        case PROP_NAME:          return "Rename group";
        case PROP_COLOR:         return "Set group color";
        case PROP_CANSELECT:     return "Toggle group lock";
        case PROP_SELECTASGROUP: return "Toggle 'select as group'";
    }

    return "Set group property";
}
