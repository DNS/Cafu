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

#include "ChangePlantSeed.hpp"

#include "../MapPlant.hpp"
#include "../MapDocument.hpp"
#include "../GameConfig.hpp"


CommandChangePlantSeedT::CommandChangePlantSeedT(MapDocumentT& MapDoc, MapPlantT* Plant, unsigned int NewRandomSeed)
    : m_MapDoc(MapDoc),
      m_Plant(Plant),
      m_NewRandomSeed(NewRandomSeed),
      m_OldRandomSeed(Plant->m_RandomSeed)
{
}


bool CommandChangePlantSeedT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Build observer notification parameters.
    ArrayT<MapElementT*>   MapElements;
    ArrayT<BoundingBox3fT> OldBounds;

    MapElements.PushBack(m_Plant);
    OldBounds.PushBack(m_Plant->GetBB());

    m_Plant->m_Tree=TreeT(m_MapDoc.GetPlantDescrMan().GetPlantDescription(std::string(m_Plant->m_DescrFileName)), m_NewRandomSeed);
    m_Plant->m_RandomSeed=m_NewRandomSeed;

    // Update all observers.
    m_MapDoc.UpdateAllObservers_Modified(MapElements, MEMD_PRIMITIVE_PROPS_CHANGED, OldBounds);

    m_Done=true;
    return true;
}


void CommandChangePlantSeedT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Build observer notification parameters.
    ArrayT<MapElementT*>   MapElements;
    ArrayT<BoundingBox3fT> OldBounds;

    MapElements.PushBack(m_Plant);
    OldBounds.PushBack(m_Plant->GetBB());

    m_Plant->m_Tree=TreeT(m_MapDoc.GetPlantDescrMan().GetPlantDescription(std::string(m_Plant->m_DescrFileName)), m_OldRandomSeed);
    m_Plant->m_RandomSeed=m_OldRandomSeed;

    // Update all observers.
    m_MapDoc.UpdateAllObservers_Modified(MapElements, MEMD_PRIMITIVE_PROPS_CHANGED, OldBounds);

    m_Done=false;
}


wxString CommandChangePlantSeedT::GetName() const
{
    return "Change plant random seed";
}
