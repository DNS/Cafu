/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ToolClip.hpp"
#include "MapDocument.hpp"
#include "MapFace.hpp"
#include "MapBrush.hpp"
#include "ChildFrame.hpp"
#include "ChildFrameViewWin2D.hpp"
#include "ChildFrameViewWin3D.hpp"
#include "Renderer2D.hpp"
#include "Renderer3D.hpp"
#include "ToolManager.hpp"
#include "ToolOptionsBars.hpp"

#include "../CommandHistory.hpp"
#include "../CursorMan.hpp"
#include "../Options.hpp"

#include "Commands/Clip.hpp"             // Includes struct ClipResultT.
#include "Commands/Group_Delete.hpp"     // For purging the groups after a clip operation.

#include "wx/wx.h"


/*** Begin of TypeSys related definitions for this class. ***/

void* ToolClipT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    const ToolCreateParamsT* TCPs=static_cast<const ToolCreateParamsT*>(&Params);

    return new ToolClipT(TCPs->MapDoc, TCPs->ToolMan, TCPs->ParentOptionsBar);
}

const cf::TypeSys::TypeInfoT ToolClipT::TypeInfo(GetToolTIM(), "ToolClipT", "ToolT", ToolClipT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


ToolClipT::ToolClipT(MapDocumentT& MapDoc, ToolManagerT& ToolMan, wxWindow* ParentOptionsBar)
    : ToolT(MapDoc, ToolMan),
      m_ToolState(IDLE_EMPTY),
      m_ClipPoints(),
      m_Clip3rdAxis(-1),
      m_ClipResults(),
      m_DrawMeasurements(false),
      m_OptionsBar(new OptionsBar_ClipBrushesToolT(ParentOptionsBar, *this))
{
}


ToolClipT::~ToolClipT()
{
    m_ToolState=IDLE_EMPTY;
    UpdateClipResults();    // Clear the current clip results.
}


wxWindow* ToolClipT::GetOptionsBar()
{
    // Cannot define this method inline in the header file, because there the compiler
    // does not yet know that the type of m_OptionsBar is in fact related to wxWindow.
    return m_OptionsBar;
}


void ToolClipT::NoteClipModeChanged()
{
    // Update the clipped objects based on the clip mode.
    UpdateClipResults();

    // Also update the views to show the new clip results.
    m_ToolMan.UpdateAllObservers(this, UPDATE_SOON);
}


void ToolClipT::OnActivate(ToolT* OldTool)
{
    if (IsActiveTool())
    {
        m_OptionsBar->CycleClipMode();
    }
    else UpdateClipResults();
}


void ToolClipT::OnDeactivate(ToolT* NewTool)
{
    const ToolStateT TS=m_ToolState;

    m_ToolState=IDLE_EMPTY;
    UpdateClipResults();    // Clear the current clip results.
    m_ToolState=TS;
}


bool ToolClipT::OnKeyDown2D(ViewWindow2DT& ViewWindow, wxKeyEvent& KE)
{
    switch (KE.GetKeyCode())
    {
        case 'O':
            m_DrawMeasurements=!m_DrawMeasurements;
            m_ToolMan.UpdateAllObservers(this, UPDATE_SOON);
            return true;
    }

    return OnKeyDown(ViewWindow, KE);
}


bool ToolClipT::OnLMouseDown2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
    ViewWindow.CaptureMouse();

    const int       ThirdAxis=ViewWindow.GetAxesInfo().ThirdAxis;
    const Vector3fT WorldPos =m_MapDoc.SnapToGrid(ViewWindow.WindowToWorld(ME.GetPosition(), 0.0f), ME.AltDown(), -1 /*Snap all axes.*/);

    if (m_ToolState==IDLE_EMPTY || (m_ToolState==IDLE_HAVE_POINTS && (ME.ShiftDown() || m_Clip3rdAxis!=ThirdAxis)))
    {
        m_ClipPoints[0]=WorldPos;
        m_ClipPoints[1]=WorldPos;

        m_Clip3rdAxis=ThirdAxis;
        m_ToolState=DRAG_POINT_1;

        UpdateClipResults();    // Clear the current clip results.
        m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
    }
    else if (m_ToolState==IDLE_HAVE_POINTS)
    {
        const int ClipPointHit=HitTest(ViewWindow, ME.GetPosition());

             if (ClipPointHit==0) m_ToolState=DRAG_POINT_0;
        else if (ClipPointHit==1) m_ToolState=DRAG_POINT_1;
    }

    return true;
}


bool ToolClipT::OnLMouseUp2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
    if (!ViewWindow.HasCapture()) return false;     // Drag started outside of ViewWindow.
    ViewWindow.ReleaseMouse();

    if (m_ToolState==DRAG_POINT_0 || m_ToolState==DRAG_POINT_1)
    {
        m_ToolState=IDLE_HAVE_POINTS;
    }

    return true;
}


bool ToolClipT::OnMouseMove2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
 // const int       HorzAxis =ViewWindow.GetAxesInfo().HorzAxis;
 // const int       VertAxis =ViewWindow.GetAxesInfo().VertAxis;
    const int       ThirdAxis=ViewWindow.GetAxesInfo().ThirdAxis;
    const Vector3fT WorldPos =m_MapDoc.SnapToGrid(ViewWindow.WindowToWorld(ME.GetPosition(), 0.0f), ME.AltDown(), -1 /*Snap all axes.*/);
    const wxCursor* NewCursor=wxSTANDARD_CURSOR;

    switch (m_ToolState)
    {
        case IDLE_EMPTY:
            // Nothing to do here.
            break;

        case IDLE_HAVE_POINTS:
            // If this is the right view, and the cursor is over one of our clip points, then set it to a cross.
            if (m_Clip3rdAxis==ThirdAxis && HitTest(ViewWindow, ME.GetPosition())!=-1)
                NewCursor=&CursorMan->GetCursor(CursorManT::CROSS);
            break;

        case DRAG_POINT_0:
        case DRAG_POINT_1:
        {
            // Make sure that our dragged m_ClipPoints are "compatible" with our current WorldPos.
            // However, this should always be true, and the test in the next line should never trigger.
            if (m_Clip3rdAxis!=ThirdAxis) break;

            const unsigned long ClipPointNr=(m_ToolState==DRAG_POINT_0) ? 0 : 1;
            NewCursor=&CursorMan->GetCursor(CursorManT::CROSS);

            if (m_ClipPoints[ClipPointNr]==WorldPos) break;

            // Update the position of the dragged clip point (or of both clip points simultaneously).
            if (ME.ControlDown()) m_ClipPoints[1-ClipPointNr]+=(WorldPos-m_ClipPoints[ClipPointNr]);
            m_ClipPoints[ClipPointNr]=WorldPos;

            UpdateClipResults();
            m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
            break;
        }
    }

    ViewWindow.SetCursor(*NewCursor);
    return true;
}


bool ToolClipT::OnKeyDown3D(ViewWindow3DT& ViewWindow, wxKeyEvent& KE)
{
    return OnKeyDown(ViewWindow, KE);
}


void ToolClipT::RenderTool2D(Renderer2DT& Renderer) const
{
    if (!IsActiveTool()) return;            // Don't render our clip plane when we're not the active tool.
    if (m_ToolState==IDLE_EMPTY) return;    // When we're in state IDLE_EMPTY, there is no clip plane defined.
    if (m_Clip3rdAxis!=Renderer.GetViewWin2D().GetAxesInfo().ThirdAxis) return; // Only render the clip plane in the view it was defined (or any "compatible" view).

    Renderer.SetLineType(wxPENSTYLE_SOLID, Renderer2DT::LINE_THICK, wxColour(255, 255, 255));

    for (unsigned long ClipNr=0; ClipNr<m_ClipResults.Size(); ClipNr++)
    {
        const MapBrushT* FrontBrush=m_ClipResults[ClipNr]->Front;
        const MapBrushT* BackBrush =m_ClipResults[ClipNr]->Back;

        if (FrontBrush)
        {
            for (unsigned long FaceNr=0; FaceNr<FrontBrush->GetFaces().Size(); FaceNr++)
            {
                Renderer.DrawLineLoop(FrontBrush->GetFaces()[FaceNr].GetVertices(), Options.view2d.DrawVertices ? 2 : 0);

                if (m_DrawMeasurements)
                    Renderer.DrawBoxDims(FrontBrush->GetBB(), wxBOTTOM | wxRIGHT);
            }
        }

        if (BackBrush)
        {
            for (unsigned long FaceNr=0; FaceNr<BackBrush->GetFaces().Size(); FaceNr++)
            {
                Renderer.DrawLineLoop(BackBrush->GetFaces()[FaceNr].GetVertices(), Options.view2d.DrawVertices ? 2 : 0);

                if (m_DrawMeasurements)
                    Renderer.DrawBoxDims(BackBrush->GetBB(), wxTOP | wxLEFT);
            }
        }
    }

    // Also draw the clip "plane".
    Renderer.SetLineColor(wxColour(0, 255, 255));
    Renderer.DrawLine(m_ClipPoints[0], m_ClipPoints[1]);

    Renderer.SetFillColor(wxColour(255, 255, 255));
    Renderer.DrawPoint(m_ClipPoints[0], 2);
    Renderer.DrawPoint(m_ClipPoints[1], 2);
}


void ToolClipT::RenderTool3D(Renderer3DT& Renderer) const
{
    if (!IsActiveTool()) return;            // Don't render our clip plane when we're not the active tool.
    if (m_ToolState==IDLE_EMPTY) return;    // When we're in state IDLE_EMPTY, there is no clip plane defined.

    for (unsigned long ClipNr=0; ClipNr<m_ClipResults.Size(); ClipNr++)
    {
        const MapBrushT* FrontBrush=m_ClipResults[ClipNr]->Front;
        const MapBrushT* BackBrush =m_ClipResults[ClipNr]->Back;

        if (FrontBrush)
            for (unsigned long FaceNr=0; FaceNr<FrontBrush->GetFaces().Size(); FaceNr++)
                FrontBrush->GetFaces()[FaceNr].Render3DBasic(Renderer.GetRMatWireframe_OffsetZ(), *wxWHITE, 255);

        if (BackBrush)
            for (unsigned long FaceNr=0; FaceNr<BackBrush->GetFaces().Size(); FaceNr++)
                BackBrush->GetFaces()[FaceNr].Render3DBasic(Renderer.GetRMatWireframe_OffsetZ(), *wxWHITE, 255);
    }
}


int ToolClipT::HitTest(ViewWindow2DT& ViewWindow, const wxPoint& PointWS)
{
    const wxPoint PointTS=ViewWindow.WindowToTool(PointWS);

    for (int ClipPointNr=0; ClipPointNr<2; ClipPointNr++)
    {
        const wxPoint CP =ViewWindow.WorldToTool(m_ClipPoints[ClipPointNr]);
        const int     EPS=4;

        wxRect Rect(CP, CP);
        Rect.Inflate(EPS, EPS);

        if (Rect.Contains(PointTS)) return ClipPointNr;
    }

    return -1;
}


void ToolClipT::UpdateClipResults()
{
    // Delete and clear the previous list of clip results.
    for (unsigned long crNr=0; crNr<m_ClipResults.Size(); crNr++)
    {
        // The ClipResultT dtor properly deletes the results (owned by us) only.
        delete m_ClipResults[crNr];
    }

    m_ClipResults.Overwrite();


    // Compute the current clip plane.
    if (m_ToolState==IDLE_EMPTY) return;
    if (m_Clip3rdAxis<0 || m_Clip3rdAxis>2) return;     // m_Clip3rdAxis is initialized to -1 in the constructor.

    Vector3fT Span1=Vector3fT(0, 0, 0); Span1[m_Clip3rdAxis]=1.0f;
    Vector3fT Span2=m_ClipPoints[1]-m_ClipPoints[0];

    if (Span2.GetLengthSqr()<0.1f) return;

    const Vector3fT Normal=normalizeOr0(cross(Span1, Span2));
    const Plane3fT  ClipPlane(Normal, dot(Normal, m_ClipPoints[0]));

    if (!ClipPlane.IsValid()) return;   // Should never happen.


    // For each brush in the selection, create an appropriate clip result record.
    const OptionsBar_ClipBrushesToolT::ClipModeT ClipMode =m_OptionsBar->GetClipMode();
    const ArrayT<MapElementT*>&                  Selection=m_MapDoc.GetSelection();

    for (unsigned long SelNr=0; SelNr<Selection.Size(); SelNr++)
    {
        if (Selection[SelNr]->GetType()!=&MapBrushT::TypeInfo) continue;

        ClipResultT* ClipResult=new ClipResultT;
        ClipResult->Workpiece=static_cast<MapBrushT*>(Selection[SelNr]);

        ClipResult->Workpiece->Split(ClipPlane,
            ClipMode!=OptionsBar_ClipBrushesToolT::KeepBack  ? &ClipResult->Front : NULL,
            ClipMode!=OptionsBar_ClipBrushesToolT::KeepFront ? &ClipResult->Back  : NULL);

        m_ClipResults.PushBack(ClipResult);
    }
}


bool ToolClipT::OnKeyDown(ViewWindowT& ViewWindow, wxKeyEvent& KE)
{
    switch (KE.GetKeyCode())
    {
        case WXK_RETURN:
        {
            if (m_ToolState==IDLE_EMPTY) return true;

            // Note that after a call to UpdateClipResults(), pointers to brushes that were selected at the time of the call
            // are kept in the m_ClipResults array. When the user then manages to interfere with these brushes (e.g. by deleting
            // them via the "Edit" menu), the kept pointers become stale/invalid.
            // Therefore it is important to call UpdateClipResults() here again, in order to refresh the clip results,
            // making sure that only "valid" brushes are passed into the command.
            UpdateClipResults();
            if (m_ClipResults.Size()==0) return true;

            m_MapDoc.CompatSubmitCommand(new CommandClipT(m_MapDoc, m_ClipResults));

            // If there are any empty groups (usually as a result from an implicit deletion by the clip), purge them now.
            // We use an explicit command for deleting the groups (instead of putting everything into a macro command)
            // so that the user has the option to undo the purge (separately from the deletion) if he wishes.
            const ArrayT<GroupT*> EmptyGroups = m_MapDoc.GetAbandonedGroups();

            if (EmptyGroups.Size() > 0)
                m_MapDoc.CompatSubmitCommand(new CommandDeleteGroupT(m_MapDoc, EmptyGroups));

            // Just clear the clip results without deleting them (they are now owned by the command).
            m_ClipResults.Overwrite();
            m_ToolState=IDLE_EMPTY;
            m_ToolMan.UpdateAllObservers(this, UPDATE_SOON);
            return true;
        }

        case WXK_ESCAPE:
        {
            if (m_ToolState!=IDLE_EMPTY)
            {
                m_ToolState=IDLE_EMPTY;
                m_ToolMan.UpdateAllObservers(this, UPDATE_SOON);
            }
            else
            {
                m_ToolMan.SetActiveTool(GetToolTIM().FindTypeInfoByName("ToolSelectionT"));
            }

            return true;
        }
    }

    return false;
}
