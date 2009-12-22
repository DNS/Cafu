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

#include "SceneView.hpp"
#include "ChildFrame.hpp"
#include "SceneSetup.hpp"
#include "../AppCaWE.hpp"
#include "../EditorMaterial.hpp"
#include "../Options.hpp"
#include "../ParentFrame.hpp"

#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"


BEGIN_EVENT_TABLE(ModelEditor::SceneViewT, wxGLCanvas)
    EVT_PAINT     (ModelEditor::SceneViewT::OnPaint     )
    EVT_IDLE      (ModelEditor::SceneViewT::OnIdle      )
    EVT_MOUSEWHEEL(ModelEditor::SceneViewT::OnMouseWheel)
    EVT_SIZE      (ModelEditor::SceneViewT::OnSize      )
    EVT_MOTION    (ModelEditor::SceneViewT::OnMouseMove )
    EVT_LEFT_DOWN (ModelEditor::SceneViewT::OnLMouseDown)
    EVT_LEFT_UP   (ModelEditor::SceneViewT::OnLMouseUp  )
    EVT_RIGHT_UP  (ModelEditor::SceneViewT::OnRMouseUp  )
    EVT_KEY_DOWN  (ModelEditor::SceneViewT::OnKeyDown   )
END_EVENT_TABLE()


ModelEditor::SceneViewT::SceneViewT(ChildFrameT* Parent)
    : wxGLCanvas(Parent, -1, ParentFrameT::OpenGLAttributeList, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS, "SceneViewWindow"),
      m_Parent(Parent),
      m_TimeOfLastPaint(0),
   // m_CameraTool(static_cast<ToolCameraT*>(m_ChildFrame->GetToolManager().GetTool(ToolCameraT::TypeInfo))),
      m_Camera(&Parent->GetSceneSetup()->m_Camera),
      m_CameraVel()
{
    m_RMatWireframe  =MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("CaWE/Wireframe"       ));
    m_RMatWireframeOZ=MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("CaWE/WireframeOffsetZ"));
}


ModelEditor::SceneViewT::~SceneViewT()
{
    MatSys::Renderer->FreeMaterial(m_RMatWireframe  );
    MatSys::Renderer->FreeMaterial(m_RMatWireframeOZ);
}


void ModelEditor::SceneViewT::OnPaint(wxPaintEvent& PE)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
 // if (&GetMapDoc()==NULL) { PE.Skip(); return; }


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
    // if (m_MouseControl.IsActive())
    // {
    //      // TODO...
    // }


    /********************/
    /*** Render Frame ***/
    /********************/

    wxPaintDC dc(this);     // It is VERY important not to omit this, or otherwise everything goes havoc.

    const SceneSetupT* SceneSetup=m_Parent->GetSceneSetup();

    // We're drawing to this view now.
    SetCurrent(*wxGetApp().GetParentFrame()->m_GLContext);    // This is the method from the wxGLCanvas for activating the given RC with this window.
    const wxSize CanvasSize=GetClientSize();

    MatSys::Renderer->SetViewport(0, 0, CanvasSize.GetWidth(), CanvasSize.GetHeight());

    // Clear the buffers.
    MatSys::Renderer->ClearColor(SceneSetup->m_BackgroundColor.Red()/255.0f,
                                 SceneSetup->m_BackgroundColor.Green()/255.0f,
                                 SceneSetup->m_BackgroundColor.Blue()/255.0f, 0);

    MatSys::Renderer->BeginFrame(TimeNow/1000.0);

    // Setup the perspective projection (view-to-clip) matrix.
    MatSys::Renderer->SetMatrix(MatSys::RendererI::PROJECTION,
        MatrixT::GetProjPerspectiveMatrix(m_Camera->VerticalFOV, (float)CanvasSize.GetWidth()/(float)CanvasSize.GetHeight(),
                                          m_Camera->NearPlaneDist, m_Camera->FarPlaneDist));

    // Setup the camera (world-to-view) matrix.
    MatrixT WorldToView=m_Camera->GetMatrix();

    // Rotate by 90 degrees around the x-axis in order to meet the MatSys's expectation of axes orientation.
    for (unsigned long i=0; i<4; i++)
    {
        std::swap(WorldToView[1][i], WorldToView[2][i]);
        WorldToView[2][i]=-WorldToView[2][i];
    }

    MatSys::Renderer->SetMatrix(MatSys::RendererI::WORLD_TO_VIEW, WorldToView);

    // Setup the model-to-world matrix.
    MatSys::Renderer->SetMatrix(MatSys::RendererI::MODEL_TO_WORLD, MatrixT());


    // Render the world axes. They're great for technical and emotional reassurance.
    if (SceneSetup->m_ShowOrigin)
    {
        static MatSys::MeshT Mesh(MatSys::MeshT::Lines);

        if (Mesh.Vertices.Size()==0)
        {
            const double l=100.0;
            Mesh.Vertices.PushBackEmpty(6);

            Mesh.Vertices[0].SetColor(1, 0, 0); Mesh.Vertices[0].SetOrigin(0, 0, 0);
            Mesh.Vertices[1].SetColor(1, 0, 0); Mesh.Vertices[1].SetOrigin(l, 0, 0);
            Mesh.Vertices[2].SetColor(0, 1, 0); Mesh.Vertices[2].SetOrigin(0, 0, 0);
            Mesh.Vertices[3].SetColor(0, 1, 0); Mesh.Vertices[3].SetOrigin(0, l, 0);
            Mesh.Vertices[4].SetColor(0, 0, 1); Mesh.Vertices[4].SetOrigin(0, 0, 0);
            Mesh.Vertices[5].SetColor(0, 0, 1); Mesh.Vertices[5].SetOrigin(0, 0, l);
        }

        MatSys::Renderer->SetCurrentMaterial(m_RMatWireframe);
        MatSys::Renderer->RenderMesh(Mesh);
    }

    // Render the ground plane.
    if (SceneSetup->m_GroundPlane_Show && SceneSetup->m_GroundPlane_Mat!=NULL)
 // if (DrawGroundPlane && GroundPlane_Mat!=NULL && (MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::AMBIENT || MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::LIGHTING))
    {
        static MatSys::MeshT GroundPlaneMesh(MatSys::MeshT::TriangleFan);

        if (GroundPlaneMesh.Vertices.Size()==0)
        {
            GroundPlaneMesh.Vertices.PushBackEmpty(4);

            GroundPlaneMesh.Vertices[0].SetTextureCoord(0.0f, 1.0f); GroundPlaneMesh.Vertices[0].SetNormal(0.0f, 0.0f, 1.0f); GroundPlaneMesh.Vertices[0].SetTangent(1.0f, 0.0f, 0.0f); GroundPlaneMesh.Vertices[0].SetBiNormal(0.0f, -1.0f, 0.0f);
            GroundPlaneMesh.Vertices[1].SetTextureCoord(0.0f, 0.0f); GroundPlaneMesh.Vertices[1].SetNormal(0.0f, 0.0f, 1.0f); GroundPlaneMesh.Vertices[1].SetTangent(1.0f, 0.0f, 0.0f); GroundPlaneMesh.Vertices[1].SetBiNormal(0.0f, -1.0f, 0.0f);
            GroundPlaneMesh.Vertices[2].SetTextureCoord(1.0f, 0.0f); GroundPlaneMesh.Vertices[2].SetNormal(0.0f, 0.0f, 1.0f); GroundPlaneMesh.Vertices[2].SetTangent(1.0f, 0.0f, 0.0f); GroundPlaneMesh.Vertices[2].SetBiNormal(0.0f, -1.0f, 0.0f);
            GroundPlaneMesh.Vertices[3].SetTextureCoord(1.0f, 1.0f); GroundPlaneMesh.Vertices[3].SetNormal(0.0f, 0.0f, 1.0f); GroundPlaneMesh.Vertices[3].SetTangent(1.0f, 0.0f, 0.0f); GroundPlaneMesh.Vertices[3].SetBiNormal(0.0f, -1.0f, 0.0f);
        }

        const double r=400.0;
        GroundPlaneMesh.Vertices[0].SetOrigin(-r, -r, SceneSetup->m_GroundPlane_zPos);
        GroundPlaneMesh.Vertices[1].SetOrigin(-r,  r, SceneSetup->m_GroundPlane_zPos);
        GroundPlaneMesh.Vertices[2].SetOrigin( r,  r, SceneSetup->m_GroundPlane_zPos);
        GroundPlaneMesh.Vertices[3].SetOrigin( r, -r, SceneSetup->m_GroundPlane_zPos);

        MatSys::Renderer->SetCurrentMaterial(SceneSetup->m_GroundPlane_Mat->GetRenderMaterial(true /*PreviewMode*/));
        MatSys::Renderer->RenderMesh(GroundPlaneMesh);
    }


    MatSys::Renderer->EndFrame();
    SwapBuffers();
}


void ModelEditor::SceneViewT::OnIdle(wxIdleEvent& IE)
{
    Refresh(false);

    if (wxGetApp().IsActive()) IE.RequestMore();    // TODO: Does this work as good as expected?
}


void ModelEditor::SceneViewT::OnMouseWheel(wxMouseEvent& ME)
{
 // // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
 // // namely during the continued event processing between the call to Destroy() and our final deletion.
 // if (&GetMapDoc()==NULL) { ME.Skip(); return; }
 //
 // // Give the active tool a chance to intercept the event.
 // ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();
 // if (Tool && Tool->OnMouseWheel3D(*this, ME)) return;

    m_Camera->Pos+=m_Camera->GetYAxis()*(ME.GetWheelRotation()/2);
    m_Parent->GetSceneSetup()->RefreshPropGrid();
}


void ModelEditor::SceneViewT::OnSize(wxSizeEvent& SE)
{
    // UpdateScrollbars();
    // CalcViewOffsets();

    Refresh(false);
}


void ModelEditor::SceneViewT::OnMouseMove(wxMouseEvent& ME)
{
    // Reset focus on mouse move, so zooming and scrolling by cursor keys always works, when the mouse is over the window.
    if (FindFocus()!=this) SetFocus();

    // ToolI* ActiveTool=m_Parent->GetToolManager()->GetActiveTool();

    // if (ActiveTool) ActiveTool->OnMouseMove(this, ME);
}


void ModelEditor::SceneViewT::OnLMouseDown(wxMouseEvent& ME)
{
    // ToolI* ActiveTool=m_Parent->GetToolManager()->GetActiveTool();

    // if (ActiveTool) ActiveTool->OnLMouseDown(this, ME);
}


void ModelEditor::SceneViewT::OnLMouseUp(wxMouseEvent& ME)
{
    // ToolI* ActiveTool=m_Parent->GetToolManager()->GetActiveTool();

    // if (ActiveTool) ActiveTool->OnLMouseUp(this, ME);
}


void ModelEditor::SceneViewT::OnRMouseUp(wxMouseEvent& ME)
{
    // ToolI* ActiveTool=m_Parent->GetToolManager()->GetActiveTool();

    // if (ActiveTool) ActiveTool->OnRMouseUp(this, ME);
}


void ModelEditor::SceneViewT::OnKeyDown(wxKeyEvent& KE)
{
    // ToolI* ActiveTool=m_Parent->GetToolManager()->GetActiveTool();

    // if (ActiveTool && ActiveTool->OnKeyDown(this, KE)) return;

#if 0
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
#endif
}
