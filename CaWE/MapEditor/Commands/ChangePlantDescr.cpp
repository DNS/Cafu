/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ChangePlantDescr.hpp"

#include "../MapPlant.hpp"
#include "../MapDocument.hpp"

#include "../../GameConfig.hpp"


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
