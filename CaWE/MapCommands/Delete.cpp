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

#include "Delete.hpp"
#include "Select.hpp"

#include "../EntityClass.hpp"
#include "../MapDocument.hpp"
#include "../MapEntityBase.hpp"
#include "../MapEntRepres.hpp"
#include "../MapPrimitive.hpp"


CommandDeleteT::CommandDeleteT(MapDocumentT& MapDoc, MapElementT* DeleteElem)
    : m_MapDoc(MapDoc),
      m_DeleteEnts(),
      m_DeletePrims(),
      m_DeletePrimsParents(),
      m_CommandSelect(NULL)
{
    ArrayT<MapElementT*> DeleteElems;

    DeleteElems.PushBack(DeleteElem);
    Init(DeleteElems);
}


CommandDeleteT::CommandDeleteT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& DeleteElems)
    : m_MapDoc(MapDoc),
      m_DeleteEnts(),
      m_DeletePrims(),
      m_DeletePrimsParents(),
      m_CommandSelect(NULL)
{
    Init(DeleteElems);
}


void CommandDeleteT::Init(const ArrayT<MapElementT*>& DeleteElems)
{
    // Split the list of elements into a list of primitives and a list of entities.
    // The lists are checked for duplicates (and kept free of them).
    for (unsigned long ElemNr = 0; ElemNr < DeleteElems.Size(); ElemNr++)
    {
        MapElementT* Elem = DeleteElems[ElemNr];

        if (Elem->GetType() == &MapEntRepresT::TypeInfo)
        {
            wxASSERT(Elem->GetParent()->GetRepres() == Elem);

            // Don't delete entity 0, the world.
            if (Elem->GetParent()->IsWorld())
                continue;

            if (m_DeleteEnts.Find(Elem->GetParent()) == -1)
            {
                m_DeleteEnts.PushBack(Elem->GetParent());
            }
        }
        else
        {
            wxASSERT(Elem->GetParent()->GetRepres() != Elem);

            // If the primitive's whole entity is deleted anyway, we can drop it here.
            if (!Elem->GetParent()->IsWorld())
                if (DeleteElems.Find(Elem->GetParent()->GetRepres()) >= 0)
                    continue;

            MapPrimitiveT* Prim = dynamic_cast<MapPrimitiveT*>(Elem);

            wxASSERT(Prim);

            if (m_DeletePrims.Find(Prim) == -1)
            {
                m_DeletePrims.PushBack(Prim);
                m_DeletePrimsParents.PushBack(Prim->GetParent());
            }
        }
    }


    // Build the combined list of all deleted elements in order to unselect them.
    ArrayT<MapElementT*> Unselect;

    for (unsigned long PrimNr=0; PrimNr<m_DeletePrims.Size(); PrimNr++)
        Unselect.PushBack(m_DeletePrims[PrimNr]);

    for (unsigned long EntNr=0; EntNr<m_DeleteEnts.Size(); EntNr++)
    {
        Unselect.PushBack(m_DeleteEnts[EntNr]->GetRepres());

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

    if (m_DeleteEnts.Size() == 0 && m_DeletePrims.Size() == 0)
    {
        // If there is nothing to delete, e.g. because only the world representation
        // was selected (and dropped in Init()), bail out early.
        return false;
    }

    // Deselect any affected elements that are selected.
    m_CommandSelect->Do();

    ArrayT<MapElementT*> DeletedElems;

    for (unsigned long EntNr=0; EntNr<m_DeleteEnts.Size(); EntNr++)
    {
        m_MapDoc.Remove(m_DeleteEnts[EntNr]);
        DeletedElems.PushBack(m_DeleteEnts[EntNr]->GetRepres());
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
        InsertedElems.PushBack(m_DeleteEnts[EntNr]->GetRepres());
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
