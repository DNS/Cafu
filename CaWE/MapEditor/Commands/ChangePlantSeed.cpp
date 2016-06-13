/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ChangePlantSeed.hpp"

#include "../MapPlant.hpp"
#include "../MapDocument.hpp"

#include "../../GameConfig.hpp"


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
