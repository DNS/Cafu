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

#include "SceneView3D.hpp"
#include "ChildFrame.hpp"
#include "ModelDocument.hpp"
#include "ScenePropGrid.hpp"
#include "Commands/UpdateGuiFixture.hpp"
#include "../AppCaWE.hpp"
#include "../Camera.hpp"
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


BEGIN_EVENT_TABLE(ModelEditor::SceneView3DT, Generic3DWindowT)
    EVT_KEY_DOWN    (ModelEditor::SceneView3DT::OnKeyDown      )
    EVT_LEFT_DOWN   (ModelEditor::SceneView3DT::OnMouseLeftDown)
    EVT_LEFT_DCLICK (ModelEditor::SceneView3DT::OnMouseLeftDown)
    EVT_LEFT_UP     (ModelEditor::SceneView3DT::OnMouseLeftUp  )
    EVT_MOTION      (ModelEditor::SceneView3DT::OnMouseMove    )
    EVT_CONTEXT_MENU(ModelEditor::SceneView3DT::OnContextMenu  )
    EVT_PAINT       (ModelEditor::SceneView3DT::OnPaint        )
    EVT_IDLE        (ModelEditor::SceneView3DT::OnIdle         )
END_EVENT_TABLE()


ModelEditor::SceneView3DT::SceneView3DT(ChildFrameT* Parent)
    : Generic3DWindowT(Parent, Parent->GetModelDoc()->GetCameras()[0]),
      m_Parent(Parent),
      m_Renderer(),
      m_TimeOfLastPaint(0)
{
}


Vector3fT ModelEditor::SceneView3DT::TraceCameraRay(const wxPoint& RefPtWin, ModelT::TraceResultT& ModelTR) const
{
    float     BestFraction=std::numeric_limits<float>::max();
    Vector3fT BestPos     =GetCamera().Pos;

    // Note that our ray does intentionally not start at GetCamera().Pos,
    // but at the point of intersection with the near clipping plane!
    const Vector3fT RayOrigin=WindowToWorld(RefPtWin);
    const Vector3fT RayDir   =normalizeOr0(RayOrigin - GetCamera().Pos);

    // Initialize the ModelTR as "invalid".
    ModelTR.Fraction=-1.0f;

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

    // Trace the ray against the model, which is a per-triangle accurate test.
    const ModelDocumentT::AnimStateT& Anim   =m_Parent->GetModelDoc()->GetAnimState();
    const ArrayT<unsigned int>&       AnimSel=m_Parent->GetModelDoc()->GetSelection(ANIM);
    ModelT::TraceResultT Result;

    if (m_Parent->GetModelDoc()->GetModel()->TraceRay(AnimSel.Size()==0 ? -1 : AnimSel[0], Anim.FrameNr, RayOrigin, RayDir, Result) && Result.Fraction<BestFraction)
    {
        BestFraction=Result.Fraction;
        BestPos     =RayOrigin + RayDir*Result.Fraction;
        ModelTR     =Result;
    }

    return BestPos;
}


Vector3fT ModelEditor::SceneView3DT::GetRefPtWorld(const wxPoint& RefPtWin) const
{
    ModelT::TraceResultT ModelTR;

    return TraceCameraRay(RefPtWin, ModelTR);
}


void ModelEditor::SceneView3DT::InfoCameraChanged()
{
    // TODO: Limit the refresh to camera attributes?
    m_Parent->GetScenePropGrid()->RefreshPropGrid();
}


// As the RMB serves multiple functions (click, context-menu, camera control),
// the Generic3DWindowT keeps tight control over it and thus its event are handled differently.
void ModelEditor::SceneView3DT::InfoRightMouseClick(wxMouseEvent& ME)
{
    // As we have no tool that might have handled the event, now show the context menu.
    // Showing the context menu manually here also makes RMB handling uniform
    // across platforms, see <http://trac.wxwidgets.org/ticket/12535> for details.
    wxContextMenuEvent CE(wxEVT_CONTEXT_MENU, GetId(), ClientToScreen(ME.GetPosition()));

    // We don't inline the OnContextMenu() code here, because context menus can also be opened via the keyboard.
    CE.SetEventObject(this);
    OnContextMenu(CE);
}


void ModelEditor::SceneView3DT::OnKeyDown(wxKeyEvent& KE)
{
    ModelDocumentT* ModelDoc=m_Parent->GetModelDoc();

    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
 // if (&GetMapDoc()==NULL) { KE.Skip(); return; }

    switch (KE.GetKeyCode())
    {
        case '+':
        case WXK_NUMPAD_ADD:
        {
            const ArrayT<unsigned int> OldSel=ModelDoc->GetSelection(ANIM);
            ModelDoc->SetSelection(ANIM, ModelDoc->GetSelection_NextAnimSequ());
            ModelDoc->UpdateAllObservers_SelectionChanged(ANIM, OldSel, ModelDoc->GetSelection(ANIM));

            ModelDoc->GetAnimState().FrameNr=0.0f;
            ModelDoc->UpdateAllObservers_AnimStateChanged();
            break;
        }

        case '-':
        case WXK_NUMPAD_SUBTRACT:
        {
            const ArrayT<unsigned int> OldSel=ModelDoc->GetSelection(ANIM);
            ModelDoc->SetSelection(ANIM, ModelDoc->GetSelection_PrevAnimSequ());
            ModelDoc->UpdateAllObservers_SelectionChanged(ANIM, OldSel, ModelDoc->GetSelection(ANIM));

            ModelDoc->GetAnimState().FrameNr=0.0f;
            ModelDoc->UpdateAllObservers_AnimStateChanged();
            break;
        }

        case WXK_TAB:
        {
            ModelDoc->SetAnimSpeed(ModelDoc->GetAnimState().Speed!=0.0f ? 0.0f : 1.0f);
            ModelDoc->UpdateAllObservers_AnimStateChanged();
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


void ModelEditor::SceneView3DT::OnContextMenu(wxContextMenuEvent& CE)
{
    // Note that GetPopupMenuSelectionFromUser() temporarily disables UI updates for the window,
    // so our menu IDs used below should be doubly clash-free.
    enum
    {
        ID_MENU_GUIFIX_NONE=wxID_HIGHEST+1+100,
        ID_MENU_SET_GUIFIX_ORIGIN,
        ID_MENU_SET_GUIFIX_ENDPOINT_X,
        ID_MENU_SET_GUIFIX_ENDPOINT_Y
    };

    ModelT::TraceResultT ModelTR;
    const Vector3fT      HitPos=TraceCameraRay(ScreenToClient(CE.GetPosition()), ModelTR);
    const CafuModelT*    Model=m_Parent->GetModelDoc()->GetModel();
    unsigned int         BestVertexNr=0;
    bool                 HaveModelHit=false;

    if (ModelTR.Fraction>0.0f && ModelTR.MeshNr<Model->GetMeshes().Size() && ModelTR.TriNr<Model->GetMeshes()[ModelTR.MeshNr].Triangles.Size())
    {
        float BestDist=0.0f;

        for (unsigned int i=0; i<3; i++)
        {
            const unsigned int VertexNr=Model->GetMeshes()[ModelTR.MeshNr].Triangles[ModelTR.TriNr].VertexIdx[i];
            const Vector3fT&   DrawPos =Model->GetMeshes()[ModelTR.MeshNr].Vertices[VertexNr].Draw_Pos;
            const float        Dist    =length(DrawPos-HitPos);

            if (i==0 || Dist<BestDist)
            {
                BestDist    =Dist;
                BestVertexNr=VertexNr;
            }
        }

        HaveModelHit=true;
    }

    wxMenu Menu;
    const ArrayT<unsigned int>& Sel=m_Parent->GetModelDoc()->GetSelection(GFIX);

    if (Sel.Size()==0)
    {
        Menu.Append(ID_MENU_GUIFIX_NONE, "No GUI fixture selected")->Enable(false);
    }
    else if (Sel.Size()>1)
    {
        Menu.Append(ID_MENU_GUIFIX_NONE, "Multiple GUI fixtures selected")->Enable(false);
    }
    else
    {
        wxMenu* SubmenuGF=new wxMenu();

        SubmenuGF->Append(ID_MENU_GUIFIX_NONE,
            HaveModelHit ? wxString::Format("Mesh %u, Vertex %u:", ModelTR.MeshNr, BestVertexNr) : wxString("Right-click did not hit model!"))->Enable(false);

        SubmenuGF->Append(ID_MENU_SET_GUIFIX_ORIGIN,     "Set Origin")->Enable(HaveModelHit);
        SubmenuGF->Append(ID_MENU_SET_GUIFIX_ENDPOINT_X, "Set X-endpoint")->Enable(HaveModelHit);
        SubmenuGF->Append(ID_MENU_SET_GUIFIX_ENDPOINT_Y, "Set Y-endpoint")->Enable(HaveModelHit);

        Menu.AppendSubMenu(SubmenuGF, "GUI fixture");
    }

    const int PointNr=GetPopupMenuSelectionFromUser(Menu)-ID_MENU_SET_GUIFIX_ORIGIN;

    if (HaveModelHit && PointNr>=0 && PointNr<=2)
    {
        CafuModelT::GuiFixtureT GF=Model->GetGuiFixtures()[Sel[0]];

        GF.Points[PointNr].MeshNr  =ModelTR.MeshNr;
        GF.Points[PointNr].VertexNr=BestVertexNr;

        m_Parent->SubmitCommand(new CommandUpdateGuiFixtureT(m_Parent->GetModelDoc(), Sel[0], GF));
    }
}


void ModelEditor::SceneView3DT::RenderSkeleton(const ArrayT<CafuModelT::JointT>& Joints, const ArrayT<MatrixT>& Matrices) const
{
    static MatSys::MeshT Skeleton(MatSys::MeshT::Lines);

    Skeleton.Vertices.Overwrite();

    for (unsigned long JointNr=0; JointNr<Joints.Size(); JointNr++)
    {
        const MatrixT& Mj=Matrices[JointNr];

        // Don't draw the line that connects the origin and the root joint of the model.
        if (Joints[JointNr].Parent!=-1)
        {
            const MatrixT& Mp=Matrices[Joints[JointNr].Parent];

            Skeleton.Vertices.PushBackEmpty(2);
            MatSys::MeshT::VertexT& V1=Skeleton.Vertices[Skeleton.Vertices.Size()-2];
            MatSys::MeshT::VertexT& V2=Skeleton.Vertices[Skeleton.Vertices.Size()-1];

            V1.SetOrigin(Mp[0][3], Mp[1][3], Mp[2][3]); V1.SetColor(1, 0, 0);
            V2.SetOrigin(Mj[0][3], Mj[1][3], Mj[2][3]); V2.SetColor(1, 1, 0, 0.5f);
        }

        // Draw the coordinate axes of Mj.
        for (unsigned int i=0; i<3; i++)
        {
            Vector3fT Axis; Axis[i]=1.0f;

            Skeleton.Vertices.PushBackEmpty(2);
            MatSys::MeshT::VertexT& V1=Skeleton.Vertices[Skeleton.Vertices.Size()-2];
            MatSys::MeshT::VertexT& V2=Skeleton.Vertices[Skeleton.Vertices.Size()-1];

            V1.SetOrigin(Mj[0][3], Mj[1][3], Mj[2][3]); V1.SetColor(Axis.x, Axis.y, Axis.z);
            V2.SetOrigin(Mj.Mul1(Axis));                V2.SetColor(Axis.x, Axis.y, Axis.z, 0.3f);
        }
    }

    MatSys::Renderer->SetCurrentMaterial(m_Renderer.GetRMatWireframe());
    MatSys::Renderer->RenderMesh(Skeleton);
}


static wxColour ScaleColor(const wxColour& Color, float Scale)
{
    return wxColour((unsigned char)(Color.Red()*Scale), (unsigned char)(Color.Green()*Scale), (unsigned char)(Color.Blue()*Scale));
}


void ModelEditor::SceneView3DT::RenderPass() const
{
    const ModelDocumentT* ModelDoc     =m_Parent->GetModelDoc();
    const ScenePropGridT* ScenePropGrid=m_Parent->GetScenePropGrid();

    // Render the ground plane.
    if (ScenePropGrid->m_GroundPlane_Show && MatSys::Renderer->GetCurrentRenderAction()!=MatSys::RendererI::STENCILSHADOW)
    {
        const MapBrushT* Ground=ModelDoc->GetGround();

        for (unsigned long FaceNr=0; FaceNr<Ground->GetFaces().Size(); FaceNr++)
        {
            const MapFaceT& Face =Ground->GetFaces()[FaceNr];
            const float     Shade=m_Renderer.GetConstShade(Face.GetPlane().Normal);

            Face.Render3DBasic(Face.GetMaterial()->GetRenderMaterial(true /*FullMats*/), ScaleColor(*wxWHITE, Shade), 255);
        }
    }


    // Render the model.
    const CafuModelT*                 Model  =ModelDoc->GetModel();
    const ModelDocumentT::AnimStateT& Anim   =ModelDoc->GetAnimState();
    const ArrayT<unsigned int>&       AnimSel=ModelDoc->GetSelection(ANIM);
    const int                         SequNr =AnimSel.Size()==0 ? -1 : AnimSel[0];

    if (ScenePropGrid->m_Model_ShowMesh)
    {
        Model->Draw(SequNr, Anim.FrameNr, 0.0f /*LodDist*/, (CafuModelT::SuperT*)NULL);

        for (unsigned long SmNr=0; SmNr<ModelDoc->GetSubmodels().Size(); SmNr++)
        {
            const ModelDocumentT::SubmodelT* SM=ModelDoc->GetSubmodels()[SmNr];
            const CafuModelT::SuperT Super(
                Model->GetDrawJointMatrices(SequNr, Anim.FrameNr),
                SM->GetJointsMap());

            SM->GetSubmodel()->Draw(0, 0.0f, 0.0f /*LodDist*/, &Super);
        }
    }


    // Render the skeleton of the model.
    if (ScenePropGrid->m_Model_ShowSkeleton && MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::AMBIENT)
    {
        RenderSkeleton(Model->GetJoints(), Model->GetDrawJointMatrices(SequNr, Anim.FrameNr));

        for (unsigned long SmNr=0; SmNr<ModelDoc->GetSubmodels().Size(); SmNr++)
        {
            const ModelDocumentT::SubmodelT* SM=ModelDoc->GetSubmodels()[SmNr];
            const CafuModelT::SuperT Super(
                Model->GetDrawJointMatrices(SequNr, Anim.FrameNr),
                SM->GetJointsMap());

            RenderSkeleton(SM->GetSubmodel()->GetJoints(), SM->GetSubmodel()->GetDrawJointMatrices(0, 0.0f, &Super));
        }
    }
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
    m_Parent->GetModelDoc()->UpdateAllObservers_AnimStateChanged();


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
