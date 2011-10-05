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

#include "AnimPose.hpp"
#include "Model_cmdl.hpp"

#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/Pluecker.hpp"
#include "Math3D/Quaternion.hpp"


AnimPoseT::AnimPoseT(const CafuModelT& Model, int SequNr, float FrameNr)
    : m_Model(Model),
      m_SequNr(SequNr),
      m_Super(NULL),
      m_FrameNr(FrameNr),
      m_NeedsRecache(true),
      m_BoundingBox()
{
}


void AnimPoseT::SetSequNr(int SequNr)
{
    if (m_SequNr==SequNr) return;

    m_SequNr=SequNr;
    NormalizeInput();

    m_NeedsRecache=true;
}


void AnimPoseT::SetFrameNr(float FrameNr)
{
    if (m_FrameNr==FrameNr) return;

    m_FrameNr=FrameNr;
    NormalizeInput();

    m_NeedsRecache=true;
}


void AnimPoseT::SetSuper(const SuperT* Super)
{
    if (m_Super==Super) return;

    m_Super=Super;

    m_NeedsRecache=true;
}


void AnimPoseT::Advance(float Time, bool ForceLoop)
{
    // TODO: Beachte korrekte Wrap-Regeln für mit loopen und ohne.
    // TODO: Sollte in NormalizeInput() die m_FrameNr gegen das jeweilige Maximum begrenzt werden?
    // TODO: Loops (next vs. ForceLoop) richtig behandeln
    const ArrayT<CafuModelT::AnimT>& Anims=m_Model.GetAnims();

    if (m_SequNr<0 || m_SequNr>=int(Anims.Size())) { SetFrameNr(0.0f); return; }
    if (Anims[m_SequNr].Frames.Size()<=1) { SetFrameNr(0.0f); return; }

    const float NumFrames=float(Anims[m_SequNr].Frames.Size());

    float FrameNr=m_FrameNr + Time*Anims[m_SequNr].FPS;

    if (ForceLoop)
    {
        // Wrap the sequence (it's a looping (repeating) sequence, like idle, walk, ...).
        FrameNr=fmod(FrameNr, NumFrames);
        if (FrameNr<0.0f) FrameNr+=NumFrames;
    }
    else
    {
        // Clamp the sequence (it's a play-once (non-repeating) sequence, like dying).
        // On clamping, stop the sequence 1/100th sec before the end of the last frame.
        if (FrameNr>=NumFrames-1.0f) FrameNr=NumFrames-1.0f-0.01f;
        if (FrameNr<0.0f) FrameNr=0.0f;
    }

    SetFrameNr(FrameNr);
}


void AnimPoseT::Draw(int SkinNr, float /*LodDist*/) const
{
    Recache();

    // Do an early check whether the light and the sequence BBs intersect.
    switch (MatSys::Renderer->GetCurrentRenderAction())
    {
        case MatSys::RendererI::AMBIENT:
            break;

        case MatSys::RendererI::LIGHTING:
        case MatSys::RendererI::STENCILSHADOW:
        {
            const float                LightRadius=MatSys::Renderer->GetCurrentLightSourceRadius();
            const Vector3T<float>      LightBox(LightRadius, LightRadius, LightRadius);
            const Vector3T<float>      LightPosOLD(MatSys::Renderer->GetCurrentLightSourcePosition());
            const BoundingBox3T<float> LightBB(LightPosOLD+LightBox, LightPosOLD-LightBox);

            if (!LightBB.Intersects(m_BoundingBox)) return;
            break;
        }
    }

    switch (MatSys::Renderer->GetCurrentRenderAction())
    {
        case MatSys::RendererI::AMBIENT:
        {
            for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
            {
                MatSys::Renderer->SetCurrentMaterial(m_Model.GetRenderMaterial(MeshNr, SkinNr));
                MatSys::Renderer->RenderMesh(m_Draw_Meshes[MeshNr]);

#if 0
                // Render the tangent space axes for each vertex.
                static MaterialT SolidColorMaterial;
                SolidColorMaterial.UseMeshColors=true;

                static MatSys::MeshT TangentSpaceAxes(MatSys::MeshT::Lines);
                TangentSpaceAxes.Vertices.Overwrite();

                for (unsigned long VertexNr=0; VertexNr<m_Draw_Meshes[MeshNr].Vertices.Size(); VertexNr++)
                {
                    const float         scale=1.0f;
                    const Vector3fT     Orig =Vector3fT(m_Draw_Meshes[MeshNr].Vertices[VertexNr].Origin);
                    const Vector3fT     S_   =Vector3fT(m_Draw_Meshes[MeshNr].Vertices[VertexNr].Tangent);
                    const Vector3fT     T_   =Vector3fT(m_Draw_Meshes[MeshNr].Vertices[VertexNr].BiNormal);
                    const Vector3fT     N_   =Vector3fT(m_Draw_Meshes[MeshNr].Vertices[VertexNr].Normal);
                    const float         col_ =m_Meshes[MeshNr].Triangles[VertexNr / 3].Polarity ? 0.5f : 0.0f;
                    const unsigned long Ofs  =TangentSpaceAxes.Vertices.Size();

                    TangentSpaceAxes.Vertices.PushBackEmpty(6);

                    TangentSpaceAxes.Vertices[Ofs+0].SetOrigin(Orig);
                    TangentSpaceAxes.Vertices[Ofs+0].SetColor(1, col_, col_);
                    TangentSpaceAxes.Vertices[Ofs+1].SetOrigin(Orig+S_*scale);
                    TangentSpaceAxes.Vertices[Ofs+1].SetColor(1, col_, col_);

                    TangentSpaceAxes.Vertices[Ofs+2].SetOrigin(Orig);
                    TangentSpaceAxes.Vertices[Ofs+2].SetColor(col_, 1, col_);
                    TangentSpaceAxes.Vertices[Ofs+3].SetOrigin(Orig+T_*scale);
                    TangentSpaceAxes.Vertices[Ofs+3].SetColor(col_, 1, col_);

                    TangentSpaceAxes.Vertices[Ofs+4].SetOrigin(Orig);
                    TangentSpaceAxes.Vertices[Ofs+4].SetColor(col_, col_, 1);
                    TangentSpaceAxes.Vertices[Ofs+5].SetOrigin(Orig+N_*scale);
                    TangentSpaceAxes.Vertices[Ofs+5].SetColor(col_, col_, 1);
                }

                MatSys::RenderMaterialT* SolidColorRenderMat=MatSys::Renderer->RegisterMaterial(&SolidColorMaterial);

                MatSys::Renderer->SetCurrentMaterial(SolidColorRenderMat);
                MatSys::Renderer->RenderMesh(TangentSpaceAxes);

                MatSys::Renderer->FreeMaterial(SolidColorRenderMat);

                // FIXME! Rendering the stencil shadows uses the same material as the ambient pass does!
                // (The call to FreeMaterial() above implies that no material is being set, and thus without this line,
                //  no stencil shadows get rendered!)
                MatSys::Renderer->SetCurrentMaterial(m_Meshes[MeshNr].RenderMaterial);
#endif
            }
            break;
        }

        case MatSys::RendererI::LIGHTING:
        {
            for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
            {
                MatSys::Renderer->SetCurrentMaterial(m_Model.GetRenderMaterial(MeshNr, SkinNr));
                MatSys::Renderer->RenderMesh(m_Draw_Meshes[MeshNr]);
            }
            break;
        }

        case MatSys::RendererI::STENCILSHADOW:
        {
            const Vector3fT LightPos(MatSys::Renderer->GetCurrentLightSourcePosition());

            for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
            {
                const CafuModelT::MeshT& Mesh    =m_Model.GetMeshes()[MeshNr];
                const AnimPoseT::MeshT&  PoseMesh=m_Meshes[MeshNr];
                const MaterialT*         MeshMat =m_Model.GetMaterial(MeshNr, SkinNr);

                if (MeshMat==NULL || MeshMat->NoShadows) continue;

                static ArrayT<bool> TriangleIsFrontFacing;
                if (TriangleIsFrontFacing.Size()<Mesh.Triangles.Size())
                    TriangleIsFrontFacing.PushBackEmpty(Mesh.Triangles.Size()-TriangleIsFrontFacing.Size());

                for (unsigned long TriNr=0; TriNr<Mesh.Triangles.Size(); TriNr++)
                {
                    const CafuModelT::MeshT::TriangleT& Tri    =Mesh.Triangles[TriNr];
                    const AnimPoseT::MeshT::TriangleT&  PoseTri=PoseMesh.Triangles[TriNr];

                    const float Dot=(LightPos-PoseMesh.Vertices[Tri.VertexIdx[0]].Draw_Pos).dot(PoseTri.Draw_Normal);

                    TriangleIsFrontFacing[TriNr]=Dot>0;
                }


                // Note that we have to cull the following polygons wrt. the *VIEWER* (not the light source)!
                static MatSys::MeshT MeshSilhouette(MatSys::MeshT::Quads);  // The default winding order is "CW".
                MeshSilhouette.Vertices.Overwrite();

                for (unsigned long TriNr=0; TriNr<Mesh.Triangles.Size(); TriNr++)
                {
                    if (!TriangleIsFrontFacing[TriNr]) continue;

                    // This triangle is front-facing wrt. the light source.
                    const CafuModelT::MeshT::TriangleT& Tri=Mesh.Triangles[TriNr];

                    for (unsigned long EdgeNr=0; EdgeNr<3; EdgeNr++)
                    {
                        // If an edge has no (-1) or more than one (-2) neighbours, make it a silhouette edge.
                        const bool IsSilhouetteEdge=(Tri.NeighbIdx[EdgeNr]<0) ? true : !TriangleIsFrontFacing[Tri.NeighbIdx[EdgeNr]];

                        if (IsSilhouetteEdge)
                        {
                            // The neighbour at edge 'EdgeNr' is back-facing (or non-existant), so we have found a possible silhouette edge.
                            const unsigned long v1=EdgeNr;
                            const unsigned long v2=(EdgeNr+1) % 3;
                            const Vector3fT     LA=PoseMesh.Vertices[Tri.VertexIdx[v1]].Draw_Pos-LightPos;
                            const Vector3fT     LB=PoseMesh.Vertices[Tri.VertexIdx[v2]].Draw_Pos-LightPos;

                            MeshSilhouette.Vertices.PushBackEmpty(4);

                            const unsigned long MeshSize=MeshSilhouette.Vertices.Size();

                            MeshSilhouette.Vertices[MeshSize-4].SetOrigin(PoseMesh.Vertices[Tri.VertexIdx[v2]].Draw_Pos);
                            MeshSilhouette.Vertices[MeshSize-3].SetOrigin(PoseMesh.Vertices[Tri.VertexIdx[v1]].Draw_Pos);
                            MeshSilhouette.Vertices[MeshSize-2].SetOrigin(LA, 0.0);
                            MeshSilhouette.Vertices[MeshSize-1].SetOrigin(LB, 0.0);
                        }
                    }
                }

                MatSys::Renderer->RenderMesh(MeshSilhouette);


                static MatSys::MeshT MeshCaps(MatSys::MeshT::Triangles);    // The default winding order is "CW".
                MeshCaps.Vertices.Overwrite();

                for (unsigned long TriNr=0; TriNr<Mesh.Triangles.Size(); TriNr++)
                {
                    if (!TriangleIsFrontFacing[TriNr]) continue;

                    // This triangle is front-facing wrt. the light source.
                    const CafuModelT::MeshT::TriangleT& Tri=Mesh.Triangles[TriNr];

                    MeshCaps.Vertices.PushBackEmpty(6);

                    const unsigned long MeshSize=MeshCaps.Vertices.Size();

                    // Render the occluder (front-facing wrt. the light source).
                    const Vector3fT& A=PoseMesh.Vertices[Tri.VertexIdx[0]].Draw_Pos;
                    const Vector3fT& B=PoseMesh.Vertices[Tri.VertexIdx[1]].Draw_Pos;
                    const Vector3fT& C=PoseMesh.Vertices[Tri.VertexIdx[2]].Draw_Pos;

                    MeshCaps.Vertices[MeshSize-6].SetOrigin(A);
                    MeshCaps.Vertices[MeshSize-5].SetOrigin(B);
                    MeshCaps.Vertices[MeshSize-4].SetOrigin(C);

                    // Render the occluder (back-facing wrt. the light source).
                    const Vector3fT LA=A-LightPos;
                    const Vector3fT LB=B-LightPos;
                    const Vector3fT LC=C-LightPos;

                    MeshCaps.Vertices[MeshSize-3].SetOrigin(LC, 0.0);
                    MeshCaps.Vertices[MeshSize-2].SetOrigin(LB, 0.0);
                    MeshCaps.Vertices[MeshSize-1].SetOrigin(LA, 0.0);
                }

                MatSys::Renderer->RenderMesh(MeshCaps);
            }

            break;
        }
    }
}


bool AnimPoseT::TraceRay(int SkinNr, const Vector3fT& RayOrigin, const Vector3fT& RayDir, ModelT::TraceResultT& Result) const
{
    float Fraction=0.0f;

    Recache();

    // If we miss the bounding-box, then we miss all the triangles as well.
    if (!GetBB().TraceRay(RayOrigin, RayDir, Fraction)) return false;

    for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
    {
        const CafuModelT::MeshT& Mesh    =m_Model.GetMeshes()[MeshNr];
        const MeshT&             DrawMesh=m_Meshes[MeshNr];
        const MaterialT*         MeshMat =m_Model.GetMaterial(MeshNr, SkinNr);

        // If the ClipFlags don't match the ClipMask, this polygon doesn't interfere with the trace.
        if (!MeshMat) continue;
        // if ((MeshMat->ClipFlags & ClipMask)==0) continue;

        for (unsigned long TriNr=0; TriNr<Mesh.Triangles.Size(); TriNr++)
        {
            // This code is a modification of the code in CollisionModelStaticT::PolygonT::TraceRay(),
            // see there for details and additional information.
            using namespace cf::math;
            const CafuModelT::MeshT::TriangleT& Tri    =Mesh.Triangles[TriNr];
            const AnimPoseT::MeshT::TriangleT&  DrawTri=DrawMesh.Triangles[TriNr];

            const Vector3fT& A=DrawMesh.Vertices[Tri.VertexIdx[0]].Draw_Pos;
            const Vector3fT& B=DrawMesh.Vertices[Tri.VertexIdx[1]].Draw_Pos;
            const Vector3fT& C=DrawMesh.Vertices[Tri.VertexIdx[2]].Draw_Pos;

            const PlueckerfT R=PlueckerfT::CreateFromRay(RayOrigin, RayDir);

            // We use Pluecker coordinates for the orientation tests.
            // Note that Christer Ericson has shown in his blog at http://realtimecollisiondetection.net/blog/?p=13
            // that scalar triple products (Spatprodukte) are equivalent and could be used as well.  ;-)
            if (!MeshMat->TwoSided)
            {
                if (R*PlueckerfT::CreateFromLine(A, B) >= 0) continue;
                if (R*PlueckerfT::CreateFromLine(B, C) >= 0) continue;
                if (R*PlueckerfT::CreateFromLine(C, A) >= 0) continue;
            }
            else
            {
                int Count=0;

                // Should not change Count if result == 0...
                Count+=(R*PlueckerfT::CreateFromLine(A, B) >= 0) ? -1 : 1;
                Count+=(R*PlueckerfT::CreateFromLine(B, C) >= 0) ? -1 : 1;
                Count+=(R*PlueckerfT::CreateFromLine(C, A) >= 0) ? -1 : 1;

                if (Count!=-3 && Count!=3) continue;
            }

            // The "aperture" test passed, now compute the fraction at which RayDir intersects the triangle plane.
            const float Nenner=dot(DrawTri.Draw_Normal, RayDir);

            if (Nenner==0) continue;                                // If Nenner==0, then RayDir is parallel to the triangle plane (no intersection).
            assert(MeshMat->TwoSided || Nenner<0);                  // If the material is single sided, then Nenner<0, a consequence of the Pluecker tests above.

            const float Dist=dot(DrawTri.Draw_Normal, RayOrigin-A); // The distance of RayOrigin to the triangle plane.
            const float F   =-(Dist-0.03125f)/Nenner;

            // The intersection is only valid in the positive direction of RayDir.
            if (F<0) continue;

            // Hit the triangle!
            Result.Fraction=F;
            Result.Normal  =(Nenner<0) ? DrawTri.Draw_Normal : -DrawTri.Draw_Normal;    // Handle two-sided materials properly.
            Result.Material=MeshMat;
            Result.MeshNr  =MeshNr;
            Result.TriNr   =TriNr;
            return true;
        }
    }

    return false;
}


unsigned int AnimPoseT::FindClosestVertex(unsigned int MeshNr, unsigned int TriNr, const Vector3fT& P) const
{
    unsigned int BestVertexNr=0;
    float        BestDist    =0.0f;

    Recache();

    for (unsigned int i=0; i<3; i++)
    {
        const unsigned int VertexNr=m_Model.GetMeshes()[MeshNr].Triangles[TriNr].VertexIdx[i];
        const Vector3fT&   DrawPos =m_Meshes[MeshNr].Vertices[VertexNr].Draw_Pos;
        const float        Dist    =length(DrawPos-P);

        if (i==0 || Dist<BestDist)
        {
            BestDist    =Dist;
            BestVertexNr=VertexNr;
        }
    }

    return BestVertexNr;
}


const Vector3fT& AnimPoseT::GetVertexPos(unsigned int MeshNr, unsigned int VertexNr) const
{
    Recache();

    return m_Meshes[MeshNr].Vertices[VertexNr].Draw_Pos;
}


const ArrayT<MatrixT>& AnimPoseT::GetJointMatrices() const
{
    Recache();

    return m_JointMatrices;
}


const ArrayT<MatSys::MeshT>& AnimPoseT::GetDrawMeshes() const
{
    Recache();

    return m_Draw_Meshes;
}


const BoundingBox3fT& AnimPoseT::GetBB() const
{
    Recache();

    return m_BoundingBox;
}


void AnimPoseT::NormalizeInput()
{
    const ArrayT<CafuModelT::AnimT>& Anims=m_Model.GetAnims();

    // m_SequNr==-1 means "use the bind pose from the model file only (no anim)".
    if (m_SequNr < -1) m_SequNr = -1;
    if (m_SequNr >= int(Anims.Size())) m_SequNr = -1;
    if (m_SequNr != -1 && (Anims[m_SequNr].FPS<0.0 || Anims[m_SequNr].Frames.Size()==0)) m_SequNr = -1;
    if (m_SequNr == -1) m_FrameNr = 0.0f;
}


// The array dimensions are normally only set once in the Cafu Engine,
// but possibly set multiply when the model is edited in the Model Editor.
void AnimPoseT::SyncDimensions() const
{
    if (m_JointMatrices.Size()!=m_Model.GetJoints().Size())
    {
        m_JointMatrices.Overwrite();
        m_JointMatrices.PushBackEmptyExact(m_Model.GetJoints().Size());
    }

    if (m_Meshes.Size()!=m_Model.GetMeshes().Size())
    {
        m_Meshes.Overwrite();
        m_Meshes.PushBackEmptyExact(m_Model.GetMeshes().Size());
    }

    if (m_Draw_Meshes.Size()!=m_Model.GetMeshes().Size())
    {
        m_Draw_Meshes.Overwrite();
        m_Draw_Meshes.PushBackEmptyExact(m_Model.GetMeshes().Size());
    }


    for (unsigned long MeshNr=0; MeshNr<m_Model.GetMeshes().Size(); MeshNr++)
    {
        const CafuModelT::MeshT& Mesh=m_Model.GetMeshes()[MeshNr];

        if (m_Meshes[MeshNr].Vertices.Size()!=Mesh.Vertices.Size())
        {
            m_Meshes[MeshNr].Vertices.Overwrite();
            m_Meshes[MeshNr].Vertices.PushBackEmptyExact(Mesh.Vertices.Size());
        }

        if (m_Meshes[MeshNr].Triangles.Size()!=Mesh.Triangles.Size())
        {
            m_Meshes[MeshNr].Triangles.Overwrite();
            m_Meshes[MeshNr].Triangles.PushBackEmptyExact(Mesh.Triangles.Size());
        }


        m_Draw_Meshes[MeshNr].Type   =MatSys::MeshT::Triangles;
     // m_Draw_Meshes[MeshNr].Winding=MatSys::MeshT::CW;    // CW is the default.

        if (m_Draw_Meshes[MeshNr].Vertices.Size()!=Mesh.Triangles.Size()*3)
        {
            m_Draw_Meshes[MeshNr].Vertices.Overwrite();
            m_Draw_Meshes[MeshNr].Vertices.PushBackEmptyExact(Mesh.Triangles.Size()*3);
        }
    }
}


namespace
{
    Vector3fT myNormalize(const Vector3fT& A)
    {
        const float Length=length(A);

        return (Length>0.000001f) ? A.GetScaled(1.0f/Length) : Vector3fT();
    }
}


void AnimPoseT::UpdateData() const
{
    const int     SequenceNr=m_SequNr;
    const float   FrameNr   =m_FrameNr;
    const SuperT* Super     =m_Super;
    const bool m_UseGivenTangentSpace=m_Model.GetUseGivenTS();

    const ArrayT<CafuModelT::JointT>& m_Joints=m_Model.GetJoints();
    const ArrayT<CafuModelT::AnimT>&  m_Anims =m_Model.GetAnims();
    typedef CafuModelT::JointT JointT;
    typedef CafuModelT::AnimT AnimT;


    // **************************************************************************************************************
    //  Obtain a joints (bone) hierarchy for the desired frame FrameNr of the desired animation sequence SequenceNr.
    //  The result will be a transformation matrix for each joint (bone).
    // **************************************************************************************************************

    if (SequenceNr==-1)
    {
        // Don't animate, just use the bind pose defined in the model file.
        for (unsigned long JointNr=0; JointNr<m_Joints.Size(); JointNr++)
        {
            if (Super && Super->HasMatrix(JointNr))
            {
                m_JointMatrices[JointNr]=Super->GetMatrix(JointNr);
                continue;
            }

            const JointT& J=m_Joints[JointNr];
            const MatrixT RelMatrix(J.Pos, cf::math::QuaternionfT::FromXYZ(J.Qtr), J.Scale);

            m_JointMatrices[JointNr]=(J.Parent==-1) ? RelMatrix : m_JointMatrices[J.Parent]*RelMatrix;
        }
    }
    else
    {
        // SequenceNr is a valid index into m_Anims, so use that.
        const AnimT& Anim=m_Anims[SequenceNr];
        const int    Frame_0=int(FrameNr);                                          // If FrameNr == 17.83, then Frame_0 == 17
        const float  Frame_f=FrameNr-Frame_0;                                       //                           Frame_f ==  0.83
        const int    Frame_1=(Frame_0+1>=int(Anim.Frames.Size())) ? 0 : Frame_0+1;  //                           Frame_1 == 18

        for (unsigned long JointNr=0; JointNr<m_Joints.Size(); JointNr++)
        {
            if (Super && Super->HasMatrix(JointNr))
            {
                m_JointMatrices[JointNr]=Super->GetMatrix(JointNr);
                continue;
            }

            const AnimT::AnimJointT& AJ=Anim.AnimJoints[JointNr];
            Vector3fT                Data_0[3]={ AJ.DefaultPos, AJ.DefaultQtr, AJ.DefaultScale };
            Vector3fT                Data_1[3]={ AJ.DefaultPos, AJ.DefaultQtr, AJ.DefaultScale };

            // Determine the position, quaternion and scale for Frame_0 and Frame_1.
            unsigned int FlagCount=0;

            for (int i=0; i<9; i++)
            {
                if ((AJ.Flags >> i) & 1)
                {
                    Data_0[i/3][i % 3]=Anim.Frames[Frame_0].AnimData[AJ.FirstDataIdx+FlagCount];
                    Data_1[i/3][i % 3]=Anim.Frames[Frame_1].AnimData[AJ.FirstDataIdx+FlagCount];

                    FlagCount++;
                }
            }

            // Interpolate the position and quaternion according to the fraction Frame_f.
            const Vector3fT              Pos  =Data_0[0]*(1.0f-Frame_f) + Data_1[0]*Frame_f;
            const cf::math::QuaternionfT Quat =slerp(cf::math::QuaternionfT::FromXYZ(Data_0[1]), cf::math::QuaternionfT::FromXYZ(Data_1[1]), Frame_f);
            const Vector3fT              Scale=Data_0[2]*(1.0f-Frame_f) + Data_1[2]*Frame_f;

            // Compute the matrix that is relative to the parent bone, and finally obtain the absolute matrix for that bone!
            const MatrixT RelMatrix(Pos, Quat, Scale);
            const JointT& J=m_Joints[JointNr];

            m_JointMatrices[JointNr]=(J.Parent==-1) ? RelMatrix : m_JointMatrices[J.Parent]*RelMatrix;
        }
    }


    // *******************************************************************************************************************
    //  The JointMatrices represent now the pose of the model at the desired frame number of the desired sequence number.
    //  For all meshes do now compute the vertices according to their weights.
    // *******************************************************************************************************************

    for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
    {
        const CafuModelT::MeshT& Mesh=m_Model.GetMeshes()[MeshNr];
        MeshT& DrawMesh=m_Meshes[MeshNr];

        for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
        {
            const CafuModelT::MeshT::VertexT& Vertex=Mesh.Vertices[VertexNr];
            MeshT::VertexT& DrawVertex=DrawMesh.Vertices[VertexNr];

            if (Vertex.GeoDups.Size()>0 && Vertex.GeoDups[0]<VertexNr)
            {
                // This vertex has a geometrically identical duplicate that has already been computed.
                // Therefore, don't bother to recompute the same position again, just copy it from the duplicate.
                DrawVertex.Draw_Pos=DrawMesh.Vertices[Vertex.GeoDups[0]].Draw_Pos;
                continue;
            }

            if (Vertex.NumWeights==1)
            {
                const CafuModelT::MeshT::WeightT& Weight=Mesh.Weights[Vertex.FirstWeightIdx];

                DrawVertex.Draw_Pos=m_JointMatrices[Weight.JointIdx].Mul_xyz1(Weight.Pos);
            }
            else
            {
                DrawVertex.Draw_Pos=Vector3fT(0.0f, 0.0f, 0.0f);

                for (unsigned int WeightNr=0; WeightNr<Vertex.NumWeights; WeightNr++)
                {
                    const CafuModelT::MeshT::WeightT& Weight=Mesh.Weights[Vertex.FirstWeightIdx+WeightNr];

                    DrawVertex.Draw_Pos+=m_JointMatrices[Weight.JointIdx].Mul_xyz1(Weight.Pos) * Weight.Weight;
                }
            }
        }
    }


    // *******************************************************************************************
    //  Compute the tangent-space basis vectors for all triangles and all vertices.
    //  This is done by first computing the per-triangle axes and then having them enter
    //  the relevant per-vertex averages as required (taking mirror corrections into account).
    //  The per-triangle normal vectors are also kept for stencil shadow silhoutte determination.
    // *******************************************************************************************

    if (m_UseGivenTangentSpace)
    {
        assert(m_Anims.Size()==0);  // It doesn't make sense to have statically given tangent-space axes with *animated* geometry...
        goto DoneComputingTS;
    }

    // For all vertices, zero the tangent-space vectors for the subsequent average accumulation.
    for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
    {
        MeshT& Mesh=m_Meshes[MeshNr];

        for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
        {
            MeshT::VertexT& Vertex=Mesh.Vertices[VertexNr];

            Vertex.Draw_Normal  =Vector3fT(0, 0, 0);
            Vertex.Draw_Tangent =Vector3fT(0, 0, 0);
            Vertex.Draw_BiNormal=Vector3fT(0, 0, 0);
        }
    }

    // Compute the per-triangle tangent-space axes and distribute them over the relevant vertices appropriately.
    for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
    {
        const CafuModelT::MeshT& Mesh=m_Model.GetMeshes()[MeshNr];
        MeshT& DrawMesh=m_Meshes[MeshNr];

        for (unsigned long TriangleNr=0; TriangleNr<Mesh.Triangles.Size(); TriangleNr++)
        {
            const CafuModelT::MeshT::TriangleT& Tri=Mesh.Triangles[TriangleNr];
            const CafuModelT::MeshT::VertexT&   V_0=Mesh.Vertices[Tri.VertexIdx[0]];
            const CafuModelT::MeshT::VertexT&   V_1=Mesh.Vertices[Tri.VertexIdx[1]];
            const CafuModelT::MeshT::VertexT&   V_2=Mesh.Vertices[Tri.VertexIdx[2]];

            MeshT::TriangleT& DrawTri=DrawMesh.Triangles[TriangleNr];
            MeshT::VertexT&   DrawV_0=DrawMesh.Vertices[Tri.VertexIdx[0]];
            MeshT::VertexT&   DrawV_1=DrawMesh.Vertices[Tri.VertexIdx[1]];
            MeshT::VertexT&   DrawV_2=DrawMesh.Vertices[Tri.VertexIdx[2]];

            const Vector3fT Edge01=DrawV_1.Draw_Pos-DrawV_0.Draw_Pos;
            const Vector3fT Edge02=DrawV_2.Draw_Pos-DrawV_0.Draw_Pos;

            // Triangles are ordered CW for md5 models and CCW for ase models, so we write
            // Normal=VectorCross(Edge02, Edge01) for md5 models and Normal=VectorCross(Edge01, Edge02) for ase models.
            DrawTri.Draw_Normal=myNormalize(Edge02.cross(Edge01));

            // Understanding what's going on here is easy. The key statement is
            // "The tangent vector is parallel to the direction of increasing S on a parametric surface."
            // First, there is a short explanation in "The Cg Tutorial", chapter 8.
            // Second, I have drawn a simple figure that leads to a simple 2x2 system of Gaussian equations, see my TechArchive.
            const Vector3fT uv01=Vector3fT(V_1.u, V_1.v, 0.0f)-Vector3fT(V_0.u, V_0.v, 0.0f);
            const Vector3fT uv02=Vector3fT(V_2.u, V_2.v, 0.0f)-Vector3fT(V_0.u, V_0.v, 0.0f);
            const float     f   =uv01.x*uv02.y-uv01.y*uv02.x>0.0 ? 1.0f : -1.0f;

            const Vector3fT Tri_Draw_Tangent =myNormalize(Edge02.GetScaled(-uv01.y*f) + Edge01.GetScaled(uv02.y*f));
            const Vector3fT Tri_Draw_BiNormal=myNormalize(Edge02.GetScaled( uv01.x*f) - Edge01.GetScaled(uv02.x*f));


            // Distribute the per-triangle tangent-space over the affected vertices.
#if 1
            const float Pi=3.14159265358979323846f;

            const float c0=dot(myNormalize(Edge01), myNormalize(Edge02));
            const float c1=dot(myNormalize(Edge01), myNormalize(DrawV_1.Draw_Pos-DrawV_2.Draw_Pos));

            const float w0=(c0>=1.0f) ? 0.0f : ( (c0<=-1.0f) ? Pi : acos(c0) );
            const float w1=(c1>=1.0f) ? 0.0f : ( (c1<=-1.0f) ? Pi : acos(c1) );

            const float TriWeight[3]={ w0, w1, Pi-TriWeight[0]-TriWeight[1] };
#else
            const float TriWeight[3]={ 1.0f, 1.0f, 1.0f };
#endif

            for (int i=0; i<3; i++)
            {
                const CafuModelT::MeshT::VertexT& Vertex=Mesh.Vertices[Tri.VertexIdx[i]];
                MeshT::VertexT& DrawVertex=DrawMesh.Vertices[Tri.VertexIdx[i]];

                assert(Tri.Polarity==Vertex.Polarity);

                DrawVertex.Draw_Normal  +=DrawTri.Draw_Normal*TriWeight[i];
                DrawVertex.Draw_Tangent +=Tri_Draw_Tangent*TriWeight[i];
                DrawVertex.Draw_BiNormal+=Tri_Draw_BiNormal*TriWeight[i];

                for (unsigned long DupNr=0; DupNr<Vertex.GeoDups.Size(); DupNr++)
                {
                    const CafuModelT::MeshT::VertexT& DupVertex=Mesh.Vertices[Vertex.GeoDups[DupNr]];
                    MeshT::VertexT&                   DrawDupVertex=DrawMesh.Vertices[Vertex.GeoDups[DupNr]];

                    DrawDupVertex.Draw_Normal  +=DrawTri.Draw_Normal*TriWeight[i];
                    DrawDupVertex.Draw_Tangent +=Tri_Draw_Tangent*(Tri.Polarity==DupVertex.Polarity ? TriWeight[i] : -TriWeight[i]);
                    DrawDupVertex.Draw_BiNormal+=Tri_Draw_BiNormal*TriWeight[i];
                }
            }
        }
    }

    // Finally normalize the per-vertex tangent-space axes; this is quasi the "division" in the average computations.
    for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
    {
        MeshT& Mesh=m_Meshes[MeshNr];

        for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
        {
            MeshT::VertexT& Vertex=Mesh.Vertices[VertexNr];

            // Normalize the tangent-space axes.
            Vertex.Draw_Normal  =myNormalize(Vertex.Draw_Normal  );
            Vertex.Draw_Tangent =myNormalize(Vertex.Draw_Tangent );
            Vertex.Draw_BiNormal=myNormalize(Vertex.Draw_BiNormal);
        }
    }

    DoneComputingTS:


    // ***************************************************************************************************************
    //  Construct explicit MatSys::MeshT meshes now.
    //  Note that this is very inefficient - we REALLY should work with index arrays! (and/or vertex buffer objects!)
    // ***************************************************************************************************************

    for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
    {
        const CafuModelT::MeshT& Mesh    =m_Model.GetMeshes()[MeshNr];
        const AnimPoseT::MeshT&  DrawMesh=m_Meshes[MeshNr];

        for (unsigned long TriNr=0; TriNr<Mesh.Triangles.Size(); TriNr++)
        {
            for (unsigned long i=0; i<3; i++)
            {
                unsigned long VertexIdx=Mesh.Triangles[TriNr].VertexIdx[i];
                const MeshT::VertexT& DrawVertex=DrawMesh.Vertices[VertexIdx];

                m_Draw_Meshes[MeshNr].Vertices[TriNr*3+i].SetOrigin(DrawVertex.Draw_Pos.x, DrawVertex.Draw_Pos.y, DrawVertex.Draw_Pos.z);
                m_Draw_Meshes[MeshNr].Vertices[TriNr*3+i].SetTextureCoord(Mesh.Vertices[VertexIdx].u, Mesh.Vertices[VertexIdx].v);
                m_Draw_Meshes[MeshNr].Vertices[TriNr*3+i].SetNormal  (DrawVertex.Draw_Normal.x,   DrawVertex.Draw_Normal.y,   DrawVertex.Draw_Normal.z  );
                m_Draw_Meshes[MeshNr].Vertices[TriNr*3+i].SetTangent (DrawVertex.Draw_Tangent.x,  DrawVertex.Draw_Tangent.y,  DrawVertex.Draw_Tangent.z );
                m_Draw_Meshes[MeshNr].Vertices[TriNr*3+i].SetBiNormal(DrawVertex.Draw_BiNormal.x, DrawVertex.Draw_BiNormal.y, DrawVertex.Draw_BiNormal.z);
            }
        }
    }


    // *******************************************************************************************
    //  Update the bounding-box for this pose.
    // *******************************************************************************************

    m_BoundingBox=BoundingBox3fT();

    for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
    {
        const MeshT& DrawMesh=m_Meshes[MeshNr];

        for (unsigned long VertexNr=0; VertexNr<DrawMesh.Vertices.Size(); VertexNr++)
            m_BoundingBox+=DrawMesh.Vertices[VertexNr].Draw_Pos;
    }
}


void AnimPoseT::Recache() const
{
    if (!m_NeedsRecache && !m_Super) return;

    SyncDimensions();
    UpdateData();

    // SyncDimensions();
    // UpdateJointMatrices();
    //
    // if (UseGivenTangentSpace)
    //     // Take it from the model
    //     x();
    // else
    //     // Compute it ourselves
    //     y();

    m_NeedsRecache=false;
}
