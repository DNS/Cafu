/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#include "ToolNewEntity.hpp"
#include "Camera.hpp"
#include "CursorMan.hpp"
#include "CommandHistory.hpp"
#include "GameConfig.hpp"
#include "MapBrush.hpp"
#include "MapDocument.hpp"
#include "ChildFrame.hpp"
#include "ChildFrameViewWin2D.hpp"
#include "ChildFrameViewWin3D.hpp"
#include "ToolManager.hpp"
#include "ToolOptionsBars.hpp"
#include "MapCommands/NewEntity.hpp"


/*** Begin of TypeSys related definitions for this class. ***/

void* ToolNewEntityT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    const ToolCreateParamsT* TCPs=static_cast<const ToolCreateParamsT*>(&Params);

    return new ToolNewEntityT(TCPs->MapDoc, TCPs->ToolMan, TCPs->ParentOptionsBar);
}

const cf::TypeSys::TypeInfoT ToolNewEntityT::TypeInfo(GetToolTIM(), "ToolNewEntityT", "ToolT", ToolNewEntityT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


ToolNewEntityT::ToolNewEntityT(MapDocumentT& MapDoc, ToolManagerT& ToolMan, wxWindow* ParentOptionsBar, bool CreateOptionsBar)
    : ToolT(MapDoc, ToolMan),
      m_OptionsBar(CreateOptionsBar ? new OptionsBar_NewEntityToolT(ParentOptionsBar, MapDoc) : NULL)
{
}


wxWindow* ToolNewEntityT::GetOptionsBar()
{
    // Cannot define this method inline in the header file, because there the compiler
    // does not yet know that the type of m_OptionsBar is in fact related to wxWindow.
    return m_OptionsBar;
}


bool ToolNewEntityT::OnKeyDown2D(ViewWindow2DT& ViewWindow, wxKeyEvent& KE)
{
    switch (KE.GetKeyCode())
    {
        case WXK_ESCAPE:
            m_ToolMan.SetActiveTool(GetToolTIM().FindTypeInfoByName("ToolSelectionT"));
            return true;
    }

    return false;
}


bool ToolNewEntityT::OnLMouseDown2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
    const Vector3fT     WorldPos=m_MapDoc.SnapToGrid(ViewWindow.WindowToWorld(ME.GetPosition(), m_MapDoc.GetMostRecentSelBB().GetCenter().z), ME.AltDown(), -1 /*Snap all axes.*/);
    const EntityClassT* EntClass=GetNewEntClass();

    wxASSERT(EntClass);
    if (!EntClass) return true;

    m_MapDoc.GetHistory().SubmitCommand(new CommandNewEntityT(m_MapDoc, EntClass, WorldPos));

    // m_ToolMan.SetActiveTool(GetToolTIM().FindTypeInfoByName("ToolSelectionT"));
    return true;
}


bool ToolNewEntityT::OnMouseMove2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
    ViewWindow.SetCursor(CursorMan->GetCursor(CursorManT::NEW_ENTITY_TOOL));
    return true;
}


bool ToolNewEntityT::OnKeyDown3D(ViewWindow3DT& ViewWindow, wxKeyEvent& KE)
{
    switch (KE.GetKeyCode())
    {
        case WXK_ESCAPE:
            m_ToolMan.SetActiveTool(GetToolTIM().FindTypeInfoByName("ToolSelectionT"));
            return true;
    }

    return false;
}


bool ToolNewEntityT::OnLMouseDown3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME)
{
    const ArrayT<ViewWindow3DT::HitInfoT> Hits=ViewWindow.GetElementsAt(ME.GetPosition());
    if (Hits.Size()==0) return true;

    const EntityClassT* EntClass=GetNewEntClass();
    if (!EntClass) return true;

    MapBrushT* Brush=dynamic_cast<MapBrushT*>(Hits[0].Object);
    // TODO: Something different from a brush was hit. We should instantiate new entities in such cases as well!
    if (Brush==NULL) return true;

    try
    {
        const MapFaceT& HitFace =Brush->GetFaces()[Hits[0].FaceNr];
        const Plane3fT& HitPlane=HitFace.GetPlane();
        const Vector3fT HitPos  =HitPlane.GetIntersection(ViewWindow.GetCamera().Pos, ViewWindow.WindowToWorld(ME.GetPosition()), 0);

        m_MapDoc.GetHistory().SubmitCommand(new CommandNewEntityT(m_MapDoc,
            EntClass,
            HitPos,
            &HitFace.GetPlane()));
    }
    catch (const DivisionByZeroE&)
    {
        // A DivisionByZeroE is thrown when the ray from the camera position to the clicked
        // pixel is parallel to the hit faces plane. We just do nothing in this case.
    }

    return true;
}


bool ToolNewEntityT::OnMouseMove3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME)
{
    ViewWindow.SetCursor(CursorMan->GetCursor(CursorManT::NEW_ENTITY_TOOL));
    return true;
}


void ToolNewEntityT::RenderTool2D(Renderer2DT& Renderer) const
{
}


void ToolNewEntityT::RenderTool3D(Renderer3DT& Renderer) const
{
/*
    if (!IsActiveTool()) return;

    const Vector3fT Pos=...;  // Whereever the cursor hits the world.

    const float MAX_MAP_COORD=m_MapDoc.GetGameConfig()->GetMaxMapCoord();
    const float MIN_MAP_COORD=m_MapDoc.GetGameConfig()->GetMinMapCoord();

    MatSys::MeshT Mesh(MatSys::MeshT::Lines);
    Mesh.Vertices.PushBackEmpty(6);

    Mesh.Vertices[0].SetOrigin(MIN_MAP_COORD, Pos.y, Pos.z);
    Mesh.Vertices[0].SetColor(1, 0, 0);
    Mesh.Vertices[1].SetOrigin(MAX_MAP_COORD, Pos.y, Pos.z);
    Mesh.Vertices[1].SetColor(1, 0, 0);

    Mesh.Vertices[2].SetOrigin(Pos.x, MIN_MAP_COORD, Pos.z);
    Mesh.Vertices[2].SetColor(0, 1, 0);
    Mesh.Vertices[3].SetOrigin(Pos.x, MAX_MAP_COORD, Pos.z);
    Mesh.Vertices[3].SetColor(0, 1, 0);

    Mesh.Vertices[4].SetOrigin(Pos.x, Pos.y, MIN_MAP_COORD);
    Mesh.Vertices[4].SetColor(0, 0, 1);
    Mesh.Vertices[5].SetOrigin(Pos.x, Pos.y, MAX_MAP_COORD);
    Mesh.Vertices[5].SetColor(0, 0, 1);

    MatSys::Renderer->SetCurrentMaterial(Renderer.GetRMatWireframe());
    MatSys::Renderer->RenderMesh(Mesh);
*/
}


const EntityClassT* ToolNewEntityT::GetNewEntClass() const
{
    return m_MapDoc.GetGameConfig()->FindClass(m_OptionsBar->m_PointEntityChoice->GetStringSelection());
}
