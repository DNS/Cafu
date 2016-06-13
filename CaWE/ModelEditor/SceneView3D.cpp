/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "SceneView3D.hpp"
#include "ChildFrame.hpp"
#include "ModelDocument.hpp"
#include "ScenePropGrid.hpp"
#include "Commands/UpdateGuiFixture.hpp"
#include "Commands/UpdateTriangle.hpp"
#include "../AppCaWE.hpp"
#include "../Camera.hpp"
#include "../EditorMaterial.hpp"
#include "../MapEditor/MapBrush.hpp"
#include "../MapEditor/MapFace.hpp"
#include "../Options.hpp"
#include "../ParentFrame.hpp"
#include "../MapEditor/Renderer3D.hpp"    // For class Renderer3DT::UseOrthoMatricesT.

#include "GuiSys/GuiImpl.hpp"
#include "GuiSys/Window.hpp"
#include "MaterialSystem/Material.hpp"
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


Vector3fT ModelEditor::SceneView3DT::TraceCameraRay(const wxPoint& RefPtWin, AnimPoseT::TraceResultT& ModelTR) const
{
    const ModelDocumentT* ModelDoc    =m_Parent->GetModelDoc();
    float                 BestFraction=std::numeric_limits<float>::max();
    Vector3fT             BestPos     =GetCamera().Pos;

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
        Elems.PushBack(ModelDoc->GetGround());

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
    AnimPoseT::TraceResultT Result;

    if (ModelDoc->GetAnimState().Pose.TraceRay(ModelDoc->GetSelSkinNr(), RayOrigin, RayDir, Result) && Result.Fraction<BestFraction)
    {
        BestFraction=Result.Fraction;
        BestPos     =RayOrigin + RayDir*Result.Fraction;
        ModelTR     =Result;
    }

    return BestPos;
}


Vector3fT ModelEditor::SceneView3DT::GetRefPtWorld(const wxPoint& RefPtWin) const
{
    AnimPoseT::TraceResultT ModelTR;

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

            // This is not needed: setting a new ANIM selection also sets the frame number to 0.
            // ModelDoc->GetAnimState().LastStdAE->SetFrameNr(0.0f);
            // ModelDoc->UpdateAllObservers_AnimStateChanged();
            break;
        }

        case '-':
        case WXK_NUMPAD_SUBTRACT:
        {
            const ArrayT<unsigned int> OldSel=ModelDoc->GetSelection(ANIM);
            ModelDoc->SetSelection(ANIM, ModelDoc->GetSelection_PrevAnimSequ());
            ModelDoc->UpdateAllObservers_SelectionChanged(ANIM, OldSel, ModelDoc->GetSelection(ANIM));

            // This is not needed: setting a new ANIM selection also sets the frame number to 0.
            // ModelDoc->GetAnimState().LastStdAE->SetFrameNr(0.0f);
            // ModelDoc->UpdateAllObservers_AnimStateChanged();
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
        ID_MENU_SET_GUIFIX_ENDPOINT_Y,
        ID_MENU_TRIANGLE_INFO,
        ID_MENU_TRIANGLE_SKIPDRAW
    };

    AnimPoseT::TraceResultT ModelTR;
    const Vector3fT         HitPos=TraceCameraRay(ScreenToClient(CE.GetPosition()), ModelTR);
    const CafuModelT*       Model=m_Parent->GetModelDoc()->GetModel();
    const AnimPoseT&        Pose=m_Parent->GetModelDoc()->GetAnimState().Pose;
    unsigned int            BestVertexNr=0;
    bool                    HaveModelHit=false;

    if (ModelTR.Fraction>0.0f && ModelTR.MeshNr<Model->GetMeshes().Size() && ModelTR.TriNr<Model->GetMeshes()[ModelTR.MeshNr].Triangles.Size())
    {
        BestVertexNr=Pose.FindClosestVertex(ModelTR.MeshNr, ModelTR.TriNr, HitPos);
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

    if (HaveModelHit)
    {
        const bool SkipDraw=Model->GetMeshes()[ModelTR.MeshNr].Triangles[ModelTR.TriNr].SkipDraw;

        Menu.AppendSeparator();
        Menu.Append(ID_MENU_TRIANGLE_INFO, wxString::Format("Mesh %u, Triangle %u:", ModelTR.MeshNr, ModelTR.TriNr))->Enable(false);
        Menu.AppendCheckItem(ID_MENU_TRIANGLE_SKIPDRAW, "Hide Triangle (skip drawing)")->Check(SkipDraw);
    }


    const int MenuSelID=GetPopupMenuSelectionFromUser(Menu);

    switch (MenuSelID)
    {
        case ID_MENU_TRIANGLE_SKIPDRAW:
        {
            if (HaveModelHit)
            {
                const bool SkipDraw=Model->GetMeshes()[ModelTR.MeshNr].Triangles[ModelTR.TriNr].SkipDraw;

                m_Parent->SubmitCommand(new CommandUpdateTriangleT(m_Parent->GetModelDoc(), ModelTR.MeshNr, ModelTR.TriNr, !SkipDraw));
            }
        }

        default:
        {
            const int PointNr=MenuSelID-ID_MENU_SET_GUIFIX_ORIGIN;

            if (HaveModelHit && PointNr>=0 && PointNr<=2)
            {
                CafuModelT::GuiFixtureT GF=Model->GetGuiFixtures()[Sel[0]];

                GF.Points[PointNr].MeshNr  =ModelTR.MeshNr;
                GF.Points[PointNr].VertexNr=BestVertexNr;

                m_Parent->SubmitCommand(new CommandUpdateGuiFixtureT(m_Parent->GetModelDoc(), Sel[0], GF));
            }
        }
    }
}


void ModelEditor::SceneView3DT::RenderSkeleton(const ArrayT<CafuModelT::JointT>& Joints, const ArrayT<MatrixT>& Matrices, bool IsSubModel) const
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

            V1.SetOrigin(Mp[0][3], Mp[1][3], Mp[2][3]);
            V2.SetOrigin(Mj[0][3], Mj[1][3], Mj[2][3]);

            if (!IsSubModel && JointNr<m_JointSelCache.Size() && m_JointSelCache[JointNr])
            {
                // Set highlight color for selected joints.
                const float c=float(sin(m_TimeOfLastPaint / 300.0)*0.4 + 0.4);

                V1.SetColor(c, 0.8f, 0.8f);
                V2.SetColor(0,    c, 0.8f);
            }
            else
            {
                // Set color for normal, unselected joints.
                V1.SetColor(1, 0, 0);
                V2.SetColor(1, 1, 0, 0.5f);
            }
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
    const CafuModelT*                 Model=ModelDoc->GetModel();
    const ModelDocumentT::AnimStateT& Anim =ModelDoc->GetAnimState();

    if (ScenePropGrid->m_Model_ShowMesh)
    {
        switch (ScenePropGrid->m_Model_DebugMaterial)
        {
            case 1:
                MatSys::Renderer->SetCurrentMaterial(m_Renderer.GetRMatWhite());
                MatSys::Renderer->LockCurrentMaterial(true);
                break;

            case 2:
                MatSys::Renderer->SetCurrentMaterial(m_Renderer.GetRMatTexturedWireframe());
                MatSys::Renderer->LockCurrentMaterial(true);
                break;
        }

        Anim.Pose.Draw(ModelDoc->GetSelSkinNr(), 0.0f /*LodDist*/);

        for (unsigned long SmNr=0; SmNr<ModelDoc->GetSubmodels().Size(); SmNr++)
        {
            ModelDocumentT::SubmodelT* SM=ModelDoc->GetSubmodels()[SmNr];
            AnimPoseT&                 SmPose=SM->GetPose();

            SmPose.SetSuperPose(&Anim.Pose);
            SmPose.Draw(-1 /*SkinNr*/, 0.0f /*LodDist*/);
            SmPose.SetSuperPose(NULL);
        }

        MatSys::Renderer->LockCurrentMaterial(false);
    }


    // Render the skeleton of the model.
    if (ScenePropGrid->m_Model_ShowSkeleton && MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::AMBIENT)
    {
        RenderSkeleton(Model->GetJoints(), Anim.Pose.GetJointMatrices(), false);

        for (unsigned long SmNr=0; SmNr<ModelDoc->GetSubmodels().Size(); SmNr++)
        {
            ModelDocumentT::SubmodelT* SM=ModelDoc->GetSubmodels()[SmNr];
            AnimPoseT&                 SmPose=SM->GetPose();

            SmPose.SetSuperPose(&Anim.Pose);
            RenderSkeleton(SM->GetSubmodel()->GetJoints(), SmPose.GetJointMatrices(), true);
            SmPose.SetSuperPose(NULL);
        }
    }


    // Render the triangle normals and/or the tangent-space axes of the model in this pose.
    if ((ScenePropGrid->m_Model_ShowTriangleNormals || ScenePropGrid->m_Model_ShowTangentSpace) && MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::AMBIENT)
    {
        static MatSys::MeshT TangentSpace(MatSys::MeshT::Lines);

        TangentSpace.Vertices.Overwrite();

        for (unsigned long MeshNr=0; MeshNr<Model->GetMeshes().Size(); MeshNr++)
        {
            const CafuModelT::MeshT&    Mesh    =Model->GetMeshes()[MeshNr];
            const AnimPoseT::MeshInfoT& MeshInfo=Anim.Pose.GetMeshInfos()[MeshNr];

            for (unsigned long TriNr=0; TriNr<Mesh.Triangles.Size(); TriNr++)
            {
                const CafuModelT::MeshT::TriangleT& Tri=Mesh.Triangles[TriNr];
                Vector3fT Center;

                for (unsigned int i=0; i<3; i++)
                {
                    const AnimPoseT::MeshInfoT::VertexT& VertexInfo=MeshInfo.Vertices[Tri.VertexIdx[i]];

                    Center+=VertexInfo.Pos;

                    if (ScenePropGrid->m_Model_ShowTangentSpace)
                    {
                        // Render the tangent-space axes at each vertex.
                        for (unsigned int AxisNr=0; AxisNr<3; AxisNr++)
                        {
                            Vector3fT Axis; Axis[AxisNr]=1.0f;

                            TangentSpace.Vertices.PushBackEmpty(2);
                            MatSys::MeshT::VertexT& V1=TangentSpace.Vertices[TangentSpace.Vertices.Size()-2];
                            MatSys::MeshT::VertexT& V2=TangentSpace.Vertices[TangentSpace.Vertices.Size()-1];

                            V1.SetOrigin(VertexInfo.Pos);
                            V1.SetColor(Axis.x, Axis.y, Axis.z);

                            const Vector3fT Vec=(AxisNr==0) ? VertexInfo.Normal :
                                                (AxisNr==1) ? VertexInfo.Tangent :
                                                              VertexInfo.BiNormal;

                            V2.SetOrigin(VertexInfo.Pos + Vec);
                            V2.SetColor(Axis.x, Axis.y, Axis.z);
                        }
                    }
                }

                // Render the triangle's normal vector.
                if (ScenePropGrid->m_Model_ShowTriangleNormals)
                {
                    Center/=3.0f;

                    TangentSpace.Vertices.PushBackEmpty(2);
                    MatSys::MeshT::VertexT& V1=TangentSpace.Vertices[TangentSpace.Vertices.Size()-2];
                    MatSys::MeshT::VertexT& V2=TangentSpace.Vertices[TangentSpace.Vertices.Size()-1];

                    V1.SetOrigin(Center);
                    V1.SetColor(0.5f, Tri.Polarity ? 0 : 0.5f, 0);

                    V2.SetOrigin(Center + MeshInfo.Triangles[TriNr].Normal);
                    V2.SetColor(0.5f, Tri.Polarity ? 0 : 0.5f, 0);
                }
            }
        }

        MatSys::Renderer->SetCurrentMaterial(m_Renderer.GetRMatWireframe());
        MatSys::Renderer->RenderMesh(TangentSpace);
    }


    // Render the GUI fixtures.
    if (MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::AMBIENT)
    {
        for (unsigned long GFNr=0; GFNr<Model->GetGuiFixtures().Size(); GFNr++)
        {
            const CafuModelT::GuiFixtureT& GF=Model->GetGuiFixtures()[GFNr];
            Vector3fT                      Points[3];
            unsigned int                   PointsOK=0;

            for (unsigned int PointNr=0; PointNr<3; PointNr++)
            {
                if (!Model->IsMeshNrOK  (GF, PointNr)) continue;
                if (!Model->IsVertexNrOK(GF, PointNr)) continue;

                Points[PointNr]=Anim.Pose.GetMeshInfos()[GF.Points[PointNr].MeshNr].Vertices[GF.Points[PointNr].VertexNr].Pos;
                PointsOK|=(1 << PointNr);
            }

            for (unsigned int PointNr=0; PointNr<3; PointNr++)
            {
                static MatSys::MeshT Cross(MatSys::MeshT::Lines);

                if ((PointsOK & (1 << PointNr))==0) continue;
                Cross.Vertices.Overwrite();

                for (unsigned int i=0; i<3; i++)
                {
                    Vector3fT Axis; Axis[i]=1.0f;

                    Cross.Vertices.PushBackEmpty(2);
                    MatSys::MeshT::VertexT& V1=Cross.Vertices[Cross.Vertices.Size()-2];
                    MatSys::MeshT::VertexT& V2=Cross.Vertices[Cross.Vertices.Size()-1];

                    V1.SetOrigin(Points[PointNr]-Axis); V1.SetColor(Axis.x+0.5f, Axis.y+0.5f, Axis.z+0.5f);
                    V2.SetOrigin(Points[PointNr]+Axis); V2.SetColor(Axis.x+0.5f, Axis.y+0.5f, Axis.z+0.5f);
                }

                MatSys::Renderer->SetCurrentMaterial(m_Renderer.GetRMatWireframe());
                MatSys::Renderer->RenderMesh(Cross);
            }

            if (PointsOK==7)
            {
                const Vector3fT AxisX =(Points[1]-Points[0])*GF.Scale[0];
                const Vector3fT AxisY =(Points[2]-Points[0])*GF.Scale[1];
                const Vector3fT Origin=Points[0] + AxisX*GF.Trans[0] + AxisY*GF.Trans[1];
                const MatrixT   ModelToWorld=MatSys::Renderer->GetMatrix(MatSys::RendererI::MODEL_TO_WORLD);

                // It's pretty easy to derive this matrix geometrically, see my TechArchive note from 2006-08-22.
                MatrixT M(AxisX.x/640.0f, AxisY.x/480.0f, 0.0f, Origin.x,
                          AxisX.y/640.0f, AxisY.y/480.0f, 0.0f, Origin.y,
                          AxisX.z/640.0f, AxisY.z/480.0f, 0.0f, Origin.z,
                                    0.0f,           0.0f, 0.0f,     1.0f);

                MatSys::Renderer->SetMatrix(MatSys::RendererI::MODEL_TO_WORLD, ModelToWorld*M);
                ModelDoc->GetGui()->Render();
                MatSys::Renderer->SetMatrix(MatSys::RendererI::MODEL_TO_WORLD, ModelToWorld);
            }
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

    // Update the m_JointSelCache member.
    const ArrayT<unsigned int>& JointSel=m_Parent->GetModelDoc()->GetSelection(JOINT);

    for (unsigned long JNr=0; JNr<m_JointSelCache.Size(); JNr++)
        m_JointSelCache[JNr]=false;

    for (unsigned long JNr=0; JNr<JointSel.Size(); JNr++)
    {
        while (JointSel[JNr]>=m_JointSelCache.Size()) m_JointSelCache.PushBack(false);
        m_JointSelCache[JointSel[JNr]]=true;
    }


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

    // Update the z-position of the ground plane.
    if (ScenePropGrid->m_GroundPlane_Show)
    {
        MapBrushT* Ground = m_Parent->GetModelDoc()->GetGround();

        // If the model is skeleton only and has no meshes, the sequence BB may not be inited.
        const float DeltaZ = (ScenePropGrid->m_GroundPlane_AutoZ && m_Parent->GetModelDoc()->GetSequenceBB().IsInited()
            ? m_Parent->GetModelDoc()->GetSequenceBB().Min.z
            : ScenePropGrid->m_GroundPlane_PosZ) - Ground->GetBB().Max.z;

        if (fabs(DeltaZ) > 0.0001f)
            Ground->TrafoMove(Vector3fT(0, 0, DeltaZ), true /*LockTexCoords*/);
    }

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

    // Render the grid.
    if (ScenePropGrid->m_ShowGrid)
    {
        for (unsigned int PlaneNr=0; PlaneNr<3; PlaneNr++)
        {
            static MatSys::MeshT Mesh(MatSys::MeshT::Lines);

            const unsigned int NumLines=17;
            const float        Spacing =ScenePropGrid->m_GridSpacing;
            const float        MaxCoord=Spacing * ((NumLines-1)/2);

            if (Mesh.Vertices.Size()==0)
            {
                Mesh.Vertices.PushBackEmptyExact(NumLines*2);

                const float r=Options.Grid.ColorHighlight1.Red()   / 255.0f;
                const float g=Options.Grid.ColorHighlight1.Green() / 255.0f;
                const float b=Options.Grid.ColorHighlight1.Blue()  / 255.0f;

                for (unsigned int LineNr=0; LineNr<NumLines; LineNr++)
                {
                    Mesh.Vertices[LineNr*2  ].SetColor(r, g, b);
                    Mesh.Vertices[LineNr*2+1].SetColor(r, g, b);
                }
            }

            for (unsigned int DirNr=0; DirNr<2; DirNr++)
            {
                for (unsigned int LineNr=0; LineNr<NumLines; LineNr++)
                {
                    Vector3fT A;
                    Vector3fT B;

                    A[(PlaneNr+1+DirNr) % 3]=-MaxCoord;
                    A[(PlaneNr+2-DirNr) % 3]=Spacing*LineNr - MaxCoord;

                    B[(PlaneNr+1+DirNr) % 3]=MaxCoord;
                    B[(PlaneNr+2-DirNr) % 3]=Spacing*LineNr - MaxCoord;

                    Mesh.Vertices[LineNr*2  ].SetOrigin(A);
                    Mesh.Vertices[LineNr*2+1].SetOrigin(B);
                }

                MatSys::Renderer->SetCurrentMaterial(m_Renderer.GetRMatWireframe());
                MatSys::Renderer->RenderMesh(Mesh);
            }
        }
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
