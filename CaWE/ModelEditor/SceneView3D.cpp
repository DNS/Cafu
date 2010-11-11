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

#include "SceneView3D.hpp"
#include "ChildFrame.hpp"
#include "ModelDocument.hpp"
#include "ScenePropGrid.hpp"
#include "../AppCaWE.hpp"
#include "../EditorMaterial.hpp"
#include "../Options.hpp"
#include "../ParentFrame.hpp"

#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Models/Model_cmdl.hpp"


BEGIN_EVENT_TABLE(ModelEditor::SceneView3DT, wxGLCanvas)
    EVT_PAINT     (ModelEditor::SceneView3DT::OnPaint     )
    EVT_IDLE      (ModelEditor::SceneView3DT::OnIdle      )
    EVT_MOUSEWHEEL(ModelEditor::SceneView3DT::OnMouseWheel)
    EVT_SIZE      (ModelEditor::SceneView3DT::OnSize      )
    EVT_MOTION    (ModelEditor::SceneView3DT::OnMouseMove )
    EVT_LEFT_DOWN (ModelEditor::SceneView3DT::OnLMouseDown)
    EVT_LEFT_UP   (ModelEditor::SceneView3DT::OnLMouseUp  )
    EVT_RIGHT_UP  (ModelEditor::SceneView3DT::OnRMouseUp  )
    EVT_KEY_DOWN  (ModelEditor::SceneView3DT::OnKeyDown   )
END_EVENT_TABLE()


ModelEditor::SceneView3DT::SceneView3DT(ChildFrameT* Parent)
    : wxGLCanvas(Parent, -1, ParentFrameT::OpenGLAttributeList, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS, "SceneViewWindow"),
      m_Parent(Parent),
      m_TimeOfLastPaint(0),
   // m_CameraTool(static_cast<ToolCameraT*>(m_ChildFrame->GetToolManager().GetTool(ToolCameraT::TypeInfo))),
      m_Camera(Parent->GetModelDoc()->GetCameras()[0]),
      m_CameraVel()
{
    m_RMatWireframe  =MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("CaWE/Wireframe"       ));
    m_RMatWireframeOZ=MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("CaWE/WireframeOffsetZ"));
}


ModelEditor::SceneView3DT::~SceneView3DT()
{
    MatSys::Renderer->FreeMaterial(m_RMatWireframe  );
    MatSys::Renderer->FreeMaterial(m_RMatWireframeOZ);
}


void ModelEditor::SceneView3DT::RenderPass() const
{
    const ScenePropGridT* ScenePropGrid=m_Parent->GetScenePropGrid();

    // Render the ground plane.
    if (ScenePropGrid->m_GroundPlane_Show && ScenePropGrid->m_GroundPlane_Mat!=NULL && MatSys::Renderer->GetCurrentRenderAction()!=MatSys::RendererI::STENCILSHADOW)
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
        GroundPlaneMesh.Vertices[0].SetOrigin(-r, -r, ScenePropGrid->m_GroundPlane_zPos);
        GroundPlaneMesh.Vertices[1].SetOrigin(-r,  r, ScenePropGrid->m_GroundPlane_zPos);
        GroundPlaneMesh.Vertices[2].SetOrigin( r,  r, ScenePropGrid->m_GroundPlane_zPos);
        GroundPlaneMesh.Vertices[3].SetOrigin( r, -r, ScenePropGrid->m_GroundPlane_zPos);

        MatSys::Renderer->SetCurrentMaterial(ScenePropGrid->m_GroundPlane_Mat->GetRenderMaterial(true /*PreviewMode*/));
        MatSys::Renderer->RenderMesh(GroundPlaneMesh);
    }

    // Render the model.
    m_Parent->GetModelDoc()->GetModel()->Draw(0, 0.0f, 0.0f, NULL);
}


void ModelEditor::SceneView3DT::OnPaint(wxPaintEvent& PE)
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

    const ScenePropGridT* ScenePropGrid=m_Parent->GetScenePropGrid();

    // Initialize the viewport.
    SetCurrent(*wxGetApp().GetParentFrame()->m_GLContext);    // This is the method from the wxGLCanvas for activating the given RC with this window.
    const wxSize CanvasSize=GetClientSize();

    MatSys::Renderer->SetViewport(0, 0, CanvasSize.GetWidth(), CanvasSize.GetHeight());

    // Clear the buffers.
    MatSys::Renderer->ClearColor(ScenePropGrid->m_BackgroundColor.Red()/255.0f,
                                 ScenePropGrid->m_BackgroundColor.Green()/255.0f,
                                 ScenePropGrid->m_BackgroundColor.Blue()/255.0f, 0);

    MatSys::Renderer->BeginFrame(TimeNow/1000.0);


    // Initialize the perspective projection (view-to-clip) matrix.
    // Note that the far clip plane MUST be located at infinity for our stencil shadows implementation!
    MatSys::Renderer->SetMatrix(MatSys::RendererI::PROJECTION,
        MatrixT::GetProjPerspectiveMatrix(m_Camera->VerticalFOV, (float)CanvasSize.GetWidth()/(float)CanvasSize.GetHeight(),
                                          m_Camera->NearPlaneDist, -1.0f));   // The far plane must be at infinity, not m_Camera->FarPlaneDist!

    // Initialize the camera (world-to-view) matrix.
    MatrixT WorldToView=m_Camera->GetMatrix();

    // Rotate by 90 degrees around the x-axis in order to meet the MatSys's expectation of axes orientation.
    for (unsigned long i=0; i<4; i++)
    {
        std::swap(WorldToView[1][i], WorldToView[2][i]);
        WorldToView[2][i]=-WorldToView[2][i];
    }

    MatSys::Renderer->SetMatrix(MatSys::RendererI::WORLD_TO_VIEW, WorldToView);

    // Initialize the model-to-world matrix.
    MatSys::Renderer->SetMatrix(MatSys::RendererI::MODEL_TO_WORLD, MatrixT());


    // 1) Draw the ambient rendering pass.
    MatSys::Renderer->SetCurrentRenderAction(MatSys::RendererI::AMBIENT);

    MatSys::Renderer->SetCurrentEyePosition(m_Camera->Pos.x, m_Camera->Pos.y, m_Camera->Pos.z);
    MatSys::Renderer->SetCurrentAmbientLightColor(ScenePropGrid->m_AmbientLightColor.Red()/255.0f,
                                                  ScenePropGrid->m_AmbientLightColor.Green()/255.0f,
                                                  ScenePropGrid->m_AmbientLightColor.Blue()/255.0f);

    // Set a proper lightmap for e.g. the ground plane. Model materials should not reference and thus not need a lightmap at all.
    MatSys::Renderer->SetCurrentLightMap(ScenePropGrid->m_AmbientTexture);
    MatSys::Renderer->SetCurrentLightDirMap(NULL);    // The MatSys provides a default for LightDirMaps when NULL is set.

    // Render the world axes. They're great for technical and emotional reassurance.
    if (ScenePropGrid->m_ShowOrigin)
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

    // Render a small cross at the (world-space) position of each active light source.
    const ArrayT<ModelDocumentT::LightSourceT*>& LightSources=m_Parent->GetModelDoc()->GetLightSources();

    for (unsigned long LsNr=0; LsNr<LightSources.Size(); LsNr++)
    {
        const ModelDocumentT::LightSourceT& LS=*LightSources[LsNr];

        if (!LS.IsOn) continue;

        const float r=LS.Color.Red()/255.0f;
        const float g=LS.Color.Green()/255.0f;
        const float b=LS.Color.Blue()/255.0f;

        static MatSys::MeshT LightSourceMesh(MatSys::MeshT::Lines);
        if (LightSourceMesh.Vertices.Size()==0) LightSourceMesh.Vertices.PushBackEmpty(6);

        LightSourceMesh.Vertices[0].SetOrigin(LS.Pos.x+50.0f, LS.Pos.y, LS.Pos.z); LightSourceMesh.Vertices[0].SetColor(r, g, b);
        LightSourceMesh.Vertices[1].SetOrigin(LS.Pos.x-50.0f, LS.Pos.y, LS.Pos.z); LightSourceMesh.Vertices[1].SetColor(r, g, b);

        LightSourceMesh.Vertices[2].SetOrigin(LS.Pos.x, LS.Pos.y+50.0f, LS.Pos.z); LightSourceMesh.Vertices[2].SetColor(r, g, b);
        LightSourceMesh.Vertices[3].SetOrigin(LS.Pos.x, LS.Pos.y-50.0f, LS.Pos.z); LightSourceMesh.Vertices[3].SetColor(r, g, b);

        LightSourceMesh.Vertices[4].SetOrigin(LS.Pos.x, LS.Pos.y, LS.Pos.z+50.0f); LightSourceMesh.Vertices[4].SetColor(r, g, b);
        LightSourceMesh.Vertices[5].SetOrigin(LS.Pos.x, LS.Pos.y, LS.Pos.z-50.0f); LightSourceMesh.Vertices[5].SetColor(r, g, b);

        MatSys::Renderer->SetCurrentMaterial(m_RMatWireframe);
        MatSys::Renderer->RenderMesh(LightSourceMesh);
    }

    RenderPass();


    // 2) For each light source, draw the dynamic lighting passes.
    for (unsigned long LsNr=0; LsNr<LightSources.Size(); LsNr++)
    {
        const ModelDocumentT::LightSourceT& LS=*LightSources[LsNr];

        if (!LS.IsOn) continue;

        const float r=LS.Color.Red()/255.0f;
        const float g=LS.Color.Green()/255.0f;
        const float b=LS.Color.Blue()/255.0f;

        // World-space and model-space are identical here, so we can directly set the world-space light source parameters as model-space parameters.
        MatSys::Renderer->SetCurrentLightSourcePosition(LS.Pos.x, LS.Pos.y, LS.Pos.z);
        MatSys::Renderer->SetCurrentLightSourceRadius(LS.Radius);
        MatSys::Renderer->SetCurrentLightSourceDiffuseColor (r, g, b);
        MatSys::Renderer->SetCurrentLightSourceSpecularColor(r, g, b);

        // 2a) Draw stencil shadow pass.
        if (LS.CastShadows)
        {
            MatSys::Renderer->SetCurrentRenderAction(MatSys::RendererI::STENCILSHADOW);
            RenderPass();
        }

        // 2b) Draw dynamic lighting pass.
        MatSys::Renderer->SetCurrentRenderAction(MatSys::RendererI::LIGHTING);
        RenderPass();
    }


    // Finish rendering.
    MatSys::Renderer->EndFrame();
    SwapBuffers();
}


void ModelEditor::SceneView3DT::OnIdle(wxIdleEvent& IE)
{
    Refresh(false);

    if (wxGetApp().IsActive()) IE.RequestMore();    // TODO: Does this work as good as expected?
}


void ModelEditor::SceneView3DT::OnMouseWheel(wxMouseEvent& ME)
{
 // // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
 // // namely during the continued event processing between the call to Destroy() and our final deletion.
 // if (&GetMapDoc()==NULL) { ME.Skip(); return; }
 //
 // // Give the active tool a chance to intercept the event.
 // ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();
 // if (Tool && Tool->OnMouseWheel3D(*this, ME)) return;

    m_Camera->Pos+=m_Camera->GetYAxis()*(ME.GetWheelRotation()/2);
    m_Parent->GetScenePropGrid()->RefreshPropGrid();
}


void ModelEditor::SceneView3DT::OnSize(wxSizeEvent& SE)
{
    // UpdateScrollbars();
    // CalcViewOffsets();

    Refresh(false);
}


void ModelEditor::SceneView3DT::OnMouseMove(wxMouseEvent& ME)
{
    // Reset focus on mouse move, so zooming and scrolling by cursor keys always works, when the mouse is over the window.
    if (FindFocus()!=this) SetFocus();

    // ToolI* ActiveTool=m_Parent->GetToolManager()->GetActiveTool();

    // if (ActiveTool) ActiveTool->OnMouseMove(this, ME);
}


void ModelEditor::SceneView3DT::OnLMouseDown(wxMouseEvent& ME)
{
    // ToolI* ActiveTool=m_Parent->GetToolManager()->GetActiveTool();

    // if (ActiveTool) ActiveTool->OnLMouseDown(this, ME);
}


void ModelEditor::SceneView3DT::OnLMouseUp(wxMouseEvent& ME)
{
    // ToolI* ActiveTool=m_Parent->GetToolManager()->GetActiveTool();

    // if (ActiveTool) ActiveTool->OnLMouseUp(this, ME);
}


void ModelEditor::SceneView3DT::OnRMouseUp(wxMouseEvent& ME)
{
    // ToolI* ActiveTool=m_Parent->GetToolManager()->GetActiveTool();

    // if (ActiveTool) ActiveTool->OnRMouseUp(this, ME);
}


void ModelEditor::SceneView3DT::OnKeyDown(wxKeyEvent& KE)
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
