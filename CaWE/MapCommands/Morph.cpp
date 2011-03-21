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

#include "Morph.hpp"

#include "../MapDocument.hpp"
#include "../MapElement.hpp"
#include "../MorphPrim.hpp"


CommandMorphT::CommandMorphT(MapDocumentT& MapDoc, const ArrayT<MorphPrimT*>& MorphPrims)
    : m_MapDoc(MapDoc),
      m_MorphPrims(MorphPrims),
      m_OldStates()
{
    for (unsigned long i=0; i<MorphPrims.Size(); i++)
        m_OldStates.PushBack(MorphPrims[i]->GetElem()->Clone());
}


CommandMorphT::~CommandMorphT()
{
    for (unsigned long i=0; i<m_MorphPrims.Size(); i++)
        delete m_MorphPrims[i];

    for (unsigned long i=0; i<m_OldStates.Size(); i++)
        delete m_OldStates[i];

    m_OldStates.Clear();
}


bool CommandMorphT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    ArrayT<MapElementT*>   UpdateObjects;
    ArrayT<BoundingBox3fT> OldBounds;

    for (unsigned long i=0; i<m_MorphPrims.Size(); i++)
    {
        MorphPrimT*  MorphPrim=m_MorphPrims[i];
        MapElementT* MapElem  =m_MorphPrims[i]->GetElem();

        OldBounds.PushBack(MapElem->GetBB());

        if (!MorphPrim->ApplyMorphToMapElem())
        {
            // Note that we cannot veto ("return;") here, not even in case of conversion failure.
            // For example, if the user deactivates this tool, we get here, but then it's too late.
            // Therefore, the conversion must always succeed, even if that means restoring the previous primitive.
        }

        UpdateObjects.PushBack(MapElem);
    }

    // Update world and views.
    m_MapDoc.UpdateAllObservers_Modified(UpdateObjects, MEMD_MORPH, OldBounds);

    m_Done=true;
    return true;
}


void CommandMorphT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    ArrayT<MapElementT*>   UpdateObjects;
    ArrayT<BoundingBox3fT> OldBounds;

    for (unsigned long i=0; i<m_MorphPrims.Size(); i++)
    {
        MapElementT* MapElem=m_MorphPrims[i]->GetElem();

        OldBounds.PushBack(MapElem->GetBB());

        MapElem->Assign(m_OldStates[i]);

        UpdateObjects.PushBack(MapElem);
    }

    // Update world and views.
    m_MapDoc.UpdateAllObservers_Modified(UpdateObjects, MEMD_MORPH, OldBounds);

    m_Done=false;
}


wxString CommandMorphT::GetName() const
{
    if (m_MorphPrims.Size()>1) return "Morph objects";

    return "Morph object";
}
