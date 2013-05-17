/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2013 Carsten Fuchs Software.

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

#include "NewEntity.hpp"
#include "Select.hpp"

#include "../EntityClass.hpp"
#include "../MapDocument.hpp"
#include "../MapEntityBase.hpp"
#include "../MapEntRepres.hpp"


CommandNewEntityT::CommandNewEntityT(MapDocumentT& MapDoc, MapEntityBaseT* Entity)
    : m_MapDoc(MapDoc),
      m_Entity(Entity),
      m_CommandSelect(NULL)
{
}


CommandNewEntityT::~CommandNewEntityT()
{
    if (!m_Done)
        delete m_Entity;

    delete m_CommandSelect;
}


bool CommandNewEntityT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Insert the entity into the map.
    m_MapDoc.Insert(m_Entity);

    ArrayT<MapEntityBaseT*> Entities;
    Entities.PushBack(m_Entity);

    m_MapDoc.UpdateAllObservers_Created(Entities);

    if (!m_CommandSelect) m_CommandSelect = CommandSelectT::Set(&m_MapDoc, m_Entity->GetRepres());
    m_CommandSelect->Do();

    m_Done=true;
    return true;
}


void CommandNewEntityT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    wxASSERT(m_CommandSelect);
    m_CommandSelect->Undo();

    // Remove the entity from the map again.
    m_MapDoc.Remove(m_Entity);

    ArrayT<MapEntityBaseT*> Entities;
    Entities.PushBack(m_Entity);

    m_MapDoc.UpdateAllObservers_Deleted(Entities);

    m_Done=false;
}


wxString CommandNewEntityT::GetName() const
{
    return "add \""+m_Entity->GetClass()->GetName()+"\" entity";
}
