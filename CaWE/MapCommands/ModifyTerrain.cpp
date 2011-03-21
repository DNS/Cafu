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

#include "ModifyTerrain.hpp"

#include "../MapTerrain.hpp"
#include "../MapDocument.hpp"

#include "Templates/Array.hpp"


CommandModifyTerrainT::CommandModifyTerrainT(MapDocumentT& MapDoc, MapTerrainT* Terrain, const ArrayT<unsigned short>& NewHeightData, const wxPoint& Position, unsigned int SizeX, unsigned int SizeY)
    : m_MapDoc(MapDoc),
      m_Terrain(Terrain),
      m_NewHeightData(NewHeightData),
      m_Offset(Position),
      m_SizeX(SizeX),
      m_SizeY(SizeY)
{
}


bool CommandModifyTerrainT::Do()
{
    wxASSERT(!m_Done);

    if (m_Done) return false;

    // Build observer notification parameters.
    ArrayT<MapElementT*> MapElements;
    MapElements.PushBack(m_Terrain);

    // If the original height data hasn't been backed up (first run).
    if (m_PrevHeightData.Size()==0)
    {
        for (unsigned int y=0; y<m_SizeY; y++)
            for (unsigned int x=0; x<m_SizeX; x++)
                m_PrevHeightData.PushBack(m_Terrain->m_HeightData[(y+m_Offset.y)*m_Terrain->GetResolution()+x+m_Offset.x]);
    }

    for (unsigned int y=0; y<m_SizeY; y++)
        for (unsigned int x=0; x<m_SizeX; x++)
        {
            unsigned short value=m_NewHeightData[y*m_SizeX+x];
            m_Terrain->m_HeightData[(y+m_Offset.y)*m_Terrain->GetResolution()+x+m_Offset.x]=value;
        }

    m_Terrain->m_NeedsUpdate=true;

    // Update observers.
    m_MapDoc.UpdateAllObservers_Modified(MapElements, MEMD_PRIMITIVE_PROPS_CHANGED);

    m_Done=true;

    return true;
}


void CommandModifyTerrainT::Undo()
{
    wxASSERT(m_Done);

    if (!m_Done) return;

    // Build observer notification parameters.
    ArrayT<MapElementT*> MapElements;
    MapElements.PushBack(m_Terrain);

    for (unsigned int y=0; y<m_SizeY; y++)
        for (unsigned int x=0; x<m_SizeX; x++)
            m_Terrain->m_HeightData[(y+m_Offset.y)*m_Terrain->GetResolution()+x+m_Offset.x]=m_PrevHeightData[y*m_SizeX+x];

    m_Terrain->m_NeedsUpdate=true;

    // Update observers.
    m_MapDoc.UpdateAllObservers_Modified(MapElements, MEMD_PRIMITIVE_PROPS_CHANGED);

    m_Done=false;
}


wxString CommandModifyTerrainT::GetName() const
{
    return "Modify terrain";
}
