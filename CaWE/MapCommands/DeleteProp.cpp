/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#include "DeleteProp.hpp"
#include "../MapDocument.hpp"
#include "../MapEntityBase.hpp"


CommandDeletePropertyT::CommandDeletePropertyT(MapDocumentT& MapDoc, MapEntityBaseT* Entity, const wxString& Key)
    : m_MapDoc(MapDoc),
      m_Entity(Entity),
      m_Index(Entity->FindPropertyIndex(Key)),
      m_PropBackup(m_Index>=0 ? Entity->GetProperties()[m_Index] : EntPropertyT())
{
}


CommandDeletePropertyT::CommandDeletePropertyT(MapDocumentT& MapDoc, MapEntityBaseT* Entity, int Index)
    : m_MapDoc(MapDoc),
      m_Entity(Entity),
      m_Index(Index),
      m_PropBackup(m_Index>=0 ? Entity->GetProperties()[m_Index] : EntPropertyT())
{
}


bool CommandDeletePropertyT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;
    if (m_Index<0) return false;

    // Note that only non-class keys can be deleted here, so we don't need to call anything MapEntityT specific here - children don't need to be notified.
    m_Entity->GetProperties().RemoveAtAndKeepOrder(m_Index);

    // FIXME Note that when a property of multiple entities is deleted, this observer notification is created
    // for EACH of these entities. We should change the command to accept an array of entities...
    ArrayT<MapElementT*> MapElements;
    MapElements.PushBack(m_Entity);

    m_MapDoc.UpdateAllObservers_Modified(MapElements, MEMD_ENTITY_PROPERTY_DELETED, m_PropBackup.Key);

    m_Done=true;
    return true;
}


void CommandDeletePropertyT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    m_Entity->GetProperties().InsertAt(m_Index, m_PropBackup);

    ArrayT<MapElementT*> MapElements;
    MapElements.PushBack(m_Entity);

    m_MapDoc.UpdateAllObservers_Modified(MapElements, MEMD_ENTITY_PROPERTY_CREATED, m_PropBackup.Key);

    m_Done=false;
}


wxString CommandDeletePropertyT::GetName() const
{
    return "Delete property '"+m_PropBackup.Key+"'";
}
