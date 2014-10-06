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

#include "ApplyMaterial.hpp"

#include "../MapBezierPatch.hpp"
#include "../MapBrush.hpp"
#include "../MapDocument.hpp"
#include "../MapTerrain.hpp"


CommandApplyMaterialT::CommandApplyMaterialT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& Objects, EditorMaterialI* Material)
    : m_MapDoc(MapDoc),
      m_Material(Material)
{
    for (unsigned int i = 0; i < Objects.Size(); i++)
    {
        MapBrushT*       Brush       = dynamic_cast<MapBrushT*>(Objects[i]);
        MapBezierPatchT* BezierPatch = dynamic_cast<MapBezierPatchT*>(Objects[i]);
        MapTerrainT*     Terrain     = dynamic_cast<MapTerrainT*>(Objects[i]);

        if (Brush)
        {
            m_Brushes.PushBack(Brush);
            m_Objects.PushBack(Brush);
        }
        else if (BezierPatch)
        {
            m_BezierPatches.PushBack(BezierPatch);
            m_Objects.PushBack(BezierPatch);
        }
        else if (Terrain)
        {
            m_Terrains.PushBack(Terrain);
            m_Objects.PushBack(Terrain);
        }
    }

    m_OldBrushMats.PushBackEmptyExact(m_Brushes.Size());

    for (unsigned int i = 0; i < m_Brushes.Size(); i++)
    {
        MapBrushT*        Brush = m_Brushes[i];
        ArrayT<MapFaceT>& Faces = Brush->GetFaces();

        m_OldBrushMats[i].PushBackEmptyExact(Faces.Size());

        for (unsigned int FaceNr = 0; FaceNr < Faces.Size(); FaceNr++)
            m_OldBrushMats[i][FaceNr] = Faces[FaceNr].GetMaterial();
    }

    m_OldBezierPatchMats.PushBackEmptyExact(m_BezierPatches.Size());

    for (unsigned int i = 0; i < m_BezierPatches.Size(); i++)
        m_OldBezierPatchMats[i] = m_BezierPatches[i]->GetMaterial();

    m_OldTerrainMats.PushBackEmptyExact(m_Terrains.Size());

    for (unsigned int i = 0; i < m_Terrains.Size(); i++)
        m_OldTerrainMats[i] = m_Terrains[i]->GetMaterial();
}


bool CommandApplyMaterialT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    for (unsigned int i = 0; i < m_Brushes.Size(); i++)
    {
        MapBrushT*        Brush = m_Brushes[i];
        ArrayT<MapFaceT>& Faces = Brush->GetFaces();

        for (unsigned int FaceNr = 0; FaceNr < Faces.Size(); FaceNr++)
            Faces[FaceNr].SetMaterial(m_Material);
    }

    for (unsigned int i = 0; i < m_BezierPatches.Size(); i++)
        m_BezierPatches[i]->SetMaterial(m_Material);

    for (unsigned int i = 0; i < m_Terrains.Size(); i++)
        m_Terrains[i]->SetMaterial(m_Material);

    m_MapDoc.UpdateAllObservers_Modified(m_Objects, MEMD_SURFACE_INFO_CHANGED);

    m_Done=true;
    return true;
}


void CommandApplyMaterialT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    for (unsigned int i = 0; i < m_Brushes.Size(); i++)
    {
        MapBrushT*        Brush = m_Brushes[i];
        ArrayT<MapFaceT>& Faces = Brush->GetFaces();

        for (unsigned int FaceNr = 0; FaceNr < Faces.Size(); FaceNr++)
            Faces[FaceNr].SetMaterial(m_OldBrushMats[i][FaceNr]);
    }

    for (unsigned int i = 0; i < m_BezierPatches.Size(); i++)
        m_BezierPatches[i]->SetMaterial(m_OldBezierPatchMats[i]);

    for (unsigned int i = 0; i < m_Terrains.Size(); i++)
        m_Terrains[i]->SetMaterial(m_OldTerrainMats[i]);

    m_MapDoc.UpdateAllObservers_Modified(m_Objects, MEMD_SURFACE_INFO_CHANGED);

    m_Done=false;
}


wxString CommandApplyMaterialT::GetName() const
{
    return "Apply material";
}
