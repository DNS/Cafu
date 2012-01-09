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

#include "GameConfig.hpp"
#include "MapBezierPatch.hpp"
#include "MapBrush.hpp"
#include "MapDocument.hpp"
#include "MapTerrain.hpp"
#include "MapWorld.hpp"
#include "TypeSys.hpp"
#include "wx/wx.h"


/*** Begin of TypeSys related definitions for this class. ***/

void* MapWorldT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return NULL;
}

const cf::TypeSys::TypeInfoT MapWorldT::TypeInfo(GetMapElemTIM(), "MapWorldT", "MapEntityBaseT", MapWorldT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


MapWorldT::MapWorldT(MapDocumentT& MapDoc)
    : MapEntityBaseT(wxColour(255, 255, 255)),
      m_MapDoc(MapDoc)
{
    const EntityClassT* WorldSpawnClass=m_MapDoc.GetGameConfig()->FindClass("worldspawn");

    wxASSERT(WorldSpawnClass);
    SetClass(WorldSpawnClass!=NULL ? WorldSpawnClass : m_MapDoc.FindOrCreateUnknownClass("worldspawn", false /*HasOrigin*/));
}


MapWorldT::MapWorldT(const MapWorldT& World)
    : MapEntityBaseT(World),
      m_MapDoc(World.m_MapDoc)
{
    // Worlds should never be copied...
    wxASSERT(false);
}


MapWorldT* MapWorldT::Clone() const
{
    return new MapWorldT(*this);
}


void MapWorldT::Assign(const MapElementT* Elem)
{
    // Worlds should never be assigned to...
    wxASSERT(false);
}


BoundingBox3fT MapWorldT::GetBB() const
{
    BoundingBox3fT BB;

    for (unsigned long PrimNr=0; PrimNr<m_Primitives.Size(); PrimNr++)
        BB+=m_Primitives[PrimNr]->GetBB();

    return BB.IsInited() ? BB : BoundingBox3fT(Vector3fT(-16, -16, -16), Vector3fT(16, 16, 16));
}
