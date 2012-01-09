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

#include "ChangeClass.hpp"

#include "../MapDocument.hpp"
#include "../MapEntity.hpp"


CommandChangeClassT::CommandChangeClassT(MapDocumentT& MapDoc, MapEntityT* Entity, const EntityClassT* NewClass)
    : m_Entity(Entity),
      m_PrevProps(Entity->GetProperties()),
      m_MapDoc(MapDoc),
      m_NewClass(NewClass),
      m_PrevClass(Entity->GetClass())
{
}


bool CommandChangeClassT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // For now a simple implementation of class change is implemented: We change the class and check for new unique keys.
    m_Entity->SetClass(m_NewClass);
    m_Entity->CheckUniqueValues(m_MapDoc);

    ArrayT<MapElementT*> MapElements;
    MapElements.PushBack(m_Entity);

    m_MapDoc.UpdateAllObservers_Modified(MapElements, MEMD_ENTITY_CLASS_CHANGED);

    m_Done=true;
    return true;
}


void CommandChangeClassT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    m_Entity->SetClass(m_PrevClass);

    // Reset previous properties.
    m_Entity->GetProperties()=m_PrevProps;

    ArrayT<MapElementT*> MapElements;
    MapElements.PushBack(m_Entity);

    m_MapDoc.UpdateAllObservers_Modified(MapElements, MEMD_ENTITY_CLASS_CHANGED);

    m_Done=false;
}


wxString CommandChangeClassT::GetName() const
{
    return "Change entity class";
}
