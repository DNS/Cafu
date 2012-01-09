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

#include "ChangePlantDescr.hpp"

#include "../MapPlant.hpp"
#include "../MapDocument.hpp"
#include "../GameConfig.hpp"


CommandChangePlantDescrT::CommandChangePlantDescrT(MapDocumentT& MapDoc, MapPlantT* Plant, wxString NewPlantDescr)
    : m_MapDoc(MapDoc),
      m_Plant(Plant),
      m_NewPlantDescr(NewPlantDescr),
      m_OldPlantDescr(m_Plant->m_DescrFileName)
{
}


bool CommandChangePlantDescrT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // Build observer notification parameters.
    ArrayT<MapElementT*>   MapElements;
    ArrayT<BoundingBox3fT> OldBounds;

    MapElements.PushBack(m_Plant);
    OldBounds.PushBack(m_Plant->GetBB());

    m_Plant->m_Tree=TreeT(m_MapDoc.GetPlantDescrMan().GetPlantDescription(std::string(m_NewPlantDescr)), m_Plant->m_RandomSeed);
    m_Plant->m_DescrFileName=m_NewPlantDescr;

    // Update all observers.
    m_MapDoc.UpdateAllObservers_Modified(MapElements, MEMD_PRIMITIVE_PROPS_CHANGED, OldBounds);

    m_Done=true;
    return true;
}


void CommandChangePlantDescrT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Build observer notification parameters.
    ArrayT<MapElementT*>   MapElements;
    ArrayT<BoundingBox3fT> OldBounds;

    MapElements.PushBack(m_Plant);
    OldBounds.PushBack(m_Plant->GetBB());

    m_Plant->m_Tree=TreeT(m_MapDoc.GetPlantDescrMan().GetPlantDescription(std::string(m_OldPlantDescr)), m_Plant->m_RandomSeed);
    m_Plant->m_DescrFileName=m_OldPlantDescr;

    // Update all observers.
    m_MapDoc.UpdateAllObservers_Modified(MapElements, MEMD_PRIMITIVE_PROPS_CHANGED, OldBounds);

    m_Done=false;
}


wxString CommandChangePlantDescrT::GetName() const
{
    return "Change plant description";
}
