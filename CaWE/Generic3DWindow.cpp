/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Generic3DWindow.hpp"
#include "AppCaWE.hpp"
#include "AxesInfo.hpp"
#include "Camera.hpp"
#include "Options.hpp"
#include "ParentFrame.hpp"

#include <algorithm>


#if defined(_WIN32) && defined(_MSC_VER)
// Turn off warning 4355: "'this' : wird in Initialisierungslisten fuer Basisklasse verwendet".
#pragma warning(disable:4355)
#endif


Generic3DWindowT::MouseControlT::MouseControlT(Generic3DWindowT& Win)
    : m_Win(Win),
      m_State(NOT_ACTIVE)
{
}


void Generic3DWindowT::MouseControlT::Activate(StateT NewState, const wxPoint& RefPt)
{
    // Did someone use this method for deactivating mouse control?
    if (NewState==NOT_ACTIVE) { Deactivate(); return; }

    // If mouse control is already active (and attempted to re-activate, i.e. the state changes from ACTIVE_* to ACTIVE_*),
    // do nothing, because we want to have the next state of ACTIVE_* always be NOT_ACTIVE.
    if (m_State!=NOT_ACTIVE) return;

    m_State     =NewState;
    m_RefPtWin  =(RefPt==wxDefaultPosition) ? wxPoint(m_Win.GetClientSize().x/2, m_Win.GetClientSize().y/2) : RefPt;
    m_RefPtWorld=m_Win.GetRefPtWorld(m_RefPtWin);

    if (!m_Win.HasCapture()) m_Win.CaptureMouse();
    m_Win.SetCursor(wxCURSOR_BLANK);
    m_Win.WarpPointer(m_RefPtWin.x, m_RefPtWin.y);
}


void Generic3DWindowT::MouseControlT::Deactivate()
{
    // The mouse control is deactivated (the state changes from ACTIVE_* to NOT_ACTIVE),
    // so propagate the changes to the camera also to the other observers.
    if (m_State!=NOT_ACTIVE) m_Win.InfoCameraChanged();

    m_State=NOT_ACTIVE;

    if (m_Win.HasCapture()) m_Win.ReleaseMouse();
    m_Win.SetCursor(*wxSTANDARD_CURSOR);
}


BEGIN_EVENT_TABLE(Generic3DWindowT, wxGLCanvas)
    EVT_KEY_DOWN          (Generic3DWindowT::OnKeyDown         )
    EVT_KEY_UP            (Generic3DWindowT::OnKeyUp           )
    EVT_MIDDLE_DOWN       (Generic3DWindowT::OnMouseMiddleDown )
    EVT_MIDDLE_DCLICK     (Generic3DWindowT::OnMouseMiddleDown )
    EVT_MIDDLE_UP         (Generic3DWindowT::OnMouseMiddleUp   )
    EVT_RIGHT_DOWN        (Generic3DWindowT::OnMouseRightDown  )
    EVT_RIGHT_DCLICK      (Generic3DWindowT::OnMouseRightDown  )
    EVT_RIGHT_UP          (Generic3DWindowT::OnMouseRightUp    )
    EVT_MOUSEWHEEL        (Generic3DWindowT::OnMouseWheel      )
    EVT_MOTION            (Generic3DWindowT::OnMouseMove       )
 // EVT_ENTER_WINDOW      (Generic3DWindowT::OnMouseEnterWindow)
 // EVT_LEAVE_WINDOW      (Generic3DWindowT::OnMouseLeaveWindow)
    EVT_KILL_FOCUS        (Generic3DWindowT::OnKillFocus       )
    EVT_MOUSE_CAPTURE_LOST(Generic3DWindowT::OnMouseCaptureLost)
END_EVENT_TABLE()


Generic3DWindowT::Generic3DWindowT(wxWindow* Parent, CameraT* InitialCamera)
    : wxGLCanvas(Parent, -1, ParentFrameT::OpenGLAttributeList, wxDefaultPosition, wxSize(400, 300), wxWANTS_CHARS),
      m_Camera(InitialCamera),
      m_CameraVel(),
      m_MouseControl(*this),
      m_RightMBState(RMB_UP_IDLE),
      m_RDownPosWin()
{
    SetMinSize(wxSize(120, 90));
}


Generic3DWindowT::~Generic3DWindowT()
{
    // I'm not sure whether this code is in the right place,
    // because wxGLCanvas::SetCurrent() (currently) requires that the related parent windows IsShown() method returns true.
    // However, when we ALT+F4 close an application with child frames open, wxWidgets first closes all windows before their
    // dtor is called...  so we should probably move this into the EVT_CLOSE handler of our child frame???
    if (wxGetApp().GetParentFrame()->IsShown())
        wxGetApp().GetParentFrame()->m_GLCanvas->SetCurrent(*wxGetApp().GetParentFrame()->m_GLContext);
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


AxesInfoT Generic3DWindowT::GetAxesInfo() const
{
    const int Horizontal=ClosestAxis(m_Camera->GetXAxis());
    const int Vertical  =ClosestAxis(m_Camera->GetZAxis());

    return AxesInfoT(Horizontal % 3, Horizontal>2, Vertical % 3, Vertical<3);
}


void Generic3DWindowT::SetCamera(CameraT* Camera)
{
    m_Camera=Camera;
    m_CameraVel=Vector3fT(0, 0, 0);
}


void Generic3DWindowT::MoveCamera(const Vector3fT& NewPos)
{
    m_Camera->Pos=NewPos;
    InfoCameraChanged();
}


void Generic3DWindowT::ProcessInput(float FrameTime)
{
    // Only move the camera if we actually have the keyboard input focus (the m_CameraVel is reset to (0, 0, 0) below if we haven't).
    // Ideally, we would implement a system with proper acceleration and air and ground friction,
    // but it's really difficult to properly "tune" such a system so that it "feels right".
    if (wxWindow::FindFocus()==this)
    {
        // Keyboard layout summary:
        //
        //   1) WASD move and strafe (4 axes).
        //   2) POS1 and END move up/down (remaining 2 axes).
        //   3) Arrow keys rotate (heading and pitch).
        //   4) Ctrl + arrow keys == WASD (first 4 axes again).
        //
        // Note that:
        //
        //   - "Shift + arrow keys" is used by the Selection and Edit Vertices tools for nudging objects and vertices.
        //   - WASD, POS1 and END cannot be used if Ctrl is down, because e.g. Ctrl+S is also used as a main menu
        //     hotkey, and if such a hotkey is used, it should not inadvertently cause any camera movement.
        //
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
        if ((!ControlDown && wxGetKeyState(wxKeyCode('D'))) || (ControlDown && RightArrowDown))
        {
            m_CameraVel.x=std::min(m_CameraVel.x+CameraAccel.x*FrameTime,  CameraMaxVel.x);
        }
        else if ((!ControlDown && wxGetKeyState(wxKeyCode('A'))) || (ControlDown && LeftArrowDown))
        {
            m_CameraVel.x=std::max(m_CameraVel.x-CameraAccel.x*FrameTime, -CameraMaxVel.x);
        }
        else
        {
            // Deceleration is three times as hard as acceleration (short stopping distance).
            m_CameraVel.x=(m_CameraVel.x>0) ? std::max(m_CameraVel.x-3.0f*CameraAccel.x*FrameTime, 0.0f)
                                            : std::min(m_CameraVel.x+3.0f*CameraAccel.x*FrameTime, 0.0f);
        }

        // Move forward / backward.
        if ((!ControlDown && wxGetKeyState(wxKeyCode('W'))) || (ControlDown && UpArrowDown))
        {
            m_CameraVel.y=std::min(m_CameraVel.y+CameraAccel.y*FrameTime,  CameraMaxVel.y);
        }
        else if ((!ControlDown && wxGetKeyState(wxKeyCode('S'))) || (ControlDown && DownArrowDown))
        {
            m_CameraVel.y=std::max(m_CameraVel.y-CameraAccel.y*FrameTime, -CameraMaxVel.y);
        }
        else
        {
            // Deceleration is three times as hard as acceleration (short stopping distance).
            m_CameraVel.y=(m_CameraVel.y>0) ? std::max(m_CameraVel.y-3.0f*CameraAccel.y*FrameTime, 0.0f)
                                            : std::min(m_CameraVel.y+3.0f*CameraAccel.y*FrameTime, 0.0f);
        }

        // Hover up / down.
        if (!ControlDown && wxGetKeyState(WXK_HOME))
        {
            m_CameraVel.z=std::min(m_CameraVel.z+CameraAccel.z*FrameTime,  CameraMaxVel.z);
        }
        else if (!ControlDown && wxGetKeyState(WXK_END))
        {
            m_CameraVel.z=std::max(m_CameraVel.z-CameraAccel.z*FrameTime, -CameraMaxVel.z);
        }
        else
        {
            // Deceleration is three times as hard as acceleration (short stopping distance).
            m_CameraVel.z=(m_CameraVel.z>0) ? std::max(m_CameraVel.z-3.0f*CameraAccel.z*FrameTime, 0.0f)
                                            : std::min(m_CameraVel.z+3.0f*CameraAccel.z*FrameTime, 0.0f);
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
                m_Camera->Angles.yaw()+=MouseDelta.x*Options.view3d.MouseSensitivity;
                m_Camera->Angles.pitch()+=MouseDelta.y*Options.view3d.MouseSensitivity*(Options.view3d.ReverseY ? -1.0f : 1.0f);
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
                m_Camera->Angles.yaw()+=MouseDelta.x*Options.view3d.MouseSensitivity;
                m_Camera->LimitAngles();

                Vector3fT   xy =m_Camera->GetYAxis(); xy.z=0.0f;
                const float len=length(xy);

                m_Camera->Pos-=(len>0.01f ? xy/len : m_Camera->GetYAxis())*(MouseDelta.y*2);
            }
        }
        else
        {
            // Orbit mode.
            const float DeltaYaw=MouseDelta.x*Options.view3d.MouseSensitivity;

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
                const Vector3fT N=normalizeOr0(m_MouseControl.GetRefPtWorld() - m_Camera->Pos);
                const Plane3fT  P=Plane3fT(N, dot(N, m_MouseControl.GetRefPtWorld()));  // The plane through the reference point.
                const float     MinDist=-(m_Camera->NearPlaneDist+1.0f);

                // Note that the camera is on the backside of P (P.GetDistance(m_Camera->Pos) is a negative number).
                m_Camera->Pos+=N*std::min(-2.0f*MouseDelta.y, MinDist-P.GetDistance(m_Camera->Pos));
            }
            else
            {
                // No modifier key is down - apply pitch in order to orbit the camera vertically.
                const float DeltaPitch=MouseDelta.y*Options.view3d.MouseSensitivity*(Options.view3d.ReverseY ? -1.0f : 1.0f);

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

                        if (m_Camera->Pos.x!=0.0f || m_Camera->Pos.y!=0.0f)
                        {
                            const float AngleZ=cf::math::AnglesfT::RadToDeg(atan2(m_Camera->Pos.x, m_Camera->Pos.y));

                            m_Camera->Pos=m_Camera->Pos.GetRotZ(AngleZ).      // AngleZ is measured clockwise, GetRotZ() rotates counter-clockwise.
                                                        GetRotX(DeltaPitch).
                                                        GetRotZ(-AngleZ);
                        }

                        m_Camera->Pos+=m_MouseControl.GetRefPtWorld();
                    }
                }
            }
        }

        if (MouseDelta.x || MouseDelta.y) WarpPointer(m_MouseControl.GetRefPtWin().x, m_MouseControl.GetRefPtWin().y);
    }

    // Note that   InfoCameraChanged();   is called in
    // OnKeyUp() and when the mouse control is deactivated.
}


void Generic3DWindowT::GetViewFrustum(Plane3fT* Planes, unsigned int NumPlanes) const
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


Vector3fT Generic3DWindowT::WindowToWorld(const wxPoint& Pixel) const
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


wxPoint Generic3DWindowT::WorldToWindow(const Vector3fT& v, bool CheckFrustum) const
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


void Generic3DWindowT::OnKeyDown(wxKeyEvent& KE)
{
    // wxLogDebug("Generic3DWindowT::OnKeyDown(), KE.GetKeyCode()==%lu", KE.GetKeyCode());

    switch (KE.GetKeyCode())
    {
        case WXK_SPACE:
            m_MouseControl.Activate(MouseControlT::ACTIVE_NORMAL);
            break;

        case 'Z':
            m_MouseControl.Activate(m_MouseControl.IsActive() ? MouseControlT::NOT_ACTIVE : MouseControlT::ACTIVE_NORMAL);
            break;

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


void Generic3DWindowT::OnKeyUp(wxKeyEvent& KE)
{
    switch (KE.GetKeyCode())
    {
        case WXK_SPACE:
            if (m_MouseControl.GetState()==MouseControlT::ACTIVE_NORMAL)
            {
                m_MouseControl.Deactivate();
            }
            break;

        default:
            KE.Skip();
            break;
    }

    // This accounts for the changes to m_Camera by pressed keys in ProcessInput().
    InfoCameraChanged();
}


void Generic3DWindowT::OnMouseMiddleDown(wxMouseEvent& ME)
{
    m_MouseControl.Activate(MouseControlT::ACTIVE_ORBIT, ME.GetPosition());
}


void Generic3DWindowT::OnMouseMiddleUp(wxMouseEvent& ME)
{
    if (m_MouseControl.GetState()==MouseControlT::ACTIVE_ORBIT)
    {
        m_MouseControl.Deactivate();
    }
}


void Generic3DWindowT::OnMouseRightDown(wxMouseEvent& ME)
{
    if (m_RightMBState==RMB_UP_IDLE)    // Keys on the keyboard may generate RMB down events as well...
    {
        m_RightMBState=RMB_DOWN_UNDECIDED;
        m_RDownPosWin =ME.GetPosition();
    }

    // Never call ME.Skip() here (always handle the event), in order to prevent
    // the default handling from opening a context-menu (under wxGTK).
}


void Generic3DWindowT::OnMouseRightUp(wxMouseEvent& ME)
{
    if (m_MouseControl.GetState()==MouseControlT::ACTIVE_NORMAL)
    {
        m_MouseControl.Deactivate();
    }

    if (m_RightMBState==RMB_DOWN_UNDECIDED)
    {
        // Implementations typically run the tool function and/or open the context-menu.
        InfoRightMouseClick(ME);
    }

    m_RightMBState=RMB_UP_IDLE;
}


void Generic3DWindowT::OnMouseWheel(wxMouseEvent& ME)
{
    int factor=2;

    // It would probably be better to use WXK_ALT here, or even better to have a "mouse sensitivity" state/member.
    if (wxGetKeyState(WXK_SHIFT  )) factor*=4;
    if (wxGetKeyState(WXK_CONTROL)) factor*=4;

    m_Camera->Pos+=normalizeOr0(WindowToWorld(ME.GetPosition()) - m_Camera->Pos)*(ME.GetWheelRotation()/factor);
    InfoCameraChanged();
}


void Generic3DWindowT::OnMouseMove(wxMouseEvent& ME)
{
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
}


void Generic3DWindowT::OnKillFocus(wxFocusEvent& FE)
{
    // When we lose the focus, make sure that the mouse cursor is shown and the mouse capture is released.
    m_MouseControl.Deactivate();
    m_RightMBState=RMB_UP_IDLE;
}


void Generic3DWindowT::OnMouseCaptureLost(wxMouseCaptureLostEvent& ME)
{
    // When we lose the capture, make sure that the mouse cursor is shown and the mouse capture is released.
    m_MouseControl.Deactivate();
}
