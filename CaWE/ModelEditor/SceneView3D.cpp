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
#include "../MapBrush.hpp"
#include "../MapFace.hpp"
#include "../Options.hpp"
#include "../ParentFrame.hpp"
#include "../Renderer3D.hpp"    // For class Renderer3DT::UseOrthoMatricesT.

#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Models/Model_cmdl.hpp"


BEGIN_EVENT_TABLE(ModelEditor::SceneView3DT, Generic3DWindowT)
    EVT_KEY_DOWN   (ModelEditor::SceneView3DT::OnKeyDown      )
    EVT_LEFT_DOWN  (ModelEditor::SceneView3DT::OnMouseLeftDown)
    EVT_LEFT_DCLICK(ModelEditor::SceneView3DT::OnMouseLeftDown)
    EVT_LEFT_UP    (ModelEditor::SceneView3DT::OnMouseLeftUp  )
    EVT_MOTION     (ModelEditor::SceneView3DT::OnMouseMove    )
    EVT_PAINT      (ModelEditor::SceneView3DT::OnPaint        )
    EVT_IDLE       (ModelEditor::SceneView3DT::OnIdle         )
END_EVENT_TABLE()


ModelEditor::SceneView3DT::SceneView3DT(ChildFrameT* Parent)
    : Generic3DWindowT(Parent, Parent->GetModelDoc()->GetCameras()[0]),
      m_Parent(Parent),
      m_Renderer(),
      m_TimeOfLastPaint(0)
{
}


Vector3fT ModelEditor::SceneView3DT::GetRefPtWorld(const wxPoint& RefPtWin) const
{
    float     BestFraction=std::numeric_limits<float>::max();
    Vector3fT BestPos     =GetCamera().Pos;

    // Note that our ray does intentionally not start at GetCamera().Pos,
    // but at the point of intersection with the near clipping plane!
    const Vector3fT RayOrigin=WindowToWorld(RefPtWin);
    const Vector3fT RayDir   =normalizeOr0(RayOrigin - GetCamera().Pos);

    // Make sure that the ray is valid. It should never be invalid though.
    if (length(RayDir)<0.9f) return BestPos;

    ArrayT<const MapElementT*> Elems;

    // Include the ground plane in the test only if it is visible - everything else confuses the user!
    if (m_Parent->GetScenePropGrid()->m_GroundPlane_Show)
        Elems.PushBack(m_Parent->GetModelDoc()->GetGround());

    for (unsigned long ElemNr=0; ElemNr<Elems.Size(); ElemNr++)
    {
        const MapElementT* Elem=Elems[ElemNr];
        float              Fraction=0;
        unsigned long      FaceNr=0;

        if (Elem->IsVisible() && Elem->TraceRay(RayOrigin, RayDir, Fraction, FaceNr))
        {
            if (Fraction<BestFraction)
            {
                BestFraction=Fraction;
                BestPos     =RayOrigin + RayDir*Fraction;
            }
        }
    }

    // As we currently cannot trace the ray against the model with per-triangle precision, use the bounding-box instead.
    const ModelDocumentT::ModelAnimationT& Anim=m_Parent->GetModelDoc()->GetAnim();
    const float*         ModelFl=m_Parent->GetModelDoc()->GetModel()->GetSequenceBB(Anim.SequNr, Anim.FrameNr);
    const BoundingBox3fT ModelBB(Vector3fT(ModelFl+0), Vector3fT(ModelFl+3));
    float Fraction;

    if (ModelBB.TraceRay(RayOrigin, RayDir, Fraction))
        if (Fraction<BestFraction)
        {
            BestFraction=Fraction;
            BestPos     =RayOrigin + RayDir*Fraction;
        }

    return BestPos;
}


void ModelEditor::SceneView3DT::InfoCameraChanged()
{
    // TODO: Limit the refresh to camera attributes?
    m_Parent->GetScenePropGrid()->RefreshPropGrid();
}


void ModelEditor::SceneView3DT::InfoRightMouseClick(wxMouseEvent& ME)
{
    ;
}


void ModelEditor::SceneView3DT::OnKeyDown(wxKeyEvent& KE)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
 // if (&GetMapDoc()==NULL) { KE.Skip(); return; }

    switch (KE.GetKeyCode())
    {
        case '+':
        case WXK_NUMPAD_ADD:
        {
            m_Parent->GetModelDoc()->SetNextAnimSequ();
            break;
        }

        case '-':
        case WXK_NUMPAD_SUBTRACT:
        {

            m_Parent->GetModelDoc()->SetPrevAnimSequ();
            break;
        }

        case WXK_TAB:
        {
            m_Parent->GetModelDoc()->SetAnimSpeed(m_Parent->GetModelDoc()->GetAnim().Speed>0.0f ? 0.0f : 1.0f);
            break;
        }

        default:
            // Event not handled here - now let the base class process it.
            KE.Skip();
            break;
    }
}


void ModelEditor::SceneView3DT::OnMouseLeftDown(wxMouseEvent& ME)
{
    // ToolI* ActiveTool=m_Parent->GetToolManager()->GetActiveTool();

    // if (ActiveTool) ActiveTool->OnLMouseDown(this, ME);
}


void ModelEditor::SceneView3DT::OnMouseLeftUp(wxMouseEvent& ME)
{
    // ToolI* ActiveTool=m_Parent->GetToolManager()->GetActiveTool();

    // if (ActiveTool) ActiveTool->OnLMouseUp(this, ME);
}


void ModelEditor::SceneView3DT::OnMouseMove(wxMouseEvent& ME)
{
    // Reset focus on mouse move, so zooming and scrolling by cursor keys always works, when the mouse is over the window.
    if (FindFocus()!=this) SetFocus();

    // Make sure that the base class always gets this event as well.
    ME.Skip();
}


static wxColour ScaleColor(const wxColour& Color, float Scale)
{
    return wxColour((unsigned char)(Color.Red()*Scale), (unsigned char)(Color.Green()*Scale), (unsigned char)(Color.Blue()*Scale));
}


void ModelEditor::SceneView3DT::RenderPass() const
{
    const ScenePropGridT* ScenePropGrid=m_Parent->GetScenePropGrid();

    // Render the ground plane.
    if (ScenePropGrid->m_GroundPlane_Show && MatSys::Renderer->GetCurrentRenderAction()!=MatSys::RendererI::STENCILSHADOW)
    {
        const MapBrushT* Ground=m_Parent->GetModelDoc()->GetGround();

        for (unsigned long FaceNr=0; FaceNr<Ground->GetFaces().Size(); FaceNr++)
        {
            const MapFaceT& Face =Ground->GetFaces()[FaceNr];
            const float     Shade=m_Renderer.GetConstShade(Face.GetPlane().Normal);

            Face.Render3DBasic(Face.GetMaterial()->GetRenderMaterial(true /*FullMats*/), ScaleColor(*wxWHITE, Shade), 255);
        }
    }

    // Render the model.
    const ModelDocumentT::ModelAnimationT& Anim=m_Parent->GetModelDoc()->GetAnim();

    m_Parent->GetModelDoc()->GetModel()->Draw(Anim.SequNr, Anim.FrameNr, 0.0f /*LodDist*/, NULL);
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

    // Drive the model documents time, i.e. to advance the animation.
    m_Parent->GetModelDoc()->AdvanceTime(FrameTime);


    /*********************/
    /*** Process Input ***/
    /*********************/

    ProcessInput(FrameTime);


    /********************/
    /*** Render Frame ***/
    /********************/

    wxPaintDC dc(this);     // It is VERY important not to omit this, or otherwise everything goes havoc.

    const ScenePropGridT* ScenePropGrid=m_Parent->GetScenePropGrid();

    // Initialize the viewport.
    SetCurrent(*wxGetApp().GetParentFrame()->m_GLContext);    // This is the method from the wxGLCanvas for activating the given RC with this window.
    const wxSize   CanvasSize=GetClientSize();
    const CameraT& Camera    =GetCamera();

    MatSys::Renderer->SetViewport(0, 0, CanvasSize.GetWidth(), CanvasSize.GetHeight());

    // Clear the buffers.
    MatSys::Renderer->ClearColor(ScenePropGrid->m_BackgroundColor.Red()/255.0f,
                                 ScenePropGrid->m_BackgroundColor.Green()/255.0f,
                                 ScenePropGrid->m_BackgroundColor.Blue()/255.0f, 0);

    MatSys::Renderer->BeginFrame(TimeNow/1000.0);


    // Initialize the perspective projection (view-to-clip) matrix.
    // Note that the far clip plane MUST be located at infinity for our stencil shadows implementation!
    MatSys::Renderer->SetMatrix(MatSys::RendererI::PROJECTION,
        MatrixT::GetProjPerspectiveMatrix(Camera.VerticalFOV, (float)CanvasSize.GetWidth()/(float)CanvasSize.GetHeight(),
                                          Camera.NearPlaneDist, -1.0f));   // The far plane must be at infinity, not Camera.FarPlaneDist!

    // Initialize the camera (world-to-view) matrix.
    MatrixT WorldToView=Camera.GetMatrix();

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

    MatSys::Renderer->SetCurrentEyePosition(Camera.Pos.x, Camera.Pos.y, Camera.Pos.z);
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

        MatSys::Renderer->SetCurrentMaterial(m_Renderer.GetRMatWireframe());
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

        MatSys::Renderer->SetCurrentMaterial(m_Renderer.GetRMatWireframe());
        MatSys::Renderer->RenderMesh(LightSourceMesh);
    }

    // Render the wire-frame BB of the model when we're in camera orbit mode,
    // because the BB (not the hit model triangle) is currently used as the reference point.
    if (GetMouseControl().GetState()==MouseControlT::ACTIVE_ORBIT &&
        GetMouseControl().GetRefPtWorld() != Camera.Pos &&
        fabs(GetMouseControl().GetRefPtWorld().z - m_Parent->GetModelDoc()->GetGround()->GetBB().Max.z) > 1.0f)
    {
        const ModelDocumentT::ModelAnimationT& Anim=m_Parent->GetModelDoc()->GetAnim();
        const float*         ModelFl=m_Parent->GetModelDoc()->GetModel()->GetSequenceBB(Anim.SequNr, Anim.FrameNr);
        const BoundingBox3fT ModelBB(Vector3fT(ModelFl+0), Vector3fT(ModelFl+3));

        m_Renderer.RenderBox(ModelBB, Options.colors.Selection, false /*Solid?*/);
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

    MatSys::Renderer->SetCurrentRenderAction(MatSys::RendererI::AMBIENT);


    // 3) Render the "HUD" of the mouse control.
    if (GetMouseControl().IsActive())
    {
        ::Renderer3DT::UseOrthoMatricesT UseOrtho(*this);

        m_Renderer.RenderCrossHair(GetMouseControl().GetRefPtWin());
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
