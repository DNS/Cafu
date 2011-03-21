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

#include "AssignPrimToEnt.hpp"
#include "Select.hpp"
#include "Delete.hpp"

#include "../EntityClass.hpp"
#include "../MapDocument.hpp"
#include "../MapEntityBase.hpp"
#include "../MapPrimitive.hpp"


CommandAssignPrimToEntT::CommandAssignPrimToEntT(MapDocumentT& MapDoc, const ArrayT<MapPrimitiveT*>& Prims, MapEntityBaseT* Entity)
    : m_MapDoc(MapDoc),
      m_Prims(Prims),
      m_Entity(Entity),
      m_CommandSelect(NULL),
      m_CommandDelete(NULL)
{
    for (unsigned long PrimNr=0; PrimNr<m_Prims.Size(); PrimNr++)
    {
        // The primitive must have had a parent entity (and thus been in the world) before.
        wxASSERT(m_Prims[PrimNr]->GetParent()!=NULL);

        m_PrevParents.PushBack(m_Prims[PrimNr]->GetParent());
    }
}


CommandAssignPrimToEntT::~CommandAssignPrimToEntT()
{
    delete m_CommandSelect;
    delete m_CommandDelete;
}


bool CommandAssignPrimToEntT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Assign all the primitives in our list to the new parent entity.
    for (unsigned long PrimNr=0; PrimNr<m_Prims.Size(); PrimNr++)
    {
        MapPrimitiveT*  Prim      =m_Prims[PrimNr];
        MapEntityBaseT* PrevParent=Prim->GetParent();

        // The primitive must have had a parent entity (and thus been in the world) before.
        wxASSERT(PrevParent!=NULL);

        // If the primitives old and new parent entity is the same, just continue.
        if (PrevParent==m_Entity) continue;

        // Remove the primitive from the previous parent entity and add it to the new.
        PrevParent->RemovePrim(Prim);
        m_Entity->AddPrim(Prim);
    }

    // Update all observers.
    ArrayT<MapElementT*> Elems;

    for (unsigned long PrimNr=0; PrimNr<m_Entity->GetPrimitives().Size(); PrimNr++)
        Elems.PushBack(m_Entity->GetPrimitives()[PrimNr]);

    m_MapDoc.UpdateAllObservers_Modified(Elems, MEMD_ASSIGN_PRIM_TO_ENTITY);

    // Delete any entities that have become empty.
    if (!m_CommandDelete)
    {
        ArrayT<MapElementT*> EmptyEntities;

        for (unsigned long EntNr=1/*skip world*/; EntNr<m_MapDoc.GetEntities().Size(); EntNr++)
        {
            MapEntityBaseT* Ent=m_MapDoc.GetEntities()[EntNr];

            // If the entity is not the world, has no origin and is empty now, just delete it.
            if (Ent->GetClass()->IsSolidClass() /*!Ent->HasOrigin()*/ && Ent->GetPrimitives().Size()==0)
            {
                EmptyEntities.PushBack(Ent);
            }
        }

        m_CommandDelete=new CommandDeleteT(m_MapDoc, EmptyEntities);
    }
    m_CommandDelete->Do();

    // Select the entity that the primitives have been assigned to.
    if (!m_CommandSelect)
    {
        ArrayT<MapElementT*> EntElems;

        EntElems.PushBack(m_Entity);
        for (unsigned long PrimNr=0; PrimNr<m_Entity->GetPrimitives().Size(); PrimNr++)
            EntElems.PushBack(m_Entity->GetPrimitives()[PrimNr]);

        m_CommandSelect=CommandSelectT::Set(&m_MapDoc, EntElems);
    }
    m_CommandSelect->Do();

    m_Done=true;
    return true;
}


void CommandAssignPrimToEntT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    m_CommandSelect->Undo();
    m_CommandDelete->Undo();

    // Assign all the primitives in our list to the previous parent entity.
    for (unsigned long PrimNr=0; PrimNr<m_Prims.Size(); PrimNr++)
    {
        if (m_PrevParents[PrimNr]==m_Entity) continue;

        m_Entity->RemovePrim(m_Prims[PrimNr]);
        m_PrevParents[PrimNr]->AddPrim(m_Prims[PrimNr]);
    }

    // Update all observers.
    ArrayT<MapElementT*> Elems;

    for (unsigned long PrimNr=0; PrimNr<m_Entity->GetPrimitives().Size(); PrimNr++)
        Elems.PushBack(m_Entity->GetPrimitives()[PrimNr]);

    m_MapDoc.UpdateAllObservers_Modified(Elems, MEMD_ASSIGN_PRIM_TO_ENTITY);

    m_Done=false;
}


wxString CommandAssignPrimToEntT::GetName() const
{
    return "Assign to entity";
}
