/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ChildFrameViewWin2D.hpp"
#include "ChildFrame.hpp"
#include "CompMapEntity.hpp"
#include "MapDocument.hpp"
#include "MapEntRepres.hpp"
#include "MapPrimitive.hpp"
#include "Renderer2D.hpp"
#include "Tool.hpp"
#include "ToolManager.hpp"

#include "../AppCaWE.hpp"
#include "../CursorMan.hpp"
#include "../GameConfig.hpp"
#include "../Options.hpp"

#include "wx/wx.h"


using namespace MapEditor;


// Information about coordinate systems:
// a) Mouse events report the mouse position in DEVICE or WINDOW COORDINATES.
//    The origin is the upper left corner of the relevant window (device), the x-axis points right, the y-axis points down, and the unit is pixels.
// b) We convert this to LOGICAL (SCROLLED) WINDOW COORDINATES "1" by adding the position of the scroll bars.
//    (Assuming that the scroll bar units are pixels, too!) This moves the coordinate origin to the upper left corner of the logical area.
// c) The origin is moved to the center of the logical area by subtracting half the range of the scroll bars (given in pixels, too).
//    We call such coordinates the TOOL COORDINATES, or coordinates in TOOL SPACE.
//    Note that the units of such coordinates is still pixels, x-axis points still right, and y-axis points down.
//    Example: MouseEvent.GetPosition() + ViewWindow2D.GetScrollPosXY() - ViewWindow2D.GetScrollRangeXY()/2
// d) This is converted to world space by dividing by the zoom factor and inverting (mirroring) the two axes, if necessary or desired.


ViewWindow2DT::MouseGrabT::MouseGrabT(ViewWindow2DT& ViewWin)
    : m_ViewWin(ViewWin),
      m_IsActive(false),
      m_RefPtWin()
{
}


void ViewWindow2DT::MouseGrabT::Activate(const wxPoint& RefPt)
{
    m_RefPtWin=RefPt;

    // If mouse control is already active, do nothing else.
    if (m_IsActive) return;

    // If some other code has already captured the mouse pointer, do nothing else.
    if (m_ViewWin.HasCapture()) return;

    m_IsActive=true;
    m_ViewWin.CaptureMouse();
    m_ViewWin.SetCursor(CursorMan->GetCursor(CursorManT::HAND_CLOSED));
}


void ViewWindow2DT::MouseGrabT::Deactivate()
{
    // If mouse control is already inactive, do nothing.
    if (!m_IsActive) return;

    m_IsActive=false;
    // wxASSERT(m_ViewWin.HasCapture());    // Fails when called from OnMouseCaptureLost().
    if (m_ViewWin.HasCapture()) m_ViewWin.ReleaseMouse();
    m_ViewWin.SetCursor(*wxSTANDARD_CURSOR);
}


BEGIN_EVENT_TABLE(ViewWindow2DT, wxWindow)
    EVT_KEY_DOWN          (ViewWindow2DT::OnKeyDown         )     // Key events.
    EVT_KEY_UP            (ViewWindow2DT::OnKeyUp           )
    EVT_CHAR              (ViewWindow2DT::OnKeyChar         )
    EVT_LEFT_DOWN         (ViewWindow2DT::OnMouseLeftDown   )     // Mouse events.
    EVT_LEFT_DCLICK       (ViewWindow2DT::OnMouseLeftDown   )
    EVT_LEFT_UP           (ViewWindow2DT::OnMouseLeftUp     )
    EVT_RIGHT_DOWN        (ViewWindow2DT::OnMouseRightDown  )
    EVT_RIGHT_DCLICK      (ViewWindow2DT::OnMouseRightDown  )
    EVT_RIGHT_UP          (ViewWindow2DT::OnMouseRightUp    )
    EVT_MIDDLE_DOWN       (ViewWindow2DT::OnMouseMiddleDown )
    EVT_MIDDLE_DCLICK     (ViewWindow2DT::OnMouseMiddleDown )
    EVT_MIDDLE_UP         (ViewWindow2DT::OnMouseMiddleUp   )
    EVT_MOUSEWHEEL        (ViewWindow2DT::OnMouseWheel      )
    EVT_MOTION            (ViewWindow2DT::OnMouseMove       )
    EVT_CONTEXT_MENU      (ViewWindow2DT::OnContextMenu     )
 // EVT_ENTER_WINDOW      (ViewWindow2DT::OnMouseEnterWindow)
 // EVT_LEAVE_WINDOW      (ViewWindow2DT::OnMouseLeaveWindow)
    EVT_SCROLLWIN         (ViewWindow2DT::OnScroll          )     // Scroll event.
    EVT_PAINT             (ViewWindow2DT::OnPaint           )     // Paint event.
    EVT_SIZE              (ViewWindow2DT::OnSize            )     // Size event.
    EVT_KILL_FOCUS        (ViewWindow2DT::OnKillFocus       )
    EVT_MOUSE_CAPTURE_LOST(ViewWindow2DT::OnMouseCaptureLost)
END_EVENT_TABLE()


#if defined(_WIN32) && defined(_MSC_VER)
// Turn off warning 4355: "'this' : wird in Initialisierungslisten fuer Basisklasse verwendet".
#pragma warning(disable:4355)
#endif


ViewWindow2DT::ViewWindow2DT(wxWindow* Parent, ChildFrameT* ChildFrame, ViewTypeT InitialViewType)
    : wxWindow(Parent, -1, wxDefaultPosition, wxSize(400, 300), wxWANTS_CHARS | wxHSCROLL | wxVSCROLL),
      ViewWindowT(ChildFrame),
      m_ViewType(VT_2D_XY),
      m_AxesInfo(0, false, 1, false),
      m_ZoomFactor(0.12345f),
      m_MouseGrab(*this),
      m_RightMBState(RMB_UP_IDLE),
      m_RDownPosWin()
{
    SetMinSize(wxSize(120, 90));
    SetBackgroundStyle(wxBG_STYLE_PAINT);   // Our paint event handler handles erasing the background.

    SetZoom(0.25);                  // Calls UpdateScrollbars(), which calls SetScrollbar(hor/ver) and Refresh(), then calls SetScrollPos(hor/ver).
    SetViewType(InitialViewType);   // Checks if InitialViewType (which was read from a config file) is valid. Also sets member m_AxesInfo.

    // Center on the center of the map.
    IntrusivePtrT<const CompMapEntityT> World = GetMapDoc().GetRootMapEntity();
    CenterView(World->GetElemsBB().GetCenter());    // Calls SetScrollPos(), then Refresh().
}


void ViewWindow2DT::NotifySubjectChanged(SubjectT* Subject, MapDocOtherDetailT OtherDetail)
{
    Refresh(false);
}


void ViewWindow2DT::NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection)
{
    // Determine the BB over all map elements affected from the selection change.
    BoundingBox3fT ChangeBB;

    for (unsigned long i=0; i<OldSelection.Size(); i++)
        ChangeBB.InsertValid(OldSelection[i]->GetBB());

    for (unsigned long i=0; i<NewSelection.Size(); i++)
        ChangeBB.InsertValid(NewSelection[i]->GetBB());

    // Increase bounds since the angle line and names of entities lie outside element bounds.
    // Note: 24 is the length of the angle vector drawn in the 2D views.
    ChangeBB.Min-=Vector3fT(24.0f, 24.0f, 24.0f);
    ChangeBB.Max+=Vector3fT(24.0f, 24.0f, 24.0f);

    RefreshRect(wxRect(WorldToWindow(ChangeBB.Min), WorldToWindow(ChangeBB.Max)));
}


void ViewWindow2DT::NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities)
{
    BoundingBox3fT BB;

    for (unsigned long i = 0; i < Entities.Size(); i++)
        BB.InsertValid(GetMapEnt(Entities[i])->GetElemsBB());

    // Add an extra margin to the BB, because the angle lines and names of entities are rendered
    // outside of the entity bounding-box (24 is the length of the angle vector drawn in the 2D views).
    BB.Min -= Vector3fT(24.0f, 24.0f, 24.0f);
    BB.Max += Vector3fT(24.0f, 24.0f, 24.0f);

    RefreshRect(wxRect(WorldToWindow(BB.Min), WorldToWindow(BB.Max)));
}


void ViewWindow2DT::NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT<MapPrimitiveT*>& Primitives)
{
    BoundingBox3fT BB;

    for (unsigned long i = 0; i < Primitives.Size(); i++)
        BB.InsertValid(Primitives[i]->GetBB());

    RefreshRect(wxRect(WorldToWindow(BB.Min), WorldToWindow(BB.Max)));
}


void ViewWindow2DT::NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities)
{
    NotifySubjectChanged_Created(Subject, Entities);
}


void ViewWindow2DT::NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT<MapPrimitiveT*>& Primitives)
{
    NotifySubjectChanged_Created(Subject, Primitives);
}


void ViewWindow2DT::NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail)
{
    if (Detail!=MEMD_PRIMITIVE_PROPS_CHANGED && Detail!=MEMD_GENERIC && Detail!=MEMD_ASSIGN_PRIM_TO_ENTITY && Detail!=MEMD_VISIBILITY) return;

    BoundingBox3fT UpdateBounds;

    for (unsigned long i=0; i<MapElements.Size(); i++)
        UpdateBounds.InsertValid(MapElements[i]->GetBB());

    // Increase bounds since the angle line and names of entities lie outside element bounds.
    // Note: 24 is the length of the angle vector drawn in the 2D views.
    UpdateBounds.Min-=Vector3fT(24.0f, 24.0f, 24.0f);
    UpdateBounds.Max+=Vector3fT(24.0f, 24.0f, 24.0f);

    RefreshRect(wxRect(WorldToWindow(UpdateBounds.Min), WorldToWindow(UpdateBounds.Max)));
}


void ViewWindow2DT::NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail, const ArrayT<BoundingBox3fT>& OldBounds)
{
    if (Detail!=MEMD_PRIMITIVE_PROPS_CHANGED && Detail!=MEMD_TRANSFORM && Detail!=MEMD_GENERIC) return;

    BoundingBox3fT UpdateBounds;

    for (unsigned long i=0; i<MapElements.Size(); i++)
    {
        UpdateBounds.InsertValid(MapElements[i]->GetBB());
        UpdateBounds.InsertValid(OldBounds[i]);
    }

    // Increase bounds since the angle line and names of entities lie outside element bounds.
    // Note: 24 is the length of the angle vector drawn in the 2D views.
    UpdateBounds.Min-=Vector3fT(24.0f, 24.0f, 24.0f);
    UpdateBounds.Max+=Vector3fT(24.0f, 24.0f, 24.0f);

    RefreshRect(wxRect(WorldToWindow(UpdateBounds.Min), WorldToWindow(UpdateBounds.Max)));
}


void ViewWindow2DT::Notify_EntChanged(SubjectT* Subject, const ArrayT< IntrusivePtrT<CompMapEntityT> >& Entities, EntityModDetailE Detail)
{
    // TODO: These days, changes in entity properties or components don't bother us. Changes in cf::TypeSys::VarT variables do.
#if 0
    if ((Detail==MEMD_ENTITY_PROPERTY_CREATED || Detail==MEMD_ENTITY_PROPERTY_DELETED || Detail==MEMD_ENTITY_PROPERTY_MODIFIED) && (Key=="angles" || Key=="name"))
    {
        wxASSERT(Entities.Size() > 0);

        // Build bounding box of all elements.
        BoundingBox3fT ElementBounds = Entities[0]->GetRepres()->GetBB();

        for (unsigned long i = 1; i < Entities.Size(); i++)
            ElementBounds.InsertValid(Entities[i]->GetRepres()->GetBB());

        // Increase bounds since the angle line and names are outside the entities bounds.
        // Note: 24 is the length of the angle vector drawn in the 2D views.
        ElementBounds.Min-=Vector3fT(24.0f, 24.0f, 24.0f);
        ElementBounds.Max+=Vector3fT(24.0f, 24.0f, 24.0f);

        RefreshRect(wxRect(WorldToWindow(ElementBounds.Min), WorldToWindow(ElementBounds.Max)));
    }
#endif
}


void ViewWindow2DT::Notify_VarChanged(SubjectT* Subject, const cf::TypeSys::VarBaseT& Var)
{
    if (strcmp(Var.GetName(), "Name") != 0 &&       // A variable of the Basics component?
        strcmp(Var.GetName(), "Show") != 0 &&
        strcmp(Var.GetName(), "Origin") != 0 &&     // A variable of the Transform component?
        strcmp(Var.GetName(), "Orientation") != 0) return;

    // Unfortunately, it seems that there is little we can do to restrict the refresh area, is there?
    // It would help performance if we refreshed only those parts that need it, but as we don't even
    // know the *old* value of Var, it is probably not reliably possible to figure them out.
    Refresh(false);
}


void ViewWindow2DT::NotifySubjectChanged(ToolsSubjectT* Subject, ToolT* Tool, ToolsUpdatePriorityT Priority)
{
    // We have to repaint the tools now even if Priority is UPDATE_SOON, because we cannot
    // defer renderig the tools layer only (calling Refresh() would repaint the map layer as well).
    wxASSERT(&GetMapDoc());     // Can be NULL between Destroy() and the dtor, but between those we should never get here.

    wxClientDC dc(this);
    DoPaint(TOOL_LAYER, dc);
}


wxWindow* ViewWindow2DT::GetWindow()
{
    return this;
}


void ViewWindow2DT::SetZoom(float NewZoom)
{
    const float MIN_ZOOM=1.0/256.0;     // 1/32 == 0.03125
    const float MAX_ZOOM=256.0;

    if (NewZoom<MIN_ZOOM) NewZoom=MIN_ZOOM;
    if (NewZoom>MAX_ZOOM) NewZoom=MAX_ZOOM;

    if (m_ZoomFactor==NewZoom) return;

    // Zoom in on cursor position.
    wxRect  WinRect =wxRect(wxPoint(0, 0), GetClientSize());    // Note: GetClientSize()  is a method of wxWindow.
    wxPoint MousePos=ScreenToClient(wxGetMousePosition());      // Note: ScreenToClient() is a method of wxWindow.

    if (!WinRect.Contains(MousePos))
    {
        // Cursor is not in window, zoom on center instead.
        MousePos.x=WinRect.GetWidth ()/2;
        MousePos.y=WinRect.GetHeight()/2;
    }


    const float OldScrollPointX=GetScrollPos(wxHORIZONTAL)-GetScrollRange(wxHORIZONTAL)/2;
    const float OldScrollPointY=GetScrollPos(wxVERTICAL  )-GetScrollRange(wxVERTICAL  )/2;

    // Position of the mouse pointer in map coords (world units), computed at old zoom level.
    const float MapPosX=(OldScrollPointX+MousePos.x)/m_ZoomFactor;
    const float MapPosY=(OldScrollPointY+MousePos.y)/m_ZoomFactor;

    m_ZoomFactor=NewZoom;
    UpdateScrollbars();

    // Now compute NewScrollPoint, such that the above equations for the same MousePos but *new* m_ZoomFactor still yield the same MapPosX and MapPosY.
    // That is, solve   MapPos=(NewScrollPoint+MousePos)/m_ZoomFactor   for NewScrollPoint:
    const int NewScrollPointX=int(MapPosX*m_ZoomFactor-MousePos.x);
    const int NewScrollPointY=int(MapPosY*m_ZoomFactor-MousePos.y);

    SetScrollPos(wxHORIZONTAL, NewScrollPointX+GetScrollRange(wxHORIZONTAL)/2);
    SetScrollPos(wxVERTICAL,   NewScrollPointY+GetScrollRange(wxVERTICAL  )/2);

    m_ChildFrame->SetStatusText(wxString::Format(" Zoom: %.3f ", m_ZoomFactor), ChildFrameT::SBP_GRID_ZOOM);
}


void ViewWindow2DT::CenterView(const Vector3fT& Point)
{
    // Convert from world space into the logical pixel space.
    float PointX=Point[m_AxesInfo.HorzAxis]*m_ZoomFactor; if (m_AxesInfo.MirrorHorz) PointX=-PointX;
    float PointY=Point[m_AxesInfo.VertAxis]*m_ZoomFactor; if (m_AxesInfo.MirrorVert) PointY=-PointY;

    // Compute the top-left window corner of that space.
    PointX-=GetScrollThumb(wxHORIZONTAL)/2.0f;
    PointY-=GetScrollThumb(wxVERTICAL  )/2.0f;

    // Set the scroll-bars appropriately.
    SetScrollPos(wxHORIZONTAL, int(PointX+GetScrollRange(wxHORIZONTAL)/2));
    SetScrollPos(wxVERTICAL,   int(PointY+GetScrollRange(wxVERTICAL  )/2));

    Refresh();
}


ArrayT<MapElementT*> ViewWindow2DT::GetElementsAt(const wxPoint& Pixel, int Radius) const
{
    wxASSERT(&GetMapDoc());     // Can be NULL between Destroy() and the dtor, but between those we should never get here.

    ArrayT<MapElementT*> Hits;
    ArrayT<MapElementT*> Elems;

    GetMapDoc().GetAllElems(Elems);

    for (unsigned int ElemNr = 0; ElemNr < Elems.Size(); ElemNr++)
    {
        MapElementT* Elem = Elems[ElemNr];

        // Radius is the "epsilon" tolerance in each direction that we allow ourselves for inaccurate clicks.
        if (Elem->IsVisible() && Elem->TracePixel(Pixel, Radius, *this)) Hits.PushBack(Elem);
    }

    return Hits;
}


wxPoint ViewWindow2DT::WindowToTool(const wxPoint& p) const
{
    return wxPoint(p.x+GetScrollPos(wxHORIZONTAL)-GetScrollRange(wxHORIZONTAL)/2,
                   p.y+GetScrollPos(wxVERTICAL  )-GetScrollRange(wxVERTICAL  )/2);
}


wxPoint ViewWindow2DT::ToolToWindow(const wxPoint& p) const
{
    return wxPoint(p.x-GetScrollPos(wxHORIZONTAL)+GetScrollRange(wxHORIZONTAL)/2,
                   p.y-GetScrollPos(wxVERTICAL  )+GetScrollRange(wxVERTICAL  )/2);
}


Vector3fT ViewWindow2DT::ToolToWorld(const wxPoint& p, float Third) const
{
    Vector3fT v;

    v[m_AxesInfo.HorzAxis ]=p.x/m_ZoomFactor;
    v[m_AxesInfo.VertAxis ]=p.y/m_ZoomFactor;
    v[m_AxesInfo.ThirdAxis]=Third;

    if (m_AxesInfo.MirrorHorz) v[m_AxesInfo.HorzAxis] = -v[m_AxesInfo.HorzAxis];
    if (m_AxesInfo.MirrorVert) v[m_AxesInfo.VertAxis] = -v[m_AxesInfo.VertAxis];

    return v;
}


wxPoint ViewWindow2DT::WorldToTool(const Vector3fT& v) const
{
    wxPoint p((int)(v[m_AxesInfo.HorzAxis]*m_ZoomFactor),
              (int)(v[m_AxesInfo.VertAxis]*m_ZoomFactor));

    if (m_AxesInfo.MirrorHorz) p.x=-p.x;
    if (m_AxesInfo.MirrorVert) p.y=-p.y;

    return p;
}


Vector3fT ViewWindow2DT::WindowToWorld(const wxPoint& p, float Third) const
{
    return ToolToWorld(WindowToTool(p), Third);
}


wxPoint ViewWindow2DT::WorldToWindow(const Vector3fT& v) const
{
    return ToolToWindow(WorldToTool(v));
}


/*static*/ bool ViewWindow2DT::RectIsIntersected(const wxRect& Rect, const wxPoint& A, const wxPoint& B)
{
    // Convert the parameters into more manageable items.
    const BoundingBox3fT BB(Vector3fT(Rect.x, Rect.y, -1.0f), Vector3fT(Rect.x+Rect.width, Rect.y+Rect.height, 1.0f));
    const Vector3fT      P1(A.x, A.y, 0.0f);
    const Vector3fT      P2(B.x, B.y, 0.0f);

    if (BB.Contains(P1)) return true;
    if (BB.Contains(P2)) return true;
    if (A==B)            return false;  // Make sure that length(P2-P1)>0.

    float Fraction=1.0f;
    if (!BB.TraceRay(P1, P2-P1, Fraction)) return false;

    return Fraction<1.0f;
}


void ViewWindow2DT::SetViewType(ViewTypeT NewViewType)
{
    m_ViewType=NewViewType;

    if (m_ViewType<VT_2D_XY) m_ViewType=VT_2D_YZ;
    if (m_ViewType>VT_2D_YZ) m_ViewType=VT_2D_XY;

    switch (m_ViewType)
    {
        case VT_2D_XY:
            m_AxesInfo=AxesInfoT(0, false, 1, true);
            break;

        case VT_2D_XZ:
            m_AxesInfo=AxesInfoT(0, false, 2, true);
            break;

        case VT_2D_YZ:
            m_AxesInfo=AxesInfoT(1, false, 2, true);
            break;

        default:
            wxASSERT(false);        // We should never get here.
            SetViewType(VT_2D_XY);  // Recurse!
            break;
    }

    m_ChildFrame->SetCaption(this, GetCaption());
    Refresh();
}


void ViewWindow2DT::ScrollWindow(int AmountX, int AmountY, const wxRect* /*Rect*/)
{
    wxASSERT(&GetMapDoc());     // Can be NULL between Destroy() and the dtor, but between those we should never get here.

    // Make sure we stay within our scrolling range.
    if (AmountX!=0)
    {
        const int PosHor     =GetScrollPos(wxHORIZONTAL);
        const int PageSizeHor=GetScrollThumb(wxHORIZONTAL);
        const int RangeHor   =GetScrollRange(wxHORIZONTAL);

        if (PosHor+AmountX            <       0) AmountX=-PosHor;
        if (PosHor+PageSizeHor+AmountX>RangeHor) AmountX=RangeHor-(PosHor+PageSizeHor);

        SetScrollPos(wxHORIZONTAL, PosHor+AmountX);
    }

    if (AmountY!=0)
    {
        const int PosVert     =GetScrollPos(wxVERTICAL);
        const int PageSizeVert=GetScrollThumb(wxVERTICAL);
        const int RangeVert   =GetScrollRange(wxVERTICAL);

        if (PosVert+AmountY             <        0) AmountY=-PosVert;
        if (PosVert+PageSizeVert+AmountY>RangeVert) AmountY=RangeVert-(PosVert+PageSizeVert);

        SetScrollPos(wxVERTICAL, PosVert+AmountY);
    }

    if (AmountX==0 && AmountY==0) return;

    // Note that the users scrolling UP means scrolling DOWN the contents of the window!
    // Thus, also note that the spec. of this function actually *differs* from the spec. of wxWindow::ScrollWindow(),
    // as we pass in what the user wants (the amount by which the window moves), not by which amount the contents should be scrolled!

    // First scroll the map and tool bitmap.
    wxMemoryDC MapOnlyDC;
    wxMemoryDC MapAndToolsDC;

    MapOnlyDC    .SelectObject(m_BitmapMapOnly    );
    MapAndToolsDC.SelectObject(m_BitmapMapAndTools);

    MapOnlyDC    .Blit(-AmountX, -AmountY, m_BitmapMapOnly    .GetWidth(), m_BitmapMapOnly    .GetHeight(), &MapOnlyDC,     0, 0, wxCOPY);
    MapAndToolsDC.Blit(-AmountX, -AmountY, m_BitmapMapAndTools.GetWidth(), m_BitmapMapAndTools.GetHeight(), &MapAndToolsDC, 0, 0, wxCOPY);

    MapOnlyDC    .SelectObject(wxNullBitmap);
    MapAndToolsDC.SelectObject(wxNullBitmap);

    wxWindow::ScrollWindow(-AmountX, -AmountY);
}


void ViewWindow2DT::UpdateScrollbars()
{
    wxASSERT(&GetMapDoc());     // Can be NULL between Destroy() and the dtor, but between those we should never get here.

    const float  MAX_MAP_COORD=GetMapDoc().GetGameConfig()->GetMaxMapCoord();
    const float  MIN_MAP_COORD=GetMapDoc().GetGameConfig()->GetMinMapCoord();
    const wxSize ClientSize   =GetClientSize();

    // Map units times zoom factor yields pixels!
    SetScrollbar(wxHORIZONTAL,
                 GetScrollPos(wxHORIZONTAL),                                    // Don't change the position of the thumb.
                 ClientSize.x,                                                  // The thumb or page size.
                 int((MAX_MAP_COORD-MIN_MAP_COORD)*m_ZoomFactor+ClientSize.x),  // The maximum position/range. The ClientSize.x is just convenience space.
                 false);

    SetScrollbar(wxVERTICAL,
                 GetScrollPos(wxVERTICAL),                                      // Don't change the position of the thumb.
                 ClientSize.y,                                                  // The thumb or page size.
                 int((MAX_MAP_COORD-MIN_MAP_COORD)*m_ZoomFactor+ClientSize.y),  // The maximum position/range. The ClientSize.y is just convenience space.
                 false);

    Refresh();
}


int ViewWindow2DT::GetBestGridSpacing() const
{
    const int   CurrentSpacing=GetMapDoc().GetGridSpacing();
    const float MinPixSp      =Options.Grid.MinPixelSpacing;

    // Increase the spacing until it is either larger than MinPixSp or the maximum is reached.
    if (CurrentSpacing                *m_ZoomFactor>=MinPixSp) return CurrentSpacing;
    if (Options.Grid.SpacingHighlight1*m_ZoomFactor>=MinPixSp) return Options.Grid.SpacingHighlight1;

    return Options.Grid.SpacingHighlight2;
}


static int clamp(int min, int v, int max)
{
    if (v<min) return min;
    if (v>max) return max;

    return v;
}


void ViewWindow2DT::Render(const wxRect& UpdateRect)
{
    wxASSERT(&GetMapDoc());     // Can be NULL between Destroy() and the dtor, but between those we should never get here.

    const int MAX_MAP_COORD=GetMapDoc().GetGameConfig()->GetMaxMapCoord();
    const int MIN_MAP_COORD=GetMapDoc().GetGameConfig()->GetMinMapCoord();

    wxMemoryDC dc;

    dc.SelectObject(m_BitmapMapOnly);

    dc.DestroyClippingRegion();
    dc.SetClippingRegion(UpdateRect);

    // Setup the DC.
    dc.SetMapMode(wxMM_TEXT);

    // Draw an untranslated square, just for debugging.
    // dc.SetPen(*wxRED_PEN);
    // dc.SetBrush(*wxGREEN_BRUSH);
    // dc.DrawRectangle(20, 20, 20, 20);

    // I believe the next line is equivalent to:
    // dc.SetWindowExt(1000, 1000); dc.SetViewportExt(1000*m_ZoomFactor, 1000*m_ZoomFactor);
    // (Assuming the Mapping Mode is (an-)isotropic, see http://wvware.sourceforge.net/caolan/mapmode.html for a good intro.)
    // However as we do all our scaling ourselves (not the dc), we don't need anything of this at all.
    // dc.SetUserScale(m_ZoomFactor, m_ZoomFactor);

    const wxPoint DevOrigin=GetScrollPosXY()-GetScrollRangeXY()/2;

    // dc.SetUserScale(m_ZoomFactor, m_ZoomFactor);    //cf: is this equivalent to:  dc.SetWindowExt(1000, 1000); dc.SetViewportExt(1000 * m_ZoomFactor, 1000 * m_ZoomFactor);  ???
    // wxLogDebug("Render: Setting device origin to: %i, %i", DevOrigin.x, DevOrigin.y);
    dc.SetDeviceOrigin(-DevOrigin.x, -DevOrigin.y);

    // Draw a square at the world origin, just for debugging.
    // dc.SetPen(*wxGREY_PEN);
    // dc.SetBrush(*wxRED_BRUSH);
    // dc.DrawRectangle(-10, -10, 20, 20);

    // Render the background grid.
    if (GetMapDoc().Is2DGridEnabled())
    {
        const int GridSpacing=GetBestGridSpacing();
        const int hl1        =Options.Grid.ShowHighlight1 ? Options.Grid.SpacingHighlight1 : 0;
        const int hl2        =Options.Grid.ShowHighlight2 ? Options.Grid.SpacingHighlight2 : 0;

        // Convert the ViewWindow2DT dimensions from tool space to world space, then round conservatively
        // such that the bounds are definitively larger than the window dimensions.
        const int MinX=clamp(MIN_MAP_COORD, int(((DevOrigin.x                             )/m_ZoomFactor-GridSpacing)/GridSpacing)*GridSpacing, MAX_MAP_COORD);
        const int MinY=clamp(MIN_MAP_COORD, int(((DevOrigin.y                             )/m_ZoomFactor-GridSpacing)/GridSpacing)*GridSpacing, MAX_MAP_COORD);

        const int MaxX=clamp(MIN_MAP_COORD, int(((DevOrigin.x+GetScrollThumb(wxHORIZONTAL))/m_ZoomFactor+GridSpacing)/GridSpacing)*GridSpacing, MAX_MAP_COORD);
        const int MaxY=clamp(MIN_MAP_COORD, int(((DevOrigin.y+GetScrollThumb(wxVERTICAL  ))/m_ZoomFactor+GridSpacing)/GridSpacing)*GridSpacing, MAX_MAP_COORD);

        // wxLogDebug("Render: Grid goes from (%i, %i) to (%i, %i), Zoom %.3f", MinX, MinY, MaxX, MaxY, m_ZoomFactor);
        // dc.SetPen(*wxRED_PEN);
        // dc.SetBrush(*wxBLUE_BRUSH);
        // dc.DrawRectangle(MinX, MinY, MaxX-MinX, MaxY-MinY);

        if (Options.Grid.UseDottedGrid)
        {
            wxColour OldColor=wxColour(0, 0, 0);

            // Draw the grid as individual dots.
            for (int y=MinY; y<=MaxY; y+=GridSpacing)
                for (int x=MinX; x<=MaxX; x+=GridSpacing)
                {
                    wxColour DotColor;

                         if (                  x==0 ||         y==0 ) DotColor=Options.Grid.ColorAxes;
                    else if (hl2>0 && ((x % hl2)==0 || (y % hl2)==0)) DotColor=Options.Grid.ColorHighlight2;
                    else if (hl1>0 && ((x % hl1)==0 || (y % hl1)==0)) DotColor=Options.Grid.ColorHighlight1;
                    else                                              DotColor=Options.Grid.ColorBaseGrid;

                    // Reduce resetting the pen.
                    if (DotColor!=OldColor)
                    {
                        dc.SetPen(wxPen(DotColor, 1, wxPENSTYLE_SOLID));
                        OldColor=DotColor;
                    }

                    dc.DrawPoint(int(x*m_ZoomFactor), int(y*m_ZoomFactor));
                }
        }
        else
        {
            // 1. Draw the basic background grid.
            dc.SetPen(wxPen(Options.Grid.ColorBaseGrid, 1, wxPENSTYLE_SOLID));

            for (int y=MinY; y<=MaxY; y+=GridSpacing) dc.DrawLine(int(MinX*m_ZoomFactor), int(y*m_ZoomFactor), int(MaxX*m_ZoomFactor), int(y*m_ZoomFactor));
            for (int x=MinX; x<=MaxX; x+=GridSpacing) dc.DrawLine(int(x*m_ZoomFactor), int(MinY*m_ZoomFactor), int(x*m_ZoomFactor), int(MaxY*m_ZoomFactor));

            // 2. Draw the first set of highlight lines (usually every 64 world units).
            if (hl1>0)
            {
                dc.SetPen(wxPen(Options.Grid.ColorHighlight1, 1, wxPENSTYLE_SOLID));

                // This is probably not the best way to write this loop, but it works even if GridSpacing>hl1.
                for (int y=MinY; y<=MaxY; y+=GridSpacing) if ((y % hl1)==0) dc.DrawLine(int(MinX*m_ZoomFactor), int(y*m_ZoomFactor), int(MaxX*m_ZoomFactor), int(y*m_ZoomFactor));
                for (int x=MinX; x<=MaxX; x+=GridSpacing) if ((x % hl1)==0) dc.DrawLine(int(x*m_ZoomFactor), int(MinY*m_ZoomFactor), int(x*m_ZoomFactor), int(MaxY*m_ZoomFactor));
            }

            // 3. Draw the second set of highlight lines (usually every 1024 world units).
            if (hl2>0)
            {
                dc.SetPen(wxPen(Options.Grid.ColorHighlight2, 1, wxPENSTYLE_SOLID));

                // This is probably not the best way to write this loop, but it works even if GridSpacing>hl2.
                for (int y=MinY; y<=MaxY; y+=GridSpacing) if ((y % hl2)==0) dc.DrawLine(int(MinX*m_ZoomFactor), int(y*m_ZoomFactor), int(MaxX*m_ZoomFactor), int(y*m_ZoomFactor));
                for (int x=MinX; x<=MaxX; x+=GridSpacing) if ((x % hl2)==0) dc.DrawLine(int(x*m_ZoomFactor), int(MinY*m_ZoomFactor), int(x*m_ZoomFactor), int(MaxY*m_ZoomFactor));
            }

            // 4. Draw the major axes through the origin.
            dc.SetPen(wxPen(Options.Grid.ColorAxes, 1, wxPENSTYLE_SOLID));

            dc.DrawLine(int(MinX*m_ZoomFactor), 0, int(MaxX*m_ZoomFactor), 0);
            dc.DrawLine(0, int(MinY*m_ZoomFactor), 0, int(MaxY*m_ZoomFactor));
        }
    }


    // Render the world.
    {
        dc.SetFont(*wxNORMAL_FONT);
        dc.SetTextForeground(wxColour(255, 0, 255));
        dc.SetBackgroundMode(wxSOLID);      // Should be dc.SetTEXTBackgroundMode(wxSOLID).
        dc.SetTextBackground(Options.Grid.ColorBackground);

        static ArrayT<MapElementT*> Elems;  // This is obviously not thread safe...

        Elems.Overwrite();
        GetMapDoc().GetAllElems(Elems);

        ToolT*      ActiveTool=m_ChildFrame->GetToolManager().GetActiveTool();
        Renderer2DT Renderer(*this, dc);

        wxRect MovedUpdateRect=UpdateRect;
        MovedUpdateRect.Offset(DevOrigin);

        // Remove all map elements from the list that are invisible, hidden by the active tool, or not in the update rectangle.
        for (unsigned long ElemNr=0; ElemNr<Elems.Size(); ElemNr++)
        {
            if (Elems[ElemNr]->IsVisible() && !ActiveTool->IsHiddenByTool(Elems[ElemNr]))
            {
                const BoundingBox3fT ElemBB  =Elems[ElemNr]->GetBB();
                const wxRect         ElemRect=wxRect(WorldToTool(ElemBB.Min), WorldToTool(ElemBB.Max));

                if (MovedUpdateRect.Intersects(ElemRect)) continue;
            }

            // The element is invisible, hidden by the active tool, or not in the update rectangle.
            Elems.RemoveAt(ElemNr);
            ElemNr--;
        }

        // Render the unselected map elements in the first pass.
        for (unsigned long ElemNr=0; ElemNr<Elems.Size(); ElemNr++)
            if (!Elems[ElemNr]->IsSelected())
                Elems[ElemNr]->Render2D(Renderer);

        // Render the selected map elements in the second pass.
        for (unsigned long ElemNr=0; ElemNr<Elems.Size(); ElemNr++)
            if (Elems[ElemNr]->IsSelected())
                Elems[ElemNr]->Render2D(Renderer);

        // Render the "zigzag" line that has been loaded from a point file and usually leads from an entity to a leak
        // or is the description of a players path through a game map as recorded with the recordPath() console function.
        const ArrayT<PtsPointT>& Points=GetMapDoc().GetPointFilePoints();
        const ArrayT<wxColour>&  Colors=GetMapDoc().GetPointFileColors();

        if (Points.Size()>0 && Colors.Size()>=4)
        {
            if (Colors[1].IsOk())
            {
                Renderer.SetLineType(wxPENSTYLE_SOLID, 2, Colors[1]);
                for (unsigned long PointNr=1; PointNr<Points.Size(); PointNr++)
                    Renderer.DrawLine(Points[PointNr-1].Pos, Points[PointNr].Pos);
            }

            if (Colors[2].IsOk())
            {
                Renderer.SetLineType(wxPENSTYLE_SOLID, 2, Colors[2]);
                for (unsigned long PointNr=0; PointNr<Points.Size(); PointNr++)
                {
                    const Vector3fT& Pos=Points[PointNr].Pos;
                    const float      Hdg=Points[PointNr].Heading/32768.0f*180.0f;

                    Renderer.DrawLine(Pos, Pos+Vector3fT(0.0f, 8.0f, 0.0f).GetRotZ(-Hdg));
                }
            }

            if (Colors[0].IsOk())
            {
                const wxPoint Offset(dc.GetCharHeight()/3, dc.GetCharHeight()/3);

                dc.SetTextForeground(Colors[0]);
                for (unsigned long PointNr=0; PointNr<Points.Size(); PointNr++)
                    Renderer.DrawText(wxString::Format("%f", Points[PointNr].Time), WorldToTool(Points[PointNr].Pos)+Offset);
            }

            if (Colors[3].IsOk())
            {
                const wxPoint Offset(dc.GetCharHeight()/3, -dc.GetCharHeight());

                dc.SetTextForeground(Colors[3]);
                for (unsigned long PointNr=0; PointNr<Points.Size(); PointNr++)
                    Renderer.DrawText(Points[PointNr].Info, WorldToTool(Points[PointNr].Pos)+Offset);
            }
        }
    }

    dc.SelectObject(wxNullBitmap);
}


void ViewWindow2DT::DoPaint(LayerT Layer, wxDC& dc, wxRegion* UpdateRegion)
{
    wxASSERT(&GetMapDoc());     // Can be NULL between Destroy() and the dtor, but between those we should never get here.

    const wxSize ClientSize=GetClientSize();

    if (ClientSize.GetWidth()<1) return;
    if (ClientSize.GetHeight()<1) return;

    if (!m_BitmapMapOnly.IsOk() || m_BitmapMapOnly.GetSize()!=ClientSize)
    {
        // This can happen (under wxMSW) when the ctor of our ChildFrameT parent creates us
        // and then sets the initial tool, which in turn wants to have the tools layer repainted.
        // If so, we will certainly see another (global) repaint request for both layers soon.
        if (Layer!=MAP_AND_TOOL_LAYER) return;

        m_BitmapMapOnly    =wxBitmap(ClientSize);
        m_BitmapMapAndTools=wxBitmap(ClientSize);
    }


    wxMemoryDC MapOnlyDC;
    wxMemoryDC MapAndToolsDC;

    MapOnlyDC    .SelectObject(m_BitmapMapOnly    );
    MapAndToolsDC.SelectObject(m_BitmapMapAndTools);


    // Render the world layer. Note that the world layer can never be re-rendered alone,
    // as the tool layer is always rendered on top of a copy of the map layer.
    if (Layer==MAP_AND_TOOL_LAYER)
    {
        // We could also loop over the rectangles that for the UpdateRegion, see wxRegionIterator documentation for more details.
        wxRect UpdateRectangle=(UpdateRegion!=NULL) ? UpdateRegion->GetBox() : wxRect(wxPoint(0, 0), ClientSize);

        MapOnlyDC.SetPen(wxPen(wxColor(0, 0, 0), 1, wxPENSTYLE_SOLID));
        MapOnlyDC.SetBrush(wxBrush(Options.Grid.ColorBackground, wxBRUSHSTYLE_SOLID));
        MapOnlyDC.DrawRectangle(UpdateRectangle);
#ifndef __WXGTK__
        // On Linux, this seems to invalidate the memory dc(??) for some reason...
        MapOnlyDC.SetBrush(wxNullBrush);
        MapOnlyDC.SetPen(wxNullPen);
#endif

        MapOnlyDC.SelectObject(wxNullBitmap);
        Render(UpdateRectangle);
        MapOnlyDC.SelectObject(m_BitmapMapOnly);
    }

    // The tools are always rendered:
    //   - If we're re-rendering the map, we must re-render the tools (as they are rendered on top of the map).
    //   - If we're re-rendering the tools, we must re-render the tools (per definition ;-) ).
    {
        const wxPoint         DevOrigin  =GetScrollPosXY()-GetScrollRangeXY()/2;
        ToolT*                CurrentTool=m_ChildFrame->GetToolManager().GetActiveTool();
        const ArrayT<ToolT*>& AllTools   =m_ChildFrame->GetToolManager().GetTools();

        // Copy the map to the composite DC.
        MapAndToolsDC.Blit(0, 0, m_BitmapMapOnly.GetWidth(), m_BitmapMapOnly.GetHeight(), &MapOnlyDC, 0, 0, wxCOPY);

        // Set the same device origin for the composite DC / tool rendering as Render() used for rendering the map.
        MapAndToolsDC.SetDeviceOrigin(-DevOrigin.x, -DevOrigin.y);

        // This loop makes one extra iteration, so that the current tool is always rendered last.
        for (unsigned long ToolNr=0; ToolNr<AllTools.Size()+1; ToolNr++)
        {
            ToolT* Tool=(ToolNr<AllTools.Size()) ? AllTools[ToolNr] : CurrentTool;

            if (Tool==NULL) continue;
            if (ToolNr<AllTools.Size() && Tool==CurrentTool) continue;

            Renderer2DT Renderer(*this, MapAndToolsDC);

            Tool->RenderTool2D(Renderer);
        }

        // Have to set this back to the old value, or the blit below will not work as expected.
        MapAndToolsDC.SetDeviceOrigin(0, 0);
    }


    // Blit the new composite bitmap into the window.
    dc.Blit(0, 0, m_BitmapMapAndTools.GetWidth(), m_BitmapMapAndTools.GetHeight(), &MapAndToolsDC, 0, 0, wxCOPY);

    MapOnlyDC    .SelectObject(wxNullBitmap);
    MapAndToolsDC.SelectObject(wxNullBitmap);
}


void ViewWindow2DT::OnKeyDown(wxKeyEvent& KE)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { KE.Skip(); return; }

    // wxMessageBox(wxString::Format("Got a key down event!    :O)\n%lu, '%c', %i", KE.GetKeyCode(), KE.GetKeyCode(), KE.GetKeyCode()-WXK_ADD));

    switch (KE.GetKeyCode())
    {
        case WXK_SPACE:
            // Activate mouse grabbing, but don't wrongly update the reference point on auto-repeat events.
            if (!m_MouseGrab.IsActive())
            {
                m_MouseGrab.Activate(ScreenToClient(wxGetMousePosition()));
            }
            return;

        case 'Z':
            if (m_MouseGrab.IsActive()) m_MouseGrab.Deactivate();
                                   else m_MouseGrab.Activate(ScreenToClient(wxGetMousePosition()));
            return;
    }

    // Give the active tool a chance to intercept the event.
    ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();

    if (Tool)
    {
        if (Tool->OnKeyDown2D(*this, KE)) return;
    }

    // As the tool didn't process the event, we now evaluate the event for some 2D view default action.
    // TODO: Handle more key codes in OnKeyChar() rather than here?
    // (Must handle e.g. the '8' in OnKeyChar() rather than in OnKeyDown(), because otherwise,
    //  pressing '[' (AltGr+8), which is used as menu accelerator key, *also* triggers a key down event for '8'!)
    switch (KE.GetKeyCode())
    {
        case '+':
        case WXK_NUMPAD_ADD:
        {
            SetZoom(m_ZoomFactor*1.2);
            if (KE.ControlDown()) m_ChildFrame->All2DViews_Zoom(m_ZoomFactor);   // If the user desires, set all doc 2d views to this zoom level.
            break;
        }

        case '-':
        case WXK_NUMPAD_SUBTRACT:
        {
            SetZoom(m_ZoomFactor/1.2);
            if (KE.ControlDown()) m_ChildFrame->All2DViews_Zoom(m_ZoomFactor);   // If the user desires, set all doc 2d views to this zoom level.
            break;
        }

        case WXK_UP:    { wxScrollWinEvent SWE(wxEVT_SCROLLWIN_LINEUP,   0, wxVERTICAL  ); OnScroll(SWE); break; }
        case WXK_DOWN:  { wxScrollWinEvent SWE(wxEVT_SCROLLWIN_LINEDOWN, 0, wxVERTICAL  ); OnScroll(SWE); break; }
        case WXK_LEFT:  { wxScrollWinEvent SWE(wxEVT_SCROLLWIN_LINEUP,   0, wxHORIZONTAL); OnScroll(SWE); break; }
        case WXK_RIGHT: { wxScrollWinEvent SWE(wxEVT_SCROLLWIN_LINEDOWN, 0, wxHORIZONTAL); OnScroll(SWE); break; }

        case WXK_TAB:
        {
            // Switch to the next or previous view type (top-down -- side -- front).
            // Note that checking for KE.ControlDown() here is meaningless,
            // because the Control+TAB combination is already caught by the MDI framework.
            const Vector3fT OldCenter=WindowToWorld(GetClientCenter(), GetMapDoc().GetMostRecentSelBB().GetCenter()[m_AxesInfo.ThirdAxis]);

            SetViewType(ViewTypeT(GetViewType() + (KE.ShiftDown() ? -1 : 1)));
            CenterView(OldCenter);
            break;
        }

        default:
            KE.Skip();
            break;
    }
}


void ViewWindow2DT::OnKeyUp(wxKeyEvent& KE)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { KE.Skip(); return; }

    switch (KE.GetKeyCode())
    {
        case WXK_SPACE:
            m_MouseGrab.Deactivate();
            return;
    }

    // Give the active tool a chance to intercept the event.
    ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();

    if (Tool)
    {
        if (Tool->OnKeyUp2D(*this, KE)) return;
    }

    KE.Skip();
}


void ViewWindow2DT::OnKeyChar(wxKeyEvent& KE)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { KE.Skip(); return; }

    // Give the active tool a chance to intercept the event.
    ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();

    if (Tool)
    {
        if (Tool->OnChar2D(*this, KE)) return;
    }

    // As the tool didn't process the event, we now evaluate the event for some 2D view default action.
    // TODO: Handle more key codes here rather than in OnKeyDown() ?
    // (Must handle e.g. the '8' in OnKeyChar() rather than in OnKeyDown(), because otherwise,
    //  pressing '[' (AltGr+8), which is used as menu accelerator key, *also* triggers a key down event for '8'!)
    switch (KE.GetKeyCode())
    {
        // Shortcuts to various zoom levels.
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '0':
        {
            SetZoom(0.03125f * (1 << (KE.GetKeyCode()=='0' ? 9 : KE.GetKeyCode()-'1')));
            break;
        }

        default:
            KE.Skip();
            break;
    }
}


/// Note that we setup the wxWidgets event table so that this method also handles the "double-click" event,
/// because when a double-click occurs, the second mouse down event is otherwise "replaced" by a double-click event.
/// That is, the normal event sequence with a double-click is always as follows:
///    1. mouse button down
///    2. mouse button up
///    3. mouse button double-click (instead of "down")
///    4. mouse button up
/// Therefore, in order to keep the handling simple, we set the event table to send double-clicks also to this method.
/// This is especially very important with capturing the mouse on down events and releasing it on up events!
/// It is still possible to learn whether the event was a normal "down" or a "double-click" event by calling ME.ButtonDClick() for distinction.
void ViewWindow2DT::OnMouseLeftDown(wxMouseEvent& ME)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { ME.Skip(); return; }

    // TODO: At this time, the tools are not prepared for dealing with the window having captured the mouse already...
    if (m_MouseGrab.IsActive()) return;

    // Give the active tool a chance to intercept the event.
    ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();
    if (Tool && Tool->OnLMouseDown2D(*this, ME)) return;

    ME.Skip();
}


void ViewWindow2DT::OnMouseLeftUp(wxMouseEvent& ME)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { ME.Skip(); return; }

    // TODO: At this time, the tools are not prepared for dealing with the window having captured the mouse already...
    if (m_MouseGrab.IsActive()) return;

    // Give the active tool a chance to intercept the event.
    ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();
    if (Tool && Tool->OnLMouseUp2D(*this, ME)) return;

    ME.Skip();
}


void ViewWindow2DT::OnMouseMiddleDown(wxMouseEvent& ME)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { ME.Skip(); return; }

    // Give the active tool a chance to intercept the event.
    ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();
    if (Tool && Tool->OnMMouseDown2D(*this, ME)) return;

    ME.Skip();
}


void ViewWindow2DT::OnMouseMiddleUp(wxMouseEvent& ME)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { ME.Skip(); return; }

    // Give the active tool a chance to intercept the event.
    ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();
    if (Tool && Tool->OnMMouseUp2D(*this, ME)) return;

    ME.Skip();
}


void ViewWindow2DT::OnMouseRightDown(wxMouseEvent& ME)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { ME.Skip(); return; }

    // The active tool cannot intercept the RMB *down* event,
    // because we don't want the tools to be able to shut off our mouse-looking feature.
    if (m_RightMBState==RMB_UP_IDLE)    // Keys on the keyboard may generate RMB down events as well...
    {
        m_RightMBState=RMB_DOWN_UNDECIDED;
        m_RDownPosWin =ME.GetPosition();
    }
}


void ViewWindow2DT::OnMouseRightUp(wxMouseEvent& ME)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { ME.Skip(); return; }

    m_MouseGrab.Deactivate();

    if (m_RightMBState==RMB_DOWN_UNDECIDED)
    {
        // Give the active tool a chance to intercept the event.
        ToolT*     Tool   =m_ChildFrame->GetToolManager().GetActiveTool();
        const bool Handled=Tool && Tool->OnRMouseClick2D(*this, ME);

        if (!Handled)
        {
            // The tool did not handle the event, now show the context menu.
            // Showing the context menu manually here also makes RMB handling uniform
            // across platforms, see <http://trac.wxwidgets.org/ticket/12535> for details.
            wxContextMenuEvent CE(wxEVT_CONTEXT_MENU, GetId(), ClientToScreen(ME.GetPosition()));

            // We don't inline the OnContextMenu() code here, because context menus can also be opened via the keyboard.
            CE.SetEventObject(this);
            OnContextMenu(CE);
        }
    }

    m_RightMBState=RMB_UP_IDLE;
}


void ViewWindow2DT::OnMouseWheel(wxMouseEvent& ME)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { ME.Skip(); return; }

    // Give the active tool a chance to intercept the event.
    ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();

    if (Tool)
    {
        if (Tool->OnMouseWheel2D(*this, ME)) return;
    }

    SetZoom(ME.GetWheelRotation()>0 ? m_ZoomFactor*1.2f : m_ZoomFactor/1.2f);
    if (ME.ControlDown()) m_ChildFrame->All2DViews_Zoom(m_ZoomFactor);
}


void ViewWindow2DT::OnMouseMove(wxMouseEvent& ME)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { ME.Skip(); return; }

    // Ok, there is mouse movement in this view, so update its MRU position in the child frames list.
    UpdateChildFrameMRU();

    // Moving the mouse over one of the 2D or 3D views also assigns the keyboard input focus to it.
    // This saves the user from having to explicitly LMB-click into it, which might have undesired side-effects depending
    // on the currently active tool. However, note that we only want to do this if we're currently the foreground application.
    if (wxGetApp().IsActive())
        if (wxWindow::FindFocus()!=this) SetFocus();

    // Update status bar with the current zoom factor of this view, and the mouse position in world coordinates.
    {
        const wxString  AxesNames[]={ "x:", "y:", "z:" };
        const Vector3fT WorldPos=GetMapDoc().SnapToGrid(WindowToWorld(ME.GetPosition(), 0.0f), ME.AltDown(), -1 /*Snap all axes.*/);

        m_ChildFrame->SetStatusText(wxString::Format(" Zoom: %.3f ", m_ZoomFactor), ChildFrameT::SBP_GRID_ZOOM);
        m_ChildFrame->SetStatusText(" "+AxesNames[m_AxesInfo.HorzAxis]+wxString::Format(" %.0f, ", WorldPos[m_AxesInfo.HorzAxis])
                                       +AxesNames[m_AxesInfo.VertAxis]+wxString::Format(" %.0f",   WorldPos[m_AxesInfo.VertAxis]), ChildFrameT::SBP_MOUSE_POS);
    }

    // If the RMB is held down but still undecided, check the drag threshold.
    if (m_RightMBState==RMB_DOWN_UNDECIDED)
    {
        const wxPoint Drag=m_RDownPosWin-ME.GetPosition();

        if (abs(Drag.x)>3 || abs(Drag.y)>3)
        {
            m_RightMBState=RMB_DOWN_DRAGGING;
            m_MouseGrab.Activate(ME.GetPosition());
        }
    }

    if (m_MouseGrab.IsActive())
    {
        const wxPoint Delta=m_MouseGrab.GetRefPt()-ME.GetPosition();

        ScrollWindow(Delta.x, Delta.y);
        m_MouseGrab.Activate(ME.GetPosition());   // Update the reference point.
    }
    else
    {
        // If we have the mouse capture (e.g. because a tool captured it),
        // make sure that the mouse pointer doesn't leave the window,
        // but instead scroll the map and move the pointer back accordingly.
        const wxRect ClientRect=wxRect(wxPoint(0, 0), GetClientSize());

        if (HasCapture() && !ClientRect.Contains(ME.GetPosition()))
        {
            const wxPoint MousePos=ME.GetPosition();

            // Additional space to move the mouse pointer truly back into the window, not just on its border.
            const int OffsetX=ClientRect.GetWidth ()/5;
            const int OffsetY=ClientRect.GetHeight()/5;

            int ScrollX=0;
            int ScrollY=0;

            if (MousePos.x<                      0) ScrollX=-OffsetX+MousePos.x;                         // Scroll left.
            if (MousePos.x>= ClientRect.GetWidth()) ScrollX= OffsetX+MousePos.x-ClientRect.GetWidth();   // Scroll right.

            if (MousePos.y<                      0) ScrollY=-OffsetY+MousePos.y;                         // Scroll up.
            if (MousePos.y>=ClientRect.GetHeight()) ScrollY= OffsetY+MousePos.y-ClientRect.GetHeight();  // Scroll down.

            if (ScrollX!=0 || ScrollY!=0)
            {
                ScrollWindow(ScrollX, ScrollY);
                WarpPointer(MousePos.x-ScrollX, MousePos.y-ScrollY);

                // Force-moved the pointer, now fix the related mouse event for the tools.
                ME.m_x-=ScrollX;
                ME.m_y-=ScrollY;
            }
        }

        // Give the active tool a chance to intercept the event.
        ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();
        if (Tool && Tool->OnMouseMove2D(*this, ME)) return;
    }
}


void ViewWindow2DT::OnContextMenu(wxContextMenuEvent& CE)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { CE.Skip(); return; }

    wxMenu Menu;

    Menu.AppendRadioItem(VT_2D_XY, "2D Top (X/Y)");
    Menu.AppendRadioItem(VT_2D_XZ, "2D Front (X/Z)");
    Menu.AppendRadioItem(VT_2D_YZ, "2D Side (Y/Z)");
    Menu.Check(GetViewType(), true);

    ToolT*    Tool     =m_ChildFrame->GetToolManager().GetActiveTool();
    const int MenuSelID=Tool ? Tool->OnContextMenu2D(*this, CE, Menu) : GetPopupMenuSelectionFromUser(Menu);

    switch (MenuSelID)
    {
        case VT_2D_XY:
        case VT_2D_XZ:
        case VT_2D_YZ:
        {
            const Vector3fT OldCenter=WindowToWorld(GetClientCenter(), GetMapDoc().GetMostRecentSelBB().GetCenter()[m_AxesInfo.ThirdAxis]);

            SetViewType(ViewTypeT(MenuSelID));
            CenterView(OldCenter);
            break;
        }
    }
}


void ViewWindow2DT::OnScroll(wxScrollWinEvent& SE)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { SE.Skip(); return; }

    // wxMessageBox("Scroll Event!!\n:O)");

    int PageSize    =SE.GetOrientation()==wxHORIZONTAL ? GetClientSize().GetWidth() : GetClientSize().GetHeight();
    int ScrollAmount=0;

    // switch-case doesn't work, as the event types are no constants.
         if (SE.GetEventType()==wxEVT_SCROLLWIN_TOP         ) { /*Should we implement this?*/ }
    else if (SE.GetEventType()==wxEVT_SCROLLWIN_BOTTOM      ) { /*Should we implement this?*/ }
    else if (SE.GetEventType()==wxEVT_SCROLLWIN_LINEUP      ) { ScrollAmount=-PageSize/4; }
    else if (SE.GetEventType()==wxEVT_SCROLLWIN_LINEDOWN    ) { ScrollAmount= PageSize/4; }
    else if (SE.GetEventType()==wxEVT_SCROLLWIN_PAGEUP      ) { ScrollAmount=-PageSize/2; }
    else if (SE.GetEventType()==wxEVT_SCROLLWIN_PAGEDOWN    ) { ScrollAmount= PageSize/2; }
    else if (SE.GetEventType()==wxEVT_SCROLLWIN_THUMBTRACK  ) { /*Intentionally do nothing here.*/ }
    else if (SE.GetEventType()==wxEVT_SCROLLWIN_THUMBRELEASE)
    {
        ScrollAmount=SE.GetPosition()-GetScrollPos(SE.GetOrientation());
        #ifndef __WXMSW__
            // This is a work-around for <http://trac.wxwidgets.org/ticket/2617>.
            // wxLogDebug(wxString::Format("scrollamount %i", ScrollAmount));
            Refresh();
        #endif
    }

    if (ScrollAmount==0) return;

    if (SE.GetOrientation()==wxHORIZONTAL) ScrollWindow(ScrollAmount, 0); else ScrollWindow(0, ScrollAmount);
}


void ViewWindow2DT::OnPaint(wxPaintEvent& PE)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { PE.Skip(); return; }

    wxPaintDC dc(this);
    wxRegion  UpdateRegion(GetUpdateRegion());

    DoPaint(MAP_AND_TOOL_LAYER, dc, &UpdateRegion);
}


void ViewWindow2DT::OnSize(wxSizeEvent& SE)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { SE.Skip(); return; }

    UpdateScrollbars();
}


void ViewWindow2DT::OnKillFocus(wxFocusEvent& FE)
{
    // When we lose the focus, make sure that the mouse cursor is reset and the mouse capture is released.
    m_MouseGrab.Deactivate();
    m_RightMBState=RMB_UP_IDLE;
}


void ViewWindow2DT::OnMouseCaptureLost(wxMouseCaptureLostEvent& ME)
{
    // When we lose the capture, make sure that the mouse cursor is reset and the mouse capture is released.
    m_MouseGrab.Deactivate();
}
