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

#include "ApplyMaterial.hpp"

#include "../MapBezierPatch.hpp"
#include "../MapBrush.hpp"
#include "../MapDocument.hpp"
#include "../MapTerrain.hpp"


CommandApplyMaterialT::CommandApplyMaterialT(MapDocumentT& MapDoc, const ArrayT<MapElementT*>& Objects, EditorMaterialI* Material)
    : m_MapDoc(MapDoc),
      m_Objects(Objects),
      m_Material(Material)
{
    for (unsigned long ObjNr=0; ObjNr<m_Objects.Size(); ObjNr++)
        m_OldStates.PushBack(m_Objects[ObjNr]->Clone());
}


CommandApplyMaterialT::~CommandApplyMaterialT()
{
    for (unsigned long i=0; i<m_OldStates.Size(); i++)
        delete m_OldStates[i];

    m_OldStates.Clear();
}


bool CommandApplyMaterialT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    for (unsigned long ObjNr=0; ObjNr<m_Objects.Size(); ObjNr++)
    {
        MapElementT* Obj=m_Objects[ObjNr];

        if (Obj->GetType()==&MapBrushT::TypeInfo)
        {
            MapBrushT*        Brush=(MapBrushT*)Obj;
            ArrayT<MapFaceT>& Faces=Brush->GetFaces();

            for (unsigned long FaceNr=0; FaceNr<Faces.Size(); FaceNr++)
                Faces[FaceNr].SetMaterial(m_Material);
        }
        else if (Obj->GetType()==&MapBezierPatchT::TypeInfo)
        {
            MapBezierPatchT* BP=(MapBezierPatchT*)Obj;

            BP->SetMaterial(m_Material);
        }
        else if (Obj->GetType()==&MapTerrainT::TypeInfo)
        {
            MapTerrainT* Terrain=(MapTerrainT*)Obj;

            Terrain->SetMaterial(m_Material);
        }
    }

    m_MapDoc.UpdateAllObservers_Modified(m_Objects, MEMD_SURFACE_INFO_CHANGED);

    m_Done=true;
    return true;
}


void CommandApplyMaterialT::Undo()
{
    wxASSERT(m_Done);

    if (!m_Done) return;

    for (unsigned long ObjNr=0; ObjNr<m_Objects.Size(); ObjNr++)
        m_Objects[ObjNr]->Assign(m_OldStates[ObjNr]);

    m_MapDoc.UpdateAllObservers_Modified(m_Objects, MEMD_SURFACE_INFO_CHANGED);

    m_Done=false;
}


wxString CommandApplyMaterialT::GetName() const
{
    return "Apply material";
}
