/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "RenderWindow.hpp"
#include "ChildFrame.hpp"
#include "GuiDocument.hpp"

#include "../ParentFrame.hpp"
#include "../AppCaWE.hpp"

#include "MaterialSystem/Renderer.hpp"
#include "GuiSys/Window.hpp"

#include "Math3D/Matrix.hpp"
#include "Math3D/Vector3.hpp"

#include "wx/dcclient.h"


using namespace GuiEditor;


static const float ZOOM_MIN        =1.0f/8.0f;
static const float ZOOM_MAX        =32.0f;
static const float WORKAREA_SPACING=100.0f;


BEGIN_EVENT_TABLE(RenderWindowT, wxGLCanvas)
    EVT_PAINT       (RenderWindowT::OnPaint     )
    EVT_MOUSEWHEEL  (RenderWindowT::OnMouseWheel)
    EVT_SIZE        (RenderWindowT::OnSize      )
    EVT_MOTION      (RenderWindowT::OnMouseMove )
    EVT_LEFT_DOWN   (RenderWindowT::OnLMouseDown)
    EVT_LEFT_UP     (RenderWindowT::OnLMouseUp  )
    EVT_RIGHT_UP    (RenderWindowT::OnRMouseUp  )
    EVT_KEY_DOWN    (RenderWindowT::OnKeyDown   )
    EVT_SCROLLWIN   (RenderWindowT::OnScroll    )
END_EVENT_TABLE()


RenderWindowT::RenderWindowT(GuiEditor::ChildFrameT* Parent)
    : wxGLCanvas(Parent, -1, ParentFrameT::OpenGLAttributeList, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS | wxHSCROLL | wxVSCROLL, "GuiRenderWindow"),
      m_GuiDocument(Parent->GetGuiDoc()),
      m_Parent(Parent),
      m_TimeLastFrame(0),
      m_Zoom(1.0f),
      m_OffsetX(0.0f),
      m_OffsetY(0.0f)
{
}


// Note: Disregarding the details of the subject changes, the render window is always refreshed completely.
void RenderWindowT::NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& OldSelection, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& NewSelection)
{
    Refresh(false);
}


void RenderWindowT::NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows)
{
    Refresh(false);
}


void RenderWindowT::NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows)
{
    Refresh(false);
}


void RenderWindowT::NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GuiSys::WindowT> >& Windows, WindowModDetailE Detail)
{
    Refresh(false);
}


void RenderWindowT::Notify_Changed(SubjectT* Subject, const cf::TypeSys::VarBaseT& Var)
{
    Refresh(false);
}


void RenderWindowT::NotifySubjectDies(SubjectT* dyingSubject)
{
    m_GuiDocument=NULL;
}


Vector2fT RenderWindowT::ClientToGui(int x, int y) const
{
    const float GuiX = (float(x) - m_OffsetX) / m_Zoom;
    const float GuiY = (float(y) - m_OffsetY) / m_Zoom;

    return Vector2fT(GuiX, GuiY);
}


wxPoint RenderWindowT::GuiToClient(const Vector2fT& Pos) const
{
    const int ClientX = int(Pos.x*m_Zoom + m_OffsetX);
    const int ClientY = int(Pos.y*m_Zoom + m_OffsetY);

    return wxPoint(ClientX, ClientY);
}


void RenderWindowT::ZoomIn()
{
    float NewZoom=m_Zoom*1.2f;

    if (NewZoom>ZOOM_MAX) NewZoom=ZOOM_MAX;

    ZoomSet(NewZoom);
}


void RenderWindowT::ZoomOut()
{
    float NewZoom=m_Zoom/1.2f;

    if (NewZoom<ZOOM_MIN) NewZoom=ZOOM_MIN;

    ZoomSet(NewZoom);
}


void RenderWindowT::ZoomFit()
{
    float NewZoom=float(GetClientSize().x)/cf::GuiSys::VIRTUAL_SCREEN_SIZE_X;

    ZoomSet(NewZoom);

    // Need to center the view here because of the workarea spacing.
    CenterView();
    CalcViewOffsets();

    Refresh(false);
}


void RenderWindowT::ZoomSet(float ZoomFactor)
{
    // Zoom in on cursor position.
    wxRect  WinRect =wxRect(wxPoint(0, 0), GetClientSize());
    wxPoint MousePos=ScreenToClient(wxGetMousePosition());

    if (!WinRect.Contains(MousePos))
    {
        // Cursor is not in window, zoom on center instead.
        MousePos.x=WinRect.GetWidth ()/2;
        MousePos.y=WinRect.GetHeight()/2;
    }

    // Calculate mouse position in GUI. We want this position to be be the same after setting the new zoom factor.
    const Vector2fT MouseGui = ClientToGui(MousePos.x, MousePos.y);

    // Set the new zoom factor.
    m_Zoom=ZoomFactor;
    UpdateScrollbars();
    CalcViewOffsets();

    // Caluclate the offset we need to reach the same mouse position in GUI coordinates at the new zoom factor.
    const float TargetOffX=MousePos.x-(MouseGui.x*m_Zoom);
    const float TargetOffY=MousePos.y-(MouseGui.y*m_Zoom);

    // The delta between the current offset and the target offset is also the delta between the scroll positions.
    const int DeltaScrollX=int(m_OffsetX-TargetOffX);
    const int DeltaScrollY=int(m_OffsetY-TargetOffY);

    SetScrollPos(wxHORIZONTAL, GetScrollPos(wxHORIZONTAL)+DeltaScrollX);
    SetScrollPos(wxVERTICAL,   GetScrollPos(wxVERTICAL  )+DeltaScrollY);

    // Recalculate the view offsets according to new scroll position.
    CalcViewOffsets();

    Refresh(false);
}


void RenderWindowT::UpdateScrollbars()
{
    wxSize ClientSize=GetClientSize();

    SetScrollbar(wxHORIZONTAL,
                 GetScrollPos(wxHORIZONTAL),                                       // Don't change the position of the thumb.
                 ClientSize.x,                                                     // The thumb or page size.
                 int((cf::GuiSys::VIRTUAL_SCREEN_SIZE_X+WORKAREA_SPACING)*m_Zoom), // The maximum position/range.
                 true);

    SetScrollbar(wxVERTICAL,
                 GetScrollPos(wxVERTICAL),                                         // Don't change the position of the thumb.
                 ClientSize.y,                                                     // The thumb or page size.
                 int((cf::GuiSys::VIRTUAL_SCREEN_SIZE_Y+WORKAREA_SPACING)*m_Zoom), // The maximum position/range.
                 true);
}


void RenderWindowT::CalcViewOffsets()
{
    wxSize ClientSize=GetClientSize();

    float VirtualSizeX=(cf::GuiSys::VIRTUAL_SCREEN_SIZE_X+WORKAREA_SPACING)*m_Zoom;
    float VirtualSizeY=(cf::GuiSys::VIRTUAL_SCREEN_SIZE_Y+WORKAREA_SPACING)*m_Zoom;

    m_OffsetX=(float(ClientSize.GetWidth ())-VirtualSizeX)/2.0f;
    m_OffsetY=(float(ClientSize.GetHeight())-VirtualSizeY)/2.0f;

    // If the virtual size is bigger than the client size, we get negative offsets. In this case
    // we use the scrollbar position to compute the offset.
    if (m_OffsetX<0) m_OffsetX=-GetScrollPos(wxHORIZONTAL);
    if (m_OffsetY<0) m_OffsetY=-GetScrollPos(wxVERTICAL);

    // Half of the workarea spacing is on the left side of the work area, so we need to add it to the offset.
    m_OffsetX+=WORKAREA_SPACING/2.0f*m_Zoom;
    m_OffsetY+=WORKAREA_SPACING/2.0f*m_Zoom;
}


void RenderWindowT::ScrollWindow(int AmountX, int AmountY)
{
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
}


void RenderWindowT::CenterView()
{
    // Set the scroll position so the view is centered.
    SetScrollPos(wxHORIZONTAL, (GetScrollRange(wxHORIZONTAL)-GetScrollThumb(wxHORIZONTAL))/2);
    SetScrollPos(wxVERTICAL,   (GetScrollRange(wxVERTICAL  )-GetScrollThumb(wxVERTICAL  ))/2);
}


void RenderWindowT::OnPaint(wxPaintEvent& PE)
{
    // Guard against accessing an already deleted GuiDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (m_GuiDocument==NULL) { PE.Skip(); return; }

    wxPaintDC dc(this);     // It is VERY important not to omit this, or otherwise everything goes havoc.

    if (!wxGetApp().IsActive()) return;

    // We're drawing to this view now.
    SetCurrent(*wxGetApp().GetParentFrame()->m_GLContext);    // This is the method from the wxGLCanvas for activating the given RC with this window.
    wxSize CanvasSize=GetClientSize();

    MatSys::Renderer->SetViewport(0, 0, CanvasSize.GetWidth(), CanvasSize.GetHeight());

    // Determine how much time has passed since the previous frame.
    unsigned long TimeNow=::wxGetLocalTimeMillis().GetLo();
 // unsigned long TimeElapsed=(m_TimeLastFrame==0) ? 0 : TimeNow-m_TimeLastFrame;

    m_TimeLastFrame=TimeNow;

    // Clear the buffers.
    MatSys::Renderer->ClearColor(0, 0, 0, 0);
    MatSys::Renderer->BeginFrame(TimeNow/1000.0);

    MatSys::Renderer->SetCurrentRenderAction(MatSys::RendererI::AMBIENT);
    MatSys::Renderer->SetCurrentLightMap(wxGetApp().GetParentFrame()->m_WhiteTexture);    // Set a proper default lightmap.
    MatSys::Renderer->SetCurrentLightDirMap(NULL);      // The MatSys provides a default for LightDirMaps when NULL is set.

    // Setup the matrices.
    MatSys::Renderer->PushMatrix(MatSys::RendererI::PROJECTION    );
    MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    MatSys::Renderer->PushMatrix(MatSys::RendererI::WORLD_TO_VIEW );

    // Calculate coordinates for ortho projection.
    const Vector2fT TopLeft     = ClientToGui(0, 0);
    const Vector2fT BottomRight = ClientToGui(CanvasSize.GetWidth()-1, CanvasSize.GetHeight()-1);

    const float zNear=0.0f;
    const float zFar =1.0f;
    MatSys::Renderer->SetMatrix(MatSys::RendererI::PROJECTION,     MatrixT::GetProjOrthoMatrix(TopLeft.x, BottomRight.x, BottomRight.y, TopLeft.y, zNear, zFar));
    MatSys::Renderer->SetMatrix(MatSys::RendererI::MODEL_TO_WORLD, MatrixT());
    MatSys::Renderer->SetMatrix(MatSys::RendererI::WORLD_TO_VIEW,  MatrixT());

    m_GuiDocument->GetRootWindow()->Render();

    ToolI* ActiveTool=m_Parent->GetToolManager()->GetActiveTool();

    if (ActiveTool) ActiveTool->RenderTool(this);

    // Restore the previously active matrices.
    MatSys::Renderer->PopMatrix(MatSys::RendererI::PROJECTION    );
    MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    MatSys::Renderer->PopMatrix(MatSys::RendererI::WORLD_TO_VIEW );

    MatSys::Renderer->EndFrame();
    SwapBuffers();
}


void RenderWindowT::OnMouseWheel(wxMouseEvent& ME)
{
    if (ME.GetWheelRotation()>0) ZoomIn();
    else                         ZoomOut();
}


void RenderWindowT::OnSize(wxSizeEvent& SE)
{
    UpdateScrollbars();

    CalcViewOffsets();

    Refresh(false);
}


void RenderWindowT::OnMouseMove(wxMouseEvent& ME)
{
    // Reset focus on mouse move, so zooming and scrolling by cursor keys always works, when the mouse is over the window.
    if (FindFocus()!=this) SetFocus();

    ToolI* ActiveTool=m_Parent->GetToolManager()->GetActiveTool();

    if (ActiveTool) ActiveTool->OnMouseMove(this, ME);
}


void RenderWindowT::OnLMouseDown(wxMouseEvent& ME)
{
    ToolI* ActiveTool=m_Parent->GetToolManager()->GetActiveTool();

    if (ActiveTool) ActiveTool->OnLMouseDown(this, ME);
}


void RenderWindowT::OnLMouseUp(wxMouseEvent& ME)
{
    ToolI* ActiveTool=m_Parent->GetToolManager()->GetActiveTool();

    if (ActiveTool) ActiveTool->OnLMouseUp(this, ME);
}


void RenderWindowT::OnRMouseUp(wxMouseEvent& ME)
{
    ToolI* ActiveTool=m_Parent->GetToolManager()->GetActiveTool();

    if (ActiveTool) ActiveTool->OnRMouseUp(this, ME);
}


void RenderWindowT::OnKeyDown(wxKeyEvent& KE)
{
    ToolI* ActiveTool=m_Parent->GetToolManager()->GetActiveTool();

    if (ActiveTool && ActiveTool->OnKeyDown(this, KE)) return;

    switch (KE.GetKeyCode())
    {
        case '+':
        case WXK_NUMPAD_ADD:
            ZoomIn();
            break;

        case '-':
        case WXK_NUMPAD_SUBTRACT:
            ZoomOut();
            break;

        case WXK_UP:    { wxScrollWinEvent SWE(wxEVT_SCROLLWIN_LINEUP,   0, wxVERTICAL  ); OnScroll(SWE); break; }
        case WXK_DOWN:  { wxScrollWinEvent SWE(wxEVT_SCROLLWIN_LINEDOWN, 0, wxVERTICAL  ); OnScroll(SWE); break; }
        case WXK_LEFT:  { wxScrollWinEvent SWE(wxEVT_SCROLLWIN_LINEUP,   0, wxHORIZONTAL); OnScroll(SWE); break; }
        case WXK_RIGHT: { wxScrollWinEvent SWE(wxEVT_SCROLLWIN_LINEDOWN, 0, wxHORIZONTAL); OnScroll(SWE); break; }

        default:
            KE.Skip();
            break;
    }
}


void RenderWindowT::OnScroll(wxScrollWinEvent& SE)
{
    int PageSize    =SE.GetOrientation()==wxHORIZONTAL ? GetClientSize().GetWidth() : GetClientSize().GetHeight();
    int ScrollAmount=0;

    // switch-case doesn't work, as the events types are no constants.
         if (SE.GetEventType()==wxEVT_SCROLLWIN_TOP         ) { /*Should we implement this?*/ }
    else if (SE.GetEventType()==wxEVT_SCROLLWIN_BOTTOM      ) { /*Should we implement this?*/ }
    else if (SE.GetEventType()==wxEVT_SCROLLWIN_LINEUP      ) { ScrollAmount=-PageSize/4; }
    else if (SE.GetEventType()==wxEVT_SCROLLWIN_LINEDOWN    ) { ScrollAmount= PageSize/4; }
    else if (SE.GetEventType()==wxEVT_SCROLLWIN_PAGEUP      ) { ScrollAmount=-PageSize/2; }
    else if (SE.GetEventType()==wxEVT_SCROLLWIN_PAGEDOWN    ) { ScrollAmount= PageSize/2; }
    else if (SE.GetEventType()==wxEVT_SCROLLWIN_THUMBTRACK  ) { /*Intentionally do nothing here.*/ }
    else if (SE.GetEventType()==wxEVT_SCROLLWIN_THUMBRELEASE) { ScrollAmount=SE.GetPosition()-GetScrollPos(SE.GetOrientation()); }

    if (ScrollAmount==0) return;

    if (SE.GetOrientation()==wxHORIZONTAL) ScrollWindow(ScrollAmount, 0); else ScrollWindow(0, ScrollAmount);

    CalcViewOffsets();

    Refresh(false);
}
