/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2014 Carsten Fuchs Software.

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

#include "ReparentPrimitive.hpp"

#include "../../CompMapEntity.hpp"
#include "../../MapDocument.hpp"
#include "../../MapPrimitive.hpp"


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
