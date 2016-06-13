/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Delete.hpp"
#include "Select.hpp"

#include "../CompMapEntity.hpp"
#include "../MapDocument.hpp"
#include "../MapEntRepres.hpp"
#include "../MapPrimitive.hpp"


using namespace MapEditor;


CommandDeleteT::CommandDeleteT(MapDocumentT& MapDoc, MapElementT* DeleteElem)
    : m_MapDoc(MapDoc),
      m_Entities(),
      m_EntityParents(),
      m_EntityIndices(),
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
      m_Entities(),
      m_EntityParents(),
      m_EntityIndices(),
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
            // Double-check that this is really a MapEntRepresT.
            wxASSERT(Elem->GetParent()->GetRepres() == Elem);

            IntrusivePtrT<cf::GameSys::EntityT> Entity = Elem->GetParent()->GetEntity();

            // The root entity (the world) cannot be deleted.
            // (The if-tests below are three versions of the logically same check.)
            if (Elem->GetParent()->IsWorld()) continue;
            if (Entity == Entity->GetRoot()) continue;
            if (Entity->GetParent() == NULL) continue;

            m_Entities.PushBack(Entity);
            m_EntityParents.PushBack(Entity->GetParent());
            m_EntityIndices.PushBack(-1);
        }
        else
        {
            // Double-check that this is really *not* a MapEntRepresT.
            wxASSERT(Elem->GetParent()->GetRepres() != Elem);

            MapPrimitiveT* Prim = dynamic_cast<MapPrimitiveT*>(Elem);

            wxASSERT(Prim);

            if (m_DeletePrims.Find(Prim) == -1)
            {
                m_DeletePrims.PushBack(Prim);
                m_DeletePrimsParents.PushBack(Prim->GetParent());
            }
        }
    }

    // Remove entities from m_Entities that are already in the tree of another entity.
    // This also removes any duplicates from m_Entities.
    for (unsigned long EntNr = 0; EntNr < m_Entities.Size(); EntNr++)
    {
        for (unsigned long TreeNr = 0; TreeNr < m_Entities.Size(); TreeNr++)
        {
            if (EntNr != TreeNr && m_Entities[TreeNr]->Has(m_Entities[EntNr]))
            {
                m_Entities.RemoveAt(EntNr);
                m_EntityParents.RemoveAt(EntNr);
                m_EntityIndices.RemoveAt(EntNr);
                EntNr--;
                break;
            }
        }
    }

    // Remove primitives from m_DeletePrims whose entire entity is deleted anyways.
    for (unsigned long PrimNr = 0; PrimNr < m_DeletePrims.Size(); PrimNr++)
    {
        for (unsigned long TreeNr = 0; TreeNr < m_Entities.Size(); TreeNr++)
        {
            if (m_Entities[TreeNr]->Has(m_DeletePrims[PrimNr]->GetParent()->GetEntity()))
            {
                m_DeletePrims.RemoveAt(PrimNr);
                m_DeletePrimsParents.RemoveAt(PrimNr);
                PrimNr--;
                break;
            }
        }
    }

    // Build the combined list of all deleted elements in order to unselect them.
    ArrayT<MapElementT*> Unselect;

    for (unsigned long PrimNr = 0; PrimNr < m_DeletePrims.Size(); PrimNr++)
        Unselect.PushBack(m_DeletePrims[PrimNr]);

    for (unsigned long EntNr = 0; EntNr < m_Entities.Size(); EntNr++)
    {
        IntrusivePtrT<CompMapEntityT> MapEnt = GetMapEnt(m_Entities[EntNr]);

        Unselect.PushBack(MapEnt->GetAllMapElements());
    }

    m_CommandSelect = CommandSelectT::Remove(&m_MapDoc, Unselect);
}


CommandDeleteT::~CommandDeleteT()
{
    delete m_CommandSelect;

    if (m_Done)
    {
        // for (unsigned long EntNr = 0; EntNr < m_Entities.Size(); EntNr++)
        //     delete m_Entities[EntNr];

        for (unsigned long PrimNr = 0; PrimNr < m_DeletePrims.Size(); PrimNr++)
            delete m_DeletePrims[PrimNr];
    }
}


bool CommandDeleteT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    if (m_Entities.Size() == 0 && m_DeletePrims.Size() == 0)
    {
        // If there is nothing to delete, e.g. because only the world representation
        // was selected (and dropped in Init()), bail out early.
        return false;
    }

    // Deselect any affected elements that are selected.
    m_CommandSelect->Do();

    for (unsigned long PrimNr = 0; PrimNr < m_DeletePrims.Size(); PrimNr++)
    {
        m_MapDoc.Remove(m_DeletePrims[PrimNr]);
    }

    for (unsigned long EntNr = 0; EntNr < m_Entities.Size(); EntNr++)
    {
        IntrusivePtrT<cf::GameSys::EntityT> Entity = m_Entities[EntNr];

        wxASSERT(m_EntityParents[EntNr]->GetChildren().Find(Entity) >= 0);

        // The proper index number can only be determined here, because removing a child
        // may change the index numbers of its siblings.
        m_EntityIndices[EntNr] = m_EntityParents[EntNr]->GetChildren().Find(Entity);

        m_MapDoc.Remove(Entity);
    }

    // Update all observers.
    m_MapDoc.UpdateAllObservers_Deleted(m_DeletePrims);
    m_MapDoc.UpdateAllObservers_Deleted(m_Entities);

    m_Done = true;
    return true;
}


void CommandDeleteT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    for (unsigned long RevNr = 0; RevNr < m_Entities.Size(); RevNr++)
    {
        const unsigned long EntNr = m_Entities.Size() - RevNr - 1;

        // This call to AddChild() should never see a reason to modify the name of the m_Entities[EntNr]
        // to make it unique among its siblings -- it used to be there and was unique, after all.
        m_MapDoc.Insert(m_Entities[EntNr], m_EntityParents[EntNr], m_EntityIndices[EntNr]);
    }

    for (unsigned long PrimNr = 0; PrimNr < m_DeletePrims.Size(); PrimNr++)
        m_MapDoc.Insert(m_DeletePrims[PrimNr], m_DeletePrimsParents[PrimNr]);

    // Update all observers.
    m_MapDoc.UpdateAllObservers_Created(m_Entities);
    m_MapDoc.UpdateAllObservers_Created(m_DeletePrims);

    // Select the previously selected elements again.
    m_CommandSelect->Undo();

    m_Done = false;
}


wxString CommandDeleteT::GetName() const
{
    const unsigned long Sum = m_Entities.Size() + m_DeletePrims.Size();

    if (m_Entities.Size() == 0)
    {
        return (Sum == 1) ? "Delete 1 primitive" : wxString::Format("Delete %lu primitives", Sum);
    }

    if (m_DeletePrims.Size() == 0)
    {
        return (Sum == 1) ? "Delete 1 entity" : wxString::Format("Delete %lu entities", Sum);
    }

    return (Sum == 1) ? "Delete 1 element" : wxString::Format("Delete %lu elements", Sum);
}
