/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ReparentPrimitive.hpp"

#include "../CompMapEntity.hpp"
#include "../MapDocument.hpp"
#include "../MapPrimitive.hpp"


using namespace MapEditor;


CommandReparentPrimitiveT::CommandReparentPrimitiveT(MapDocumentT& MapDoc, MapPrimitiveT* Prim, IntrusivePtrT<CompMapEntityT> NewParent)
    : m_MapDoc(MapDoc),
      m_Prims(),
      m_OldParents(),
      m_NewParent(NewParent)
{
    m_Prims.PushBack(Prim);
    m_OldParents.PushBack(Prim->GetParent());
}


CommandReparentPrimitiveT::CommandReparentPrimitiveT(MapDocumentT& MapDoc, const ArrayT<MapPrimitiveT*>& Prims, IntrusivePtrT<CompMapEntityT> NewParent)
    : m_MapDoc(MapDoc),
      m_Prims(Prims),
      m_OldParents(),
      m_NewParent(NewParent)
{
    for (unsigned long PrimNr = 0; PrimNr < m_Prims.Size(); PrimNr++)
        m_OldParents.PushBack(m_Prims[PrimNr]->GetParent());
}


bool CommandReparentPrimitiveT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Assign all the primitives in our list to the new parent entity.
    for (unsigned long PrimNr = 0; PrimNr < m_Prims.Size(); PrimNr++)
    {
        IntrusivePtrT<CompMapEntityT> OldParent = m_OldParents[PrimNr];

        // The primitive must have had a parent entity (and thus been in the world) before.
        wxASSERT(OldParent != NULL);

        // If the primitive's old and new parent entity are the same, just continue.
        if (OldParent == m_NewParent) continue;

        // Remove the primitive from the previous parent entity and add it to the new.
        OldParent->RemovePrim(m_Prims[PrimNr]);
        m_NewParent->AddPrim(m_Prims[PrimNr]);
    }

    // Update all observers.
    ArrayT<MapElementT*> Elems;

    for (unsigned long PrimNr = 0; PrimNr < m_Prims.Size(); PrimNr++)
        Elems.PushBack(m_Prims[PrimNr]);

    m_MapDoc.UpdateAllObservers_Modified(Elems, MEMD_ASSIGN_PRIM_TO_ENTITY);

    m_Done = true;
    return true;
}


void CommandReparentPrimitiveT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Assign all the primitives in our list to the previous parent entity.
    for (unsigned long PrimNr = 0; PrimNr < m_Prims.Size(); PrimNr++)
    {
        IntrusivePtrT<CompMapEntityT> OldParent = m_OldParents[PrimNr];

        if (OldParent == m_NewParent) continue;

        m_NewParent->RemovePrim(m_Prims[PrimNr]);
        OldParent->AddPrim(m_Prims[PrimNr]);
    }

    // Update all observers.
    ArrayT<MapElementT*> Elems;

    for (unsigned long PrimNr = 0; PrimNr < m_Prims.Size(); PrimNr++)
        Elems.PushBack(m_Prims[PrimNr]);

    m_MapDoc.UpdateAllObservers_Modified(Elems, MEMD_ASSIGN_PRIM_TO_ENTITY);

    m_Done = false;
}


wxString CommandReparentPrimitiveT::GetName() const
{
    return "Reparent map primitive";
}
