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

#include "ChangeTerrainRes.hpp"

#include "../MapDocument.hpp"
#include "../MapTerrain.hpp"


unsigned short CommandChangeTerrainResT::BilinearInterpolation(float x, float y)
{
    // Read integer part and fraction of x/y values.
    const int   x_int =(int)x;
    const int   y_int =(int)y;
    const float x_frac=x-(float)x_int;
    const float y_frac=y-(float)y_int;

    // Create these variables here just to make code more readable.
    float Value00=0.0f;
    float Value10=0.0f;
    float Value11=0.0f;
    float Value01=0.0f;

    // Only read values that used in the formula below (not multiplied with zero). This prevents from accessing the
    // array over its borders.
                                      Value00=m_PrevHeightData[y_int*    m_PrevResolution+x_int  ];
    if (x_frac!=0.0f)                 Value10=m_PrevHeightData[y_int*    m_PrevResolution+x_int+1];
    if (x_frac!=0.0f && y_frac!=0.0f) Value11=m_PrevHeightData[(y_int+1)*m_PrevResolution+x_int+1];
    if (y_frac!=0.0f)                 Value01=m_PrevHeightData[(y_int+1)*m_PrevResolution+x_int  ];

    // Calculate weighted value and return it.
    return Value00*(1.0f-x_frac)*(1.0f-y_frac)+Value10*x_frac*(1.0f-y_frac)+Value11*x_frac*y_frac+Value01*(1.0f-x_frac)*y_frac;
}


CommandChangeTerrainResT::CommandChangeTerrainResT(MapDocumentT& MapDoc, MapTerrainT* Terrain, unsigned int NewResolution)
    : m_MapDoc(MapDoc),
      m_Terrain(Terrain),
      m_PrevHeightData(m_Terrain->GetHeightData()),
      m_NewResolution(NewResolution),
      m_PrevResolution(m_Terrain->GetResolution())
{
    float ScaleFactor=float(m_PrevResolution-1)/float(m_NewResolution-1);

    // Build height data array for new resolution.
    for (unsigned int y=0; y<m_NewResolution; y++)
    {
        for (unsigned int x=0; x<m_NewResolution; x++)
        {
            m_NewHeightData.PushBack(BilinearInterpolation(float(x)*ScaleFactor, float(y)*ScaleFactor));
        }
    }
}


bool CommandChangeTerrainResT::Do()
{
    wxASSERT(!m_Done);

    if (m_Done) return false;

    // Build observer notification parameters.
    ArrayT<MapElementT*> MapElements;
    MapElements.PushBack(m_Terrain);

    m_Terrain->m_HeightData=m_NewHeightData;
    m_Terrain->m_Resolution=m_NewResolution;

    m_Terrain->m_NeedsUpdate=true;

    // Update observers.
    m_MapDoc.UpdateAllObservers_Modified(MapElements, MEMD_PRIMITIVE_PROPS_CHANGED);

    m_Done=true;

    return true;
}


void CommandChangeTerrainResT::Undo()
{
    wxASSERT(m_Done);

    if (!m_Done) return;

    // Build observer notification parameters.
    ArrayT<MapElementT*> MapElements;
    MapElements.PushBack(m_Terrain);

    m_Terrain->m_HeightData=m_PrevHeightData;
    m_Terrain->m_Resolution=m_PrevResolution;

    m_Terrain->m_NeedsUpdate=true;

    // Update observers.
    m_MapDoc.UpdateAllObservers_Modified(MapElements, MEMD_PRIMITIVE_PROPS_CHANGED);

    m_Done=false;
}


wxString CommandChangeTerrainResT::GetName() const
{
    return "Change terrain resolution";
}
