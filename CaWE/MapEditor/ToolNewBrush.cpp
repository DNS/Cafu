/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ToolNewBrush.hpp"
#include "CompMapEntity.hpp"
#include "ChildFrame.hpp"
#include "ChildFrameViewWin2D.hpp"
#include "ChildFrameViewWin3D.hpp"
#include "Group.hpp"
#include "MapBrush.hpp"
#include "MapDocument.hpp"
#include "Renderer2D.hpp"
#include "ToolManager.hpp"
#include "ToolOptionsBars.hpp"
#include "DialogCreateArch.hpp"

#include "../CursorMan.hpp"
#include "../CommandHistory.hpp"
#include "../GameConfig.hpp"

#include "Commands/AddPrim.hpp"
#include "Commands/Group_Assign.hpp"
#include "Commands/Group_New.hpp"

#include "wx/wx.h"


/*** Begin of TypeSys related definitions for this class. ***/

void* ToolNewBrushT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    const ToolCreateParamsT* TCPs=static_cast<const ToolCreateParamsT*>(&Params);

    return new ToolNewBrushT(TCPs->MapDoc, TCPs->ToolMan, TCPs->ParentOptionsBar);
}

const cf::TypeSys::TypeInfoT ToolNewBrushT::TypeInfo(GetToolTIM(), "ToolNewBrushT", "ToolT", ToolNewBrushT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


ToolNewBrushT::ToolNewBrushT(MapDocumentT& MapDoc, ToolManagerT& ToolMan, wxWindow* ParentOptionsBar)
    : ToolT(MapDoc, ToolMan),
      m_NewBrush(NULL),
      m_NewBrushType(0),
      m_DragBegin(),
      m_DragCurrent(),
      m_OptionsBar(new OptionsBar_NewBrushToolT(ParentOptionsBar))
{
    // TODO: OnActivate: Set Status bar:  Click and drag in a 2D view in order to create a new brush.
}


ToolNewBrushT::~ToolNewBrushT()
{
    delete m_NewBrush;
    m_NewBrush=NULL;
}


wxWindow* ToolNewBrushT::GetOptionsBar()
{
    // Cannot define this method inline in the header file, because there the compiler
    // does not yet know that the type of m_OptionsBar is in fact related to wxWindow.
    return m_OptionsBar;
}


bool ToolNewBrushT::OnKeyDown2D(ViewWindow2DT& ViewWindow, wxKeyEvent& KE)
{
    return OnKeyDown(ViewWindow, KE);
}


bool ToolNewBrushT::OnLMouseDown2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
    const int       ThirdAxis=ViewWindow.GetAxesInfo().ThirdAxis;
    const Vector3fT WorldPos =m_MapDoc.SnapToGrid(ViewWindow.WindowToWorld(ME.GetPosition(), 0.0f), ME.AltDown(), -1 /*Snap all axes.*/);

 // ViewWindow.SetCursor(CursorMan->GetCursor(CursorManT::NEW_BRUSH_TOOL));     // Not really neeeded - cursor is already set in OnMouseMove2D().
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


bool ToolNewBrushT::OnLMouseUp2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
    if (!ViewWindow.HasCapture()) return false;     // Drag started outside of ViewWindow.
    ViewWindow.ReleaseMouse();

    if (!m_NewBrush) return true;                   // Something went wrong - user has to try again.

    if (m_NewBrushType!=5)
    {
        // It's a normal brush, not an arch.
        const char* CmdDescr="";    // Describes the command that is submitted to the command history for actually adding m_NewBrush to the world.

        switch (m_NewBrushType)
        {
            case  0: CmdDescr="new block brush";    break;
            case  1: CmdDescr="new wedge brush";    break;
            case  2: CmdDescr="new cylinder brush"; break;
            case  3: CmdDescr="new pyramid brush";  break;
            case  4: CmdDescr="new sphere brush";   break;
        }

        m_MapDoc.CompatSubmitCommand(new CommandAddPrimT(m_MapDoc, m_NewBrush, m_MapDoc.GetRootMapEntity(), CmdDescr));
        m_NewBrush=NULL;    // Instance is now "owned" by the command.
    }
    else
    {
        // It's an arch.
        ArchDialogT ArchDlg(m_NewBrush->GetBB(), ViewWindow.GetAxesInfo());

        // We *must* delete the brush before ArchDlg is shown - event processing continues (e.g. incoming mouse move events)!
        delete m_NewBrush;
        m_NewBrush = NULL;

        if (ArchDlg.ShowModal() == wxID_OK)
        {
            const ArrayT<MapPrimitiveT*> ArchSegments = ArchDlg.GetArch(m_MapDoc.GetGameConfig()->GetMatMan().GetDefaultMaterial());

            ArrayT<CommandT*> SubCommands;

            // 1. Add the arch segments to the world.
            CommandAddPrimT* CmdAddSegments = new CommandAddPrimT(m_MapDoc, ArchSegments, m_MapDoc.GetRootMapEntity(), "new arch segments");

            CmdAddSegments->Do();
            SubCommands.PushBack(CmdAddSegments);

            // 2. Create a new group.
            CommandNewGroupT* CmdNewGroup = new CommandNewGroupT(m_MapDoc, wxString::Format("arch (%lu side%s)", ArchSegments.Size(), ArchSegments.Size()==1 ? "" : "s"));
            GroupT*              NewGroup = CmdNewGroup->GetGroup();

            NewGroup->SelectAsGroup = true;

            CmdNewGroup->Do();
            SubCommands.PushBack(CmdNewGroup);

            // 3. Put the ArchSegments into the new group.
            ArrayT<MapElementT*> ArchSegmentsAsElems;

            for (unsigned long SegNr = 0; SegNr < ArchSegments.Size(); SegNr++)
                ArchSegmentsAsElems.PushBack(ArchSegments[SegNr]);

            CommandAssignGroupT* CmdAssign = new CommandAssignGroupT(m_MapDoc, ArchSegmentsAsElems, NewGroup);

            CmdAssign->Do();
            SubCommands.PushBack(CmdAssign);

            // 4. Submit the composite macro command.
            m_MapDoc.CompatSubmitCommand(new CommandMacroT(SubCommands, "new arch"));
        }
    }

    m_ToolMan.UpdateAllObservers(this, UPDATE_SOON);
    return true;
}


bool ToolNewBrushT::OnMouseMove2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
    const int HorzAxis=ViewWindow.GetAxesInfo().HorzAxis;
    const int VertAxis=ViewWindow.GetAxesInfo().VertAxis;

    ViewWindow.SetCursor(CursorMan->GetCursor(CursorManT::NEW_BRUSH_TOOL));
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


bool ToolNewBrushT::OnKeyDown3D(ViewWindow3DT& ViewWindow, wxKeyEvent& KE)
{
    return OnKeyDown(ViewWindow, KE);
}


bool ToolNewBrushT::OnRMouseClick3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME)
{
    if (ME.ShiftDown() && ME.ControlDown())
    {
        // Create a brush in the shape of the current view frustum.
        // This has little practical relevance for the user, it is just a tool for debugging the view frustum computations.
        // It is in the RMB *up* instead of the RMB *down* handler in order to not have the context menu shown.
        ArrayT<Plane3fT> Planes;

        Planes.PushBackEmpty(6);
        ViewWindow.GetViewFrustum(&Planes[0]);

        MapBrushT* NewBrush=new MapBrushT(Planes, m_MapDoc.GetGameConfig()->GetMatMan().GetDefaultMaterial(), true);

        m_MapDoc.CompatSubmitCommand(new CommandAddPrimT(m_MapDoc, NewBrush, m_MapDoc.GetRootMapEntity(), "new view frustum brush"));
        return true;
    }

    return false;
}


bool ToolNewBrushT::OnMouseMove3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME)
{
    ViewWindow.SetCursor(CursorMan->GetCursor(CursorManT::NEW_BRUSH_TOOL));
    return true;
}


void ToolNewBrushT::RenderTool2D(Renderer2DT& Renderer) const
{
    if (!IsActiveTool()) return;
    if (!m_NewBrush) return;

    const BoundingBox3fT BB     =m_NewBrush->GetBB();
    const ViewWindow2DT& ViewWin=Renderer.GetViewWin2D();

    Renderer.SetLineColor(wxColor(128, 128, 0));    // I want "dirty yellow".
    Renderer.Rectangle(wxRect(ViewWin.WorldToTool(BB.Min), ViewWin.WorldToTool(BB.Max)), false);

    const bool WasSelected=m_NewBrush->IsSelected();

    m_NewBrush->SetSelected(true);
    m_NewBrush->Render2D(Renderer);
    m_NewBrush->SetSelected(WasSelected);

    Renderer.DrawBoxDims(BB, wxRIGHT | wxTOP);
}


void ToolNewBrushT::RenderTool3D(Renderer3DT& Renderer) const
{
    if (!IsActiveTool()) return;
    if (!m_NewBrush) return;

    for (unsigned long FaceNr=0; FaceNr<m_NewBrush->GetFaces().Size(); FaceNr++)
        m_NewBrush->GetFaces()[FaceNr].Render3DBasic(Renderer.GetRMatWireframe_OffsetZ(), *wxRED, 255);
}


bool ToolNewBrushT::OnKeyDown(ViewWindowT& ViewWindow, wxKeyEvent& KE)
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

            m_ToolMan.UpdateAllObservers(this, UPDATE_SOON);
            return true;
    }

    return false;
}


void ToolNewBrushT::UpdateNewBrush(ViewWindow2DT& ViewWindow)
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

    EditorMaterialI*     Material =m_MapDoc.GetGameConfig()->GetMatMan().GetDefaultMaterial();
    const BoundingBox3fT BrushBB  =BoundingBox3fT(m_DragBegin, m_DragBegin+Drag);
    const unsigned long  NrOfFaces=m_OptionsBar->GetNrOfFaces();

    delete m_NewBrush;
    m_NewBrush=NULL;
    m_NewBrushType=m_OptionsBar->GetBrushIndex();

    switch (m_NewBrushType)
    {
        case  0: m_NewBrush=MapBrushT::CreateBlock   (BrushBB, Material);            break;
        case  1: m_NewBrush=MapBrushT::CreateWedge   (BrushBB, Material);            break;
        case  2: m_NewBrush=MapBrushT::CreateCylinder(BrushBB, NrOfFaces, Material); break;
        case  3: m_NewBrush=MapBrushT::CreatePyramid (BrushBB, NrOfFaces, Material); break;
        case  4: m_NewBrush=MapBrushT::CreateSphere  (BrushBB, NrOfFaces, Material); break;
        default: m_NewBrush=MapBrushT::CreateBlock   (BrushBB, Material);            break;
    }
}
