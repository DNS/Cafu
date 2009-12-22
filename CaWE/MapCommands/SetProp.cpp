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

#include "SetProp.hpp"
#include "../MapDocument.hpp"
#include "../MapEntityBase.hpp"


CommandSetPropertyT::CommandSetPropertyT(MapDocumentT& MapDoc, MapEntityBaseT* Ent, const wxString& Key, const wxString& NewValue)
    : m_MapDoc(MapDoc),
      m_Ent(Ent),
      m_CreatedProp(!(Ent->FindProperty(Key))),
      m_Key(Key),
      m_OldValue(!m_CreatedProp ? Ent->FindProperty(Key)->Value : ""),
      m_NewValue(NewValue)
{
}


bool CommandSetPropertyT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    m_Ent->FindProperty(m_Key, NULL, true)->Value=m_NewValue;

    // Our subject (the m_MapDoc) has changed, because the properties of one or more of its entities have changed.
    // Therefore, all its observers (e.g. 2D and 3D views) must be notified appropriately:
    // they might be visualizing certain properties like detail models, angles, targeted entities, etc.
    //
    // FIXME Note that when a property of multiple entities is modified, this observer notification is created
    // for EACH of these entities. We really should change the command to accept an array of entities...
    ArrayT<MapElementT*> MapElements;
    MapElements.PushBack(m_Ent);

    m_MapDoc.UpdateAllObservers_Modified(MapElements, m_CreatedProp ? MEMD_ENTITY_PROPERTY_CREATED : MEMD_ENTITY_PROPERTY_MODIFIED, m_Key);

    m_Done=true;
    return true;
}


void CommandSetPropertyT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    if (!m_CreatedProp)
    {
        m_Ent->FindProperty(m_Key)->Value=m_OldValue;
    }
    else
    {
        m_Ent->GetProperties().RemoveAtAndKeepOrder(m_Ent->FindPropertyIndex(m_Key));
    }

    ArrayT<MapElementT*> MapElements;
    MapElements.PushBack(m_Ent);

    m_MapDoc.UpdateAllObservers_Modified(MapElements, m_CreatedProp ? MEMD_ENTITY_PROPERTY_DELETED : MEMD_ENTITY_PROPERTY_MODIFIED, m_Key);

    m_Done=false;
}


wxString CommandSetPropertyT::GetName() const
{
    return "Change property '"+m_Key+"'";
}
