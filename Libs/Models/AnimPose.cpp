/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "AnimPose.hpp"
#include "AnimExpr.hpp"
#include "Model_cmdl.hpp"

#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/Pluecker.hpp"
#include "Math3D/Quaternion.hpp"


AnimPoseT::AnimPoseT(const CafuModelT& Model, IntrusivePtrT<AnimExpressionT> AnimExpr)
    : m_Model(Model),
      m_AnimExpr(AnimExpr),
      m_SuperPose(NULL),
      m_DlodPose(NULL),
      m_CachedAE(NULL),
      m_BoundingBox()
{
    if (m_Model.GetDlodModel())
    {
        // Recursively create the chain of dlod poses matching the chain of dlod models.
        // Note that all dlod poses share the *SAME* AnimExpr as the top-most pose!
        m_DlodPose = new AnimPoseT(*m_Model.GetDlodModel(), m_AnimExpr);
    }
}


AnimPoseT::~AnimPoseT()
{
    delete m_DlodPose;
}


void AnimPoseT::SetAnimExpr(IntrusivePtrT<AnimExpressionT> AnimExpr)
{
    for (AnimPoseT* Pose=this; Pose; Pose=Pose->m_DlodPose)
    {
        Pose->m_AnimExpr = AnimExpr;
    }
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

    if (m_MeshInfos.Size()!=m_Model.GetMeshes().Size())
    {
        m_MeshInfos.Overwrite();
        m_MeshInfos.PushBackEmptyExact(m_Model.GetMeshes().Size());
    }

    if (m_Draw_Meshes.Size()!=m_Model.GetMeshes().Size())
    {
        m_Draw_Meshes.Overwrite();
        m_Draw_Meshes.PushBackEmptyExact(m_Model.GetMeshes().Size());
    }


    for (unsigned long MeshNr=0; MeshNr<m_Model.GetMeshes().Size(); MeshNr++)
    {
        const CafuModelT::MeshT& Mesh=m_Model.GetMeshes()[MeshNr];

        if (m_MeshInfos[MeshNr].Vertices.Size()!=Mesh.Vertices.Size())
        {
            m_MeshInfos[MeshNr].Vertices.Overwrite();
            m_MeshInfos[MeshNr].Vertices.PushBackEmptyExact(Mesh.Vertices.Size());
        }

        if (m_MeshInfos[MeshNr].Triangles.Size()!=Mesh.Triangles.Size())
        {
            m_MeshInfos[MeshNr].Triangles.Overwrite();
            m_MeshInfos[MeshNr].Triangles.PushBackEmptyExact(Mesh.Triangles.Size());
        }


        m_Draw_Meshes[MeshNr].Type   =MatSys::MeshT::Triangles;
     // m_Draw_Meshes[MeshNr].Winding=MatSys::MeshT::CW;    // CW is the default.

        unsigned long NumDrawTris=0;
        for (unsigned long TriNr=0; TriNr<Mesh.Triangles.Size(); TriNr++)
            if (!Mesh.Triangles[TriNr].SkipDraw)
                NumDrawTris++;

        if (m_Draw_Meshes[MeshNr].Vertices.Size()!=NumDrawTris*3)
        {
            m_Draw_Meshes[MeshNr].Vertices.Overwrite();
            m_Draw_Meshes[MeshNr].Vertices.PushBackEmptyExact(NumDrawTris*3);
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


/// Obtain a joints (bone) hierarchy for the desired frame m_FrameNr of the desired animation sequence m_SequNr.
/// The result will be a transformation matrix for each joint (bone).
void AnimPoseT::UpdateJointMatrices() const
{
    typedef CafuModelT::JointT JointT;

    const ArrayT<JointT>& Joints=m_Model.GetJoints();

    for (unsigned long JointNr=0; JointNr<Joints.Size(); JointNr++)
    {
        if (m_SuperPose)
        {
            const MatrixT* SuperMat=m_SuperPose->GetJointMatrix(Joints[JointNr].Name);

            if (SuperMat)
            {
                m_JointMatrices[JointNr]=*SuperMat;
                continue;
            }
        }

        float                  Weight;
        Vector3fT              Pos;
        cf::math::QuaternionfT Quat;
        Vector3fT              Scale;

        m_AnimExpr->GetData(JointNr, Weight, Pos, Quat, Scale);

        // Compute the matrix that is relative to the parent bone, and finally obtain the absolute matrix for that bone!
        const MatrixT RelMatrix(Pos, Quat, Scale);
        const JointT& J=Joints[JointNr];

        // assert(Weight==1.0f);
        m_JointMatrices[JointNr]=(J.Parent==-1) ? RelMatrix : m_JointMatrices[J.Parent]*RelMatrix;
    }
}


/// The m_JointMatrices represent now the pose of the model at the desired frame number of the desired sequence number.
/// For all meshes do now compute the vertices according to their weights.
void AnimPoseT::UpdateVertexPositions() const
{
    typedef CafuModelT::MeshT MeshT;

    const ArrayT<MeshT>& Meshes=m_Model.GetMeshes();
    m_BoundingBox=BoundingBox3fT();

    for (unsigned long MeshNr=0; MeshNr<Meshes.Size(); MeshNr++)
    {
        const MeshT& Mesh    =Meshes[MeshNr];
        MeshInfoT&   MeshInfo=m_MeshInfos[MeshNr];

        for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
        {
            const MeshT::VertexT& Vertex    =Mesh.Vertices[VertexNr];
            MeshInfoT::VertexT&   VertexInfo=MeshInfo.Vertices[VertexNr];

            if (Vertex.GeoDups.Size()>0 && Vertex.GeoDups[0]<VertexNr)
            {
                // This vertex has a geometrically identical duplicate that has already been computed.
                // Therefore, don't bother to recompute the same position again, just copy it from the duplicate.
                VertexInfo.Pos=MeshInfo.Vertices[Vertex.GeoDups[0]].Pos;
                continue;
            }

            if (Vertex.NumWeights==1)
            {
                const MeshT::WeightT& Weight=Mesh.Weights[Vertex.FirstWeightIdx];

                VertexInfo.Pos=m_JointMatrices[Weight.JointIdx].Mul_xyz1(Weight.Pos);
            }
            else
            {
                VertexInfo.Pos=Vector3fT(0.0f, 0.0f, 0.0f);

                for (unsigned int WeightNr=0; WeightNr<Vertex.NumWeights; WeightNr++)
                {
                    const MeshT::WeightT& Weight=Mesh.Weights[Vertex.FirstWeightIdx+WeightNr];

                    VertexInfo.Pos+=m_JointMatrices[Weight.JointIdx].Mul_xyz1(Weight.Pos) * Weight.Weight;
                }
            }

            m_BoundingBox+=MeshInfo.Vertices[VertexNr].Pos;
        }
    }
}


void AnimPoseT::UpdateTangentSpaceHard(unsigned long MeshNr) const
{
    const CafuModelT::MeshT& Mesh     =m_Model.GetMeshes()[MeshNr];
    MeshInfoT&               MeshInfo =m_MeshInfos[MeshNr];
    unsigned long            DrawTriNr=0;


    // *******************************************************************************************
    //  Compute the tangent-space basis vectors for all triangles and all vertices.
    //  This is done by first computing the per-triangle axes and then having them enter
    //  the relevant per-vertex averages as required (taking mirror corrections into account).
    //  The per-triangle normal vectors are also kept for stencil shadow silhoutte determination.
    // *******************************************************************************************

    // For all vertices, zero the tangent-space vectors for the subsequent average accumulation.
    for (unsigned long VertexNr=0; VertexNr<MeshInfo.Vertices.Size(); VertexNr++)
    {
        MeshInfoT::VertexT& VertexInfo=MeshInfo.Vertices[VertexNr];

        VertexInfo.Normal  =Vector3fT(0, 0, 0);
        VertexInfo.Tangent =Vector3fT(0, 0, 0);
        VertexInfo.BiNormal=Vector3fT(0, 0, 0);
    }

    // Compute the per-triangle tangent-space axes and distribute them over the relevant vertices appropriately.
    for (unsigned long TriangleNr=0; TriangleNr<Mesh.Triangles.Size(); TriangleNr++)
    {
        const CafuModelT::MeshT::TriangleT& Tri=Mesh.Triangles[TriangleNr];
        const CafuModelT::MeshT::VertexT&   V_0=Mesh.Vertices[Tri.VertexIdx[0]];
        const CafuModelT::MeshT::VertexT&   V_1=Mesh.Vertices[Tri.VertexIdx[1]];
        const CafuModelT::MeshT::VertexT&   V_2=Mesh.Vertices[Tri.VertexIdx[2]];

        MeshInfoT::TriangleT& TriInfo=MeshInfo.Triangles[TriangleNr];
        MeshInfoT::VertexT&   V_0Info=MeshInfo.Vertices[Tri.VertexIdx[0]];
        MeshInfoT::VertexT&   V_1Info=MeshInfo.Vertices[Tri.VertexIdx[1]];
        MeshInfoT::VertexT&   V_2Info=MeshInfo.Vertices[Tri.VertexIdx[2]];

        const Vector3fT       Edge01=V_1Info.Pos-V_0Info.Pos;
        const Vector3fT       Edge02=V_2Info.Pos-V_0Info.Pos;

        // Triangles are ordered CW for md5 models and CCW for ase models, so we write
        // Normal=VectorCross(Edge02, Edge01) for md5 models and Normal=VectorCross(Edge01, Edge02) for ase models.
        TriInfo.Normal=myNormalize(Edge02.cross(Edge01));

        if (Mesh.Triangles[TriangleNr].SkipDraw)
            continue;

        // Understanding what's going on here is easy. The key statement is
        // "The tangent vector is parallel to the direction of increasing S on a parametric surface."
        // First, there is a short explanation in "The Cg Tutorial", chapter 8.
        // Second, I have drawn a simple figure that leads to a simple 2x2 system of Gaussian equations, see my TechArchive.
        const Vector3fT uv01=Vector3fT(V_1.u, V_1.v, 0.0f)-Vector3fT(V_0.u, V_0.v, 0.0f);
        const Vector3fT uv02=Vector3fT(V_2.u, V_2.v, 0.0f)-Vector3fT(V_0.u, V_0.v, 0.0f);
        const float     f   =uv01.x*uv02.y-uv01.y*uv02.x>0.0 ? 1.0f : -1.0f;

        const Vector3fT TriInfo_Tangent =myNormalize(Edge02.GetScaled(-uv01.y*f) + Edge01.GetScaled(uv02.y*f));
        const Vector3fT TriInfo_BiNormal=myNormalize(Edge02.GetScaled( uv01.x*f) - Edge01.GetScaled(uv02.x*f));

        m_Draw_Meshes[MeshNr].Vertices[DrawTriNr*3+0].SetOrigin(V_0Info.Pos.x, V_0Info.Pos.y, V_0Info.Pos.z);
        m_Draw_Meshes[MeshNr].Vertices[DrawTriNr*3+0].SetTextureCoord(V_0.u, V_0.v);
        m_Draw_Meshes[MeshNr].Vertices[DrawTriNr*3+1].SetOrigin(V_1Info.Pos.x, V_1Info.Pos.y, V_1Info.Pos.z);
        m_Draw_Meshes[MeshNr].Vertices[DrawTriNr*3+1].SetTextureCoord(V_1.u, V_1.v);
        m_Draw_Meshes[MeshNr].Vertices[DrawTriNr*3+2].SetOrigin(V_2Info.Pos.x, V_2Info.Pos.y, V_2Info.Pos.z);
        m_Draw_Meshes[MeshNr].Vertices[DrawTriNr*3+2].SetTextureCoord(V_2.u, V_2.v);

        for (unsigned long i=0; i<3; i++)
        {
            m_Draw_Meshes[MeshNr].Vertices[DrawTriNr*3+i].SetNormal  (TriInfo.Normal.x,   TriInfo.Normal.y,   TriInfo.Normal.z  );
            m_Draw_Meshes[MeshNr].Vertices[DrawTriNr*3+i].SetTangent (TriInfo_Tangent.x,  TriInfo_Tangent.y,  TriInfo_Tangent.z );
            m_Draw_Meshes[MeshNr].Vertices[DrawTriNr*3+i].SetBiNormal(TriInfo_BiNormal.x, TriInfo_BiNormal.y, TriInfo_BiNormal.z);
        }

        DrawTriNr++;
    }
}


void AnimPoseT::UpdateTangentSpaceGlobal(unsigned long MeshNr) const
{
    const CafuModelT::MeshT& Mesh    =m_Model.GetMeshes()[MeshNr];
    MeshInfoT&               MeshInfo=m_MeshInfos[MeshNr];


    // *******************************************************************************************
    //  Compute the tangent-space basis vectors for all triangles and all vertices.
    //  This is done by first computing the per-triangle axes and then having them enter
    //  the relevant per-vertex averages as required (taking mirror corrections into account).
    //  The per-triangle normal vectors are also kept for stencil shadow silhoutte determination.
    // *******************************************************************************************

    // For all vertices, zero the tangent-space vectors for the subsequent average accumulation.
    for (unsigned long VertexNr=0; VertexNr<MeshInfo.Vertices.Size(); VertexNr++)
    {
        MeshInfoT::VertexT& VertexInfo=MeshInfo.Vertices[VertexNr];

        VertexInfo.Normal  =Vector3fT(0, 0, 0);
        VertexInfo.Tangent =Vector3fT(0, 0, 0);
        VertexInfo.BiNormal=Vector3fT(0, 0, 0);
    }

    // Compute the per-triangle tangent-space axes and distribute them over the relevant vertices appropriately.
    for (unsigned long TriangleNr=0; TriangleNr<Mesh.Triangles.Size(); TriangleNr++)
    {
        const CafuModelT::MeshT::TriangleT& Tri=Mesh.Triangles[TriangleNr];
        const CafuModelT::MeshT::VertexT&   V_0=Mesh.Vertices[Tri.VertexIdx[0]];
        const CafuModelT::MeshT::VertexT&   V_1=Mesh.Vertices[Tri.VertexIdx[1]];
        const CafuModelT::MeshT::VertexT&   V_2=Mesh.Vertices[Tri.VertexIdx[2]];

        MeshInfoT::TriangleT& TriInfo=MeshInfo.Triangles[TriangleNr];
        MeshInfoT::VertexT&   V_0Info=MeshInfo.Vertices[Tri.VertexIdx[0]];
        MeshInfoT::VertexT&   V_1Info=MeshInfo.Vertices[Tri.VertexIdx[1]];
        MeshInfoT::VertexT&   V_2Info=MeshInfo.Vertices[Tri.VertexIdx[2]];

        const Vector3fT       Edge01=V_1Info.Pos-V_0Info.Pos;
        const Vector3fT       Edge02=V_2Info.Pos-V_0Info.Pos;

        // Triangles are ordered CW for md5 models and CCW for ase models, so we write
        // Normal=VectorCross(Edge02, Edge01) for md5 models and Normal=VectorCross(Edge01, Edge02) for ase models.
        TriInfo.Normal=myNormalize(Edge02.cross(Edge01));

        // Understanding what's going on here is easy. The key statement is
        // "The tangent vector is parallel to the direction of increasing S on a parametric surface."
        // First, there is a short explanation in "The Cg Tutorial", chapter 8.
        // Second, I have drawn a simple figure that leads to a simple 2x2 system of Gaussian equations, see my TechArchive.
        const Vector3fT uv01=Vector3fT(V_1.u, V_1.v, 0.0f)-Vector3fT(V_0.u, V_0.v, 0.0f);
        const Vector3fT uv02=Vector3fT(V_2.u, V_2.v, 0.0f)-Vector3fT(V_0.u, V_0.v, 0.0f);
        const float     f   =uv01.x*uv02.y-uv01.y*uv02.x>0.0 ? 1.0f : -1.0f;

        const Vector3fT TriInfo_Tangent =myNormalize(Edge02.GetScaled(-uv01.y*f) + Edge01.GetScaled(uv02.y*f));
        const Vector3fT TriInfo_BiNormal=myNormalize(Edge02.GetScaled( uv01.x*f) - Edge01.GetScaled(uv02.x*f));


        // Distribute the per-triangle tangent-space over the affected vertices.
#if 1
        const float Pi=3.14159265358979323846f;

        const float c0=dot(myNormalize(Edge01), myNormalize(Edge02));
        const float c1=dot(myNormalize(Edge01), myNormalize(V_1Info.Pos-V_2Info.Pos));

        const float w0=(c0>=1.0f) ? 0.0f : ( (c0<=-1.0f) ? Pi : acos(c0) );
        const float w1=(c1>=1.0f) ? 0.0f : ( (c1<=-1.0f) ? Pi : acos(c1) );

        const float TriWeight[3] = { w0, w1, Pi - w0 - w1 };
#else
        const float TriWeight[3] = { 1.0f, 1.0f, 1.0f };
#endif

        for (int i=0; i<3; i++)
        {
            const CafuModelT::MeshT::VertexT& Vertex    =Mesh.Vertices[Tri.VertexIdx[i]];
            MeshInfoT::VertexT&               VertexInfo=MeshInfo.Vertices[Tri.VertexIdx[i]];

            assert(Tri.Polarity==Vertex.Polarity);

            VertexInfo.Normal  +=TriInfo.Normal*TriWeight[i];
            VertexInfo.Tangent +=TriInfo_Tangent*TriWeight[i];
            VertexInfo.BiNormal+=TriInfo_BiNormal*TriWeight[i];

            for (unsigned long DupNr=0; DupNr<Vertex.GeoDups.Size(); DupNr++)
            {
                const CafuModelT::MeshT::VertexT& DupVertex    =Mesh.Vertices[Vertex.GeoDups[DupNr]];
                MeshInfoT::VertexT&               DupVertexInfo=MeshInfo.Vertices[Vertex.GeoDups[DupNr]];

                DupVertexInfo.Normal  +=TriInfo.Normal*TriWeight[i];
                DupVertexInfo.Tangent +=TriInfo_Tangent*(Tri.Polarity==DupVertex.Polarity ? TriWeight[i] : -TriWeight[i]);
                DupVertexInfo.BiNormal+=TriInfo_BiNormal*TriWeight[i];
            }
        }
    }

    // Finally normalize the per-vertex tangent-space axes; this is quasi the "division" in the average computations.
    for (unsigned long VertexNr=0; VertexNr<MeshInfo.Vertices.Size(); VertexNr++)
    {
        MeshInfoT::VertexT& VertexInfo=MeshInfo.Vertices[VertexNr];

        // Normalize the tangent-space axes.
        VertexInfo.Normal  =myNormalize(VertexInfo.Normal  );
        VertexInfo.Tangent =myNormalize(VertexInfo.Tangent );
        VertexInfo.BiNormal=myNormalize(VertexInfo.BiNormal);
    }


    // ***************************************************************************************************************
    //  Construct explicit MatSys::MeshT meshes now.
    //  Note that this is very inefficient - we REALLY should work with index arrays! (and/or vertex buffer objects!)
    // ***************************************************************************************************************

    unsigned long DrawTriNr=0;

    for (unsigned long TriNr=0; TriNr<Mesh.Triangles.Size(); TriNr++)
    {
        if (Mesh.Triangles[TriNr].SkipDraw)
            continue;

        for (unsigned long i=0; i<3; i++)
        {
            const unsigned long       VertexIdx =Mesh.Triangles[TriNr].VertexIdx[i];
            const MeshInfoT::VertexT& VertexInfo=MeshInfo.Vertices[VertexIdx];

            m_Draw_Meshes[MeshNr].Vertices[DrawTriNr*3+i].SetOrigin(VertexInfo.Pos.x, VertexInfo.Pos.y, VertexInfo.Pos.z);
            m_Draw_Meshes[MeshNr].Vertices[DrawTriNr*3+i].SetTextureCoord(Mesh.Vertices[VertexIdx].u, Mesh.Vertices[VertexIdx].v);
            m_Draw_Meshes[MeshNr].Vertices[DrawTriNr*3+i].SetNormal  (VertexInfo.Normal.x,   VertexInfo.Normal.y,   VertexInfo.Normal.z  );
            m_Draw_Meshes[MeshNr].Vertices[DrawTriNr*3+i].SetTangent (VertexInfo.Tangent.x,  VertexInfo.Tangent.y,  VertexInfo.Tangent.z );
            m_Draw_Meshes[MeshNr].Vertices[DrawTriNr*3+i].SetBiNormal(VertexInfo.BiNormal.x, VertexInfo.BiNormal.y, VertexInfo.BiNormal.z);
        }

        DrawTriNr++;
    }
}


#if 0
/// This method implements "local" smoothing, which, unfortunately,
/// seems not to work well for most meshes (possibly all meshes but cones).
///
/// In this context, "global" smoothing means that when the side of a pyramid or cone is considered,
/// for computing the tangent-space of its tip vertex, all sides of the pyramid enter the average,
/// because they all share the common tip vertex.
/// As a result, the normal vector at the tip is straight (axial), the same for all sides, and the
/// interpolated "shape" in tangent-space is really a half-sphere rather than a cone.
///
/// In contrast, "local" smoothing averages only the current side with its left and right neighbours.
/// The resulting tangent-space normal vectors at the tip vertices remain orthogonal to their triangle,
/// and the interpolated tangent-space shape is a cone rather than a half-sphere.
void AnimPoseT::UpdateTangentSpaceSgLocal(unsigned long MeshNr) const
{
    const CafuModelT::MeshT& Mesh    =m_Model.GetMeshes()[MeshNr];
    MeshInfoT&               MeshInfo=m_MeshInfos[MeshNr];


    // *******************************************************************************************
    //  Compute the tangent-space basis vectors for all triangles and all vertices.
    //  This is done by first computing the per-triangle axes and then having them enter
    //  the relevant per-vertex averages as required (taking mirror corrections into account).
    //  The per-triangle normal vectors are also kept for stencil shadow silhoutte determination.
    // *******************************************************************************************

    // For all vertices, zero the tangent-space vectors for the subsequent average accumulation.
    for (unsigned long VertexNr=0; VertexNr<MeshInfo.Vertices.Size(); VertexNr++)
    {
        MeshInfoT::VertexT& VertexInfo=MeshInfo.Vertices[VertexNr];

        VertexInfo.Normal  =Vector3fT(0, 0, 0);
        VertexInfo.Tangent =Vector3fT(0, 0, 0);
        VertexInfo.BiNormal=Vector3fT(0, 0, 0);
    }

    static ArrayT<Vector3fT> TriInfo_Tangents;
    static ArrayT<Vector3fT> TriInfo_BiNormals;

    TriInfo_Tangents.Overwrite();
    TriInfo_Tangents.PushBackEmpty(Mesh.Triangles.Size());

    TriInfo_BiNormals.Overwrite();
    TriInfo_BiNormals.PushBackEmpty(Mesh.Triangles.Size());

    // Compute the per-triangle tangent-space axes and distribute them over the relevant vertices appropriately.
    for (unsigned long TriangleNr=0; TriangleNr<Mesh.Triangles.Size(); TriangleNr++)
    {
        const CafuModelT::MeshT::TriangleT& Tri=Mesh.Triangles[TriangleNr];
        const CafuModelT::MeshT::VertexT&   V_0=Mesh.Vertices[Tri.VertexIdx[0]];
        const CafuModelT::MeshT::VertexT&   V_1=Mesh.Vertices[Tri.VertexIdx[1]];
        const CafuModelT::MeshT::VertexT&   V_2=Mesh.Vertices[Tri.VertexIdx[2]];

        MeshInfoT::TriangleT& TriInfo=MeshInfo.Triangles[TriangleNr];
        MeshInfoT::VertexT&   V_0Info=MeshInfo.Vertices[Tri.VertexIdx[0]];
        MeshInfoT::VertexT&   V_1Info=MeshInfo.Vertices[Tri.VertexIdx[1]];
        MeshInfoT::VertexT&   V_2Info=MeshInfo.Vertices[Tri.VertexIdx[2]];

        const Vector3fT       Edge01=V_1Info.Pos-V_0Info.Pos;
        const Vector3fT       Edge02=V_2Info.Pos-V_0Info.Pos;

        // Triangles are ordered CW for md5 models and CCW for ase models, so we write
        // Normal=VectorCross(Edge02, Edge01) for md5 models and Normal=VectorCross(Edge01, Edge02) for ase models.
        TriInfo.Normal=myNormalize(Edge02.cross(Edge01));

        // Understanding what's going on here is easy. The key statement is
        // "The tangent vector is parallel to the direction of increasing S on a parametric surface."
        // First, there is a short explanation in "The Cg Tutorial", chapter 8.
        // Second, I have drawn a simple figure that leads to a simple 2x2 system of Gaussian equations, see my TechArchive.
        const Vector3fT uv01=Vector3fT(V_1.u, V_1.v, 0.0f)-Vector3fT(V_0.u, V_0.v, 0.0f);
        const Vector3fT uv02=Vector3fT(V_2.u, V_2.v, 0.0f)-Vector3fT(V_0.u, V_0.v, 0.0f);
        const float     f   =uv01.x*uv02.y-uv01.y*uv02.x>0.0 ? 1.0f : -1.0f;

        TriInfo_Tangents [TriangleNr]=myNormalize(Edge02.GetScaled(-uv01.y*f) + Edge01.GetScaled(uv02.y*f));
        TriInfo_BiNormals[TriangleNr]=myNormalize(Edge02.GetScaled( uv01.x*f) - Edge01.GetScaled(uv02.x*f));
    }


    // ***************************************************************************************************************
    //  Construct explicit MatSys::MeshT meshes now.
    //  Note that this is very inefficient - we REALLY should work with index arrays! (and/or vertex buffer objects!)
    // ***************************************************************************************************************

    unsigned long DrawTriNr=0;

    for (unsigned long TriNr=0; TriNr<Mesh.Triangles.Size(); TriNr++)
    {
        if (Mesh.Triangles[TriNr].SkipDraw)
            continue;

        Vector3fT VertexNormal[3];
        Vector3fT VertexTangent[3];
        Vector3fT VertexBiNormal[3];

        for (unsigned int i=0; i<3; i++)
        {
            VertexNormal  [i]=MeshInfo.Triangles[TriNr].Normal;
            VertexTangent [i]=TriInfo_Tangents[TriNr];
            VertexBiNormal[i]=TriInfo_BiNormals[TriNr];
        }

        for (unsigned int i=0; i<3; i++)
        {
            const int NeighbIdx=Mesh.Triangles[TriNr].NeighbIdx[i];

            if (NeighbIdx>=0)
            {
                VertexNormal  [i]+=MeshInfo.Triangles[NeighbIdx].Normal;
                VertexTangent [i]+=TriInfo_Tangents[NeighbIdx];
                VertexBiNormal[i]+=TriInfo_BiNormals[NeighbIdx];

                VertexNormal  [(i+1) % 3]+=MeshInfo.Triangles[NeighbIdx].Normal;
                VertexTangent [(i+1) % 3]+=TriInfo_Tangents[NeighbIdx];
                VertexBiNormal[(i+1) % 3]+=TriInfo_BiNormals[NeighbIdx];
            }
        }

        for (unsigned int i=0; i<3; i++)
        {
            VertexNormal  [i]=myNormalize(VertexNormal  [i]);
            VertexTangent [i]=myNormalize(VertexTangent [i]);
            VertexBiNormal[i]=myNormalize(VertexBiNormal[i]);
        }

        for (unsigned long i=0; i<3; i++)
        {
            const unsigned long       VertexIdx =Mesh.Triangles[TriNr].VertexIdx[i];
            const MeshInfoT::VertexT& VertexInfo=MeshInfo.Vertices[VertexIdx];

            m_Draw_Meshes[MeshNr].Vertices[DrawTriNr*3+i].SetOrigin(VertexInfo.Pos.x, VertexInfo.Pos.y, VertexInfo.Pos.z);
            m_Draw_Meshes[MeshNr].Vertices[DrawTriNr*3+i].SetTextureCoord(Mesh.Vertices[VertexIdx].u, Mesh.Vertices[VertexIdx].v);
            m_Draw_Meshes[MeshNr].Vertices[DrawTriNr*3+i].SetNormal  (VertexNormal[i].x,   VertexNormal[i].y,   VertexNormal[i].z  );
            m_Draw_Meshes[MeshNr].Vertices[DrawTriNr*3+i].SetTangent (VertexTangent[i].x,  VertexTangent[i].y,  VertexTangent[i].z );
            m_Draw_Meshes[MeshNr].Vertices[DrawTriNr*3+i].SetBiNormal(VertexBiNormal[i].x, VertexBiNormal[i].y, VertexBiNormal[i].z);
        }

        DrawTriNr++;
    }
}
#endif


void AnimPoseT::UpdateTangentSpaceSmGroups(unsigned long MeshNr) const
{
    // TODO: At this time, this is the same as GLOBAL.
    UpdateTangentSpaceGlobal(MeshNr);
}


void AnimPoseT::Recache() const
{
    if (!m_SuperPose && m_AnimExpr->IsEqual(m_CachedAE)) return;

    SyncDimensions();
    UpdateJointMatrices();
    UpdateVertexPositions();

    for (unsigned long MeshNr=0; MeshNr<m_Model.GetMeshes().Size(); MeshNr++)
    {
        // Compute the tangent-space ourselves.
        switch (m_Model.GetMeshes()[MeshNr].TSMethod)
        {
            case CafuModelT::MeshT::HARD:
                UpdateTangentSpaceHard(MeshNr);
                break;

            case CafuModelT::MeshT::GLOBAL:
                UpdateTangentSpaceGlobal(MeshNr);
                break;

            case CafuModelT::MeshT::SM_GROUPS:
                UpdateTangentSpaceSmGroups(MeshNr);
                break;
        }
    }

    if (m_SuperPose)
    {
        assert(m_CachedAE==NULL);
    }
    else
    {
        m_CachedAE=m_AnimExpr->Clone();
    }
}


void AnimPoseT::SetSuperPose(const AnimPoseT* SuperPose)
{
    if (m_SuperPose==SuperPose) return;

    m_SuperPose=SuperPose;
    m_CachedAE=NULL;

    // Recursively update the chain of dlod poses.
    if (m_DlodPose) m_DlodPose->SetSuperPose(SuperPose);
}


void AnimPoseT::Draw(int SkinNr, float LodDist) const
{
    if (m_Model.GetDlodModel() && LodDist >= m_Model.GetDlodDist())
    {
        m_DlodPose->Draw(SkinNr, LodDist);
        return;
    }

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

            // If the model is skeleton only and has no meshes, the m_BoundingBox may not be inited.
            if (m_BoundingBox.IsInited() && !LightBB.Intersects(m_BoundingBox)) return;
            break;
        }
    }

    switch (MatSys::Renderer->GetCurrentRenderAction())
    {
        case MatSys::RendererI::AMBIENT:
        {
            for (unsigned long MeshNr=0; MeshNr<m_Draw_Meshes.Size(); MeshNr++)
            {
                MatSys::Renderer->SetCurrentMaterial(m_Model.GetRenderMaterial(MeshNr, SkinNr));
                MatSys::Renderer->RenderMesh(m_Draw_Meshes[MeshNr]);
            }
            break;
        }

        case MatSys::RendererI::LIGHTING:
        {
            for (unsigned long MeshNr=0; MeshNr<m_Draw_Meshes.Size(); MeshNr++)
            {
                MatSys::Renderer->SetCurrentMaterial(m_Model.GetRenderMaterial(MeshNr, SkinNr));
                MatSys::Renderer->RenderMesh(m_Draw_Meshes[MeshNr]);
            }
            break;
        }

        case MatSys::RendererI::STENCILSHADOW:
        {
            typedef CafuModelT::MeshT MeshT;

            const ArrayT<MeshT>& Meshes=m_Model.GetMeshes();
            const Vector3fT      LightPos(MatSys::Renderer->GetCurrentLightSourcePosition());

            for (unsigned long MeshNr=0; MeshNr<Meshes.Size(); MeshNr++)
            {
                const MeshT&     Mesh    =Meshes[MeshNr];
                const MeshInfoT& MeshInfo=m_MeshInfos[MeshNr];
                const MaterialT* MeshMat =m_Model.GetMaterial(MeshNr, SkinNr);

                if (!Mesh.CastShadows) continue;
                if (MeshMat==NULL || MeshMat->NoShadows) continue;

                static ArrayT<bool> TriangleIsFrontFacing;
                if (TriangleIsFrontFacing.Size()<Mesh.Triangles.Size())
                    TriangleIsFrontFacing.PushBackEmpty(Mesh.Triangles.Size()-TriangleIsFrontFacing.Size());

                for (unsigned long TriNr=0; TriNr<Mesh.Triangles.Size(); TriNr++)
                {
                    const MeshT::TriangleT&     Tri    =Mesh.Triangles[TriNr];
                    const MeshInfoT::TriangleT& TriInfo=MeshInfo.Triangles[TriNr];

                    const float Dot=(LightPos-MeshInfo.Vertices[Tri.VertexIdx[0]].Pos).dot(TriInfo.Normal);

                    TriangleIsFrontFacing[TriNr]=Dot>0;
                }


                // Note that we have to cull the following polygons wrt. the *VIEWER* (not the light source)!
                static MatSys::MeshT MeshSilhouette(MatSys::MeshT::Quads);  // The default winding order is "CW".
                MeshSilhouette.Vertices.Overwrite();

                for (unsigned long TriNr=0; TriNr<Mesh.Triangles.Size(); TriNr++)
                {
                    if (!TriangleIsFrontFacing[TriNr]) continue;

                    // This triangle is front-facing wrt. the light source.
                    const MeshT::TriangleT& Tri=Mesh.Triangles[TriNr];

                    for (unsigned long EdgeNr=0; EdgeNr<3; EdgeNr++)
                    {
                        // If an edge has no (-1) or more than one (-2) neighbours, make it a silhouette edge.
                        const bool IsSilhouetteEdge=(Tri.NeighbIdx[EdgeNr]<0) ? true : !TriangleIsFrontFacing[Tri.NeighbIdx[EdgeNr]];

                        if (IsSilhouetteEdge)
                        {
                            // The neighbour at edge 'EdgeNr' is back-facing (or non-existant), so we have found a possible silhouette edge.
                            const unsigned long v1=EdgeNr;
                            const unsigned long v2=(EdgeNr+1) % 3;
                            const Vector3fT     LA=MeshInfo.Vertices[Tri.VertexIdx[v1]].Pos-LightPos;
                            const Vector3fT     LB=MeshInfo.Vertices[Tri.VertexIdx[v2]].Pos-LightPos;

                            MeshSilhouette.Vertices.PushBackEmpty(4);

                            const unsigned long MeshSize=MeshSilhouette.Vertices.Size();

                            MeshSilhouette.Vertices[MeshSize-4].SetOrigin(MeshInfo.Vertices[Tri.VertexIdx[v2]].Pos);
                            MeshSilhouette.Vertices[MeshSize-3].SetOrigin(MeshInfo.Vertices[Tri.VertexIdx[v1]].Pos);
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
                    const Vector3fT& A=MeshInfo.Vertices[Tri.VertexIdx[0]].Pos;
                    const Vector3fT& B=MeshInfo.Vertices[Tri.VertexIdx[1]].Pos;
                    const Vector3fT& C=MeshInfo.Vertices[Tri.VertexIdx[2]].Pos;

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


bool AnimPoseT::GetGuiPlane(unsigned int GFNr, Vector3fT& Origin, Vector3fT& AxisX, Vector3fT& AxisY) const
{
    // if (m_Model.GetDlodModel() && LodDist >= m_Model.GetDlodDist())
    // {
    //     return m_DlodPose->GetGuiPlane(GFNr, Origin, AxisX, AxisY);
    // }

    Recache();

    if (GFNr >= m_Model.GetGuiFixtures().Size()) return false;

    const CafuModelT::GuiFixtureT& GF=m_Model.GetGuiFixtures()[GFNr];
    Vector3fT                      Points[3];

    for (unsigned int PointNr=0; PointNr<3; PointNr++)
    {
        if (!m_Model.IsMeshNrOK  (GF, PointNr)) return false;
        if (!m_Model.IsVertexNrOK(GF, PointNr)) return false;

        Points[PointNr]=m_MeshInfos[GF.Points[PointNr].MeshNr].Vertices[GF.Points[PointNr].VertexNr].Pos;
    }

    AxisX =(Points[1]-Points[0])*GF.Scale[0];
    AxisY =(Points[2]-Points[0])*GF.Scale[1];
    Origin=Points[0] + AxisX*GF.Trans[0] + AxisY*GF.Trans[1];

    // // It's pretty easy to derive this matrix geometrically, see my TechArchive note from 2006-08-22.
    // MatrixT M(AxisX.x/640.0f, AxisY.x/480.0f, 0.0f, Origin.x,
    //           AxisX.y/640.0f, AxisY.y/480.0f, 0.0f, Origin.y,
    //           AxisX.z/640.0f, AxisY.z/480.0f, 0.0f, Origin.z,
    //                     0.0f,           0.0f, 0.0f,     1.0f);

    return true;
}


bool AnimPoseT::TraceRay(int SkinNr, const Vector3fT& RayOrigin, const Vector3fT& RayDir, TraceResultT& Result) const
{
    // if (m_Model.GetDlodModel() && LodDist >= m_Model.GetDlodDist())
    // {
    //     return m_DlodPose->TraceRay(SkinNr, RayOrigin, RayDir, Result);
    // }

    Recache();

    // If we miss the bounding-box, then we miss all the triangles as well.
    float Fraction=0.0f;
    if (!GetBB()/*.GetEpsilonBox(...)*/.Contains(RayOrigin) && !GetBB().TraceRay(RayOrigin, RayDir, Fraction)) return false;

    typedef CafuModelT::MeshT MeshT;

    const ArrayT<MeshT>& Meshes=m_Model.GetMeshes();

    for (unsigned long MeshNr=0; MeshNr<Meshes.Size(); MeshNr++)
    {
        const MeshT&     Mesh    =Meshes[MeshNr];
        const MeshInfoT& MeshInfo=m_MeshInfos[MeshNr];
        const MaterialT* MeshMat =m_Model.GetMaterial(MeshNr, SkinNr);

        // If the ClipFlags don't match the ClipMask, this polygon doesn't interfere with the trace.
        if (!MeshMat) continue;
        // if ((MeshMat->ClipFlags & ClipMask)==0) continue;

        for (unsigned long TriNr=0; TriNr<Mesh.Triangles.Size(); TriNr++)
        {
            // This code is a modification of the code in CollisionModelStaticT::PolygonT::TraceRay(),
            // see there for details and additional information.
            using namespace cf::math;
            const MeshT::TriangleT&     Tri    =Mesh.Triangles[TriNr];
            const MeshInfoT::TriangleT& TriInfo=MeshInfo.Triangles[TriNr];

            const Vector3fT& A=MeshInfo.Vertices[Tri.VertexIdx[0]].Pos;
            const Vector3fT& B=MeshInfo.Vertices[Tri.VertexIdx[1]].Pos;
            const Vector3fT& C=MeshInfo.Vertices[Tri.VertexIdx[2]].Pos;

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
            const float Nenner=dot(TriInfo.Normal, RayDir);

            if (Nenner==0) continue;                            // If Nenner==0, then RayDir is parallel to the triangle plane (no intersection).
            assert(MeshMat->TwoSided || Nenner<0);              // If the material is single sided, then Nenner<0, a consequence of the Pluecker tests above.

            const float Dist=dot(TriInfo.Normal, RayOrigin-A);  // The distance of RayOrigin to the triangle plane.
            const float F   =-(Dist-0.03125f)/Nenner;

            // The intersection is only valid in the positive direction of RayDir.
            if (F<0) continue;

            // Hit the triangle!
            Result.Fraction=F;
            Result.Normal  =(Nenner<0) ? TriInfo.Normal : -TriInfo.Normal;  // Handle two-sided materials properly.
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
        const Vector3fT&   DrawPos =m_MeshInfos[MeshNr].Vertices[VertexNr].Pos;
        const float        Dist    =length(DrawPos-P);

        if (i==0 || Dist<BestDist)
        {
            BestDist    =Dist;
            BestVertexNr=VertexNr;
        }
    }

    return BestVertexNr;
}


const ArrayT<MatrixT>& AnimPoseT::GetJointMatrices() const
{
    Recache();

    return m_JointMatrices;
}


const MatrixT* AnimPoseT::GetJointMatrix(const std::string& JointName) const
{
    Recache();

    for (unsigned int JointNr=0; JointNr<m_Model.GetJoints().Size(); JointNr++)
        if (m_Model.GetJoints()[JointNr].Name == JointName)
            return &m_JointMatrices[JointNr];

    return NULL;
}


const ArrayT<AnimPoseT::MeshInfoT>& AnimPoseT::GetMeshInfos() const
{
    Recache();

    return m_MeshInfos;
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
