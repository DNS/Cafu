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

#include "ChildFrameViewWin3D.hpp"
#include "AppCaWE.hpp"
#include "AxesInfo.hpp"
#include "Camera.hpp"
#include "ChildFrame.hpp"
#include "ParentFrame.hpp"
#include "MapDocument.hpp"
#include "MapElement.hpp"
#include "Options.hpp"
#include "ToolManager.hpp"
#include "ToolCamera.hpp"

#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"

#include "wx/wx.h"


#if defined(_WIN32) && defined(_MSC_VER)
// Turn off warning 4355: "'this' : wird in Initialisierungslisten fuer Basisklasse verwendet".
#pragma warning(disable:4355)
#endif


ViewWindow3DT::MouseControlT::MouseControlT(ViewWindow3DT& ViewWin)
    : m_ViewWin(ViewWin),
      m_State(NOT_ACTIVE)
{
}


void ViewWindow3DT::MouseControlT::Activate(StateT NewState, const wxPoint& RefPt)
{
    // Did someone use this method for deactivating mouse control?
    if (NewState==NOT_ACTIVE) { Deactivate(); return; }

    // If mouse control is already active (and attempted to re-activate, i.e. the state changes from ACTIVE_* to ACTIVE_*),
    // do nothing, because we want to have the next state of ACTIVE_* always be NOT_ACTIVE.
    if (m_State!=NOT_ACTIVE) return;

    m_State=NewState;
    m_RefPtWin=(RefPt==wxDefaultPosition) ? wxPoint(m_ViewWin.GetClientSize().x/2, m_ViewWin.GetClientSize().y/2) : RefPt;

    const ArrayT<ViewWindow3DT::HitInfoT> Hits=m_ViewWin.GetElementsAt(m_RefPtWin);
    m_RefPtWorld=Hits.Size()>0 ? Hits[0].Pos : m_ViewWin.GetCamera().Pos;

    if (!m_ViewWin.HasCapture()) m_ViewWin.CaptureMouse();
    m_ViewWin.SetCursor(wxCURSOR_BLANK);
    m_ViewWin.WarpPointer(m_RefPtWin.x, m_RefPtWin.y);
}


void ViewWindow3DT::MouseControlT::Deactivate()
{
    // The mouse control is deactivated (the state changes from ACTIVE_* to NOT_ACTIVE),
    // so propagate the changes to the camera also to the other observers.
    if (m_State!=NOT_ACTIVE) m_ViewWin.m_CameraTool->NotifyCameraChanged(m_ViewWin.m_Camera);

    m_State=NOT_ACTIVE;

    if (m_ViewWin.HasCapture()) m_ViewWin.ReleaseMouse();
    m_ViewWin.SetCursor(*wxSTANDARD_CURSOR);
}


BEGIN_EVENT_TABLE(ViewWindow3DT, wxGLCanvas)
    EVT_KEY_DOWN          (ViewWindow3DT::OnKeyDown         )     // Key events.
    EVT_KEY_UP            (ViewWindow3DT::OnKeyUp           )
    EVT_CHAR              (ViewWindow3DT::OnKeyChar         )
    EVT_LEFT_DOWN         (ViewWindow3DT::OnMouseLeftDown   )     // Mouse events.
    EVT_LEFT_DCLICK       (ViewWindow3DT::OnMouseLeftDown   )
    EVT_LEFT_UP           (ViewWindow3DT::OnMouseLeftUp     )
    EVT_MIDDLE_DOWN       (ViewWindow3DT::OnMouseMiddleDown )
    EVT_MIDDLE_DCLICK     (ViewWindow3DT::OnMouseMiddleDown )
    EVT_MIDDLE_UP         (ViewWindow3DT::OnMouseMiddleUp   )
    EVT_RIGHT_DOWN        (ViewWindow3DT::OnMouseRightDown  )
    EVT_RIGHT_DCLICK      (ViewWindow3DT::OnMouseRightDown  )
    EVT_RIGHT_UP          (ViewWindow3DT::OnMouseRightUp    )
    EVT_MOUSEWHEEL        (ViewWindow3DT::OnMouseWheel      )
    EVT_MOTION            (ViewWindow3DT::OnMouseMove       )
    EVT_CONTEXT_MENU      (ViewWindow3DT::OnContextMenu     )
 // EVT_ENTER_WINDOW      (ViewWindow3DT::OnMouseEnterWindow)
 // EVT_LEAVE_WINDOW      (ViewWindow3DT::OnMouseLeaveWindow)
    EVT_PAINT             (ViewWindow3DT::OnPaint           )     // Paint event.
    EVT_SIZE              (ViewWindow3DT::OnSize            )     // Size event.
    EVT_SET_FOCUS         (ViewWindow3DT::OnSetFocus        )     // Focus event.
    EVT_KILL_FOCUS        (ViewWindow3DT::OnKillFocus       )
    EVT_MOUSE_CAPTURE_LOST(ViewWindow3DT::OnMouseCaptureLost)
END_EVENT_TABLE()


ViewWindow3DT::ViewWindow3DT(wxWindow* Parent, ChildFrameT* ChildFrame, CameraT* InitialCamera, ViewTypeT InitialViewType)
    : wxGLCanvas(Parent, -1, ParentFrameT::OpenGLAttributeList, wxDefaultPosition, wxSize(400, 300), wxWANTS_CHARS, "ViewWin3DCanvas"),
      ViewWindowT(ChildFrame),
      m_ViewType(InitialViewType),
      m_Renderer(*this),
      m_TimeOfLastPaint(0),
      m_CameraTool(static_cast<ToolCameraT*>(m_ChildFrame->GetToolManager().GetTool(ToolCameraT::TypeInfo))),
      m_Camera(InitialCamera ? InitialCamera : m_CameraTool->GetCameras()[0]),
      m_CameraVel(),
      m_MouseControl(*this),
      m_RightMBState(RMB_UP_IDLE),
      m_RDownPosWin()
{
    // Make sure that if we were given an InitialCamera for use, is known to the Camera tool
    // (the AddCamera() method in turn makes sure that InitialCamera is not added twice).
    if (InitialCamera) m_CameraTool->AddCamera(InitialCamera);

    SetMinSize(wxSize(120, 90));

    // The initial view type was read from a config file - make sure that it is valid.
    if (m_ViewType<VT_3D_WIREFRAME) m_ViewType=VT_3D_WIREFRAME;
}


ViewWindow3DT::~ViewWindow3DT()
{
    // I'm not sure whether this code is in the right place,
    // because wxGLCanvas::SetCurrent() (currently) requires that the related parent windows IsShown() method returns true.
    // However, when we ALT+F4 close an application with child frames open, wxWidgets first closes all windows before their
    // dtor is called...  so we should probably move this into the EVT_CLOSE handler of our child frame???
    if (wxGetApp().GetParentFrame()->IsShown())
        wxGetApp().GetParentFrame()->m_GLCanvas->SetCurrent(*wxGetApp().GetParentFrame()->m_GLContext);
}


void ViewWindow3DT::NotifySubjectChanged(ToolsSubjectT* Subject, ToolT* Tool, ToolsUpdatePriorityT Priority)
{
    wxASSERT((Tool->GetType()==&ToolCameraT::TypeInfo)==(Tool==m_CameraTool));

    if (Tool==m_CameraTool && m_CameraTool->GetActiveCamera()!=m_Camera)
    {
        // A camera has changed, but its not the one we use for this view.
        // So don't waste performance with updating when nothing has changed.
        return;
    }

    switch (Priority)
    {
        case UPDATE_NOW:
            Refresh(false /*eraseBackground*/);
            Update();
            break;

        case UPDATE_SOON:
            // No need to call Refresh(false) here as well,
            // just wait for the next idle event to trigger the repaint.
            break;
    }
}


wxWindow* ViewWindow3DT::GetWindow()
{
    return this;
}


ViewWindowT::ViewTypeT ViewWindow3DT::GetViewType() const
{
    // Note that keeping m_ViewType as a member is much faster than reading it from
    // a control on each frame. See <http://trac.cafu.de/changeset/163> for details.
    return m_ViewType;
}


/// Returns the number of the main axis that the given vector v is closest to.
static int ClosestAxis(const Vector3fT& v)
{
    const Vector3fT MainAxes[6]=
    {
        Vector3fT( 1.0f,  0.0f,  0.0f),
        Vector3fT( 0.0f,  1.0f,  0.0f),
        Vector3fT( 0.0f,  0.0f,  1.0f),
        Vector3fT(-1.0f,  0.0f,  0.0f),
        Vector3fT( 0.0f, -1.0f,  0.0f),
        Vector3fT( 0.0f,  0.0f, -1.0f)
    };

    float BestCosAng=-1.0f;
    int   BestAxisNr=0;

    for (int AxisNr=0; AxisNr<6; AxisNr++)
    {
        const float CosAng=dot(v, MainAxes[AxisNr]);

        if (CosAng>BestCosAng)
        {
            BestCosAng=CosAng;
            BestAxisNr=AxisNr;
        }
    }

    return BestAxisNr;
}


AxesInfoT ViewWindow3DT::GetAxesInfo() const
{
    const int Horizontal=ClosestAxis(m_Camera->GetXAxis());
    const int Vertical  =ClosestAxis(m_Camera->GetZAxis());

    return AxesInfoT(Horizontal % 3, Horizontal>2, Vertical % 3, Vertical<3);
}


void ViewWindow3DT::GetViewFrustum(Plane3fT* Planes, unsigned int NumPlanes) const
{
    const wxSize  CanvasSize=GetClientSize();
    const MatrixT ViewToClip=MatrixT::GetProjPerspectiveMatrix(m_Camera->VerticalFOV, (float)CanvasSize.GetWidth()/(float)CanvasSize.GetHeight(),
                                                               m_Camera->NearPlaneDist, m_Camera->FarPlaneDist);

    MatrixT WorldToView=m_Camera->GetMatrix();

    // Rotate by 90 degrees around the x-axis in order to meet the MatSys's expectation of axes orientation.
    // TODO: Why do we still need this...?
    for (unsigned long i=0; i<4; i++)
    {
        std::swap(WorldToView[1][i], WorldToView[2][i]);
        WorldToView[2][i]=-WorldToView[2][i];
    }

    const MatrixT mpv=ViewToClip * WorldToView;

    // Compute the six view frustum planes in object space. The order is: left, right, bottom, top, near, far.
    if (NumPlanes>6) NumPlanes=6;

    for (unsigned long PlaneNr=0; PlaneNr<NumPlanes; PlaneNr++)
    {
        // m can be used to easily minify / shrink the view frustum. This is a nice toy for debugging.
        // The values should be between 0 and 8. 0 is the default (no minification), 8 is the reasonable maximum.
        const float m=0;
        const float d=(PlaneNr<4 && m>0) ? 1.0f-0.75f*m/8.0f : 1.0f;
        float       PlaneCoeffs[4];

        for (unsigned long j=0; j<4; j++)
            PlaneCoeffs[j]=((PlaneNr & 1) ? mpv.m[PlaneNr/2][j] : -mpv.m[PlaneNr/2][j]) - d*mpv.m[3][j];

        Planes[PlaneNr].Normal=Vector3fT(PlaneCoeffs);
        Planes[PlaneNr].Dist  =PlaneCoeffs[3];

        const float l=length(Planes[PlaneNr].Normal);

        Planes[PlaneNr].Normal/=l;
        Planes[PlaneNr].Dist=-Planes[PlaneNr].Dist/l;
    }
}


void ViewWindow3DT::MoveCamera(const Vector3fT& NewPos)
{
    m_Camera->Pos=NewPos;
    m_CameraTool->NotifyCameraChanged(m_Camera);
}


static bool CompareHitDepths(const ViewWindow3DT::HitInfoT& Hit1, const ViewWindow3DT::HitInfoT& Hit2)
{
    return Hit1.Depth<Hit2.Depth;
}


class RayIntersectionTestT : public IterationHandlerI
{
    public:

    /// The method that is called back on each element of the iteration.
    bool Handle(MapElementT* Child)
    {
        float         Fraction=0;
        unsigned long FaceNr=0;

        if (Child->IsVisible() && Child->TraceRay(RayOrigin, RayDir, Fraction, FaceNr))
        {
            ViewWindow3DT::HitInfoT Hit;

            Hit.Object=Child;
            Hit.FaceNr=FaceNr;
            Hit.Depth =Fraction;
            Hit.Pos   =RayOrigin + RayDir*Fraction;

            Hits.PushBack(Hit);
        }

        return true;
    }


    ArrayT<ViewWindow3DT::HitInfoT> Hits;
    Vector3fT                       RayOrigin;
    Vector3fT                       RayDir;
};


ArrayT<ViewWindow3DT::HitInfoT> ViewWindow3DT::GetElementsAt(const wxPoint& Pixel) const
{
    wxASSERT(&GetMapDoc());     // Can be NULL between Destroy() and the dtor, but between those we should never get here.

    RayIntersectionTestT RayIntTest;

    // Note that our ray does intentionally not start at m_Camera->Pos,
    // but at the point of intersection with the near clipping plane!
    RayIntTest.RayOrigin=WindowToWorld(Pixel);
    RayIntTest.RayDir   =normalizeOr0(RayIntTest.RayOrigin - m_Camera->Pos);

    // Make sure that the ray is valid. It should never be invalid though.
    if (length(RayIntTest.RayDir)<0.9f) return RayIntTest.Hits;

    // This just iterates over all elements in the world (entities and primitives, but not the bare world entity itself) in a brute force manner.
    // It would certainly be nice to optimize this in some way, e.g. by making use of the BSP tree,
    // but our Ray-AABB intersection tests are really fast, so this is OK for now.
    GetMapDoc().IterateElems(RayIntTest);

    RayIntTest.Hits.QuickSort(CompareHitDepths);
    return RayIntTest.Hits;
}


Vector3fT ViewWindow3DT::WindowToWorld(const wxPoint& Pixel) const
{
    // First transform the pixel manually from window (viewport) space to clip space.
    const int       ViewX      =0;
    const int       ViewY      =0;
    const float     ViewWidth  =GetClientSize().GetWidth();
    const float     ViewHeight =GetClientSize().GetHeight();
    const Vector3fT Clip       =Vector3fT(2.0f*(Pixel.x-ViewX)/ViewWidth-1.0f, -2.0f*(Pixel.y-ViewY)/ViewHeight+1.0f, 0.0f);

    // Then transform it from clip space to world space using the appropriate matrices.
    MatrixT         WorldToView=m_Camera->GetMatrix();
    const MatrixT   ViewToClip =MatrixT::GetProjPerspectiveMatrix(m_Camera->VerticalFOV, ViewWidth/ViewHeight, m_Camera->NearPlaneDist, m_Camera->FarPlaneDist);

    // Rotate by 90 degrees around the x-axis in order to meet the MatSys's expectation of axes orientation.
    // TODO: Why do we still need this...?
    for (unsigned long i=0; i<4; i++)
    {
        std::swap(WorldToView[1][i], WorldToView[2][i]);
        WorldToView[2][i]=-WorldToView[2][i];
    }

    const MatrixT   ClipToWorld=(ViewToClip * WorldToView).GetInverse();
    const Vector3fT World      =ClipToWorld.ProjectPoint(Clip);

    return World;
}


wxPoint ViewWindow3DT::WorldToWindow(const Vector3fT& v, bool CheckFrustum) const
{
    const int     ViewX      =0;
    const int     ViewY      =0;
    const float   ViewWidth  =GetClientSize().GetWidth();
    const float   ViewHeight =GetClientSize().GetHeight();
    MatrixT       WorldToView=m_Camera->GetMatrix();
    const MatrixT ViewToClip =MatrixT::GetProjPerspectiveMatrix(m_Camera->VerticalFOV, ViewWidth/ViewHeight, m_Camera->NearPlaneDist, m_Camera->FarPlaneDist);

    // Rotate by 90 degrees around the x-axis in order to meet the MatSys's expectation of axes orientation.
    // TODO: Why do we still need this...?
    for (unsigned long i=0; i<4; i++)
    {
        std::swap(WorldToView[1][i], WorldToView[2][i]);
        WorldToView[2][i]=-WorldToView[2][i];
    }

    const MatrixT WorldToClip=ViewToClip * WorldToView;

    if (CheckFrustum)
    {
        // This is essentially a repetition of the code in GetViewFrustum(),
        // but having it here avoids computing all the above matrices twice.
        // It also allows for massive simplifications, because we don't have to normalize the planes normal vectors!
        for (unsigned long PlaneNr=0; PlaneNr<6; PlaneNr++)
        {
            float Dist=0.0f;

            for (unsigned long j=0; j<4; j++)
                Dist+=(((PlaneNr & 1) ? WorldToClip.m[PlaneNr/2][j] : -WorldToClip.m[PlaneNr/2][j]) - WorldToClip.m[3][j]) * (j<3 ? v[j] : 1.0f);

            if (Dist>0.0f) return wxPoint(-1, -1);
        }
    }

    // First transform the vector from world space to clip space using the appropriate matrices,
    // then transform it manually from clip space to window (viewport) space.
    const Vector3fT Clip =WorldToClip.ProjectPoint(v);
    const wxPoint   Pixel=wxPoint(int(0.5f*( Clip.x+1.0f)*ViewWidth )+ViewX,
                                  int(0.5f*(-Clip.y+1.0f)*ViewHeight)+ViewY);

    return Pixel;
}


void ViewWindow3DT::OnKeyDown(wxKeyEvent& KE)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { KE.Skip(); return; }

    // wxLogDebug("ViewWindow3DT::OnKeyDown(), KE.GetKeyCode()==%lu", KE.GetKeyCode());

    switch (KE.GetKeyCode())
    {
        case WXK_SPACE:
            m_MouseControl.Activate(MouseControlT::ACTIVE_NORMAL);
            return;

        case 'Z':
            m_MouseControl.Activate(m_MouseControl.IsActive() ? MouseControlT::NOT_ACTIVE : MouseControlT::ACTIVE_NORMAL);
            return;
    }

    // Give the active tool a chance to intercept the event.
    ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();
    if (Tool && Tool->OnKeyDown3D(*this, KE)) return;

    switch (KE.GetKeyCode())
    {
        case WXK_TAB:
        {
            // Cycle the view to the next or previous 3D view type.
            // Note that checking for KE.ControlDown() here is meaningless,
            // because the Control+TAB combination is already caught by the MDI framework.
            m_ViewType=ViewTypeT(m_ViewType+(KE.ShiftDown() ? -1 : 1));

            if (m_ViewType<VT_3D_WIREFRAME) m_ViewType=VT_3D_FULL_MATS;
            if (m_ViewType>VT_3D_FULL_MATS) m_ViewType=VT_3D_WIREFRAME;

            m_ChildFrame->SetCaption(this, GetCaption());
            break;
        }

        case WXK_PAGEDOWN:
        {
            const int CamNr=m_CameraTool->GetCameras().Find(m_Camera);

            wxASSERT(CamNr!=-1);

            m_Camera=m_CameraTool->GetCameras()[CamNr+1>=int(m_CameraTool->GetCameras().Size()) ? 0 : CamNr+1];
            m_CameraTool->NotifyCameraChanged(m_Camera);
            break;
        }

        case WXK_PAGEUP:
        {
            const int CamNr=m_CameraTool->GetCameras().Find(m_Camera);

            wxASSERT(CamNr!=-1);

            m_Camera=m_CameraTool->GetCameras()[CamNr<=0 ? m_CameraTool->GetCameras().Size()-1 : CamNr-1];
            m_CameraTool->NotifyCameraChanged(m_Camera);
            break;
        }

        case '1':
            // Move the far clip plane closer to the camera. We will see less, but this may improve rendering performance.
            m_Camera->FarPlaneDist=std::max(m_Camera->FarPlaneDist/1.2f, m_Camera->NearPlaneDist*4.0f);
            break;

        case '2':
            // Move the far clip plane farther from the camera away. We will see more, but this may hamper rendering performance.
            m_Camera->FarPlaneDist=std::min(m_Camera->FarPlaneDist*1.2f, float(Options.view3d.BackPlane));
            break;

        case WXK_UP:
        case WXK_DOWN:
        case WXK_LEFT:
        case WXK_RIGHT:
            // Handle the arrow keys here (so that KE.Skip() is not called for them), in order to
            // prevent (under wxGTK) that the default processing moves the focus to another window.
            break;

        default:
            KE.Skip();
            break;
    }
}


void ViewWindow3DT::OnKeyUp(wxKeyEvent& KE)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { KE.Skip(); return; }

    switch (KE.GetKeyCode())
    {
        case WXK_SPACE:
            if (m_MouseControl.GetState()==MouseControlT::ACTIVE_NORMAL)
            {
                m_MouseControl.Deactivate();
            }
            return;
    }

    // Give the active tool a chance to intercept the event.
    ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();
    if (Tool && Tool->OnKeyUp3D(*this, KE)) return;

    // This accounts for the changes to m_Camera by pressed keys in OnPaint().
    m_CameraTool->NotifyCameraChanged(m_Camera);
    KE.Skip();
}


void ViewWindow3DT::OnKeyChar(wxKeyEvent& KE)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { KE.Skip(); return; }

    // Give the active tool a chance to intercept the event.
    ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();
    if (Tool && Tool->OnChar3D(*this, KE)) return;

    // As the tool didn't process the event, we now evaluate the event for some 3D view default action.
 // switch (KE.GetKeyCode())
 // {
 //     default:
            KE.Skip();
 //         break;
 // }
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
void ViewWindow3DT::OnMouseLeftDown(wxMouseEvent& ME)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { ME.Skip(); return; }

    // Give the active tool a chance to intercept the event.
    ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();
    if (Tool && Tool->OnLMouseDown3D(*this, ME)) return;

    ME.Skip();
}


void ViewWindow3DT::OnMouseLeftUp(wxMouseEvent& ME)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { ME.Skip(); return; }

    // Give the active tool a chance to intercept the event.
    ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();
    if (Tool && Tool->OnLMouseUp3D(*this, ME)) return;

    ME.Skip();
}


void ViewWindow3DT::OnMouseMiddleDown(wxMouseEvent& ME)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { ME.Skip(); return; }

    // Give the active tool a chance to intercept the event.
    ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();
    if (Tool && Tool->OnMMouseDown3D(*this, ME)) return;

    m_MouseControl.Activate(MouseControlT::ACTIVE_ORBIT, ME.GetPosition());
}


void ViewWindow3DT::OnMouseMiddleUp(wxMouseEvent& ME)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { ME.Skip(); return; }

    if (m_MouseControl.GetState()==MouseControlT::ACTIVE_ORBIT)
    {
        m_MouseControl.Deactivate();
    }

    // Give the active tool a chance to intercept the event.
    ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();
    if (Tool && Tool->OnMMouseUp3D(*this, ME)) return;
}


void ViewWindow3DT::OnMouseRightDown(wxMouseEvent& ME)
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


void ViewWindow3DT::OnMouseRightUp(wxMouseEvent& ME)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { ME.Skip(); return; }

    if (m_MouseControl.GetState()==MouseControlT::ACTIVE_NORMAL)
    {
        m_MouseControl.Deactivate();
    }

    if (m_RightMBState==RMB_DOWN_UNDECIDED)
    {
        // Give the active tool a chance to intercept the event.
        ToolT*     Tool   =m_ChildFrame->GetToolManager().GetActiveTool();
        const bool Handled=Tool && Tool->OnRMouseClick3D(*this, ME);

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


void ViewWindow3DT::OnMouseWheel(wxMouseEvent& ME)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { ME.Skip(); return; }

    // Give the active tool a chance to intercept the event.
    ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();
    if (Tool && Tool->OnMouseWheel3D(*this, ME)) return;

    m_Camera->Pos+=normalizeOr0(WindowToWorld(ME.GetPosition()) - m_Camera->Pos)*(ME.GetWheelRotation()/2);
    m_CameraTool->NotifyCameraChanged(m_Camera);
}


void ViewWindow3DT::OnMouseMove(wxMouseEvent& ME)
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

    // Update the status bar info.
    wxASSERT(m_ChildFrame);     // See the description of NotifyChildFrameDies() for why this should never trigger.
    m_ChildFrame->SetStatusText("", ChildFrameT::SBP_GRID_ZOOM);
    m_ChildFrame->SetStatusText("", ChildFrameT::SBP_MOUSE_POS);

    // If the RMB is held down but still undecided, check the drag threshold.
    if (m_RightMBState==RMB_DOWN_UNDECIDED)
    {
        const wxPoint Drag=m_RDownPosWin-ME.GetPosition();

        if (abs(Drag.x)>3 || abs(Drag.y)>3)
        {
            m_RightMBState=RMB_DOWN_DRAGGING;
            m_MouseControl.Activate(MouseControlT::ACTIVE_NORMAL);
        }
    }

    if (!m_MouseControl.IsActive())
    {
        // Give the active tool a chance to intercept the event.
        ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();
        if (Tool && Tool->OnMouseMove3D(*this, ME)) return;
    }
}


void ViewWindow3DT::OnContextMenu(wxContextMenuEvent& CE)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { CE.Skip(); return; }

    wxMenu Menu;

    Menu.AppendRadioItem(VT_3D_WIREFRAME,  "3D Wireframe" );
    Menu.AppendRadioItem(VT_3D_FLAT,       "3D Flat"      );
    Menu.AppendRadioItem(VT_3D_EDIT_MATS,  "3D Edit Mats" );
    Menu.AppendRadioItem(VT_3D_FULL_MATS,  "3D Full Mats" );
    Menu.AppendRadioItem(VT_3D_LM_GRID,    "3D LM Grid"   );
    Menu.AppendRadioItem(VT_3D_LM_PREVIEW, "3D LM Preview");
    Menu.Check(GetViewType(), true);

    ToolT*    Tool     =m_ChildFrame->GetToolManager().GetActiveTool();
    const int MenuSelID=Tool ? Tool->OnContextMenu3D(*this, CE, Menu) : GetPopupMenuSelectionFromUser(Menu);

    if (MenuSelID>=VT_3D_WIREFRAME && MenuSelID<=VT_3D_LM_PREVIEW)
    {
        m_ViewType=ViewTypeT(MenuSelID);

        m_ChildFrame->SetCaption(this, GetCaption());
    }
}


void ViewWindow3DT::OnPaint(wxPaintEvent& PE)
{
    const MapDocumentT& MapDoc=GetMapDoc();

    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&MapDoc==NULL) { PE.Skip(); return; }


    // Determine how much time has passed since the previous frame.
    if (m_TimeOfLastPaint==0) m_TimeOfLastPaint=wxGetLocalTimeMillis().GetLo();

    const unsigned long TimeNow  =wxGetLocalTimeMillis().GetLo();
    const float         FrameTime=std::min(float(TimeNow-m_TimeOfLastPaint)/1000.0f, 0.5f);

    m_TimeOfLastPaint=TimeNow;


    /*********************/
    /*** Process Input ***/
    /*********************/

    // Only move the camera if we actually have the keyboard input focus (the m_Camera is reset to (0, 0, 0) below if we haven't).
    // Ideally, we would implement a system with proper acceleration and air and ground friction,
    // but it's really difficult to properly "tune" such a system so that it "feels right".
    if (wxWindow::FindFocus()==this)
    {
        // Keyboard layout summary:
        //   1) WASD move and strafe (4 axes).
        //   2) POS1 and END move up/down (remaining 2 axes).
        //   3) Arrow keys rotate (heading and pitch).
        //   4) Ctrl + arrow keys == WASD (first 4 axes again).
        // Note that "Shift + arrow keys" is used by the Selection and Edit Vertices tools for nudging objects and vertices.
        const bool ControlDown   =wxGetKeyState(WXK_CONTROL);
        const bool ShiftDown     =wxGetKeyState(WXK_SHIFT);
        const bool LeftArrowDown =wxGetKeyState(WXK_LEFT)  && !ShiftDown;
        const bool RightArrowDown=wxGetKeyState(WXK_RIGHT) && !ShiftDown;
        const bool UpArrowDown   =wxGetKeyState(WXK_UP)    && !ShiftDown;
        const bool DownArrowDown =wxGetKeyState(WXK_DOWN)  && !ShiftDown;

        // The maximum allowed camera speed, in camera space.
        const Vector3fT CameraMaxVel=Vector3fT(Options.view3d.MaxCameraVelocity*0.75f,
                                               Options.view3d.MaxCameraVelocity,
                                               Options.view3d.MaxCameraVelocity*0.5f);

        const Vector3fT CameraAccel=CameraMaxVel*(1000.0f/std::max(float(Options.view3d.TimeToMaxSpeed), 10.0f));


        // Strafe left / right.
        if (wxGetKeyState(wxKeyCode('D')) || (ControlDown && RightArrowDown))
        {
            m_CameraVel.x=std::min(m_CameraVel.x+CameraAccel.x*FrameTime,  CameraMaxVel.x);
        }
        else if (wxGetKeyState(wxKeyCode('A')) || (ControlDown && LeftArrowDown))
        {
            m_CameraVel.x=std::max(m_CameraVel.x-CameraAccel.x*FrameTime, -CameraMaxVel.x);
        }
        else
        {
            // Halts are instantaneous (infinite negative acceleration).
            m_CameraVel.x=0.0f;
        }

        // Move forward / backward.
        if (wxGetKeyState(wxKeyCode('W')) || (ControlDown && UpArrowDown))
        {
            m_CameraVel.y=std::min(m_CameraVel.y+CameraAccel.y*FrameTime,  CameraMaxVel.y);
        }
        else if (wxGetKeyState(wxKeyCode('S')) || (ControlDown && DownArrowDown))
        {
            m_CameraVel.y=std::max(m_CameraVel.y-CameraAccel.y*FrameTime, -CameraMaxVel.y);
        }
        else
        {
            // Halts are instantaneous (infinite negative acceleration).
            m_CameraVel.y=0.0f;
        }

        // Hover up / down.
        if (wxGetKeyState(WXK_HOME))
        {
            m_CameraVel.z=std::min(m_CameraVel.z+CameraAccel.z*FrameTime,  CameraMaxVel.z);
        }
        else if (wxGetKeyState(WXK_END))
        {
            m_CameraVel.z=std::max(m_CameraVel.z-CameraAccel.z*FrameTime, -CameraMaxVel.z);
        }
        else
        {
            // Halts are instantaneous (infinite negative acceleration).
            m_CameraVel.z=0.0f;
        }

        // Rotate (heading and pitch).
        if (!ControlDown)
        {
            if (   UpArrowDown) m_Camera->Angles.pitch()-=FrameTime*180.0f;
            if ( DownArrowDown) m_Camera->Angles.pitch()+=FrameTime*180.0f;

            if ( LeftArrowDown) m_Camera->Angles.yaw()-=FrameTime*180.0f;
            if (RightArrowDown) m_Camera->Angles.yaw()+=FrameTime*180.0f;

            m_Camera->LimitAngles();
        }
    }
    else
    {
        // Make sure that the camera halts whenever we lose or don't have the focus.
        m_CameraVel=Vector3fT(0, 0, 0);
    }

    // The cameras new position in camera space is at m_CameraVel*FrameTime.
    if (m_CameraVel.x!=0) m_Camera->Pos+=m_Camera->GetXAxis()*(m_CameraVel.x*FrameTime);
    if (m_CameraVel.y!=0) m_Camera->Pos+=m_Camera->GetYAxis()*(m_CameraVel.y*FrameTime);
    if (m_CameraVel.z!=0) m_Camera->Pos+=m_Camera->GetZAxis()*(m_CameraVel.z*FrameTime);


    // Process mouse input.
    if (m_MouseControl.IsActive())
    {
        const bool    Shift     =wxGetKeyState(WXK_SHIFT);
        const bool    Control   =wxGetKeyState(WXK_CONTROL);
        const wxPoint MousePos  =ScreenToClient(wxGetMousePosition());      // ScreenToClient() is a method of wxWindow.
        const wxPoint MouseDelta=MousePos-m_MouseControl.GetRefPtWin();

        if (m_MouseControl.GetState()==MouseControlT::ACTIVE_NORMAL)
        {
            if (!Shift && !Control)
            {
                // No modifier key is down - look around.
                m_Camera->Angles.yaw()+=MouseDelta.x*0.4f;
                m_Camera->Angles.pitch()+=MouseDelta.y*(Options.view3d.ReverseY ? -0.4f : 0.4f);
                m_Camera->LimitAngles();
            }
            else if (Shift && !Control)
            {
                // Shift is down, but not Control - pan the view (shift up/down and strafe left/right).
                m_Camera->Pos+=m_Camera->GetXAxis()*(MouseDelta.x*2);
                m_Camera->Pos-=m_Camera->GetZAxis()*(MouseDelta.y*2);
            }
            else if (!Shift && Control)
            {
                // Control is down, but not Shift - move the camera along its depth axis and strafe left/right.
                m_Camera->Pos+=m_Camera->GetXAxis()*(MouseDelta.x*2);
                m_Camera->Pos-=m_Camera->GetYAxis()*(MouseDelta.y*2);
            }
            else
            {
                // Both Shift and Control is down - "fly mode".
                m_Camera->Angles.yaw()+=MouseDelta.x*0.4f;
                m_Camera->LimitAngles();

                Vector3fT   xy =m_Camera->GetYAxis(); xy.z=0.0f;
                const float len=length(xy);

                m_Camera->Pos-=(len>0.01f ? xy/len : m_Camera->GetYAxis())*(MouseDelta.y*2);
            }
        }
        else
        {
            // Orbit mode.
            const float DeltaYaw=MouseDelta.x*0.4f;

            // Apply yaw in order to orbit the camera horizontally (around the z-axis).
            if (DeltaYaw!=0.0f)
            {
                m_Camera->Angles.yaw()+=DeltaYaw;
                m_Camera->LimitAngles();

                m_Camera->Pos-=m_MouseControl.GetRefPtWorld();
                m_Camera->Pos=m_Camera->Pos.GetRotZ(-DeltaYaw);
                m_Camera->Pos+=m_MouseControl.GetRefPtWorld();
            }

            if (Shift || Control)
            {
                // Shift or Control is down - move the camera closer or farther from the reference point.
                m_Camera->Pos-=normalizeOr0(m_MouseControl.GetRefPtWorld() - m_Camera->Pos)*(MouseDelta.y*2);
            }
            else
            {
                // No modifier key is down - apply pitch in order to orbit the camera vertically.
                const float DeltaPitch=MouseDelta.y*(Options.view3d.ReverseY ? -0.4f : 0.4f);

                if (DeltaPitch!=0.0f)
                {
                    const float OldElev =90.0f - cf::math::AnglesfT::RadToDeg(acos(normalizeOr0(m_Camera->Pos - m_MouseControl.GetRefPtWorld()).z));
                    const float NewElev =OldElev + DeltaPitch;
                    const float NewPitch=m_Camera->Angles.pitch() + DeltaPitch;

                    if (NewElev<85.0f && NewElev>-85.0f && NewPitch<90.0f && NewPitch>-90.0f)
                    {
                        m_Camera->Angles.pitch()+=DeltaPitch;
                        m_Camera->LimitAngles();

                        m_Camera->Pos-=m_MouseControl.GetRefPtWorld();

                        wxASSERT(m_Camera->Pos.x!=0.0f || m_Camera->Pos.y!=0.0f);
                        const float AngleZ=cf::math::AnglesfT::RadToDeg(atan2(m_Camera->Pos.x, m_Camera->Pos.y));
                        m_Camera->Pos=m_Camera->Pos.GetRotZ(AngleZ).      // AngleZ is measured clockwise, GetRotZ() rotates counter-clockwise.
                                                    GetRotX(DeltaPitch).
                                                    GetRotZ(-AngleZ);

                        m_Camera->Pos+=m_MouseControl.GetRefPtWorld();
                    }
                }
            }
        }

        if (MouseDelta.x || MouseDelta.y) WarpPointer(m_MouseControl.GetRefPtWin().x, m_MouseControl.GetRefPtWin().y);
    }

    // Note that   m_CameraTool->NotifyCameraChanged(m_Camera);   is
    // called in OnKeyUp() and when the mouse control is deactivated.



    /********************/
    /*** Render Frame ***/
    /********************/

    wxPaintDC dc(this);     // It is VERY important not to omit this, or otherwise everything goes havoc.

    if (!wxGetApp().IsActive()) return;


    // ********************
    // Pre-rendering stuff.
    // ********************

    m_Renderer.InitFrame();

    // Initialize the viewport.
    SetCurrent(*wxGetApp().GetParentFrame()->m_GLContext);    // This is the method from the wxGLCanvas for activating the given RC with this window.
    const wxSize CanvasSize=GetClientSize();

    MatSys::Renderer->SetViewport(0, 0, CanvasSize.GetWidth(), CanvasSize.GetHeight());

    // Initialize the perspective projection (view-to-clip) matrix.
    MatSys::Renderer->SetMatrix(MatSys::RendererI::PROJECTION,
        MatrixT::GetProjPerspectiveMatrix(m_Camera->VerticalFOV, (float)CanvasSize.GetWidth()/(float)CanvasSize.GetHeight(),
                                          m_Camera->NearPlaneDist, m_Camera->FarPlaneDist));

    // Initialize the camera (world-to-view) matrix.
    MatrixT WorldToView=m_Camera->GetMatrix();

    // Rotate by 90 degrees around the x-axis in order to meet the MatSys's expectation of axes orientation.
    for (unsigned long i=0; i<4; i++)
    {
        std::swap(WorldToView[1][i], WorldToView[2][i]);
        WorldToView[2][i]=-WorldToView[2][i];
    }

    MatSys::Renderer->SetMatrix(MatSys::RendererI::WORLD_TO_VIEW, WorldToView);
    MatSys::Renderer->SetCurrentRenderAction(MatSys::RendererI::AMBIENT);
    MatSys::Renderer->SetCurrentLightMap(wxGetApp().GetParentFrame()->m_WhiteTexture);    // Set a proper default lightmap.
    MatSys::Renderer->SetCurrentLightDirMap(NULL);      // The MatSys provides a default for LightDirMaps when NULL is set.
    MatSys::Renderer->ClearColor(0, 0, 0, 0);
    MatSys::Renderer->BeginFrame(TimeNow/1000.0);


    // ************************
    // Do the actual rendering.
    // ************************

    // Render the world axes. They're great for technical and emotional reassurance.
    {
        static MatSys::MeshT Mesh(MatSys::MeshT::Lines);

        if (Mesh.Vertices.Size()==0) Mesh.Vertices.PushBackEmpty(6);

        Mesh.Vertices[0].SetColor(1, 0, 0); Mesh.Vertices[0].SetOrigin(0, 0, 0);
        Mesh.Vertices[1].SetColor(1, 0, 0); Mesh.Vertices[1].SetOrigin(100, 0, 0);
        Mesh.Vertices[2].SetColor(0, 1, 0); Mesh.Vertices[2].SetOrigin(0, 0, 0);
        Mesh.Vertices[3].SetColor(0, 1, 0); Mesh.Vertices[3].SetOrigin(0, 100, 0);
        Mesh.Vertices[4].SetColor(0, 0, 1); Mesh.Vertices[4].SetOrigin(0, 0, 0);
        Mesh.Vertices[5].SetColor(0, 0, 1); Mesh.Vertices[5].SetOrigin(0, 0, 100);

        MatSys::Renderer->SetCurrentMaterial(m_Renderer.GetRMatWireframe());
        MatSys::Renderer->RenderMesh(Mesh);
    }


    // Get the back-to-front ordered list of map elements that are in the view frustum and visible.
    const ArrayT<MapElementT*>& VisElemsBackToFront=m_Renderer.GetVisElemsBackToFront();

    // Render the opaque elements front-to-back.
    for (unsigned long ElemNr=VisElemsBackToFront.Size(); ElemNr>0; ElemNr--)
        if (!VisElemsBackToFront[ElemNr-1]->IsTranslucent())
            VisElemsBackToFront[ElemNr-1]->Render3D(m_Renderer);

    // Render the translucent elements back-to-front.
    for (unsigned long ElemNr=0; ElemNr<VisElemsBackToFront.Size(); ElemNr++)
        if (VisElemsBackToFront[ElemNr]->IsTranslucent())
            VisElemsBackToFront[ElemNr]->Render3D(m_Renderer);


    // Render the tools.
    const ArrayT<ToolT*>& Tools=m_ChildFrame->GetToolManager().GetTools();

    for (unsigned long ToolNr=0; ToolNr<Tools.Size(); ToolNr++)
        Tools[ToolNr]->RenderTool3D(m_Renderer);


    // Render the "zigzag" line that has been loaded from a point file and usually leads from an entity to a leak
    // or is the description of a players path through a game map as recorded with the recordPath() console function.
    const ArrayT<PtsPointT>& Points=GetMapDoc().GetPointFilePoints();
    const ArrayT<wxColour>&  Colors=GetMapDoc().GetPointFileColors();

    if (Points.Size()>0 && Colors.Size()>=4)
    {
        MatSys::MeshT Mesh(MatSys::MeshT::LineStrip);

        if (Colors[1].IsOk())
        {
            for (unsigned long PointNr=0; PointNr<Points.Size(); PointNr++)
            {
                Mesh.Vertices.PushBackEmpty();

                const Vector3fT&        Pos   =Points[PointNr].Pos;
                MatSys::MeshT::VertexT& Vertex=Mesh.Vertices[Mesh.Vertices.Size()-1];

                Vertex.SetOrigin(Pos.x, Pos.y, Pos.z);
                Vertex.SetColor(Colors[1].Red()/255.0f, Colors[1].Green()/255.0f, Colors[1].Blue()/255.0f);
            }

            MatSys::Renderer->SetCurrentMaterial(m_Renderer.GetRMatWireframe());
            MatSys::Renderer->RenderMesh(Mesh);
        }

        if (Colors[2].IsOk())
        {
            Mesh.Type=MatSys::MeshT::Lines;
            Mesh.Vertices.Overwrite();

            for (unsigned long PointNr=0; PointNr<Points.Size(); PointNr++)
            {
                Mesh.Vertices.PushBackEmpty(2);

                const float             Hdg    =Points[PointNr].Heading/32768.0f*180.0f;
                const Vector3fT&        Pos1   =Points[PointNr].Pos;
                const Vector3fT         Pos2   =Points[PointNr].Pos+Vector3fT(0.0f, 8.0f, 0.0f).GetRotZ(-Hdg);
                MatSys::MeshT::VertexT& Vertex1=Mesh.Vertices[Mesh.Vertices.Size()-2];
                MatSys::MeshT::VertexT& Vertex2=Mesh.Vertices[Mesh.Vertices.Size()-1];

                Vertex1.SetOrigin(Pos1.x, Pos1.y, Pos1.z);
                Vertex1.SetColor(Colors[2].Red()/255.0f, Colors[2].Green()/255.0f, Colors[2].Blue()/255.0f);

                Vertex2.SetOrigin(Pos2.x, Pos2.y, Pos2.z);
                Vertex2.SetColor(Colors[2].Red()/255.0f, Colors[2].Green()/255.0f, Colors[2].Blue()/255.0f);
            }

            MatSys::Renderer->SetCurrentMaterial(m_Renderer.GetRMatWireframe());
            MatSys::Renderer->RenderMesh(Mesh);
        }
    }


    // Render the split planes of the BSP tree (for debugging).
    m_Renderer.RenderSplitPlanes(MapDoc.GetBspTree()->GetRootNode(), Options.view3d.SplitPlanesDepth);


    // Render the "HUD" of the mouse control.
    if (m_MouseControl.IsActive())
    {
        Renderer3DT::UseOrthoMatricesT UseOrtho(*this);

        m_Renderer.RenderCrossHair(m_MouseControl.GetRefPtWin());
    }


    // *********************
    // Post-rendering stuff.
    // *********************

    MatSys::Renderer->EndFrame();
    SwapBuffers();
}


void ViewWindow3DT::OnSize(wxSizeEvent& SE)
{
    wxSize Size=SE.GetSize();

    if (Size.x>0 && Size.y>0)
        MatSys::Renderer->SetViewport(0, 0, Size.x, Size.y);
}


void ViewWindow3DT::OnSetFocus(wxFocusEvent& FE)
{
    Refresh(false);
}


void ViewWindow3DT::OnKillFocus(wxFocusEvent& FE)
{
    // When we lose the focus, make sure that the mouse cursor is shown and the mouse capture is released.
    m_MouseControl.Deactivate();
    m_RightMBState=RMB_UP_IDLE;
}


void ViewWindow3DT::OnMouseCaptureLost(wxMouseCaptureLostEvent& ME)
{
    // When we lose the capture, make sure that the mouse cursor is shown and the mouse capture is released.
    m_MouseControl.Deactivate();
}
