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

#include "Delete.hpp"
#include "Select.hpp"

#include "../EntityClass.hpp"
#include "../MapDocument.hpp"
#include "../MapEntity.hpp"
#include "../MapPrimitive.hpp"


// An entity is deleted entirely if it is among the DeleteElems itself,
// or if it is of solid class and all of its primitives are among the DeleteElems.
static bool IsEntirelyDeleted(MapEntityT* Ent, const ArrayT<MapElementT*>& DeleteElems)
{
    // If the entity is among the elements to delete, it is deleted entirely.
    if (DeleteElems.Find(Ent)>=0) return true;

    // If the entity is non-solid (it has an origin and acts as a placeholder, e.g. a lightsource),
    // and is not among the elements to delete, it is not deleted entirely.
    if (!Ent->GetClass()->IsSolidClass() /*Entity->GetClass()->HasOrigin()*/) return false;

    // The entity is not among the elements to delete, but it is solid.
    // If all its primitives are among the elements to delete, the entity is deleted as well.
    for (unsigned long PrimNr=0; PrimNr<Ent->GetPrimitives().Size(); PrimNr++)
        if (DeleteElems.Find(Ent->GetPrimitives()[PrimNr])==-1) return false;

    return true;
}


CommandDeleteT::CommandDeleteT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& DeleteElems)
    : m_MapDoc(MapDoc),
      m_DeleteEnts(),
      m_DeletePrims(),
      m_DeletePrimsParents(),
      m_CommandSelect(NULL)
{
    // Split the list of elements into a list of primitives and a list of entities.
    // The lists are checked for duplicates and kept free of them as well.
    for (unsigned long ElemNr=0; ElemNr<DeleteElems.Size(); ElemNr++)
    {
        MapElementT*   Elem=DeleteElems[ElemNr];
        MapPrimitiveT* Prim=dynamic_cast<MapPrimitiveT*>(Elem);
        MapEntityT*    Ent =dynamic_cast<MapEntityT*>(Elem);

        if (Prim)
        {
            MapEntityT* Parent=dynamic_cast<MapEntityT*>(Prim->GetParent());

            if (Parent && IsEntirelyDeleted(Parent, DeleteElems))
            {
                // If the parent is a regular entity (not the world!) that is entirely deleted anyway,
                // add the parent to the records instead of the individual primitive.
                if (m_DeleteEnts.Find(Parent)==-1) m_DeleteEnts.PushBack(Parent);
            }
            else
            {
                if (m_DeletePrims.Find(Prim)==-1)
                {
                    m_DeletePrims.PushBack(Prim);
                    m_DeletePrimsParents.PushBack(Prim->GetParent());
                }
            }
            continue;
        }

        if (Ent)
        {
            if (m_DeleteEnts.Find(Ent)==-1) m_DeleteEnts.PushBack(Ent);
            continue;
        }
    }


    // Build the combined list of all deleted elements in order to unselect them.
    ArrayT<MapElementT*> Unselect;

    for (unsigned long PrimNr=0; PrimNr<m_DeletePrims.Size(); PrimNr++)
        Unselect.PushBack(m_DeletePrims[PrimNr]);

    for (unsigned long EntNr=0; EntNr<m_DeleteEnts.Size(); EntNr++)
    {
        Unselect.PushBack(m_DeleteEnts[EntNr]);

        for (unsigned long PrimNr=0; PrimNr<m_DeleteEnts[EntNr]->GetPrimitives().Size(); PrimNr++)
            Unselect.PushBack(m_DeleteEnts[EntNr]->GetPrimitives()[PrimNr]);
    }

    m_CommandSelect=CommandSelectT::Remove(&m_MapDoc, Unselect);
}


CommandDeleteT::~CommandDeleteT()
{
    delete m_CommandSelect;

    if (m_Done)
    {
        for (unsigned long EntNr=0; EntNr<m_DeleteEnts.Size(); EntNr++)
            delete m_DeleteEnts[EntNr];

        for (unsigned long PrimNr=0; PrimNr<m_DeletePrims.Size(); PrimNr++)
            delete m_DeletePrims[PrimNr];
    }
}


bool CommandDeleteT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Deselect any affected elements that are selected.
    m_CommandSelect->Do();

    ArrayT<MapElementT*> DeletedElems;

    for (unsigned long EntNr=0; EntNr<m_DeleteEnts.Size(); EntNr++)
    {
        m_MapDoc.Remove(m_DeleteEnts[EntNr]);
        DeletedElems.PushBack(m_DeleteEnts[EntNr]);
    }

    for (unsigned long PrimNr=0; PrimNr<m_DeletePrims.Size(); PrimNr++)
    {
        m_MapDoc.Remove(m_DeletePrims[PrimNr]);
        DeletedElems.PushBack(m_DeletePrims[PrimNr]);
    }

    // Update all observers.
    m_MapDoc.UpdateAllObservers_Deleted(DeletedElems);

    m_Done=true;
    return true;
}


void CommandDeleteT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    ArrayT<MapElementT*> InsertedElems;

    for (unsigned long PrimNr=0; PrimNr<m_DeletePrims.Size(); PrimNr++)
    {
        m_MapDoc.Insert(m_DeletePrims[PrimNr], m_DeletePrimsParents[PrimNr]);
        InsertedElems.PushBack(m_DeletePrims[PrimNr]);
    }

    for (unsigned long EntNr=0; EntNr<m_DeleteEnts.Size(); EntNr++)
    {
        m_MapDoc.Insert(m_DeleteEnts[EntNr]);
        InsertedElems.PushBack(m_DeleteEnts[EntNr]);
    }

    // Update all observers.
    m_MapDoc.UpdateAllObservers_Created(InsertedElems);

    // Select the previously selected elements again.
    m_CommandSelect->Undo();

    m_Done=false;
}


wxString CommandDeleteT::GetName() const
{
    const unsigned long Sum=m_DeleteEnts.Size() + m_DeletePrims.Size();

    if (m_DeleteEnts.Size()==0)
    {
        return (Sum==1) ? "Delete 1 primitive" : wxString::Format("Delete %lu primitives", Sum);
    }

    if (m_DeletePrims.Size()==0)
    {
        return (Sum==1) ? "Delete 1 entity" : wxString::Format("Delete %lu entities", Sum);
    }

    return (Sum==1) ? "Delete 1 element" : wxString::Format("Delete %lu elements", Sum);
}
