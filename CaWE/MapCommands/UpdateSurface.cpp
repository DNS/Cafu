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

#include "UpdateSurface.hpp"

#include "../MapFace.hpp"
#include "../MapBrush.hpp"
#include "../MapBezierPatch.hpp"
#include "../MapTerrain.hpp"
#include "../MapDocument.hpp"


CommandUpdateSurfaceT::CommandUpdateSurfaceT(MapDocumentT& MapDoc_, const SurfaceInfoT& SurfaceInfoNew_, const SurfaceInfoT& SurfaceInfoOld_, EditorMaterialI* MaterialNew_,  EditorMaterialI* MaterialOld_)
    : MapDoc(MapDoc_),
      SurfaceInfoNew(SurfaceInfoNew_),
      SurfaceInfoOld(SurfaceInfoOld_),
      MaterialNew(MaterialNew_),
      MaterialOld(MaterialOld_)
{
}


CommandUpdateSurfaceFaceT::CommandUpdateSurfaceFaceT(MapDocumentT& MapDoc_, MapBrushT* Brush_, unsigned long FaceIndex_, const SurfaceInfoT& SurfaceInfoNew_, EditorMaterialI* MaterialNew_)
    : CommandUpdateSurfaceT(MapDoc_, SurfaceInfoNew_, Brush_->GetFaces()[FaceIndex_].GetSurfaceInfo(), MaterialNew_, Brush_->GetFaces()[FaceIndex_].GetMaterial()),
      Brush(Brush_),
      FaceIndex(FaceIndex_)
{
}


bool CommandUpdateSurfaceFaceT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    MapFaceT& Face=Brush->GetFaces()[FaceIndex];

    if (MaterialNew!=NULL)
        Face.SetMaterial(MaterialNew);

    Face.SetSurfaceInfo(SurfaceInfoNew);

    ArrayT<MapElementT*> UpdateObjects;
    UpdateObjects.PushBack(Brush);

    MapDoc.UpdateAllObservers_Modified(UpdateObjects, MEMD_SURFACE_INFO_CHANGED);

    m_Done=true;
    return true;
}


void CommandUpdateSurfaceFaceT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    MapFaceT& Face=Brush->GetFaces()[FaceIndex];

    if (MaterialNew!=NULL)
        Face.SetMaterial(MaterialOld);

    Face.SetSurfaceInfo(SurfaceInfoOld);

    ArrayT<MapElementT*> UpdateObjects;
    UpdateObjects.PushBack(Brush);

    MapDoc.UpdateAllObservers_Modified(UpdateObjects, MEMD_SURFACE_INFO_CHANGED);

    m_Done=false;
}


wxString CommandUpdateSurfaceFaceT::GetName() const
{
    return "Update face surface";
}


CommandUpdateSurfaceBezierPatchT::CommandUpdateSurfaceBezierPatchT(MapDocumentT& MapDoc_, MapBezierPatchT* BezierPatch_, const SurfaceInfoT& SurfaceInfoNew_, EditorMaterialI* MaterialNew_)
    : CommandUpdateSurfaceT(MapDoc_, SurfaceInfoNew_, BezierPatch_->GetSurfaceInfo(), MaterialNew_, BezierPatch_->GetMaterial()),
      BezierPatch(BezierPatch_)
{
}


bool CommandUpdateSurfaceBezierPatchT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    if (MaterialNew!=NULL)
        BezierPatch->SetMaterial(MaterialNew);

    BezierPatch->SetSurfaceInfo(SurfaceInfoNew);

    // Update observers.
    ArrayT<MapElementT*> UpdateObjects;
    UpdateObjects.PushBack(BezierPatch);

    MapDoc.UpdateAllObservers_Modified(UpdateObjects, MEMD_SURFACE_INFO_CHANGED);

    m_Done=true;
    return true;
}


void CommandUpdateSurfaceBezierPatchT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    if (MaterialNew!=NULL)
        BezierPatch->SetMaterial(MaterialOld);

    BezierPatch->SetSurfaceInfo(SurfaceInfoOld);

    // Update observers.
    ArrayT<MapElementT*> UpdateObjects;
    UpdateObjects.PushBack(BezierPatch);

    MapDoc.UpdateAllObservers_Modified(UpdateObjects, MEMD_SURFACE_INFO_CHANGED);

    m_Done=false;
}


wxString CommandUpdateSurfaceBezierPatchT::GetName() const
{
    return "Update bezier patch surface";
}


CommandUpdateSurfaceTerrainT::CommandUpdateSurfaceTerrainT(MapDocumentT& MapDoc_, MapTerrainT* Terrain_, EditorMaterialI* MaterialNew_)
    : CommandUpdateSurfaceT(MapDoc_, SurfaceInfoT(), SurfaceInfoT(), MaterialNew_, Terrain_->GetMaterial()),
      Terrain(Terrain_)
{
}


bool CommandUpdateSurfaceTerrainT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    Terrain->SetMaterial(MaterialNew);

    m_Done=true;
    return true;
}


void CommandUpdateSurfaceTerrainT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    Terrain->SetMaterial(MaterialOld);

    m_Done=false;
}


wxString CommandUpdateSurfaceTerrainT::GetName() const
{
    return "Update terrain material";
}
