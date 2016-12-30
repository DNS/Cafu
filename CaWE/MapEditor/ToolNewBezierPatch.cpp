/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ToolNewBezierPatch.hpp"
#include "CompMapEntity.hpp"
#include "ChildFrame.hpp"
#include "ChildFrameViewWin2D.hpp"
#include "ChildFrameViewWin3D.hpp"
#include "Group.hpp"
#include "MapBezierPatch.hpp"
#include "MapDocument.hpp"
#include "Renderer2D.hpp"
#include "ToolManager.hpp"
#include "ToolOptionsBars.hpp"

#include "../CursorMan.hpp"
#include "../CommandHistory.hpp"
#include "../GameConfig.hpp"

#include "Commands/AddPrim.hpp"
#include "Commands/Group_Assign.hpp"
#include "Commands/Group_New.hpp"

#include "wx/wx.h"

#undef Convex


/*** Begin of TypeSys related definitions for this class. ***/

void* ToolNewBezierPatchT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    const ToolCreateParamsT* TCPs=static_cast<const ToolCreateParamsT*>(&Params);

    return new ToolNewBezierPatchT(TCPs->MapDoc, TCPs->ToolMan, TCPs->ParentOptionsBar);
}

const cf::TypeSys::TypeInfoT ToolNewBezierPatchT::TypeInfo(GetToolTIM(), "ToolNewBezierPatchT", "ToolT", ToolNewBezierPatchT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


ToolNewBezierPatchT::ToolNewBezierPatchT(MapDocumentT& MapDoc, ToolManagerT& ToolMan, wxWindow* ParentOptionsBar)
    : ToolT(MapDoc, ToolMan),
      m_NewBPs(),
   // m_NewBPType(0),
      m_DragBegin(),
      m_DragCurrent(),
      m_OptionsBar(new OptionsBar_NewBezierPatchToolT(ParentOptionsBar, MapDoc))
{
    // TODO: OnActivate: Set Status bar:  Click and drag in a 2D view in order to create a new patch.
}


ToolNewBezierPatchT::~ToolNewBezierPatchT()
{
    DeleteNewBPs();
}


wxWindow* ToolNewBezierPatchT::GetOptionsBar()
{
    // Cannot define this method inline in the header file, because there the compiler
    // does not yet know that the type of m_OptionsBar is in fact related to wxWindow.
    return m_OptionsBar;
}


bool ToolNewBezierPatchT::OnKeyDown2D(ViewWindow2DT& ViewWindow, wxKeyEvent& KE)
{
    return OnKeyDown(ViewWindow, KE);
}


bool ToolNewBezierPatchT::OnLMouseDown2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
    const int       ThirdAxis=ViewWindow.GetAxesInfo().ThirdAxis;
    const Vector3fT WorldPos =m_MapDoc.SnapToGrid(ViewWindow.WindowToWorld(ME.GetPosition(), 0.0f), ME.AltDown(), -1 /*Snap all axes.*/);

 // ViewWindow.SetCursor(CursorMan->GetCursor(CursorManT::NEW_BEZIERPATCH_TOOL));   // Not really neeeded - cursor is already set in OnMouseMove2D().
    ViewWindow.CaptureMouse();

    // (Try to) determine good initial points for the drag rectangle.
    // Especially the initial heights are problematic - can we further improve the current strategy?
    m_DragBegin  =WorldPos;
    m_DragCurrent=WorldPos;

    m_DragBegin  [ThirdAxis]=m_MapDoc.GetMostRecentSelBB().Min[ThirdAxis];
    m_DragCurrent[ThirdAxis]=m_MapDoc.GetMostRecentSelBB().Max[ThirdAxis];

    if (fabs(m_DragBegin[ThirdAxis]-m_DragCurrent[ThirdAxis])<8.0f)
        m_DragCurrent[ThirdAxis]=m_DragBegin[ThirdAxis]+8.0f;

    // Update the new BP instance(s) according to the current drag rectangle.
    UpdateNewBPs(ViewWindow);
    m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
    return true;
}


bool ToolNewBezierPatchT::OnLMouseUp2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
    if (!ViewWindow.HasCapture()) return false;     // Drag started outside of ViewWindow.
    ViewWindow.ReleaseMouse();

    if (m_NewBPs.Size()==0) return true;

    if (m_NewBPs.Size()==1)
    {
        m_MapDoc.CompatSubmitCommand(new CommandAddPrimT(m_MapDoc, m_NewBPs[0], m_MapDoc.GetRootMapEntity(), "new bezier patch"));
    }
    else
    {
        ArrayT<CommandT*> SubCommands;

        // 1. Add the bezier patches to the world.
        ArrayT<MapElementT*>   BPsAsElems;
        ArrayT<MapPrimitiveT*> BPsAsPrims;

        for (unsigned long BPNr = 0; BPNr < m_NewBPs.Size(); BPNr++)
        {
            BPsAsElems.PushBack(m_NewBPs[BPNr]);
            BPsAsPrims.PushBack(m_NewBPs[BPNr]);
        }

        CommandAddPrimT* CmdAddBPs = new CommandAddPrimT(m_MapDoc, BPsAsPrims, m_MapDoc.GetRootMapEntity(), "new bezier patch parts");

        CmdAddBPs->Do();
        SubCommands.PushBack(CmdAddBPs);

        // 2. Create a new group.
        CommandNewGroupT* CmdNewGroup = new CommandNewGroupT(m_MapDoc, wxString::Format("bezier patch (%lu parts)", m_NewBPs.Size()));
        GroupT*              NewGroup = CmdNewGroup->GetGroup();

        NewGroup->SelectAsGroup = true;

        CmdNewGroup->Do();
        SubCommands.PushBack(CmdNewGroup);

        // 3. Put the BPsAsElems into the new group.
        CommandAssignGroupT* CmdAssign = new CommandAssignGroupT(m_MapDoc, BPsAsElems, NewGroup);

        CmdAssign->Do();
        SubCommands.PushBack(CmdAssign);

        // 4. Submit the composite macro command.
        m_MapDoc.CompatSubmitCommand(new CommandMacroT(SubCommands, "new bezier patch"));
    }

    // Instances are now "owned" by the command.
    for (unsigned long BPNr=0; BPNr<m_NewBPs.Size(); BPNr++) m_NewBPs[BPNr]=NULL;
    m_NewBPs.Overwrite();

    m_ToolMan.UpdateAllObservers(this, UPDATE_SOON);
    return true;
}


bool ToolNewBezierPatchT::OnMouseMove2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
    const int HorzAxis=ViewWindow.GetAxesInfo().HorzAxis;
    const int VertAxis=ViewWindow.GetAxesInfo().VertAxis;

    ViewWindow.SetCursor(CursorMan->GetCursor(CursorManT::NEW_BEZIERPATCH_TOOL));
    if (m_NewBPs.Size()==0) return true;

    const Vector3fT WorldPos=m_MapDoc.SnapToGrid(ViewWindow.WindowToWorld(ME.GetPosition(), 0.0f), ME.AltDown(), -1 /*Snap all axes.*/);
    const Vector3fT OldPos  =m_DragCurrent;

    // Update the drag rectangle.
    m_DragCurrent[HorzAxis]=WorldPos[HorzAxis];
    m_DragCurrent[VertAxis]=WorldPos[VertAxis];

    if (m_DragCurrent==OldPos) return true;

    // Update the new BP instance(s) according to the current drag rectangle.
    UpdateNewBPs(ViewWindow);

    m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
    return true;
}


bool ToolNewBezierPatchT::OnKeyDown3D(ViewWindow3DT& ViewWindow, wxKeyEvent& KE)
{
    return OnKeyDown(ViewWindow, KE);
}


bool ToolNewBezierPatchT::OnLMouseDown3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME)
{
    return false;
}


bool ToolNewBezierPatchT::OnMouseMove3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME)
{
    ViewWindow.SetCursor(CursorMan->GetCursor(CursorManT::NEW_BEZIERPATCH_TOOL));
    return true;
}


void ToolNewBezierPatchT::RenderTool2D(Renderer2DT& Renderer) const
{
    if (!IsActiveTool()) return;
    if (m_NewBPs.Size()==0) return;

    const ViewWindow2DT& ViewWin=Renderer.GetViewWin2D();
    BoundingBox3fT       BB;

    for (unsigned long BPNr=0; BPNr<m_NewBPs.Size(); BPNr++)
        BB.InsertValid(m_NewBPs[BPNr]->GetBB());

    if (!BB.IsInited()) return;

    Renderer.SetLineColor(wxColor(128, 128, 0));    // I want "dirty yellow".
    Renderer.Rectangle(wxRect(ViewWin.WorldToTool(BB.Min), ViewWin.WorldToTool(BB.Max)), false);

    for (unsigned long BPNr=0; BPNr<m_NewBPs.Size(); BPNr++)
    {
        MapBezierPatchT* BP         =m_NewBPs[BPNr];
        const bool       WasSelected=BP->IsSelected();

        BP->SetSelected(true);
        BP->Render2D(Renderer);
        BP->SetSelected(WasSelected);
    }

    Renderer.DrawBoxDims(BB, wxRIGHT | wxTOP);
}


void ToolNewBezierPatchT::RenderTool3D(Renderer3DT& Renderer) const
{
    if (!IsActiveTool()) return;
    if (m_NewBPs.Size()==0) return;

    for (unsigned long BPNr=0; BPNr<m_NewBPs.Size(); BPNr++)
    {
        MapBezierPatchT* BP         =m_NewBPs[BPNr];
        const bool       WasSelected=BP->IsSelected();

        BP->SetSelected(true);
        BP->Render3D_Basic(Renderer.GetRMatWireframe_OffsetZ(), *wxRED, 255);
        BP->SetSelected(WasSelected);
    }
}


bool ToolNewBezierPatchT::OnKeyDown(ViewWindowT& ViewWindow, wxKeyEvent& KE)
{
    switch (KE.GetKeyCode())
    {
        case WXK_ESCAPE:
            if (m_NewBPs.Size()>0)
            {
                // Abort the dragging (while the mouse button is still down).
                DeleteNewBPs();
            }
            else
            {
                m_ToolMan.SetActiveTool(GetToolTIM().FindTypeInfoByName("ToolSelectionT"));
            }
            return true;
    }

    return false;
}


void ToolNewBezierPatchT::UpdateNewBPs(ViewWindow2DT& ViewWindow)
{
    const int HorzAxis=ViewWindow.GetAxesInfo().HorzAxis;
    const int VertAxis=ViewWindow.GetAxesInfo().VertAxis;

    Vector3fT Drag=m_DragCurrent-m_DragBegin;

    // If they have not yet made a choice, make one for them now.
    if (Drag[HorzAxis]==0.0f) Drag[HorzAxis]=ViewWindow.GetAxesInfo().MirrorHorz ? -1.0f : 1.0f;
    if (Drag[VertAxis]==0.0f) Drag[VertAxis]=ViewWindow.GetAxesInfo().MirrorVert ? -1.0f : 1.0f;

    // Make sure that the drag is large enough in the chosen direction.
    if (fabs(Drag.x)<8.0f) Drag.x=(Drag.x<0.0f) ? -8.0f : 8.0f;
    if (fabs(Drag.y)<8.0f) Drag.y=(Drag.y<0.0f) ? -8.0f : 8.0f;
    if (fabs(Drag.z)<8.0f) Drag.z=(Drag.z<0.0f) ? -8.0f : 8.0f;

    EditorMaterialI*              Material   =m_MapDoc.GetGameConfig()->GetMatMan().GetDefaultMaterial();
    cf::SceneGraph::LightMapManT& LMM        =m_MapDoc.GetLightMapMan();
    const unsigned long           PatchResX  =m_OptionsBar->GetPatchResX();
    const unsigned long           PatchResY  =m_OptionsBar->GetPatchResY();
    const int                     SubdivsHorz=m_OptionsBar->m_SpinCtrlSubdivsHorz->GetValue();
    const int                     SubdivsVert=m_OptionsBar->m_SpinCtrlSubdivsVert->GetValue();
    const bool                    Convex     =m_OptionsBar->WithConvexEndCaps();
    const bool                    Concave    =m_OptionsBar->WithConcaveEndCaps();
    const BoundingBox3fT          PatchBB    =BoundingBox3fT(m_DragBegin, m_DragBegin+Drag);

    DeleteNewBPs();

    const float x_step=(PatchBB.Max.x-PatchBB.Min.x)/2;
    const float y_step=(PatchBB.Max.y-PatchBB.Min.y)/2;
    const float z_step=(PatchBB.Max.z-PatchBB.Min.z)/2;

    switch (m_OptionsBar->m_ChoicePatchType->GetSelection())
    {
        case 0:
            m_NewBPs.PushBack(MapBezierPatchT::CreateSimplePatch(Material, LMM, PatchResX, PatchResY, PatchBB.Min, PatchBB.Max, SubdivsHorz, SubdivsVert));
            break;

        case 1:
            m_NewBPs.PushBack(MapBezierPatchT::CreatePatchCylinder(Material, LMM, PatchResY, PatchBB.Min, PatchBB.Max, SubdivsHorz, SubdivsVert));
            if (!Convex && !Concave) break;

            for (int j=0; j<2; j++) // Loop over top and bottom endcaps.
            {
                // Loop over 4 quarter endcaps that build a whole cylinder endcap.
                for (MapBezierPatchT::EndCapPosE pos=MapBezierPatchT::TOP_RIGHT; pos<=MapBezierPatchT::BOTTOM_LEFT; pos=MapBezierPatchT::EndCapPosE(pos+1))
                {
                    // Reset values for each endcap.
                    Vector3fT  tmp_min=PatchBB.Min;
                    Vector3fT  tmp_max=PatchBB.Max;
                    if (j==1) tmp_max.z=PatchBB.Min.z; // For bottom endcaps set max z (endcaps are always created at max.z) to minimal value.

                    if (pos==MapBezierPatchT::TOP_RIGHT)    { tmp_min.x=PatchBB.Min.x+x_step; tmp_min.y=PatchBB.Min.y+y_step; }
                    if (pos==MapBezierPatchT::TOP_LEFT)     { tmp_max.x=PatchBB.Min.x+x_step; tmp_min.y=PatchBB.Min.y+y_step; }
                    if (pos==MapBezierPatchT::BOTTOM_RIGHT) { tmp_min.x=PatchBB.Min.x+x_step; tmp_max.y=PatchBB.Min.y+y_step; }
                    if (pos==MapBezierPatchT::BOTTOM_LEFT)  { tmp_max.x=PatchBB.Min.x+x_step; tmp_max.y=PatchBB.Min.y+y_step; }

                    if (Convex)
                    {
                        MapBezierPatchT* EndCap=MapBezierPatchT::CreateQuarterDisc(Material, LMM, tmp_min, tmp_max, SubdivsHorz, SubdivsVert, pos);
                        if (j==1) EndCap->InvertPatch(); // Invert bottom endcaps, so their faces are visible to the outside.
                        m_NewBPs.PushBack(EndCap);
                    }

                    if (Concave)
                    {
                        MapBezierPatchT* EndCap=MapBezierPatchT::CreateConcaveEndcap(Material, LMM, tmp_min, tmp_max, SubdivsHorz, SubdivsVert, pos);
                        if (j==0) EndCap->InvertPatch(); // Invert top endcaps, so their faces are oriented to the bottom.
                        m_NewBPs.PushBack(EndCap);
                    }
                }
            }
            break;

        case 2:
            m_NewBPs.PushBack(MapBezierPatchT::CreateSquareCylinder(Material, LMM, PatchResY, PatchBB.Min, PatchBB.Max, SubdivsHorz, SubdivsVert));

            if (Convex) // Note: a square cylinder can only have Convex endcaps.
            {
                Vector3fT  tmp_min=PatchBB.Min;
                Vector3fT  tmp_max=PatchBB.Max;
                // Set minimal z-value to max, so patch will be created on top of the open box.
                tmp_min.z=PatchBB.Max.z;

                m_NewBPs.PushBack(MapBezierPatchT::CreateSimplePatch(Material, LMM, 3, 3, tmp_min, tmp_max, SubdivsHorz, SubdivsVert));

                // Set z-value to minimum for bottom endcap.
                tmp_min.z=PatchBB.Min.z;
                tmp_max.z=PatchBB.Min.z;
                MapBezierPatchT* EndCap=MapBezierPatchT::CreateSimplePatch(Material, LMM, 3, 3, tmp_min, tmp_max, SubdivsHorz, SubdivsVert);
                EndCap->InvertPatch(); // Invert bottom endcap, so its face is visible to the outside.
                m_NewBPs.PushBack(EndCap);
            }
            break;

        case 3:
            m_NewBPs.PushBack(MapBezierPatchT::CreateHalfCylinder(Material, LMM, PatchResY, PatchBB.Min, PatchBB.Max, SubdivsHorz, SubdivsVert));
            if (!Convex && !Concave) break;

            for (int j=0; j<2; j++) // Loop over top and bottom endcaps.
            {
                // Loop over 2 quarter endcaps that build a half cylinder endcap.
                for (MapBezierPatchT::EndCapPosE pos=MapBezierPatchT::BOTTOM_RIGHT; pos<=MapBezierPatchT::BOTTOM_LEFT; pos=MapBezierPatchT::EndCapPosE(pos+1))
                {
                    // Reset values for each endcap.
                    Vector3fT  tmp_min=PatchBB.Min;
                    Vector3fT  tmp_max=PatchBB.Max;
                    if (j==1) tmp_max.z=PatchBB.Min.z; // For bottom endcaps set max to minimal value.

                    if (pos==MapBezierPatchT::BOTTOM_RIGHT) tmp_min.x=PatchBB.Min.x+x_step;
                    pos=MapBezierPatchT::BOTTOM_RIGHT;
                    if (pos==MapBezierPatchT::BOTTOM_LEFT)  tmp_max.x=PatchBB.Min.x+x_step;
                    pos=MapBezierPatchT::BOTTOM_LEFT;

                    if (Convex)
                    {
                        MapBezierPatchT* EndCap=MapBezierPatchT::CreateQuarterDisc(Material, LMM, tmp_min, tmp_max, SubdivsHorz, SubdivsVert, pos);
                        if (j==1) EndCap->InvertPatch(); // Invert bottom endcaps, so their faces are visible to the outside.
                        m_NewBPs.PushBack(EndCap);
                    }

                    if (Concave)
                    {
                        MapBezierPatchT* EndCap=MapBezierPatchT::CreateConcaveEndcap(Material, LMM, tmp_min, tmp_max, SubdivsHorz, SubdivsVert, pos);
                        if (j==0) EndCap->InvertPatch(); // Invert top endcaps, so their faces are oriented to the bottom.
                        m_NewBPs.PushBack(EndCap);
                    }
                }
            }
            break;

        case 4:
            m_NewBPs.PushBack(MapBezierPatchT::CreateQuarterCylinder(Material, LMM, PatchResY, PatchBB.Min, PatchBB.Max, SubdivsHorz, SubdivsVert));

            if (Convex) // Convex part.
            {
                Vector3fT  tmp_min=PatchBB.Min;
                Vector3fT  tmp_max=PatchBB.Max;

                m_NewBPs.PushBack(MapBezierPatchT::CreateQuarterDisc(Material, LMM, tmp_min, tmp_max, SubdivsHorz, SubdivsVert, MapBezierPatchT::BOTTOM_RIGHT));

                // Endcaps are always created at max.z, so max.z has to be min.z to create bottom endcap.
                tmp_max.z=PatchBB.Min.z;

                MapBezierPatchT* EndCap=MapBezierPatchT::CreateQuarterDisc(Material, LMM, tmp_min, tmp_max, SubdivsHorz, SubdivsVert, MapBezierPatchT::BOTTOM_RIGHT);
                EndCap->InvertPatch(); // Invert bottom endcap, so its face is visible to the outside.
                m_NewBPs.PushBack(EndCap);
            }

            if (Concave) // Concave part.
            {
                Vector3fT  tmp_min=PatchBB.Min;
                Vector3fT  tmp_max=PatchBB.Max;
                tmp_max.z=PatchBB.Max.z;

                MapBezierPatchT* EndCap=MapBezierPatchT::CreateConcaveEndcap(Material, LMM, tmp_min, tmp_max, SubdivsHorz, SubdivsVert, MapBezierPatchT::BOTTOM_RIGHT);
                EndCap->InvertPatch(); // Invert top endcap.
                m_NewBPs.PushBack(EndCap);

                // Endcaps are always created at max.z, so max.z has to be min.z to create bottom endcap.
                tmp_max.z=PatchBB.Min.z;

                m_NewBPs.PushBack(MapBezierPatchT::CreateConcaveEndcap(Material, LMM, tmp_min, tmp_max, SubdivsHorz, SubdivsVert, MapBezierPatchT::BOTTOM_RIGHT));
            }
            break;

        case 5:
            m_NewBPs.PushBack(MapBezierPatchT::CreateEdgePipe(Material, LMM, PatchBB.Min, PatchBB.Max, SubdivsHorz, SubdivsVert));
            if (!Convex && !Concave) break;

            // Loop over 4 quarter endcaps that build a cylinder endcap.
            for (MapBezierPatchT::EndCapPosE pos=MapBezierPatchT::TOP_RIGHT; pos<=MapBezierPatchT::BOTTOM_LEFT; pos=MapBezierPatchT::EndCapPosE((pos+1)))
            {
                // Reset values for each endcap.
                Vector3fT  tmp_min=PatchBB.Min;
                Vector3fT  tmp_max=PatchBB.Max;
                tmp_max.z=PatchBB.Min.z; // Set max z to minimal value, so endcaps are created at minimal z.

                Vector3fT tmp_vec; // Variable to temporarily store a CVs vector to manipulate it.

                // Create the bottom endcap first.
                if (pos==MapBezierPatchT::TOP_RIGHT)    { tmp_min.x=PatchBB.Min.x+x_step/2; tmp_min.y=PatchBB.Min.y+y_step; tmp_max.x=PatchBB.Min.x+x_step; }
                if (pos==MapBezierPatchT::TOP_LEFT)     { tmp_max.x=PatchBB.Min.x+x_step/2; tmp_min.y=PatchBB.Min.y+y_step;                         }
                if (pos==MapBezierPatchT::BOTTOM_RIGHT) { tmp_min.x=PatchBB.Min.x+x_step/2; tmp_max.y=PatchBB.Min.y+y_step; tmp_max.x=PatchBB.Min.x+x_step; }
                if (pos==MapBezierPatchT::BOTTOM_LEFT)  { tmp_max.x=PatchBB.Min.x+x_step/2; tmp_max.y=PatchBB.Min.y+y_step;                         }

                if (Convex)
                {
                    MapBezierPatchT* EndCap=MapBezierPatchT::CreateQuarterDisc(Material, LMM, tmp_min, tmp_max, SubdivsHorz, SubdivsVert, pos);
                    EndCap->InvertPatch(); // Invert convex endcaps, so their faces are visible to the outside.
                    m_NewBPs.PushBack(EndCap);
                }

                if (Concave)
                {
                    MapBezierPatchT* EndCap=MapBezierPatchT::CreateConcaveEndcap(Material, LMM, tmp_min, tmp_max, SubdivsHorz, SubdivsVert, pos);
                    m_NewBPs.PushBack(EndCap);
                }

                // Create second endcap and rotate it by 90 degree around the y axis.
                // Reset values for second endcap.
                tmp_min=PatchBB.Min;
                tmp_max=PatchBB.Max;
                tmp_max.z=PatchBB.Max.z-z_step/2; // Set new z position for endcap.

                // Note: The position of the second endcap in choosen in a way, so it will fit exactely on the second opening
                // of the edge pipe, when it is are rotated by 90 degree around the y axis.
                if (pos==MapBezierPatchT::TOP_RIGHT)    { tmp_min.x=PatchBB.Max.x;          tmp_min.y=PatchBB.Min.y+y_step; tmp_max.x=PatchBB.Max.x+z_step/2; }
                if (pos==MapBezierPatchT::TOP_LEFT)     { tmp_min.x=PatchBB.Max.x-z_step/2; tmp_min.y=PatchBB.Min.y+y_step;                           }
                if (pos==MapBezierPatchT::BOTTOM_RIGHT) { tmp_min.x=PatchBB.Max.x;          tmp_max.y=PatchBB.Min.y+y_step; tmp_max.x=PatchBB.Max.x+z_step/2; }
                if (pos==MapBezierPatchT::BOTTOM_LEFT)  { tmp_min.x=PatchBB.Max.x-z_step/2; tmp_max.y=PatchBB.Min.y+y_step;                           }

                if (Convex)
                {
                    MapBezierPatchT* EndCap=MapBezierPatchT::CreateQuarterDisc(Material, LMM, tmp_min, tmp_max, SubdivsHorz, SubdivsVert, pos);
                    EndCap->InvertPatch(); // Invert convex endcaps, so their faces are visible to the outside.

                    // Rotate the created patch around the y axis.
                    for (unsigned long y=0; y<3; y++)
                    {
                        for (unsigned long x=0; x<3; x++)
                        {
                            tmp_vec=EndCap->GetCvPos(x, y);
                            if (pos==MapBezierPatchT::TOP_LEFT || pos==MapBezierPatchT::BOTTOM_LEFT)
                            {
                                tmp_vec.z-=tmp_max.x-tmp_vec.x;
                                tmp_vec.x =tmp_max.x;
                                EndCap->SetCvPos(x, y, tmp_vec);
                            }
                            if (pos==MapBezierPatchT::TOP_RIGHT || pos==MapBezierPatchT::BOTTOM_RIGHT)
                            {
                                tmp_vec.z+=tmp_vec.x-tmp_min.x;
                                tmp_vec.x =tmp_min.x;
                                EndCap->SetCvPos(x, y, tmp_vec);
                            }
                        }
                    }

                    m_NewBPs.PushBack(EndCap);
                }

                if (Concave)
                {
                    MapBezierPatchT* EndCap=MapBezierPatchT::CreateConcaveEndcap(Material, LMM, tmp_min, tmp_max, SubdivsHorz, SubdivsVert, pos);

                    // Rotate the created patch around the y axis.
                    for (unsigned long y=0; y<3; y++)
                    {
                        for (unsigned long x=0; x<3; x++)
                        {
                            tmp_vec=EndCap->GetCvPos(x, y);
                            if (pos==MapBezierPatchT::TOP_LEFT || pos==MapBezierPatchT::BOTTOM_LEFT)
                            {
                                tmp_vec.z-=tmp_max.x-tmp_vec.x;
                                tmp_vec.x =tmp_max.x;
                                EndCap->SetCvPos(x, y, tmp_vec);
                            }
                            if (pos==MapBezierPatchT::TOP_RIGHT || pos==MapBezierPatchT::BOTTOM_RIGHT)
                            {
                                tmp_vec.z+=tmp_vec.x-tmp_min.x;
                                tmp_vec.x =tmp_min.x;
                                EndCap->SetCvPos(x, y, tmp_vec);
                            }
                        }
                    }

                    m_NewBPs.PushBack(EndCap);
                }
            }
            break;

        case 6:
            m_NewBPs.PushBack(MapBezierPatchT::CreateCone(Material, LMM, PatchResY, PatchBB.Min, PatchBB.Max, SubdivsHorz, SubdivsVert));

            // Loop over 4 quarter endcaps that build the cone endcap.
            for (MapBezierPatchT::EndCapPosE pos=MapBezierPatchT::TOP_RIGHT; pos<=MapBezierPatchT::BOTTOM_LEFT; pos=MapBezierPatchT::EndCapPosE(pos+1))
            {
                // Reset values for each endcap.
                Vector3fT  tmp_min=PatchBB.Min;
                Vector3fT  tmp_max=PatchBB.Max;
                tmp_max.z=PatchBB.Min.z; // A cone needs only a bottom endcap.

                if (pos==MapBezierPatchT::TOP_RIGHT)    { tmp_min.x=PatchBB.Min.x+x_step; tmp_min.y=PatchBB.Min.y+y_step; }
                if (pos==MapBezierPatchT::TOP_LEFT)     { tmp_max.x=PatchBB.Min.x+x_step; tmp_min.y=PatchBB.Min.y+y_step; }
                if (pos==MapBezierPatchT::BOTTOM_RIGHT) { tmp_min.x=PatchBB.Min.x+x_step; tmp_max.y=PatchBB.Min.y+y_step; }
                if (pos==MapBezierPatchT::BOTTOM_LEFT)  { tmp_max.x=PatchBB.Min.x+x_step; tmp_max.y=PatchBB.Min.y+y_step; }

                if (Convex)
                {
                    MapBezierPatchT* EndCap=MapBezierPatchT::CreateQuarterDisc(Material, LMM, tmp_min, tmp_max, SubdivsHorz, SubdivsVert, pos);
                    EndCap->InvertPatch(); // Invert bottom endcaps, so their faces are visible to the outside.
                    m_NewBPs.PushBack(EndCap);
                }

                if (Concave)
                {
                    MapBezierPatchT* EndCap=MapBezierPatchT::CreateConcaveEndcap(Material, LMM, tmp_min, tmp_max, SubdivsHorz, SubdivsVert, pos);
                    m_NewBPs.PushBack(EndCap);
                }
            }
            break;

        case 7:
            m_NewBPs.PushBack(MapBezierPatchT::CreateSphere(Material, LMM, PatchBB.Min, PatchBB.Max, SubdivsHorz, SubdivsVert));
            break;

        case 8:
            m_NewBPs.PushBack(MapBezierPatchT::CreateQuarterDisc(Material, LMM, PatchBB.Min, PatchBB.Max, SubdivsHorz, SubdivsVert));
            break;

        case 9:
            m_NewBPs.PushBack(MapBezierPatchT::CreateConcaveEndcap(Material, LMM, PatchBB.Min, PatchBB.Max, SubdivsHorz, SubdivsVert));
            break;
    }
}


void ToolNewBezierPatchT::DeleteNewBPs()
{
    for (unsigned long BPNr=0; BPNr<m_NewBPs.Size(); BPNr++)
    {
        delete m_NewBPs[BPNr];
        m_NewBPs[BPNr]=NULL;
    }

    m_NewBPs.Overwrite();
}
