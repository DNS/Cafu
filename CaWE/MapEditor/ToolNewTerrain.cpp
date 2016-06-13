/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ToolNewTerrain.hpp"
#include "CompMapEntity.hpp"
#include "ChildFrame.hpp"
#include "ChildFrameViewWin2D.hpp"
#include "ChildFrameViewWin3D.hpp"
#include "MapBrush.hpp"
#include "MapTerrain.hpp"
#include "MapDocument.hpp"
#include "Renderer2D.hpp"
#include "ToolManager.hpp"
#include "ToolOptionsBars.hpp"
#include "Commands/AddPrim.hpp"

#include "../CommandHistory.hpp"
#include "../CursorMan.hpp"
#include "../DocumentAdapter.hpp"
#include "../EditorMaterial.hpp"
#include "../GameConfig.hpp"

#include "../MaterialBrowser/MaterialBrowserDialog.hpp"

#include "MaterialSystem/Material.hpp"


/*** Begin of TypeSys related definitions for this class. ***/

void* ToolNewTerrainT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    const ToolCreateParamsT* TCPs=static_cast<const ToolCreateParamsT*>(&Params);

    return new ToolNewTerrainT(TCPs->MapDoc, TCPs->ToolMan, TCPs->ParentOptionsBar);
}

const cf::TypeSys::TypeInfoT ToolNewTerrainT::TypeInfo(GetToolTIM(), "ToolNewTerrainT", "ToolT", ToolNewTerrainT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


ToolNewTerrainT::ToolNewTerrainT(MapDocumentT& MapDoc, ToolManagerT& ToolMan, wxWindow* ParentOptionsBar)
    : ToolT(MapDoc, ToolMan),
      m_NewBrush(NULL),
      m_DragBegin(),
      m_DragCurrent(),
      m_OptionsBar(new OptionsBar_NewTerrainToolT(ParentOptionsBar, MapDoc))
{
    // TODO: OnActivate: Set Status bar:  Click and drag in a 2D view in order to create a new brush.
}


ToolNewTerrainT::~ToolNewTerrainT()
{
    delete m_NewBrush;
    m_NewBrush=NULL;
}


wxWindow* ToolNewTerrainT::GetOptionsBar()
{
    // Cannot define this method inline in the header file, because there the compiler
    // does not yet know that the type of m_OptionsBar is in fact related to wxWindow.
    return m_OptionsBar;
}


bool ToolNewTerrainT::OnKeyDown2D(ViewWindow2DT& ViewWindow, wxKeyEvent& KE)
{
    return OnKeyDown(ViewWindow, KE);
}


bool ToolNewTerrainT::OnLMouseDown2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
    const int       ThirdAxis=ViewWindow.GetAxesInfo().ThirdAxis;
    const Vector3fT WorldPos =m_MapDoc.SnapToGrid(ViewWindow.WindowToWorld(ME.GetPosition(), 0.0f), ME.AltDown(), -1 /*Snap all axes.*/);

 // ViewWindow.SetCursor(CursorMan->GetCursor(CursorManT::NEW_TERAIN_TOOL));     // Not really neeeded - cursor is already set in OnMouseMove2D().
    ViewWindow.CaptureMouse();

    // (Try to) determine good initial points for the drag rectangle.
    // Especially the initial heights are problematic - can we further improve the current strategy?
    m_DragBegin  =WorldPos;
    m_DragCurrent=WorldPos;

    m_DragBegin  [ThirdAxis]=m_MapDoc.GetMostRecentSelBB().Min[ThirdAxis];
    m_DragCurrent[ThirdAxis]=m_MapDoc.GetMostRecentSelBB().Max[ThirdAxis];

    if (fabs(m_DragBegin[ThirdAxis]-m_DragCurrent[ThirdAxis])<8.0f)
        m_DragCurrent[ThirdAxis]=m_DragBegin[ThirdAxis]+8.0f;

    // Update the new brush instance according to the current drag rectangle.
    UpdateNewBrush(ViewWindow);
    m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
    return true;
}


bool ToolNewTerrainT::OnLMouseUp2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
    if (!ViewWindow.HasCapture()) return false;     // Drag started outside of ViewWindow.
    ViewWindow.ReleaseMouse();

    if (!m_NewBrush) return true;                   // Something went wrong - user has to try again.

    EditorMaterialI*       TerrainMat=m_MapDoc.GetGameConfig()->GetMatMan().GetDefaultMaterial();
    BoundingBox3fT         TerrainBB =m_NewBrush->GetBB();
    ArrayT<MapPrimitiveT*> TerrainPrims;

    // Make sure that TerrainBB meets a minimum size.
    TerrainBB.Insert(TerrainBB.Min+Vector3fT(64.0f, 64.0f, 32.0f));

    // We *must* delete the brush before the dialogs are shown - event processing continues (e.g. incoming mouse move events)!
    delete m_NewBrush;
    m_NewBrush=NULL;

    if (TerrainMat->GetMaterial()->AmbientShaderName!="A_Terrain")
    {
        if (wxMessageBox("Please select a material for the new terrain", "Select terrain material", wxOK | wxCANCEL)!=wxOK)
        {
            m_ToolMan.UpdateAllObservers(this, UPDATE_SOON);
            return true;
        }

        MaterialBrowser::DialogT MatBrowser(&ViewWindow, m_MapDoc.GetAdapter(), MaterialBrowser::ConfigT()
            .InitialMaterial(NULL)  // No initial material.
            .InitialNameFilter("Terrains"));

        if (MatBrowser.ShowModal()!=wxID_OK || MatBrowser.GetCurrentMaterial()==NULL)
        {
            m_ToolMan.UpdateAllObservers(this, UPDATE_SOON);
            return true;
        }

        TerrainMat=MatBrowser.GetCurrentMaterial();

        if (TerrainMat->GetMaterial()->AmbientShaderName!="A_Terrain")
        {
            wxMessageBox("The selected material doesn't employ the \"A_Terrain\" shader,\n"
                         "and thus may not work properly with terrains.\n\n"
                         "You can change the material of the new terrain at any time\n"
                         "using the \"Edit Surfaces\" tool.", "Possibly incompatible material");
        }
    }

    TerrainPrims.PushBack(new MapTerrainT(TerrainBB, m_MapDoc.GetGameConfig()->ModDir+"/"+m_OptionsBar->m_ComboBoxHeightmapName->GetValue(), TerrainMat));

    if (m_OptionsBar->m_CheckBoxAddWallsAndCeil->IsChecked())
    {
        if (wxMessageBox("You asked me to add (sky) walls and ceiling to the new terrain.\n\n"
                         "Press OK now to select the material to be applied.\n"
                         "Press Cancel to create no walls and no ceiling.", "Select material for walls and ceiling", wxOK | wxCANCEL)==wxOK)
        {
            MaterialBrowser::DialogT MatBrowser(&ViewWindow, m_MapDoc.GetAdapter(), MaterialBrowser::ConfigT()
                .InitialMaterial(NULL)  // No initial material.
                .InitialNameFilter("sky"));

            if (MatBrowser.ShowModal()==wxID_OK && MatBrowser.GetCurrentMaterial()!=NULL)
            {
                const Vector3fT  Min=TerrainBB.Min;
                const Vector3fT  Max=TerrainBB.Max;
                const float      thk=64.0f;     // Thickness of the walls.
                EditorMaterialI* WallCeilMat=MatBrowser.GetCurrentMaterial();

                TerrainPrims.PushBack(MapBrushT::CreateBlock(BoundingBox3fT(Min-Vector3fT(thk, 0, 0), Vector3fT(Min.x, Max.y, Max.z)), WallCeilMat));  // Left wall.
                TerrainPrims.PushBack(MapBrushT::CreateBlock(BoundingBox3fT(Vector3fT(Max.x, Min.y, Min.z), Max+Vector3fT(thk, 0, 0)), WallCeilMat));  // Right wall.
                TerrainPrims.PushBack(MapBrushT::CreateBlock(BoundingBox3fT(Min-Vector3fT(0, thk, 0), Vector3fT(Max.x, Min.y, Max.z)), WallCeilMat));  // Near wall.
                TerrainPrims.PushBack(MapBrushT::CreateBlock(BoundingBox3fT(Vector3fT(Min.x, Max.y, Min.z), Max+Vector3fT(0, thk, 0)), WallCeilMat));  // Far wall.
                TerrainPrims.PushBack(MapBrushT::CreateBlock(BoundingBox3fT(Vector3fT(Min.x, Min.y, Max.z), Max+Vector3fT(0, 0, thk)), WallCeilMat));  // Ceiling.
            }
        }
    }

    if (m_OptionsBar->m_CheckBoxAddFloor->IsChecked())
    {
        const Vector3fT  Min=TerrainBB.Min;
        const Vector3fT  Max=TerrainBB.Max;
        const float      thk=64.0f;     // Thickness of the floor.

        TerrainPrims.PushBack(MapBrushT::CreateBlock(BoundingBox3fT(Min-Vector3fT(0, 0, thk),  Vector3fT(Max.x, Max.y, Min.z)),
            m_MapDoc.GetGameConfig()->GetMatMan().FindMaterial("Textures/meta/caulk", true /*Create dummy if not found.*/)));
    }

    m_MapDoc.CompatSubmitCommand(new CommandAddPrimT(m_MapDoc, TerrainPrims, m_MapDoc.GetRootMapEntity(), "new terrain"));

    m_ToolMan.UpdateAllObservers(this, UPDATE_SOON);
    return true;
}


bool ToolNewTerrainT::OnMouseMove2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
    const int HorzAxis=ViewWindow.GetAxesInfo().HorzAxis;
    const int VertAxis=ViewWindow.GetAxesInfo().VertAxis;

    ViewWindow.SetCursor(CursorMan->GetCursor(CursorManT::NEW_TERAIN_TOOL));
    if (!m_NewBrush) return true;

    const Vector3fT WorldPos=m_MapDoc.SnapToGrid(ViewWindow.WindowToWorld(ME.GetPosition(), 0.0f), ME.AltDown(), -1 /*Snap all axes.*/);
    const Vector3fT OldPos  =m_DragCurrent;

    // Update the drag rectangle.
    m_DragCurrent[HorzAxis]=WorldPos[HorzAxis];
    m_DragCurrent[VertAxis]=WorldPos[VertAxis];

    if (m_DragCurrent==OldPos) return true;

    // Update the new brush instance according to the current drag rectangle.
    UpdateNewBrush(ViewWindow);

    m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
    return true;
}


bool ToolNewTerrainT::OnKeyDown3D(ViewWindow3DT& ViewWindow, wxKeyEvent& KE)
{
    return OnKeyDown(ViewWindow, KE);
}


bool ToolNewTerrainT::OnLMouseDown3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME)
{
    return false;
}


bool ToolNewTerrainT::OnMouseMove3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME)
{
    ViewWindow.SetCursor(CursorMan->GetCursor(CursorManT::NEW_TERAIN_TOOL));
    return true;
}


void ToolNewTerrainT::RenderTool2D(Renderer2DT& Renderer) const
{
    if (!IsActiveTool()) return;
    if (!m_NewBrush) return;

    const bool WasSelected=m_NewBrush->IsSelected();

    m_NewBrush->SetSelected(true);
    m_NewBrush->Render2D(Renderer);
    m_NewBrush->SetSelected(WasSelected);

    Renderer.DrawBoxDims(m_NewBrush->GetBB(), wxRIGHT | wxTOP);
}


void ToolNewTerrainT::RenderTool3D(Renderer3DT& Renderer) const
{
    if (!IsActiveTool()) return;
    if (!m_NewBrush) return;

    for (unsigned long FaceNr=0; FaceNr<m_NewBrush->GetFaces().Size(); FaceNr++)
        m_NewBrush->GetFaces()[FaceNr].Render3DBasic(Renderer.GetRMatWireframe_OffsetZ(), *wxRED, 255);
}


bool ToolNewTerrainT::OnKeyDown(ViewWindowT& ViewWindow, wxKeyEvent& KE)
{
    switch (KE.GetKeyCode())
    {
        case WXK_ESCAPE:
            if (m_NewBrush)
            {
                // Abort the dragging (while the mouse button is still down).
                delete m_NewBrush;
                m_NewBrush=NULL;
            }
            else
            {
                m_ToolMan.SetActiveTool(GetToolTIM().FindTypeInfoByName("ToolSelectionT"));
            }
            return true;
    }

    return false;
}


void ToolNewTerrainT::UpdateNewBrush(ViewWindow2DT& ViewWindow)
{
    const int HorzAxis=ViewWindow.GetAxesInfo().HorzAxis;
    const int VertAxis=ViewWindow.GetAxesInfo().VertAxis;

    Vector3fT Drag=m_DragCurrent-m_DragBegin;

    // If they have not yet made a choice, make one for them now.
    if (Drag[HorzAxis]==0.0f) Drag[HorzAxis]=ViewWindow.GetAxesInfo().MirrorHorz ? -1.0f : 1.0f;
    if (Drag[VertAxis]==0.0f) Drag[VertAxis]=ViewWindow.GetAxesInfo().MirrorVert ? -1.0f : 1.0f;

    // Make sure that the drag is large enough in the chosen direction.
    if (fabs(Drag.x)<64.0f) Drag.x=(Drag.x<0.0f) ? -64.0f : 64.0f;
    if (fabs(Drag.y)<64.0f) Drag.y=(Drag.y<0.0f) ? -64.0f : 64.0f;
    if (fabs(Drag.z)<32.0f) Drag.z=(Drag.z<0.0f) ? -32.0f : 32.0f;

    delete m_NewBrush;
    m_NewBrush=MapBrushT::CreateBlock(BoundingBox3fT(m_DragBegin, m_DragBegin+Drag), m_MapDoc.GetGameConfig()->GetMatMan().GetDefaultMaterial());
}
