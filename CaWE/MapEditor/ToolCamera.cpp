/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ToolCamera.hpp"
#include "MapDocument.hpp"
#include "ChildFrame.hpp"
#include "ChildFrameViewWin2D.hpp"
#include "ChildFrameViewWin3D.hpp"
#include "Renderer2D.hpp"
#include "ToolManager.hpp"
#include "ToolOptionsBars.hpp"

#include "../Camera.hpp"
#include "../CursorMan.hpp"

#include "wx/wx.h"


/*** Begin of TypeSys related definitions for this class. ***/

void* ToolCameraT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    const ToolCreateParamsT* TCPs=static_cast<const ToolCreateParamsT*>(&Params);

    return new ToolCameraT(TCPs->MapDoc, TCPs->ToolMan, TCPs->ParentOptionsBar);
}

const cf::TypeSys::TypeInfoT ToolCameraT::TypeInfo(GetToolTIM(), "ToolCameraT", "ToolT", ToolCameraT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


static const unsigned long CAMERA_HANDLE_RADIUS=5;


ToolCameraT::ToolCameraT(MapDocumentT& MapDoc, ToolManagerT& ToolMan, wxWindow* ParentOptionsBar)
    : ToolT(MapDoc, ToolMan),
      m_Cameras(),
      m_ActiveCameraNr(0),
      m_ToolState(IDLE),
      m_OptionsBar(new OptionsBar_CameraToolT(ParentOptionsBar))
{
    // Create the first camera.
    m_Cameras.PushBack(new CameraT());
}


ToolCameraT::~ToolCameraT()
{
    ChildFrameT* ChildFrame=m_MapDoc.GetChildFrame();

    // Before we delete all our cameras, set the pointers in the 3D views to a safe location.
    for (unsigned long ViewWinNr=0; ViewWinNr<ChildFrame->GetViewWindows().Size(); ViewWinNr++)
    {
        ViewWindowT*   ViewWin  =ChildFrame->GetViewWindows()[ViewWinNr];
        ViewWindow3DT* ViewWin3D=dynamic_cast<ViewWindow3DT*>(ViewWin);

        if (ViewWin3D)
        {
            static CameraT SafeCamera;

            ViewWin3D->SetCamera(&SafeCamera);
        }
    }

    // Now we can safely delete our cameras.
    for (unsigned long CameraNr=0; CameraNr<m_Cameras.Size(); CameraNr++)
        delete m_Cameras[CameraNr];
}


void ToolCameraT::AddCamera(CameraT* Camera)
{
    if (Camera==NULL) return;

    // Add Camera to our set of m_Cameras, if it is not already contained therein.
    if (m_Cameras.Find(Camera)==-1) m_Cameras.PushBack(Camera);

    NotifyCameraChanged(Camera);
}


void ToolCameraT::NotifyCameraChanged(const CameraT* Camera)
{
    // Ok, somebody (either a ViewWindow3DT or we ourselves) notifies us that one of our cameras has changed.
    const int CamNr=m_Cameras.Find(const_cast<CameraT*>(Camera));

    wxASSERT(CamNr!=-1);
    if (CamNr>=0) m_ActiveCameraNr=CamNr;

    // Have the views update themselves immediately ("now"), as this method is called during busy periods of
    // mouse-dragging, keys being held down, etc., when paint events may not get through.
    // Note that the 3D views are clever enough to just do nothing when not "their" camera has changed!
    m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
}


void ToolCameraT::DeleteActiveCamera()
{
    if (m_Cameras.Size()<=1) return;
    if (m_ToolState!=IDLE) return;      // Change the active camera only when m_ToolState==IDLE.

    CameraT* OldActiveCamera=m_Cameras[m_ActiveCameraNr];

    delete m_Cameras[m_ActiveCameraNr];
    m_Cameras.RemoveAtAndKeepOrder(m_ActiveCameraNr);

    if (m_ActiveCameraNr>=m_Cameras.Size()) m_ActiveCameraNr=m_Cameras.Size()-1;

    // Update 3D views that used the old active camera to use the new active camera.
    ChildFrameT* ChildFrame=m_MapDoc.GetChildFrame();

    for (unsigned long ViewWinNr=0; ViewWinNr<ChildFrame->GetViewWindows().Size(); ViewWinNr++)
    {
        ViewWindowT*   ViewWin  =ChildFrame->GetViewWindows()[ViewWinNr];
        ViewWindow3DT* ViewWin3D=dynamic_cast<ViewWindow3DT*>(ViewWin);

        if (ViewWin3D && &ViewWin3D->GetCamera()==OldActiveCamera)
        {
            ViewWin3D->SetCamera(m_Cameras[m_ActiveCameraNr]);
        }
    }

    NotifyCameraChanged(m_Cameras[m_ActiveCameraNr]);
}


wxWindow* ToolCameraT::GetOptionsBar()
{
    // Cannot define this method inline in the header file, because there the compiler
    // does not yet know that the type of m_OptionsBar is in fact related to wxWindow.
    return m_OptionsBar;
}


bool ToolCameraT::OnKeyDown2D(ViewWindow2DT& ViewWindow, wxKeyEvent& KE)
{
    return OnKeyDown(ViewWindow, KE);
}


bool ToolCameraT::OnLMouseDown2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
    ViewWindow.CaptureMouse();
    if (m_ToolState!=IDLE) return false;    // Change the active camera only when m_ToolState==IDLE.

    if (ME.ShiftDown())
    {
        const Vector3fT WorldPos=ViewWindow.WindowToWorld(ME.GetPosition(), m_MapDoc.GetMostRecentSelBB().GetCenter().z);

        m_Cameras.PushBack(new CameraT());
        m_ActiveCameraNr=m_Cameras.Size()-1;

        m_Cameras[m_ActiveCameraNr]->Pos          =WorldPos;
        m_Cameras[m_ActiveCameraNr]->ViewDirLength=0.0f;

        m_ToolState=DRAG_CAM_DIR;

        m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
    }
    else
    {
        // Try to grab one of the camera handles for dragging.
        unsigned long CamNr;
        CameraPartT   CamPart;

        if (FindCamera(ViewWindow, ME.GetPosition(), CamNr, CamPart))
        {
            m_ActiveCameraNr=CamNr;
            m_ToolState     =(CamPart==POSITION) ? DRAG_CAM_POS : DRAG_CAM_DIR;
        }
    }

    return true;
}


bool ToolCameraT::OnLMouseUp2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
    if (!ViewWindow.HasCapture()) return false;     // Drag started outside of ViewWindow.
    ViewWindow.ReleaseMouse();

    if (m_ToolState!=IDLE)
    {
        NotifyCameraChanged(m_Cameras[m_ActiveCameraNr]);   // Not strictly necessary here...
        m_ToolState=IDLE;
    }

    return true;
}


bool ToolCameraT::OnMouseMove2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
    const int       HorzAxis=ViewWindow.GetAxesInfo().HorzAxis;
    const int       VertAxis=ViewWindow.GetAxesInfo().VertAxis;
    const Vector3fT WorldPos=m_MapDoc.SnapToGrid(ViewWindow.WindowToWorld(ME.GetPosition(), 0.0f), ME.AltDown(), -1 /*Snap all axes.*/);

    switch (m_ToolState)
    {
        case IDLE:
        {
            unsigned long CamNr;
            CameraPartT   CamPart;

            ViewWindow.SetCursor(FindCamera(ViewWindow, ME.GetPosition(), CamNr, CamPart) ? CursorMan->GetCursor(CursorManT::CROSS) : *wxSTANDARD_CURSOR);
            break;
        }

        case DRAG_CAM_POS:
        {
            CameraT&        Cam      =*m_Cameras[m_ActiveCameraNr];
            const Vector3fT LookAtPos=Cam.Pos+Cam.GetYAxis()*Cam.ViewDirLength;

            if (Cam.Pos[HorzAxis]!=WorldPos[HorzAxis] ||
                Cam.Pos[VertAxis]!=WorldPos[VertAxis])
            {
                Cam.Pos[HorzAxis]=WorldPos[HorzAxis];
                Cam.Pos[VertAxis]=WorldPos[VertAxis];

                // If the user NOT wishes to move BOTH the camera position AND the look-at position
                // (the normal case, Control is up), restore the previous look-at position.
                if (!ME.ControlDown())
                {
                    Cam.SetLookAtPos(LookAtPos);
                    Cam.ViewDirLength=length(LookAtPos-Cam.Pos);
                }

                NotifyCameraChanged(m_Cameras[m_ActiveCameraNr]);
            }

            break;
        }

        case DRAG_CAM_DIR:
        {
            CameraT&        Cam      =*m_Cameras[m_ActiveCameraNr];
            const Vector3fT LookAtPos=Cam.Pos+Cam.GetYAxis()*Cam.ViewDirLength;

            if (LookAtPos[HorzAxis]!=WorldPos[HorzAxis] ||
                LookAtPos[VertAxis]!=WorldPos[VertAxis])
            {
                // If the user NOT wishes to move BOTH the camera position AND the look-at position
                // (the normal case, Control is up), just set the new look-at position.
                // Otherwise, move the entire camera by updating its position.
                if (!ME.ControlDown())
                {
                    Vector3fT NewLookAtPos=LookAtPos;   // Keep the value in the third axis.

                    NewLookAtPos[HorzAxis]=WorldPos[HorzAxis];
                    NewLookAtPos[VertAxis]=WorldPos[VertAxis];

                    Cam.SetLookAtPos(NewLookAtPos);
                    Cam.ViewDirLength=length(NewLookAtPos-Cam.Pos);
                }
                else
                {
                    Cam.Pos[HorzAxis]=WorldPos[HorzAxis]-Cam.GetYAxis()[HorzAxis]*Cam.ViewDirLength;
                    Cam.Pos[VertAxis]=WorldPos[VertAxis]-Cam.GetYAxis()[VertAxis]*Cam.ViewDirLength;
                }

                NotifyCameraChanged(m_Cameras[m_ActiveCameraNr]);
            }

            break;
        }
    }

    return true;
}


bool ToolCameraT::OnKeyDown3D(ViewWindow3DT& ViewWindow, wxKeyEvent& KE)
{
    return OnKeyDown(ViewWindow, KE);
}


bool ToolCameraT::OnMouseMove3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME)
{
    const int CamNr=m_Cameras.Find(const_cast<CameraT*>(&ViewWindow.GetCamera()));

    wxASSERT(CamNr!=-1);
    if (CamNr<0 || int(m_ActiveCameraNr)==CamNr) return false;

    // Have ViewWindow.GetCamera() become the newly active camera.
    NotifyCameraChanged(&ViewWindow.GetCamera());
    return true;
}


void ToolCameraT::RenderTool2D(Renderer2DT& Renderer) const
{
    // Render all cameras.
    for (unsigned long CameraNr=0; CameraNr<m_Cameras.Size(); CameraNr++)
    {
        wxColour HandleColor(0, 0, 0);
        wxColour LineColor  (0, 0, 0);

        const CameraT& DrawCam=*m_Cameras[CameraNr];
        const wxPoint  DCP    =Renderer.GetViewWin2D().WorldToTool(DrawCam.Pos);
        const wxPoint  DCL    =Renderer.GetViewWin2D().WorldToTool(DrawCam.Pos+DrawCam.GetYAxis()*DrawCam.ViewDirLength);

        // Set color for camera handle and line.
        if (IsActiveTool())
        {
            if (CameraNr==m_ActiveCameraNr)
            {
                HandleColor.Set(  0, 255, 255);
                LineColor.Set  (255,   0,   0);
            }
            else
            {
                HandleColor.Set(  0, 128, 128);
                LineColor.Set  (128,   0,   0);
            }
        }
        else
        {
            if (CameraNr==m_ActiveCameraNr)
            {
                HandleColor.Set(192, 192, 192);
                LineColor.Set  (192, 192, 192);
            }
            else
            {
                HandleColor.Set( 96,  96,  96);
                LineColor.Set  ( 96,  96,  96);
            }
        }

        // Draw the viewing vector of the camera.
        Renderer.SetLineType(wxPENSTYLE_SOLID, Renderer2DT::LINE_THIN, LineColor);
        Renderer.DrawLine(DCP, DCL);

        // Draw camera "tripod" (or rather "monopod").
        Renderer.SetLineType(wxPENSTYLE_SOLID, Renderer2DT::LINE_THIN, wxColour(0, 0, 0)); // Set the border color of the handles ellipse.
        Renderer.SetFillColor(HandleColor);

        Renderer.DrawEllipse(DCP, CAMERA_HANDLE_RADIUS, CAMERA_HANDLE_RADIUS, true);
    }
}


bool ToolCameraT::FindCamera(ViewWindow2DT& ViewWindow, const wxPoint& PointWS, unsigned long& CamNr, CameraPartT& CamPart) const
{
    const wxPoint PointTS=ViewWindow.WindowToTool(PointWS);

    for (unsigned long CameraNr=0; CameraNr<m_Cameras.Size(); CameraNr++)
    {
        const CameraT& Cam    =*m_Cameras[CameraNr];
        const wxPoint  CamPos =ViewWindow.WorldToTool(Cam.Pos);
        const wxPoint  LookPos=ViewWindow.WorldToTool(Cam.Pos+Cam.GetYAxis()*Cam.ViewDirLength);

        wxRect CamRect(CamPos, CamPos);
        CamRect.Inflate(CAMERA_HANDLE_RADIUS);

        if (CamRect.Contains(PointTS))
        {
            CamNr  =CameraNr;
            CamPart=POSITION;
            return true;
        }

        wxRect LookRect(LookPos, LookPos);
        LookRect.Inflate(CAMERA_HANDLE_RADIUS);

        if (LookRect.Contains(PointTS))
        {
            CamNr  =CameraNr;
            CamPart=LOOKAT;
            return true;
        }
    }

    return false;
}


bool ToolCameraT::OnKeyDown(ViewWindowT& ViewWindow, wxKeyEvent& KE)
{
    switch (KE.GetKeyCode())
    {
        case WXK_ESCAPE:
            if (m_ToolState!=IDLE)
            {
                m_ToolState=IDLE;
                NotifyCameraChanged(m_Cameras[m_ActiveCameraNr]);   // Not strictly necessary here...
            }
            else
            {
                m_ToolMan.SetActiveTool(GetToolTIM().FindTypeInfoByName("ToolSelectionT"));
            }
            return true;
    }

    return false;
}
