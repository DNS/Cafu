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

#include "wx/wx.h"
#include "Camera.hpp"
#include "CursorMan.hpp"
#include "CommandHistory.hpp"
#include "ParentFrame.hpp"
#include "MapBrush.hpp"
#include "MapDocument.hpp"
#include "MorphPrim.hpp"
#include "ChildFrame.hpp"
#include "ChildFrameViewWin2D.hpp"
#include "ChildFrameViewWin3D.hpp"
#include "ToolManager.hpp"
#include "ToolMorph.hpp"
#include "Renderer2D.hpp"
#include "AppCaWE.hpp"
#include "MapBezierPatch.hpp"
#include "ToolOptionsBars.hpp"
#include "MapCommands/AddPrim.hpp"
#include "MapCommands/Delete.hpp"


/*** Begin of TypeSys related definitions for this class. ***/

void* ToolMorphT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    const ToolCreateParamsT* TCPs=static_cast<const ToolCreateParamsT*>(&Params);

    return new ToolMorphT(TCPs->MapDoc, TCPs->ToolMan, TCPs->ParentOptionsBar);
}

const cf::TypeSys::TypeInfoT ToolMorphT::TypeInfo(GetToolTIM(), "ToolMorphT", "ToolT", ToolMorphT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


ToolMorphT::ToolMorphT(MapDocumentT& MapDoc, ToolManagerT& ToolMan, wxWindow* ParentOptionsBar)
    : ToolT(MapDoc, ToolMan),
      m_MorphPrims(),
      m_DragState(DragNothing),
      m_IsRecursiveSelfNotify(false),
      m_OptionsBar(new OptionsBar_EditVerticesToolT(ParentOptionsBar, *this))
{
    m_MapDoc.RegisterObserver(this);
}


ToolMorphT::~ToolMorphT()
{
    m_MapDoc.UnregisterObserver(this);
}


wxWindow* ToolMorphT::GetOptionsBar()
{
    // Cannot define this method inline in the header file, because there the compiler
    // does not yet know that the type of m_OptionsBar is in fact related to wxWindow.
    return m_OptionsBar;
}


ArrayT<MorphHandleT> ToolMorphT::GetHandles(bool SelectedOnly, bool Vertices, bool Edges) const
{
    ArrayT<MorphHandleT> Handles;

    unsigned long SelVertsCount=0;
    unsigned long SelEdgesCount=0;

    for (unsigned long MPNr=0; MPNr<m_MorphPrims.Size(); MPNr++)
    {
        MorphPrimT* MorphPrim=m_MorphPrims[MPNr];

        if (Vertices)
        {
            for (unsigned long VertexNr=0; VertexNr<MorphPrim->m_Vertices.Size(); VertexNr++)
            {
                if (MorphPrim->m_Vertices[VertexNr]->m_Selected) SelVertsCount++;
                if (SelectedOnly && !MorphPrim->m_Vertices[VertexNr]->m_Selected) continue;

                MorphHandleT mh;

                mh.MorphPrim=MorphPrim;
                mh.Part     =MorphPrim->m_Vertices[VertexNr];

                Handles.PushBack(mh);
            }
        }

        if (Edges)
        {
            for (unsigned long EdgeNr=0; EdgeNr<MorphPrim->m_Edges.Size(); EdgeNr++)
            {
                if (MorphPrim->m_Edges[EdgeNr]->m_Selected) SelEdgesCount++;
                if (SelectedOnly && !MorphPrim->m_Edges[EdgeNr]->m_Selected) continue;

                MorphHandleT mh;

                mh.MorphPrim=MorphPrim;
                mh.Part     =MorphPrim->m_Edges[EdgeNr];

                Handles.PushBack(mh);
            }
        }
    }

    // Make sure that only ever vertices *OR* edges are selected, never both at the same time.
    // Update: This is not needed any more, as it just doesn't hurt if we have both types selected at the same time.
    // wxASSERT(SelVertsCount==0 || SelEdgesCount==0);

    return Handles;
}


void ToolMorphT::OnActivate(ToolT* OldTool)
{
    if (IsActiveTool())
    {
        m_OptionsBar->CycleEditMode();
    }
    else
    {
        MorphPrims_CommitAndClear();

        // For each brush or bezier patch in the documents selection, create a related instance here.
        for (unsigned long SelNr=0; SelNr<m_MapDoc.GetSelection().Size(); SelNr++)
        {
            MapPrimitiveT* MapPrim=dynamic_cast<MapPrimitiveT*>(m_MapDoc.GetSelection()[SelNr]);

            if (MapPrim)
                MorphPrims_TogglePrim(MapPrim);
        }
    }
}


bool ToolMorphT::CanDeactivate()
{
    return true;
}


void ToolMorphT::OnDeactivate(ToolT* NewTool)
{
    MorphPrims_CommitAndClear();
}


int ToolMorphT::MorphPrims_Find(const MapElementT* Elem) const
{
    for (unsigned long MPNr=0; MPNr<m_MorphPrims.Size(); MPNr++)
        if (m_MorphPrims[MPNr]->GetMapPrim()==Elem)
            return MPNr;

    return -1;
}


void ToolMorphT::MorphPrims_CommitAndClear()
{
    // Set the map elements of all morph primitives back to "visible"
    // and remove all unmodified morph primitives from our list.

    // TODO: Doing it like this is not particularly efficient, but this is going to be refactored anyway.
    while (m_MorphPrims.Size()>0)
    {
        MorphPrims_TogglePrim(m_MorphPrims[0]->GetMapPrim());
    }
}


void ToolMorphT::MorphPrims_TogglePrim(const MapPrimitiveT* MapPrim)
{
    // Only needed for observer message.
    ArrayT<MapElementT*> MapElements;
    MapElements.PushBack(const_cast<MapPrimitiveT*>(MapPrim));

    const int MP_Index=MorphPrims_Find(MapPrim);

    if (MP_Index>=0)
    {
        MorphPrimT* MorphPrim=m_MorphPrims[MP_Index];

        m_MorphPrims.RemoveAtAndKeepOrder(MP_Index);
        wxASSERT(MapPrim == MorphPrim->GetMapPrim());

        if (MorphPrim->IsModified())
        {
            MapPrimitiveT* MorphedMapPrim=MorphPrim->GetMorphedMapPrim();

            if (MorphedMapPrim)
            {
                m_IsRecursiveSelfNotify=true;

                ArrayT<CommandT*> Commands;
                Commands.PushBack(new CommandDeleteT(m_MapDoc, const_cast<MapPrimitiveT*>(MorphPrim->GetMapPrim())));
                Commands.PushBack(new CommandAddPrimT(m_MapDoc, MorphedMapPrim, MorphPrim->GetMapPrim()->GetParent()));

                m_MapDoc.GetHistory().SubmitCommand(new CommandMacroT(Commands, "Edit Vertices"));

                m_IsRecursiveSelfNotify=false;
            }
        }

        delete MorphPrim;

        // Elem is now no longer mentioned in the m_MorphPrims list, and thus no longer affected by IsHiddenByTool().
        m_IsRecursiveSelfNotify=true;
        m_MapDoc.UpdateAllObservers_Modified(MapElements, MEMD_VISIBILITY);     // TODO: Is this still needed? The element was *deleted* above, after all (but only if modified, mind'ya).
        m_IsRecursiveSelfNotify=false;
        return;
    }

    if (dynamic_cast<const MapBrushT*>(MapPrim)==NULL && dynamic_cast<const MapBezierPatchT*>(MapPrim)==NULL) return;

    MorphPrimT* MorphPrim=new MorphPrimT(MapPrim);
    m_MorphPrims.PushBack(MorphPrim);

    // Elem is now mentioned in the m_MorphPrims list, and thus affected by IsHiddenByTool().
    m_IsRecursiveSelfNotify=true;
    m_MapDoc.UpdateAllObservers_Modified(MapElements, MEMD_VISIBILITY);
    m_IsRecursiveSelfNotify=false;
}


ArrayT<MorphHandleT> ToolMorphT::GetMorphHandlesAt(ViewWindow2DT& ViewWindow, const wxPoint& Point)
{
    ArrayT<MorphHandleT> MorphHandles;

    for (unsigned long MPNr=0; MPNr<m_MorphPrims.Size(); MPNr++)
    {
        if (m_OptionsBar->IsEditingVertices())
        {
            for (unsigned long i=0; i<m_MorphPrims[MPNr]->m_Vertices.Size(); i++)
            {
                MP_VertexT* v  =m_MorphPrims[MPNr]->m_Vertices[i];
                wxPoint     ptv=ViewWindow.WorldToTool(v->pos);
                wxRect      Rect(ptv.x-3, ptv.y-3, 6, 6);

                if (Rect.Contains(Point))
                {
                    MorphHandleT mh;

                    mh.MorphPrim=m_MorphPrims[MPNr];
                    mh.Part=v;

                    MorphHandles.PushBack(mh);
                }
            }
        }

        if (m_OptionsBar->IsEditingEdges())
        {
            for (unsigned long i=0; i<m_MorphPrims[MPNr]->m_Edges.Size(); i++)
            {
                MP_EdgeT* e  =m_MorphPrims[MPNr]->m_Edges[i];
                wxPoint   ptv=ViewWindow.WorldToTool(e->GetPos());
                wxRect    Rect(ptv.x-3, ptv.y-3, 6, 6);

                if (Rect.Contains(Point))
                {
                    MorphHandleT mh;

                    mh.MorphPrim=m_MorphPrims[MPNr];
                    mh.Part=e;

                    MorphHandles.PushBack(mh);
                }
            }
        }
    }

    return MorphHandles;
}


bool ToolMorphT::GetMorphHandleAt(ViewWindow3DT& ViewWindow, const wxPoint& Point, MorphHandleT& FoundMH)
{
    float BestDist=1000000.0f;

    FoundMH.MorphPrim=NULL;
    FoundMH.Part     =NULL;

    for (unsigned long MPNr=0; MPNr<m_MorphPrims.Size(); MPNr++)
    {
        if (m_OptionsBar->IsEditingVertices())
        {
            for (unsigned long i=0; i<m_MorphPrims[MPNr]->m_Vertices.Size(); i++)
            {
                MP_VertexT*   v  =m_MorphPrims[MPNr]->m_Vertices[i];
                const wxPoint ptv=ViewWindow.WorldToWindow(v->pos, true /*Yes, check against view frustum.*/);

                if (ptv.x<0 || ptv.y<0) continue;

                const wxRect Rect(ptv.x-3, ptv.y-3, 6, 6);

                if (Rect.Contains(Point))
                {
                    const float Dist=length(v->pos-ViewWindow.GetCamera().Pos);

                    if (Dist<BestDist)
                    {
                        FoundMH.MorphPrim=m_MorphPrims[MPNr];
                        FoundMH.Part=v;

                        BestDist=Dist;
                    }
                }
            }
        }

        if (m_OptionsBar->IsEditingEdges())
        {
            for (unsigned long i=0; i<m_MorphPrims[MPNr]->m_Edges.Size(); i++)
            {
                MP_EdgeT*     e  =m_MorphPrims[MPNr]->m_Edges[i];
                const wxPoint ptv=ViewWindow.WorldToWindow(e->GetPos(), true /*Yes, check against view frustum.*/);

                if (ptv.x<0 || ptv.y<0) continue;

                const wxRect Rect(ptv.x-3, ptv.y-3, 6, 6);

                if (Rect.Contains(Point))
                {
                    const float Dist=length(e->GetPos()-ViewWindow.GetCamera().Pos);

                    if (Dist<BestDist)
                    {
                        FoundMH.MorphPrim=m_MorphPrims[MPNr];
                        FoundMH.Part=e;

                        BestDist=Dist;
                    }
                }
            }
        }
    }

    return FoundMH.Part!=NULL;
}


void ToolMorphT::MoveSelectedHandles(const Vector3fT& Delta)
{
    m_DragHandleCurrentPos+=Delta;

    for (unsigned long MPNr=0; MPNr<m_MorphPrims.Size(); MPNr++)
        m_MorphPrims[MPNr]->MoveSelectedHandles(Delta);
}


bool ToolMorphT::IsHiddenByTool(const MapElementT* Elem) const
{
    // Elements that are currently being morphed are hidden by this tool from normal rendering.
    return MorphPrims_Find(Elem) >= 0;
}


void ToolMorphT::RenderTool2D(Renderer2DT& Renderer) const
{
    const ViewWindow2DT& ViewWin=Renderer.GetViewWin2D();

    for (unsigned long MPNr=0; MPNr<m_MorphPrims.Size(); MPNr++)
    {
        m_MorphPrims[MPNr]->Render(Renderer,
            m_OptionsBar->IsEditingVertices(),
            m_OptionsBar->IsEditingEdges());
    }

    if (m_DragState==DragBoxSelection && ViewWin.HasCapture())
    {
        Renderer.SetLineType(wxPENSTYLE_DOT, Renderer2DT::LINE_THIN, wxColor(128, 128, 0));  // Color is "dirty yellow".
        Renderer.Rectangle(wxRect(ViewWin.WorldToTool(m_DragHandleOrigPos), ViewWin.WorldToTool(m_DragHandleCurrentPos)), false);
    }
}


void ToolMorphT::NudgeSelectedHandles(const AxesInfoT& AxesInfo, const wxKeyEvent& KE)
{
    const float Dist=(KE.AltDown()!=m_MapDoc.IsSnapEnabled()) ? m_MapDoc.GetGridSpacing() : 1.0f;   // "!=" implements "xor" for bools.
    Vector3fT   NudgeVec;

    switch (KE.GetKeyCode())
    {
        case WXK_RIGHT: NudgeVec[AxesInfo.HorzAxis]=AxesInfo.MirrorHorz ? -Dist :  Dist; break;
        case WXK_LEFT:  NudgeVec[AxesInfo.HorzAxis]=AxesInfo.MirrorHorz ?  Dist : -Dist; break;
        case WXK_DOWN:  NudgeVec[AxesInfo.VertAxis]=AxesInfo.MirrorVert ? -Dist :  Dist; break;
        case WXK_UP:    NudgeVec[AxesInfo.VertAxis]=AxesInfo.MirrorVert ?  Dist : -Dist; break;
    }


    const ArrayT<MorphHandleT> SelVertHandles=GetHandles(true, true, false);

    if (SelVertHandles.Size()==1)
    {
        // If there is currently exactly one vertex selected, conveniently snap it to the grid,
        // rather than just offsetting its position by the current grid stepping.
        const Vector3fT OldPos=SelVertHandles[0].Part->GetPos();
        const int       Axis  =NudgeVec[AxesInfo.HorzAxis]!=0.0f ? AxesInfo.HorzAxis : AxesInfo.VertAxis;

        Vector3fT NewPos=OldPos+NudgeVec;
        NewPos[Axis]=m_MapDoc.SnapToGrid(NewPos[Axis], KE.AltDown());

        NudgeVec=NewPos-OldPos;
    }

    MoveSelectedHandles(NudgeVec);
}


void ToolMorphT::NoteEditModeChanged()
{
    // TODO: Clear the selection!!?!

    m_ToolMan.UpdateAllObservers(this, UPDATE_SOON);
}


void ToolMorphT::InsertVertex()
{
    if (m_MorphPrims.Size()!=1)
    {
        wxMessageBox("Adding a vertex is only possible if exactly one brush is selected for morphing (vertex editing).", "Not exactly 1 brush selected.");
        return;
    }

    if (dynamic_cast<const MapBrushT*>(m_MorphPrims[0]->GetMapPrim())==NULL)
    {
        wxMessageBox("The morph tool can add new vertices only to brushes (not to Bezier patches).\n"
                     "(The number of subdivisions of Bezier patches can be changed in the Properties dialog.)", "Item being morphed is not a brush.");
        return;
    }

    if (m_MorphPrims[0]->m_Vertices.Size()==0)
    {
        // wxMessageBox("This brush has no old vertices, which is a situation that should never happen.\nPlease create a new brush.", "Brush has no vertices.");
        return;
    }

    // Now find the center of the m_MorphPrims[0].
    Vector3fT Center=m_MorphPrims[0]->m_Vertices[0]->pos;

    for (unsigned long VertexNr=1; VertexNr<m_MorphPrims[0]->m_Vertices.Size(); VertexNr++)
        Center+=m_MorphPrims[0]->m_Vertices[VertexNr]->pos;

    Center/=m_MorphPrims[0]->m_Vertices.Size();

    // Add the center vertex to the m_MorphPrims[0].
    m_MorphPrims[0]->m_Vertices.PushBack(new MP_VertexT);
    m_MorphPrims[0]->m_Vertices[m_MorphPrims[0]->m_Vertices.Size()-1]->pos=Center;

    // I *don't* bother to update the m_MorphPrims[0] geometry e.g. by a dummy-call to MoveSelectedHandles(),
    // because firstly the convex hull should not have been affected anyway, and secondly, the user is supposed
    // to trigger the update himself by dragging the new vertex.
    // We *have* to update the views, though.
    m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
}


void ToolMorphT::RenderTool3D(Renderer3DT& Renderer) const
{
    for (unsigned long MPNr=0; MPNr<m_MorphPrims.Size(); MPNr++)
    {
        m_MorphPrims[MPNr]->Render(Renderer,
            m_OptionsBar->IsEditingVertices(),
            m_OptionsBar->IsEditingEdges());
    }
}


bool ToolMorphT::OnKeyDown2D(ViewWindow2DT& ViewWindow, wxKeyEvent& KE)
{
    switch (KE.GetKeyCode())
    {
        case WXK_ESCAPE:
        {
            OnEscape(ViewWindow);
            return true;
        }

        case WXK_UP:
        case WXK_DOWN:
        case WXK_LEFT:
        case WXK_RIGHT:
        {
            if (!KE.ShiftDown()) break;
            if (GetHandles(true).Size()==0) break;

            NudgeSelectedHandles(ViewWindow.GetAxesInfo(), KE);

            m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
            return true;
        }
    }

    return false;
}


bool ToolMorphT::OnLMouseDown2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
    // When we get here,
    // a) any number (zero or more) of morph handles may be selected,
    // b) a selection box may or may not have been dragged open before,
    // c) we are currently not dragging anything (because this is a mouse *down* event).

    // First make sure that c) is actually true.
    wxASSERT(m_DragState==DragNothing);

    ViewWindow.CaptureMouse();

    const wxPoint MousePos_WinSpace =ME.GetPosition();                              // The position of the mouse in Window Space.
    const wxPoint MousePos_ToolSpace=ViewWindow.WindowToTool(ME.GetPosition());     // The position of the mouse in Tool Space.

    const ArrayT<MorphHandleT> MorphHandlesAtMousePos=GetMorphHandlesAt(ViewWindow, MousePos_ToolSpace);

    if (MorphHandlesAtMousePos.Size()>0)
    {
        // The button was pressed over one or more morph handles.
        // First assign DragHandle to one of them, if possible to a *selected* one!
        MP_PartT* DragHandle=MorphHandlesAtMousePos[0].Part;

        for (unsigned long mhNr=1; mhNr<MorphHandlesAtMousePos.Size(); mhNr++)
            if (MorphHandlesAtMousePos[mhNr].Part->m_Selected)
            {
                DragHandle=MorphHandlesAtMousePos[mhNr].Part;
                break;
            }

        // Depending on the CTRL button state and on whether among the MorphHandlesAtMousePos some were already selected, update the selection.
        if (ME.ControlDown())
        {
            // Just toggle the selection state of all those handles (inclusive that of the DragHandle).
            for (unsigned long mhNr=0; mhNr<MorphHandlesAtMousePos.Size(); mhNr++)
                MorphHandlesAtMousePos[mhNr].Part->m_Selected=!MorphHandlesAtMousePos[mhNr].Part->m_Selected;
        }
        else
        {
            if (!DragHandle->m_Selected)
            {
                // There are no selected handles among the MorphHandlesAtMousePos, and CTRL is not down.
                // Thus clear the selection of handles elsewhere, and select all handles under the mouse pointer.
                // Ignore the possibility that BOTH vertex AND edge handles might get selected simultaneously,
                // that should not induce a problem.
                ArrayT<MorphHandleT> SelectedHandles=GetHandles(true);

                for (unsigned long HandleNr=0; HandleNr<SelectedHandles.Size(); HandleNr++)
                    SelectedHandles[HandleNr].Part->m_Selected=false;

                for (unsigned long mhNr=0; mhNr<MorphHandlesAtMousePos.Size(); mhNr++)
                    MorphHandlesAtMousePos[mhNr].Part->m_Selected=true;
            }
            else
            {
                // *INTENTIONALLY* do nothing here!
                // The left mouse button was pressed over handles among which was at least one already selected handle,
                // and CTRL was not down.
                // So the users intention was probably just to "grab" the already selected handle for dragging it.
            }
        }

        // Intentionally(!) do not re-search the MorphHandlesAtMousePos for a selected handle,
        // but use the DragHandle as-is after the above code for descisions!
        if (DragHandle->m_Selected)
        {
            m_DragHandleOrigPos   =DragHandle->GetPos();
            m_DragHandleCurrentPos=DragHandle->GetPos();
            m_DragState=DragMorphHandles;
        }
        else
        {
            // The click made the handle become unselected, so we are now not dragging anything.
            wxASSERT(m_DragState==DragNothing);
        }

        m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
        return true;
    }


    // Did not hit anything of relevance so far, so check if another map element can be selected for morphing.
    // FIXME: Only do this if the ALT or CTRL (i.e. the "Eyedropper") key is also down!
    ArrayT<MapElementT*> HitElems=ViewWindow.GetElementsAt(MousePos_WinSpace);

    for (unsigned long HitNr=0; HitNr<HitElems.Size(); HitNr++)
    {
        MapPrimitiveT* HitPrim=dynamic_cast<MapPrimitiveT*>(HitElems[HitNr]);

        if (HitPrim && (HitPrim->GetType()==&MapBrushT::TypeInfo || HitPrim->GetType()==&MapBezierPatchT::TypeInfo))
        {
            if (!ME.ControlDown()) MorphPrims_CommitAndClear();
            MorphPrims_TogglePrim(HitPrim);

            m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
            return true;
        }
    }


    // If that didn't work either, start a new selection box.
    m_DragState           =DragBoxSelection;
    m_DragHandleOrigPos   =m_MapDoc.SnapToGrid(ViewWindow.WindowToWorld(MousePos_WinSpace, 0.0f), ME.AltDown(), -1 /*Snap all axes.*/);
    m_DragHandleCurrentPos=m_DragHandleOrigPos;

    m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
    return true;
}


bool ToolMorphT::OnLMouseUp2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
    if (!ViewWindow.HasCapture()) return false;     // Drag started outside of ViewWindow.
    ViewWindow.ReleaseMouse();

    const int ThirdAxis=ViewWindow.GetAxesInfo().ThirdAxis;

    switch (m_DragState)
    {
        case DragBoxSelection:
            // If the dragged selection box is too small, the user probably clicked by accident, so just do nothing.
            if (length(m_DragHandleCurrentPos-m_DragHandleOrigPos)<2.0f) break;

            // If Control is not down, clear the selection of handles first.
            if (!ME.ControlDown())
            {
                const ArrayT<MorphHandleT> SelectedHandles=GetHandles(true /*SelectedOnly*/);

                for (unsigned long HandleNr=0; HandleNr<SelectedHandles.Size(); HandleNr++)
                    SelectedHandles[HandleNr].Part->m_Selected=false;
            }

            // Now toggle all the vertices in the dragged box.
            for (unsigned long MPNr=0; MPNr<m_MorphPrims.Size(); MPNr++)
            {
                for (unsigned long VertexNr=0; VertexNr<m_MorphPrims[MPNr]->m_Vertices.Size(); VertexNr++)
                {
                    MP_VertexT* Vertex=m_MorphPrims[MPNr]->m_Vertices[VertexNr];

                    m_DragHandleOrigPos   [ThirdAxis]=Vertex->pos[ThirdAxis]-1.0f;
                    m_DragHandleCurrentPos[ThirdAxis]=Vertex->pos[ThirdAxis]+1.0f;

                    if (BoundingBox3fT(m_DragHandleOrigPos, m_DragHandleCurrentPos).Contains(Vertex->pos))
                        Vertex->m_Selected=!Vertex->m_Selected;
                }
            }
            break;

        case DragMorphHandles:
            break;

        case DragNothing:
            // Well, it's actually possible to get here and drag nothing, namely when the user CTRL-clicked a previously selected handle,
            // when a new map element was selected for morphing, drag started outside of view, or when ESC was pressed to cancel the drag.
            // wxASSERT(false);
            break;
    }

    // The button went up, so we are not dragging anything any more.
    m_DragState=DragNothing;

    m_ToolMan.UpdateAllObservers(this, UPDATE_SOON);
    return true;
}


bool ToolMorphT::OnMouseMove2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
 // const int HorzAxis =ViewWindow.GetAxesInfo().HorzAxis;
 // const int VertAxis =ViewWindow.GetAxesInfo().VertAxis;
    const int ThirdAxis=ViewWindow.GetAxesInfo().ThirdAxis;

    const wxCursor* NewCursor=wxSTANDARD_CURSOR;
    const Vector3fT MousePos_WorldSpace=ViewWindow.WindowToWorld(ME.GetPosition(), 0.0f);   // The position of the mouse in World Space.

    switch (m_DragState)
    {
        case DragNothing:
            if (GetMorphHandlesAt(ViewWindow, ViewWindow.WindowToTool(ME.GetPosition())).Size()>0)
            {
                NewCursor=&CursorMan->GetCursor(CursorManT::CROSS);
            }
            break;

        case DragBoxSelection:
        {
            m_DragHandleCurrentPos=m_MapDoc.SnapToGrid(MousePos_WorldSpace, ME.AltDown(), -1 /*Snap all axes.*/);

            m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
            break;
        }

        case DragMorphHandles:
        {
            NewCursor=&CursorMan->GetCursor(CursorManT::CROSS);

            // As soon as there is at least one edge handle among the dragged handles,
            // we assume that it actually is the edge handle that is being dragged, and thus we make only "relative" moves:
            // A relative move offsets the selected handles by the grid stepping, but doesn't force the dragged handle into
            // the absolute grid positions (doing so would not make sense for edge handles).
            // When only vertex handles are being dragged, we implement "absolute" moves, forcing the handles into the grid.
            Vector3fT DragDelta=(GetHandles(true, false, true).Size()>0) ?
                  m_MapDoc.SnapToGrid(MousePos_WorldSpace - m_DragHandleCurrentPos, ME.AltDown(), -1 /*Snap all axes (though it doesn't make sense for ThirdAxis here).*/)
                : m_MapDoc.SnapToGrid(MousePos_WorldSpace, ME.AltDown(), -1 /*Snap all axes.*/) - m_DragHandleCurrentPos;

            DragDelta[ThirdAxis]=0.0f;  // Never change anything along the third axis.

            if (length(DragDelta)>0.5f)
            {
                MoveSelectedHandles(DragDelta);

                m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
            }
            break;
        }
    }

    ViewWindow.SetCursor(*NewCursor);
    return true;
}


bool ToolMorphT::OnKeyDown3D(ViewWindow3DT& ViewWindow, wxKeyEvent& KE)
{
    switch (KE.GetKeyCode())
    {
        case WXK_ESCAPE:
        {
            OnEscape(ViewWindow);
            return true;
        }

        case WXK_UP:
        case WXK_DOWN:
        case WXK_LEFT:
        case WXK_RIGHT:
        {
            if (!KE.ShiftDown()) break;
            if (GetHandles(true).Size()==0) break;

            NudgeSelectedHandles(ViewWindow.GetAxesInfo(), KE);

            m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
            return true;
        }
    }

    return false;
}


void ToolMorphT::OnEscape(ViewWindowT& ViewWindow)
{
    switch (m_DragState)
    {
        case DragNothing:
        {
            const ArrayT<MorphHandleT> SelectedHandles=GetHandles(true /*SelectedOnly*/);

            if (SelectedHandles.Size()>0)
            {
                for (unsigned long HandleNr=0; HandleNr<SelectedHandles.Size(); HandleNr++)
                    SelectedHandles[HandleNr].Part->m_Selected=false;
            }
            else
            {
                m_ToolMan.SetActiveTool(GetToolTIM().FindTypeInfoByName("ToolSelectionT"));
            }
            break;
        }

        case DragBoxSelection:
            break;

        case DragMorphHandles:
            // Move back to original positions.
            MoveSelectedHandles(m_DragHandleOrigPos-m_DragHandleCurrentPos);
            break;
    }

    m_DragState=DragNothing;
    m_ToolMan.UpdateAllObservers(this, UPDATE_SOON);
}


// Implementation is analogous to ToolMorphT::OnLMouseDown2D().
bool ToolMorphT::OnLMouseDown3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME)
{
    // When we get here,
    // a) any number (zero or more) of morph handles may be selected,
    // b) a selection box may or may not have been dragged open before,
    // c) we are currently not dragging anything (because this is a mouse *down* event).

    // First make sure that c) is actually true.
    wxASSERT(m_DragState==DragNothing);

    ViewWindow.CaptureMouse();


    MorphHandleT mh;
    if (GetMorphHandleAt(ViewWindow, ME.GetPosition(), mh))
    {
        // The button was pressed over a morph handle whose details have been stored in mh.
        MP_PartT* DragHandle=mh.Part;

        // Depending on the CTRL button state and on whether among the MorphHandlesAtMousePos some were already selected, update the selection.
        if (ME.ControlDown())
        {
            // Just toggle the selection state of the hit handle.
            DragHandle->m_Selected=!DragHandle->m_Selected;
        }
        else
        {
            if (!DragHandle->m_Selected)
            {
                // The handle that was hit is not selected, and CTRL is not down.
                // Thus clear the selection of handles elsewhere, and select the hit handle.
                ArrayT<MorphHandleT> SelectedHandles=GetHandles(true);

                for (unsigned long HandleNr=0; HandleNr<SelectedHandles.Size(); HandleNr++)
                    SelectedHandles[HandleNr].Part->m_Selected=false;

                DragHandle->m_Selected=true;
            }
            else
            {
                // *INTENTIONALLY* do nothing here!
                // The left mouse button was pressed over a selected handle, and CTRL was not down.
                // So the users intention was probably just to "grab" the already selected handle for dragging it
                // (and others that may already be selected at the same time).
            }
        }

        // Intentionally(!) use the DragHandle as-is after the above code for descisions!
        if (DragHandle->m_Selected)
        {
            m_DragHandleOrigPos   =DragHandle->GetPos();
            m_DragHandleCurrentPos=DragHandle->GetPos();
            m_DragState=DragMorphHandles;
        }
        else
        {
            // The click made the handle become unselected, so we are now not dragging anything.
            wxASSERT(m_DragState==DragNothing);
        }

        m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
        return true;
    }


    // Did not hit anything of relevance so far, so check if another map element can be selected for morphing.
    // FIXME: Only do this if the ALT or CTRL (i.e. the "Eyedropper") key is also down!
    ArrayT<ViewWindow3DT::HitInfoT> HitElems=ViewWindow.GetElementsAt(ME.GetPosition());

    if (HitElems.Size()>0)      // Only consider the nearest hit, if any.
    {
        MapPrimitiveT* HitPrim=dynamic_cast<MapPrimitiveT*>(HitElems[0].Object);

        if (HitPrim && (HitPrim->GetType()==&MapBrushT::TypeInfo || HitPrim->GetType()==&MapBezierPatchT::TypeInfo))
        {
            if (!ME.ControlDown()) MorphPrims_CommitAndClear();
            MorphPrims_TogglePrim(HitPrim);

            m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
            return true;
        }
    }


    // Click was neither on a handle nor on a new element for morphing, so just conclude here.
    return true;
}


bool ToolMorphT::OnMouseMove3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME)
{
    const wxCursor* NewCursor=wxSTANDARD_CURSOR;

    switch (m_DragState)
    {
        case DragNothing:
        {
            MorphHandleT DummyMH;

            if (GetMorphHandleAt(ViewWindow, ME.GetPosition(), DummyMH)) NewCursor=&CursorMan->GetCursor(CursorManT::CROSS);
            break;
        }

        case DragBoxSelection:
            // We should never be in this drag state in the 3d views.
            wxASSERT(false);
            break;

        case DragMorphHandles:
        {
            NewCursor=&CursorMan->GetCursor(CursorManT::CROSS);

            // Compute the drag plane for the ViewWindow.
            const AxesInfoT BestAxes=ViewWindow.GetAxesInfo();
            Plane3T<float>  Plane;

            Plane.Normal[BestAxes.ThirdAxis]=1.0f;
            Plane.Dist=m_DragHandleCurrentPos[BestAxes.ThirdAxis];

            try
            {
                // Compute the drag delta.
                const Vector3fT MousePos_WorldSpace=Plane.GetIntersection(ViewWindow.GetCamera().Pos, ViewWindow.WindowToWorld(ME.GetPosition()), 0);

                // As soon as there is at least one edge handle among the dragged handles,
                // we assume that it actually is the edge handle that is being dragged, and thus we make only "relative" moves:
                // A relative move offsets the selected handles by the grid stepping, but doesn't force the dragged handle into
                // the absolute grid positions (doing so would not make sense for edge handles).
                // When only vertex handles are being dragged, we implement "absolute" moves, forcing the handles into the grid.
                Vector3fT DragDelta=(GetHandles(true, false, true).Size()>0) ?
                      m_MapDoc.SnapToGrid(MousePos_WorldSpace - m_DragHandleCurrentPos, ME.AltDown(), -1 /*Snap all axes (though it doesn't make sense for ThirdAxis here).*/)
                    : m_MapDoc.SnapToGrid(MousePos_WorldSpace, ME.AltDown(), -1 /*Snap all axes.*/) - m_DragHandleCurrentPos;

                if (ME.ControlDown()) DragDelta[BestAxes.HorzAxis ]=0.0f;
                if (ME.ShiftDown()  ) DragDelta[BestAxes.VertAxis ]=0.0f;
                /* Always do this: */ DragDelta[BestAxes.ThirdAxis]=0.0f;   // Never change anything along the third axis.

                // Move all the selected handles.
                if (length(DragDelta)>0.5f)
                {
                    MoveSelectedHandles(DragDelta);

                    m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
                }
            }
            catch (const DivisionByZeroE&)
            {
                // A DivisionByZeroE is thrown when the ray from the camera position to the clicked pixel is parallel to
                // the dragging plane. Due to our setup, this should never happen, but if it does, we just ignore the case.
            }
            break;
        }
    }

    ViewWindow.SetCursor(*NewCursor);
    return true;
}


bool ToolMorphT::OnLMouseUp3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME)
{
    if (!ViewWindow.HasCapture()) return false;     // Drag started outside of ViewWindow.
    ViewWindow.ReleaseMouse();

    switch (m_DragState)
    {
        case DragBoxSelection:
            // We should never be in this drag state in the 3d views.
            wxASSERT(false);
            break;

        case DragMorphHandles:
            m_ToolMan.UpdateAllObservers(this, UPDATE_SOON);
            break;

        case DragNothing:
            // Well, it's actually possible to get here and drag nothing, namely when the user CTRL-clicked a previously selected handle,
            // when a new map element was selected for morphing, drag started outside of view, or when ESC was pressed to cancel the drag.
            // wxASSERT(false);
            break;
    }

    // The button went up, so we are not dragging anything any more.
    m_DragState=DragNothing;
    return true;
}


void ToolMorphT::NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection)
{
    if (!IsActiveTool() || m_IsRecursiveSelfNotify) return;

    // An external event caused a selection change, such as the user clicking "Undo".
    //
    //   - What we can *not* do is calling MorphPrims_CommitAndClear(), because that
    //     would attempt to submit another command to the command history while the
    //     command history is attempting to run the "Undo".
    //
    //   - Technically, it would be possible to do nothing: A change in selection
    //     does not require any alterations of our tool state, the user can continue
    //     to morph the objects that he previously begun to morph.
    //
    //   - Although the user might lose some morph work, probably the least confusion
    //     action is to just discard and clear the tool state.
    //
    for (unsigned long MPNr=0; MPNr<m_MorphPrims.Size(); MPNr++)
        delete m_MorphPrims[MPNr];
    m_MorphPrims.Overwrite();

    m_ToolMan.UpdateAllObservers(this, UPDATE_SOON);
    m_DragState=DragNothing;
}


void ToolMorphT::NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements)
{
    if (!IsActiveTool() || m_IsRecursiveSelfNotify) return;

    for (unsigned long i=0; i<MapElements.Size(); i++)
    {
        MapPrimitiveT* MapPrim=dynamic_cast<MapPrimitiveT*>(MapElements[i]);

        if (!MapPrim) continue;

        for (unsigned long j=0; j<m_MorphPrims.Size(); j++)
        {
            if (MapPrim==m_MorphPrims[j]->GetMapPrim())
            {
                // Remove this morph primitive from our list.
                delete m_MorphPrims[j];
                m_MorphPrims.RemoveAtAndKeepOrder(j);

                // The element has been found so we can safely break the inner loop and check the next.
                break;
            }
        }
    }

    m_ToolMan.UpdateAllObservers(this, UPDATE_SOON);
}


void ToolMorphT::NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail)
{
    if (!IsActiveTool() || m_IsRecursiveSelfNotify) return;
    if (Detail!=MEMD_GENERIC && Detail!=MEMD_VISIBILITY) return;

    for (unsigned long i=0; i<MapElements.Size(); i++)
    {
        MapPrimitiveT* MapPrim=dynamic_cast<MapPrimitiveT*>(MapElements[i]);

        if (!MapPrim) continue;

        for (unsigned long j=0; j<m_MorphPrims.Size(); j++)
        {
            if (MapPrim==m_MorphPrims[j]->GetMapPrim())
            {
                // Update the morph primitive of this map element, discarding all prior changes, if any.
                delete m_MorphPrims[j];
                m_MorphPrims[j]=new MorphPrimT(MapPrim);

                // Object has been found so we can safely break the inner loop and check the next object.
                break;
            }
        }
    }

    m_ToolMan.UpdateAllObservers(this, UPDATE_SOON);
}


void ToolMorphT::NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail, const ArrayT<BoundingBox3fT>& OldBounds)
{
    if (!IsActiveTool() || m_IsRecursiveSelfNotify) return;
    if (Detail!=MEMD_GENERIC && Detail!=MEMD_TRANSFORM && Detail!=MEMD_PRIMITIVE_PROPS_CHANGED && Detail!=MEMD_MORPH) return;

    for (unsigned long i=0; i<MapElements.Size(); i++)
    {
        MapPrimitiveT* MapPrim=dynamic_cast<MapPrimitiveT*>(MapElements[i]);

        if (!MapPrim) continue;

        for (unsigned long j=0; j<m_MorphPrims.Size(); j++)
        {
            if (MapPrim==m_MorphPrims[j]->GetMapPrim())
            {
                // Update the morph primitive of this map element, discarding all prior changes, if any.
                delete m_MorphPrims[j];
                m_MorphPrims[j]=new MorphPrimT(MapPrim);

                // Object has been found so we can safely break the inner loop and check the next object.
                break;
            }
        }
    }

    m_ToolMan.UpdateAllObservers(this, UPDATE_SOON);
}


void ToolMorphT::NotifySubjectDies(SubjectT* dyingSubject)
{
    // We should never get here, since the map document always dies after the tool.
    wxASSERT(false);
}
