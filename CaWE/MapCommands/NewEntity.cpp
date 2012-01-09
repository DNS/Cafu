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

#include "NewEntity.hpp"
#include "Select.hpp"

#include "../EntityClass.hpp"
#include "../MapDocument.hpp"
#include "../MapEntity.hpp"


CommandNewEntityT::CommandNewEntityT(MapDocumentT& MapDoc, const EntityClassT* EntityClass, const Vector3fT& Position, const Plane3fT* AdjustPlane)
    : m_MapDoc(MapDoc),
      m_NewEntity(new MapEntityT()),
      m_CommandSelect(NULL)
{
    m_NewEntity->SetOrigin(Position);
    m_NewEntity->SetClass(EntityClass);

    if (AdjustPlane)
    {
        const BoundingBox3fT EntBB  =m_NewEntity->GetBB();
        const float          OffsetZ=(AdjustPlane->Normal.z>0.0f) ? Position.z-EntBB.Min.z : EntBB.Max.z-Position.z;

        m_NewEntity->SetOrigin(Position+AdjustPlane->Normal*(OffsetZ+1.0f));  // The +1.0f is some additional epsilon for the OffsetZ.
    }
}


CommandNewEntityT::~CommandNewEntityT()
{
    if (!m_Done)
        delete m_NewEntity;

    delete m_CommandSelect;
}


bool CommandNewEntityT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Insert the new entity into the map.
    m_MapDoc.Insert(m_NewEntity);

    ArrayT<MapElementT*> Elems;
    Elems.PushBack(m_NewEntity);

    m_MapDoc.UpdateAllObservers_Created(Elems);

    if (!m_CommandSelect) m_CommandSelect=CommandSelectT::Set(&m_MapDoc, m_NewEntity);
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
    m_MapDoc.Remove(m_NewEntity);

    ArrayT<MapElementT*> Elems;
    Elems.PushBack(m_NewEntity);

    m_MapDoc.UpdateAllObservers_Deleted(Elems);

    m_Done=false;
}


wxString CommandNewEntityT::GetName() const
{
    return "new \""+m_NewEntity->GetClass()->GetName()+"\" entity";
}
