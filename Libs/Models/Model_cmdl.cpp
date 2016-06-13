/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Model_cmdl.hpp"
#include "Loader.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/BoundingBox.hpp"
#include "Math3D/Quaternion.hpp"
#include "String.hpp"

#include <iostream>

#if defined(_WIN32) && defined(_MSC_VER)
    // Turn off warning C4355: 'this' : used in base member initializer list.
    #pragma warning(disable:4355)
#endif


/*static*/ const unsigned int CafuModelT::CMDL_FILE_VERSION=1;


/// This function serializes a given float f1 to a string s, such that:
///   - s is minimal (uses the least number of decimal digits required),
///   - unserializing s back to a float f2 yields f1==f2.
/// See my post "float to string to float, with first float == second float"
/// to comp.lang.c++ on 2009-10-06 for additional details.
static std::string serialize(float f1)
{
    // Make sure that if f1 is -0, "0" instead of "-0" is returned.
    if (f1==0.0f) return "0";

    // From MSDN documentation: "digits10 returns the number of decimal digits that the type can represent without loss of precision."
    // For floats, that's usually 6, for doubles, that's usually 15. However, we want to use the number of *significant* decimal digits here,
    // that is, max_digits10. See http://www.open-std.org/JTC1/sc22/wg21/docs/papers/2006/n2005.pdf for details.
    const unsigned int DIGITS10    =std::numeric_limits<float>::digits10;
    const unsigned int MAX_DIGITS10=DIGITS10+3;

    std::string  s;
    unsigned int prec;

    for (prec=DIGITS10; prec<=MAX_DIGITS10; prec++)
    {
        std::stringstream ss;

        ss.precision(prec);
        ss << f1;

        s=ss.str();

#if defined(_MSC_VER) && (_MSC_VER <= 1900)     // 1900 == Visual C++ 14.0 (2015)
        // There is a bug in Microsoft's iostream implementation up to Visual C++ 2015,
        // see http://trac.cafu.de/ticket/150 for details.
        const float f2 = float(atof(s.c_str()));
#else
        float f2;
        ss >> f2;
#endif

        if (f2==f1) break;
    }

    assert(prec<=MAX_DIGITS10);
    return s;
}


static std::string serialize(const Vector3fT& v)
{
    return serialize(v.x)+", "+serialize(v.y)+", "+serialize(v.z);
}


CafuModelT::MeshT::TriangleT::TriangleT(unsigned int v0, unsigned int v1, unsigned int v2)
    : SmoothGroups(0),
      Polarity(false),
      SkipDraw(false)
{
    VertexIdx[0]=v0;
    VertexIdx[1]=v1;
    VertexIdx[2]=v2;

    NeighbIdx[0]=-1;
    NeighbIdx[1]=-1;
    NeighbIdx[2]=-1;
}


bool CafuModelT::MeshT::AreGeoDups(unsigned int Vertex1Nr, unsigned int Vertex2Nr) const
{
    // A vertex is a geodup of itself!
    // (This explicit check is somewhat redundant, but I leave it in for clarity.)
    if (Vertex1Nr==Vertex2Nr) return true;

    // If the number of weights differs, the vertices cannot be geodups.
    if (Vertices[Vertex1Nr].NumWeights!=Vertices[Vertex2Nr].NumWeights) return false;

    // The number of weights matches. If the FirstWeightIdx is identical, too, the vertices trivially are a geodup.
    if (Vertices[Vertex1Nr].FirstWeightIdx==Vertices[Vertex2Nr].FirstWeightIdx) return true;

    // The number of weights matches, but the FirstWeightIdx does not.
    // Now compare all the weights manually. If all their contents match, they are geodups.
    for (unsigned int WeightNr=0; WeightNr<Vertices[Vertex1Nr].NumWeights; WeightNr++)
    {
        const WeightT& Weight1=Weights[Vertices[Vertex1Nr].FirstWeightIdx+WeightNr];
        const WeightT& Weight2=Weights[Vertices[Vertex2Nr].FirstWeightIdx+WeightNr];

        if (Weight1.JointIdx!=Weight2.JointIdx) return false;
        if (Weight1.Weight  !=Weight2.Weight  ) return false;   // Bitwise float compare...
        if (Weight1.Pos     !=Weight2.Pos     ) return false;   // Bitwise float compare...
    }

    // All weights were equal - the vertices are geodups of each other!
    return true;
}


std::string CafuModelT::MeshT::GetTSMethod() const
{
    switch (TSMethod)
    {
        case HARD:      return "HARD";
        case GLOBAL:    return "GLOBAL";
        case SM_GROUPS: return "SM_GROUPS";
    }

    return "?";
}


void CafuModelT::MeshT::SetTSMethod(const std::string& m)
{
    TSMethod=GLOBAL;

         if (m=="HARD")      TSMethod=HARD;
    else if (m=="GLOBAL")    TSMethod=GLOBAL;
    else if (m=="SM_GROUPS") TSMethod=SM_GROUPS;
}


CafuModelT::GuiFixtureT::GuiFixtureT()
    : Name("GUI Fixture")
{
    for (unsigned int PointNr=0; PointNr<3; PointNr++)
    {
        Points[PointNr].MeshNr  =std::numeric_limits<unsigned int>::max();
        Points[PointNr].VertexNr=std::numeric_limits<unsigned int>::max();
    }

    Trans[0]=0.0f;
    Trans[1]=0.0f;

    Scale[0]=1.0f;
    Scale[1]=1.0f;
}


void CafuModelT::AnimT::RecomputeBB(unsigned int FrameNr, const ArrayT<JointT>& Joints, const ArrayT<MeshT>& Meshes)
{
    // This is an auxiliary method that is only needed after an AnimT instance has been newly
    // created (e.g. in one of the model loaders) or manipulated (e.g. in the Model Editor).
    static ArrayT<MatrixT> JointMatrices;

    JointMatrices.Overwrite();
    JointMatrices.PushBackEmpty(Joints.Size());

    // This is a modified version of the loop in CafuModelT::UpdateCachedDrawData().
    for (unsigned long JointNr=0; JointNr<Joints.Size(); JointNr++)
    {
        const CafuModelT::AnimT::AnimJointT& AJ=AnimJoints[JointNr];
        Vector3fT Data_0[3]={ AJ.DefaultPos, AJ.DefaultQtr, AJ.DefaultScale };

        // Determine the position, quaternion and scale at FrameNr.
        unsigned int FlagCount=0;

        for (int i=0; i<9; i++)
        {
            if ((AJ.Flags >> i) & 1)
            {
                Data_0[i/3][i % 3]=Frames[FrameNr].AnimData[AJ.FirstDataIdx+FlagCount];

                FlagCount++;
            }
        }

        // Compute the matrix that is relative to the parent bone, and finally obtain the absolute matrix for that bone!
        const MatrixT RelMatrix(Data_0[0], cf::math::QuaternionfT::FromXYZ(Data_0[1]), Data_0[2]);
        const CafuModelT::JointT& J=Joints[JointNr];

        JointMatrices[JointNr]=(J.Parent==-1) ? RelMatrix : JointMatrices[J.Parent]*RelMatrix;
    }

    // "Clear" the old bounding-box.
    Frames[FrameNr].BB=BoundingBox3fT();

    // This is very similar to the code in AnimPoseT::UpdateData().
    for (unsigned long MeshNr=0; MeshNr<Meshes.Size(); MeshNr++)
    {
        const CafuModelT::MeshT& Mesh=Meshes[MeshNr];

        for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
        {
            const CafuModelT::MeshT::VertexT& Vertex=Mesh.Vertices[VertexNr];
            Vector3fT OutVert;

            if (Vertex.NumWeights==1)
            {
                const CafuModelT::MeshT::WeightT& Weight=Mesh.Weights[Vertex.FirstWeightIdx];

                OutVert=JointMatrices[Weight.JointIdx].Mul_xyz1(Weight.Pos);
            }
            else
            {
                for (unsigned int WeightNr=0; WeightNr<Vertex.NumWeights; WeightNr++)
                {
                    const CafuModelT::MeshT::WeightT& Weight=Mesh.Weights[Vertex.FirstWeightIdx+WeightNr];

                    OutVert+=JointMatrices[Weight.JointIdx].Mul_xyz1(Weight.Pos) * Weight.Weight;
                }
            }

            Frames[FrameNr].BB+=OutVert;
        }
    }
}


bool CafuModelT::AnimT::IsLastFrameDup() const
{
    if (Frames.Size() < 2) return false;

    return Frames[0].AnimData == Frames[Frames.Size()-1].AnimData;
}


bool CafuModelT::ChannelT::IsMember(unsigned int JointNr) const
{
    const unsigned int BlockNr=JointNr >> 5;
    const unsigned int BitNr  =JointNr & 31;
    const unsigned int BitMask=1 << BitNr;

    if (BlockNr >= m_BitBlocks.Size()) return false;

    return (m_BitBlocks[BlockNr] & BitMask) != 0;
}


void CafuModelT::ChannelT::SetMember(unsigned int JointNr, bool Member)
{
    const unsigned int BlockNr=JointNr >> 5;
    const unsigned int BitNr  =JointNr & 31;
    const unsigned int BitMask=1 << BitNr;

    // Grow as needed.
    while (BlockNr >= m_BitBlocks.Size())
        m_BitBlocks.PushBack(0);

    if (Member) m_BitBlocks[BlockNr] |=  BitMask;
           else m_BitBlocks[BlockNr] &= ~BitMask;
}


CafuModelT::CafuModelT(ModelLoaderT& Loader)
    : m_FileName(Loader.GetFileName()),
      m_MaterialMan(),
      m_DlodModel(NULL),
      m_DlodDist(0.0f),
      m_AnimExprPool(*this),
      m_TEMP_Pose(NULL)
{
    // No matter the actual model file format (that is, even if the file format is not "cmdl"),
    // the model artist might have prepared materials that should be used instead of the ones the Loader would otherwise generate.
    ArrayT<MaterialT*> AllMats=m_MaterialMan.RegisterMaterialScript(cf::String::StripExt(Loader.GetFileName())+".cmat", cf::String::GetPath(Loader.GetFileName())+"/");

    for (unsigned long MatNr=0; MatNr<AllMats.Size(); MatNr++)
        if (!AllMats[MatNr]->LightMapComp.IsEmpty())
        {
            // TODO: Use ModelLoaderT::UserCallbacksI::GetLog() instead.
            std::cout << "Model \"" << m_FileName << "\" uses material \"" << AllMats[MatNr]->Name << "\", which in turn has lightmaps defined.\n" <<
                "It will work in the ModelViewer, but for other applications like Cafu itself you should use a material without lightmaps.\n";
                // It works in the ModelViewer because the ModelViewer is kind enough to provide a default lightmap...
        }

    // Have the model loader load the model file.
    Loader.Load(m_Joints, m_Meshes, m_Anims, m_MaterialMan);
    Loader.Load(m_Skins, m_MaterialMan);
    Loader.Load(m_GuiFixtures);
    Loader.Load(m_Channels);
    Loader.Postprocess(m_Meshes);

    if (m_Joints.Size()==0) throw ModelLoaderT::LoadErrorT("This model has no joints.");
 // if (m_Meshes.Size()==0) throw ModelLoaderT::LoadErrorT("This model has no meshes.");  // Consider models with no meshes as valid, skeleton-only meshes are sometimes useful for testing.

    // Load the chain of dlod models, if any.
    {
        unsigned int Num=1;
        CafuModelT*  Model=this;

        while (Loader.Load(Num, Model->m_DlodModel, Model->m_DlodDist))
        {
            Num++;
            Model=Model->m_DlodModel;
        }
    }

    m_TEMP_Pose=new AnimPoseT(*this, m_AnimExprPool.GetStandard(-1, 0.0f));

    // Make sure that each skin has as many materials as there are meshes.
    for (unsigned long SkinNr=0; SkinNr<m_Skins.Size(); SkinNr++)
    {
        SkinT& Skin=m_Skins[SkinNr];

        while (Skin.Materials.Size() > m_Meshes.Size()) Skin.Materials.DeleteBack();
        while (Skin.Materials.Size() < m_Meshes.Size()) Skin.Materials.PushBack(NULL);

        while (Skin.RenderMaterials.Size() > m_Meshes.Size()) Skin.RenderMaterials.DeleteBack();
        while (Skin.RenderMaterials.Size() < m_Meshes.Size()) Skin.RenderMaterials.PushBack(NULL);
    }

    InitMeshes();

    // Allocate the render materials for the meshes (the default skin).
    for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
    {
        MeshT& Mesh=m_Meshes[MeshNr];

        assert(Mesh.RenderMaterial==NULL);
        Mesh.RenderMaterial=MatSys::Renderer!=NULL ? MatSys::Renderer->RegisterMaterial(Mesh.Material) : NULL;
    }

    // Allocate the render materials for the skins.
    for (unsigned long SkinNr=0; SkinNr<m_Skins.Size(); SkinNr++)
    {
        SkinT& Skin=m_Skins[SkinNr];

        assert(Skin.Materials.Size()      ==m_Meshes.Size());
        assert(Skin.RenderMaterials.Size()==m_Meshes.Size());

        for (unsigned long MatNr=0; MatNr<Skin.Materials.Size(); MatNr++)
        {
            assert(Skin.RenderMaterials[MatNr]==NULL);

            if (Skin.Materials[MatNr]!=NULL && MatSys::Renderer!=NULL)
                Skin.RenderMaterials[MatNr]=MatSys::Renderer->RegisterMaterial(Skin.Materials[MatNr]);
        }
    }
}


CafuModelT::~CafuModelT()
{
    delete m_TEMP_Pose;

    delete m_DlodModel;
    m_DlodModel=NULL;

    if (MatSys::Renderer==NULL) return;

    // Free all render materials used in skins.
    for (unsigned long SkinNr=0; SkinNr<m_Skins.Size(); SkinNr++)
        for (unsigned long MatNr=0; MatNr<m_Skins[SkinNr].RenderMaterials.Size(); MatNr++)
            MatSys::Renderer->FreeMaterial(m_Skins[SkinNr].RenderMaterials[MatNr]);

    // Free all render materials used in the meshes (the default skin).
    for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
        MatSys::Renderer->FreeMaterial(m_Meshes[MeshNr].RenderMaterial);
}


void CafuModelT::Import(AnimImporterT& Importer)
{
    m_Anims.PushBack(Importer.Import(m_Joints, m_Meshes));
}


void CafuModelT::InitMeshes()
{
    // Compute auxiliary data for each mesh.
    for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
    {
        MeshT& Mesh=m_Meshes[MeshNr];


        // ********************************************************************************
        //  Find the GeoDups of each vertex. See MeshT::AreGeoDups() for more information.
        // ********************************************************************************

        for (unsigned long Vertex1Nr=0; Vertex1Nr<Mesh.Vertices.Size(); Vertex1Nr++)
            for (unsigned long Vertex2Nr=0; Vertex2Nr<Mesh.Vertices.Size(); Vertex2Nr++)
                if (Vertex1Nr!=Vertex2Nr && Mesh.AreGeoDups(Vertex1Nr, Vertex2Nr))
                {
                    // Vertex 2 is a "pure" geometrical duplicate of vertex 1 (Vertex1Nr!=Vertex2Nr), so record its index at vertex 1.
                    // Note that the outer loops were written to guarantee that each GeoDups array contains indices in increasing order.
                    Mesh.Vertices[Vertex1Nr].GeoDups.PushBack(Vertex2Nr);
                }

#ifdef DEBUG
        for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
        {
            const MeshT::VertexT& Vertex=Mesh.Vertices[VertexNr];

            // Assert that the index of Vertex does not occur in Vertex.GeoDups, i.e. that Vertex.GeoDups not contains self (the own vertex index).
            for (unsigned long DupNr=0; DupNr<Vertex.GeoDups.Size(); DupNr++)
                assert(Vertex.GeoDups[DupNr]!=VertexNr);

            // Assert that the Vertex.GeoDups elements are indeed stored in increasing order.
            for (unsigned long DupNr=1; DupNr<Vertex.GeoDups.Size(); DupNr++)
                assert(Vertex.GeoDups[DupNr-1]<Vertex.GeoDups[DupNr]);
        }
#endif


        // ************************************************************************************
        //  For each triangle, determine its polarity. (Is the texture on it mirrored or not?)
        // ************************************************************************************

        for (unsigned long TriangleNr=0; TriangleNr<Mesh.Triangles.Size(); TriangleNr++)
        {
            MeshT::TriangleT& Tri=Mesh.Triangles[TriangleNr];

            // This code is analogous to the code that determines the per-triangle tangent-space vectors.
            const MeshT::VertexT& V_0=Mesh.Vertices[Tri.VertexIdx[0]];
            const MeshT::VertexT& V_1=Mesh.Vertices[Tri.VertexIdx[1]];
            const MeshT::VertexT& V_2=Mesh.Vertices[Tri.VertexIdx[2]];

            const Vector3fT uv01=Vector3fT(V_1.u, V_1.v, 0.0f)-Vector3fT(V_0.u, V_0.v, 0.0f);
            const Vector3fT uv02=Vector3fT(V_2.u, V_2.v, 0.0f)-Vector3fT(V_0.u, V_0.v, 0.0f);

            // This is analogous to cross(uv01, uv02).z > 0.0.
            Tri.Polarity=(uv01.x*uv02.y-uv01.y*uv02.x) > 0.0;
        }


        // *******************************************************************************
        //  Precompute the tables that store for each vertex which triangles refer to it.
        //  This helps with quickly determining the neighbours of a triangle below.
        // *******************************************************************************

        // For each vertex, this is the list of triangles that use (refer to) this vertex,
        // *without* the triangles that refer to geometrically identical vertices (i.e. the vertices in GeoDups).
        ArrayT< ArrayT<int> > Vertices_RefTris;
        Vertices_RefTris.PushBackEmpty(Mesh.Vertices.Size());

        for (unsigned long TriangleNr=0; TriangleNr<Mesh.Triangles.Size(); TriangleNr++)
        {
            const MeshT::TriangleT& Tri=Mesh.Triangles[TriangleNr];

            Vertices_RefTris[Tri.VertexIdx[0]].PushBack(TriangleNr);
            Vertices_RefTris[Tri.VertexIdx[1]].PushBack(TriangleNr);
            Vertices_RefTris[Tri.VertexIdx[2]].PushBack(TriangleNr);
        }


        // For each vertex, this is the list of triangles that use (refer to) this vertex,
        // *inclusive* the triangles that refer to geometrically identical vertices (i.e. the vertices in GeoDups).
        // That means that vertices that are dups of each other have identical RefTrisInclDups arrays (barring their elements order!).
        ArrayT< ArrayT<int> > Vertices_RefTrisInclDups;
        Vertices_RefTrisInclDups.PushBackEmpty(Mesh.Vertices.Size());

        for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
        {
            const MeshT::VertexT& Vertex=Mesh.Vertices[VertexNr];

         // Vertices_RefTrisInclDups[VertexNr]=Vertices_RefTris[VertexNr] + the RefTris of all duplicate vertices.
            Vertices_RefTrisInclDups[VertexNr]=Vertices_RefTris[VertexNr];

            // Concatenate the RefTris arrays of all geometrically identical vertices.
            // Note that the resulting Vertices_RefTrisInclDups[VertexNr] array will *not* contain any element more than once, no duplicate indices occur!
            for (unsigned long DupNr=0; DupNr<Vertex.GeoDups.Size(); DupNr++)
                Vertices_RefTrisInclDups[VertexNr].PushBack(Vertices_RefTris[Vertex.GeoDups[DupNr]]);
        }

#ifdef DEBUG
        for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
        {
            const MeshT::VertexT& Vertex=Mesh.Vertices[VertexNr];

            // Assert that the elements of Vertices_RefTrisInclDups[VertexNr] are all unique (no triangle is mentioned twice).
            for (unsigned long r1=0; r1+1<Vertices_RefTrisInclDups[VertexNr].Size(); r1++)
                for (unsigned long r2=r1+1; r2<Vertices_RefTrisInclDups[VertexNr].Size(); r2++)
                    assert(Vertices_RefTrisInclDups[VertexNr][r1]!=Vertices_RefTrisInclDups[VertexNr][r2]);

            // Assert that the RefTrisInclDups of all vertices in Vertex.GeoDups are identical to Vertices_RefTrisInclDups[VertexNr], barring the order.
            for (unsigned long DupNr=0; DupNr<Vertex.GeoDups.Size(); DupNr++)
                for (unsigned long TriNr=0; TriNr<Vertices_RefTrisInclDups[VertexNr].Size(); TriNr++)
                    assert(Vertices_RefTrisInclDups[Vertex.GeoDups[DupNr]].Find(Vertices_RefTrisInclDups[VertexNr][TriNr])!=-1);
        }
#endif


        // ********************************************************************************
        //  Pre-compute all triangle neighbourhood relationships.
        //  This information is required for the stencil shadows silhouette determination.
        // ********************************************************************************

        for (unsigned long TriangleNr=0; TriangleNr<Mesh.Triangles.Size(); TriangleNr++)
        {
            MeshT::TriangleT& Tri=Mesh.Triangles[TriangleNr];

            // Init with "no neighbours".
            Tri.NeighbIdx[0]=-1;
            Tri.NeighbIdx[1]=-1;
            Tri.NeighbIdx[2]=-1;
        }

        // Important note: If three triangles share a common edge, the relevant edge of *all* three triangles must be flagged with -2
        // (have multiple neighbours at this edge, treat like it was a free edge with no neighbour).
        // However, the fact that the three triangles share a common edge IS TYPICALLY DETECTED FOR ONLY *ONE* OF THE THREE TRIANGLES,
        // namely the one that has an orientation different from the two others.
        // We therefore also have to modify other triangles except for Tri1 at iteration Tri1Nr in order to make sure that all
        // triangle-edges at a triply-shared edge are set to -2 when such a case is detected.
        for (unsigned long Tri1Nr=0; Tri1Nr<Mesh.Triangles.Size(); Tri1Nr++)
        {
            MeshT::TriangleT& Tri1=Mesh.Triangles[Tri1Nr];

            for (unsigned long v1=0; v1<3; v1++)
            {
                // Note that the neighbour of edge <v1, (v1+1) % 3> is contained in the set of triangles that refer to v1.
                // Note that the Vertices_RefTrisInclDups array *includes* triangles that refer to geometrical duplicates of Mesh.Vertices[Tri1.VertexIdx[v1]].
                for (unsigned long RefNr=0; RefNr<Vertices_RefTrisInclDups[Tri1.VertexIdx[v1]].Size(); RefNr++)
                {
                    const unsigned long Tri2Nr=Vertices_RefTrisInclDups[Tri1.VertexIdx[v1]][RefNr];
                    MeshT::TriangleT&   Tri2  =Mesh.Triangles[Tri2Nr];

                    if (Tri1Nr==Tri2Nr) continue;

                    for (unsigned long v2=0; v2<3; v2++)
                    {
                        // The condition used to be as in the following line, which however does
                        // not take into account that vertices may be geometrical duplicates.
                     // if (Tri1.VertexIdx[v1]==Tri2.VertexIdx[(v2+1) % 3] && Tri1.VertexIdx[(v1+1) % 3]==Tri2.VertexIdx[v2])
                        if (Mesh.AreGeoDups(Tri1.VertexIdx[v1], Tri2.VertexIdx[(v2+1) % 3]) && Mesh.AreGeoDups(Tri1.VertexIdx[(v1+1) % 3], Tri2.VertexIdx[v2]))
                        {
                            // Tri1 and Tri2 are neighbours!
                            if (Tri1.NeighbIdx[v1]==-1)
                            {
                                // Tri1 had no neighbour at this edge before, set it now.
                                // This is the normal case.
                                Tri1.NeighbIdx[v1]=Tri2Nr;
                            }
                            else if (Tri1.NeighbIdx[v1]>=0)
                            {
                                // Tri1 had a single valid neighbour at this edge before, but we just found a second.
                                // That means that three triangles share a common edge!
                                // printf("WARNING: Triangle %lu has two neighbours at edge %lu: triangles %lu and %lu.\n", Tri1Nr, v1, Tri1.NeighbIdx[v1], Tri2Nr);

                                // Re-find the matching edge in the old neighbour.
                                MeshT::TriangleT& Tri3=Mesh.Triangles[Tri1.NeighbIdx[v1]];
                                unsigned long     v3;

                                for (v3=0; v3<2; v3++)      // The  v3<2  instead of  v3<3  is intentional, to be safe that v3 never gets 3 (out-of-range).
                                    if (Mesh.AreGeoDups(Tri1.VertexIdx[v1], Tri3.VertexIdx[(v3+1) % 3]) && Mesh.AreGeoDups(Tri1.VertexIdx[(v1+1) % 3], Tri3.VertexIdx[v3])) break;

                                // Set the shared edge of ALL THREE triangles to -2 in order to indicate that this edge leads to more than one neighbour.
                                Tri3.NeighbIdx[v3]=-2;
                                Tri2.NeighbIdx[v2]=-2;
                                Tri1.NeighbIdx[v1]=-2;
                            }
                            else /* (Tri1.NeighbIdx[v1]==-2) */
                            {
                                // This edge of Tri1 was either determined to be a triply-shared edge by some an earlier neighbour triangle,
                                // or there are even more than two neighbours at this edge...
                                // In any case, be sure to properly flag the relevant edge at the neighbour!
                                Tri2.NeighbIdx[v2]=-2;
                            }
                            break;
                        }
                    }
                }
            }
        }


        // *******************************************************************************************************
        //  For each vertex, determine the polarity of the triangle(s) is belongs to.
        //  If a vertex belongs to two or more triangles with different polarity, the vertex is on a mirror seam.
        //  In this case, the mesh is "split" by adding adding a new GeoDup for that vertex.
        // *******************************************************************************************************

        for (unsigned long TriangleNr=0; TriangleNr<Mesh.Triangles.Size(); TriangleNr++)
        {
            const MeshT::TriangleT& Tri=Mesh.Triangles[TriangleNr];

            // Init each the polarity of each vertex with *some* meaningful value (overwrites with different value possible here)!
            for (unsigned long i=0; i<3; i++)
                Mesh.Vertices[Tri.VertexIdx[i]].Polarity=Tri.Polarity;
        }

        for (unsigned long TriangleNr=0; TriangleNr<Mesh.Triangles.Size(); TriangleNr++)
        {
            const MeshT::TriangleT& Tri=Mesh.Triangles[TriangleNr];

            for (unsigned long i=0; i<3; i++)
                if (Mesh.Vertices[Tri.VertexIdx[i]].Polarity!=Tri.Polarity)
                {
                    // We found a vertex whose index is mentioned in a triangle with positive and in a triangle with negative polarity.
                    // Fix the situation by duplicating the vertex (it will become a GeoDup of the originial vertex).
                    const unsigned int OldVertexNr=Tri.VertexIdx[i];
                    const unsigned int NewVertexNr=Mesh.Vertices.Size();

                    Mesh.Vertices.PushBack(Mesh.Vertices[OldVertexNr]);

                    Mesh.Vertices[OldVertexNr].Polarity=true;
                    Mesh.Vertices[NewVertexNr].Polarity=false;


                    // Fix the GeoDups of all related vertices.
                    ArrayT<unsigned int> AllOldGeoDups=Mesh.Vertices[OldVertexNr].GeoDups;

                    unsigned long InsertPos;

                    for (InsertPos=0; InsertPos<AllOldGeoDups.Size(); InsertPos++)
                        if (AllOldGeoDups[InsertPos]>OldVertexNr)
                            break;

                    AllOldGeoDups.InsertAt(InsertPos, OldVertexNr);

                    for (unsigned long DupNr=0; DupNr<AllOldGeoDups.Size(); DupNr++)
                        Mesh.Vertices[AllOldGeoDups[DupNr]].GeoDups.PushBack(NewVertexNr);

                    Mesh.Vertices[NewVertexNr].GeoDups=AllOldGeoDups;


                    // Fix all the triangles that refer to OldVertexNr.
                    for (unsigned long FixTriNr=0; FixTriNr<Mesh.Triangles.Size(); FixTriNr++)
                    {
                        MeshT::TriangleT& FixTri=Mesh.Triangles[FixTriNr];

                        for (unsigned int VNr=0; VNr<3; VNr++)
                            if (FixTri.VertexIdx[VNr]==OldVertexNr)
                            {
                                // Re-assign the vertex indices so that the polarity of the triangle matches the polarity of the vertex.
                             // if (FixTri.Polarity==Mesh.Vertices[OldVertexNr].Polarity) FixTri.VertexIdx[VNr]=OldVertexNr;    // Already have OldVertexNr here.
                                if (FixTri.Polarity==Mesh.Vertices[NewVertexNr].Polarity) FixTri.VertexIdx[VNr]=NewVertexNr;
                            }
                    }
                }
        }

#ifdef DEBUG
        for (unsigned long TriangleNr=0; TriangleNr<Mesh.Triangles.Size(); TriangleNr++)
        {
            const MeshT::TriangleT& Tri=Mesh.Triangles[TriangleNr];

            for (unsigned int i=0; i<3; i++)
                assert(Mesh.Vertices[Tri.VertexIdx[i]].Polarity==Tri.Polarity);
        }
#endif
    }
}


void CafuModelT::Save(std::ostream& OutStream) const
{
    OutStream << "-- Cafu Model File\n"
              << "-- Written by CaWE, the Cafu World Editor.\n"
              << "Version(" << CMDL_FILE_VERSION << ")\n";


    // *** Write the joints. ***
    OutStream << "\nJoints=\n{\n";

    for (unsigned long JointNr=0; JointNr<m_Joints.Size(); JointNr++)
    {
        const JointT& Joint=m_Joints[JointNr];

        OutStream << "\t"
                  << "{ "
                  << "name=\"" << Joint.Name << "\"; "
                  << "parent=" << Joint.Parent << "; "
                  << "pos={ " << serialize(Joint.Pos) << " }; "
                  << "qtr={ " << serialize(Joint.Qtr) << " }; "
                  << "scale={ " << serialize(Joint.Scale) << " }; "
                  << "},\n";
    }

    OutStream << "}\n";


    // *** Write the meshes. ***
    OutStream << "\nMeshes=\n{\n";

    for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
    {
        const MeshT& Mesh=m_Meshes[MeshNr];

        OutStream << "\t-- Mesh " << MeshNr << "\n";
        OutStream << "\t{\n";

        // Write the mesh name and material.
        OutStream << "\t\t" << "name=\"" << Mesh.Name << "\";\n";
        OutStream << "\t\t" << "Material=\"" << (Mesh.Material ? Mesh.Material->Name : "") << "\";\n";
        OutStream << "\t\t" << "tsMethod=\"" << Mesh.GetTSMethod() << "\";\n";
        OutStream << "\t\t" << "castShadows=" << (Mesh.CastShadows ? "true" : "false") << ";\n";    // Don't rely on the proper locale being set...

        // Write the mesh weights.
        OutStream << "\n\t\t" << "Weights=\n\t\t{\n";
        for (unsigned long WeightNr=0; WeightNr<Mesh.Weights.Size(); WeightNr++)
        {
            const MeshT::WeightT& Weight=Mesh.Weights[WeightNr];

            OutStream << "\t\t\t"
                      << "{ "
                      << "joint=" << Weight.JointIdx << "; "
                      << "weight=" << serialize(Weight.Weight) << "; "
                      << "pos={ " << serialize(Weight.Pos) << " }; "
                      << "},\n";
        }
        OutStream << "\t\t};\n";

        // Write the mesh vertices.
        OutStream << "\n\t\t" << "Vertices=\n\t\t{\n";
        for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
        {
            const MeshT::VertexT& Vertex=Mesh.Vertices[VertexNr];

            OutStream << "\t\t\t"
                      << "{ "
                      << "firstWeight=" << Vertex.FirstWeightIdx << "; "
                      << "numWeights=" << Vertex.NumWeights << "; "
                      << "uv={ " << serialize(Vertex.u) << ", " << serialize(Vertex.v) << " }; "
                      << "},\n";
        }
        OutStream << "\t\t};\n";

        // Write the mesh triangles.
        OutStream << "\n\t\tTriangles=\n\t\t{\n";
        for (unsigned long TriNr=0; TriNr<Mesh.Triangles.Size(); TriNr++)
        {
            const MeshT::TriangleT& Triangle=Mesh.Triangles[TriNr];

            OutStream << "\t\t\t"
                      << "{ "
                      << Triangle.VertexIdx[0] << ", " << Triangle.VertexIdx[1] << ", " << Triangle.VertexIdx[2]
                      << ", sg=" << Triangle.SmoothGroups;
            if (Triangle.SkipDraw) OutStream << ", skipDraw=true";
            OutStream << " "
                      << "},\n";
        }
        OutStream << "\t\t};\n";

        OutStream << "\t},\n";
    }

    OutStream << "}\n";


    // *** Write the skins. ***
    OutStream << "\nSkins=\n{\n";

    for (unsigned long SkinNr=0; SkinNr<m_Skins.Size(); SkinNr++)
    {
        const SkinT& Skin=m_Skins[SkinNr];

        OutStream << "\t{\n"
                  << "\t\tname=\"" << Skin.Name << "\";\n"
                  << "\t\tmaterials={ ";

        for (unsigned long MatNr=0; MatNr<Skin.Materials.Size(); MatNr++)
        {
            OutStream << "\"";
            if (Skin.Materials[MatNr]!=NULL) OutStream << Skin.Materials[MatNr]->Name;
            OutStream << "\"";
            if (MatNr+1 < Skin.Materials.Size()) OutStream << ", ";
        }

        OutStream << " };\n"
                  << "\t},\n";
    }

    OutStream << "}\n";


    // *** Write the GUI fixtures. ***
    OutStream << "\nGuiFixtures=\n{\n";

    for (unsigned long FixNr=0; FixNr<m_GuiFixtures.Size(); FixNr++)
    {
        const GuiFixtureT& GuiFixture=m_GuiFixtures[FixNr];

        OutStream << "\t{\n"
                  << "\t\tname=\"" << GuiFixture.Name << "\";\n"
                  << "\t\tpoints={ " << GuiFixture.Points[0].MeshNr << ", " << GuiFixture.Points[0].VertexNr << ", "
                                     << GuiFixture.Points[1].MeshNr << ", " << GuiFixture.Points[1].VertexNr << ", "
                                     << GuiFixture.Points[2].MeshNr << ", " << GuiFixture.Points[2].VertexNr << " };\n"
                  << "\t\ttrans={ " << GuiFixture.Trans[0] << ", " << GuiFixture.Trans[1] << " };\n"
                  << "\t\tscale={ " << GuiFixture.Scale[0] << ", " << GuiFixture.Scale[1] << " };\n"
                  << "\t},\n";
    }

    OutStream << "}\n";


    // *** Write the animations. ***
    OutStream << "\nAnimations=\n{\n";

    for (unsigned long AnimNr=0; AnimNr<m_Anims.Size(); AnimNr++)
    {
        const AnimT& Anim=m_Anims[AnimNr];

        OutStream << "\t-- Animation " << AnimNr << "\n";
        OutStream << "\t{\n";

        // Write the anim name, FPS and next sequence number.
        OutStream << "\t\t" << "name=\"" << Anim.Name << "\";\n";
        OutStream << "\t\t" << "FPS="    << Anim.FPS << ";\n";
        OutStream << "\t\t" << "next=\"" << Anim.Next << "\";\n";

        // Write the anim joints.
        OutStream << "\n\t\tAnimJoints=\n\t\t{\n";
        for (unsigned long JointNr=0; JointNr<Anim.AnimJoints.Size(); JointNr++)
        {
            const AnimT::AnimJointT& Joint=Anim.AnimJoints[JointNr];

            OutStream << "\t\t\t"
                      << "{ "
                      << "pos={ " << serialize(Joint.DefaultPos) << " }; "
                      << "qtr={ " << serialize(Joint.DefaultQtr) << " }; "
                      << "scale={ " << serialize(Joint.DefaultScale) << " }; "
                      << "flags=" << Joint.Flags << "; "
                      << "firstData=" << Joint.FirstDataIdx << "; "
                      << "},\n";
        }
        OutStream << "\t\t};\n";

        // Write the anim frames.
        OutStream << "\n\t\tFrames=\n\t\t{\n";
        for (unsigned long FrameNr=0; FrameNr<Anim.Frames.Size(); FrameNr++)
        {
            const AnimT::FrameT& Frame=Anim.Frames[FrameNr];

            OutStream << "\t\t\t"
                      << "{ "
                      << "bb={ " << serialize(Frame.BB.Min) << ", " << serialize(Frame.BB.Max) << " };\n"
                      << "\t\t\t  "
                      << "data={ ";
            for (unsigned long i=0; i<Frame.AnimData.Size(); i++)
                OutStream << serialize(Frame.AnimData[i]) << (i+1<Frame.AnimData.Size() ? ", " : " ");
            OutStream << "}; "
                      << "},\n";
        }
        OutStream << "\t\t};\n";

        OutStream << "\t},\n";
    }

    OutStream << "}\n";


    // *** Write the channels. ***
    OutStream << "\nChannels=\n{\n";

    for (unsigned long ChanNr=0; ChanNr<m_Channels.Size(); ChanNr++)
    {
        const ChannelT& Channel=m_Channels[ChanNr];
        bool            IsFirst=true;

        OutStream << "\t{\n"
                  << "\t\tname=\"" << Channel.Name << "\";\n"
                  << "\t\tjoints={ ";

        for (unsigned long JointNr=0; JointNr<m_Joints.Size(); JointNr++)
        {
            if (Channel.IsMember(JointNr))
            {
                if (!IsFirst) OutStream << ", ";
                OutStream << JointNr;
                IsFirst=false;
            }
        }

        OutStream << " };\n"
                  << "\t},\n";
    }

    OutStream << "}\n";


#if 0
    // [No need for global model properties at this time.]
    // *** Write the global properties. ***
    OutStream << "\nProperties=\n{\n";
    OutStream << "\t" << "useGivenTS=" << (m_UseGivenTangentSpace ? "true" : "false") << ";\n";   // Don't rely on the proper locale being set...
 // OutStream << "\t" << "castShadows=" << m_CastShadows << ";\n";
    OutStream << "}\n";
#endif
}


const MaterialT* CafuModelT::GetMaterial(unsigned long MeshNr, int SkinNr) const
{
    assert(MeshNr<m_Meshes.Size());

    if (SkinNr<0 || SkinNr>=int(m_Skins.Size()))
        return m_Meshes[MeshNr].Material;

    if (MeshNr>=m_Skins[SkinNr].Materials.Size())
        return m_Meshes[MeshNr].Material;

    if (!m_Skins[SkinNr].Materials[MeshNr])
        return m_Meshes[MeshNr].Material;

    return m_Skins[SkinNr].Materials[MeshNr];
}


MatSys::RenderMaterialT* CafuModelT::GetRenderMaterial(unsigned long MeshNr, int SkinNr) const
{
    assert(MeshNr<m_Meshes.Size());

    if (SkinNr<0 || SkinNr>=int(m_Skins.Size()))
        return m_Meshes[MeshNr].RenderMaterial;

    if (MeshNr>=m_Skins[SkinNr].RenderMaterials.Size())
        return m_Meshes[MeshNr].RenderMaterial;

    if (!m_Skins[SkinNr].RenderMaterials[MeshNr])
        return m_Meshes[MeshNr].RenderMaterial;

    return m_Skins[SkinNr].RenderMaterials[MeshNr];
}


bool CafuModelT::IsMeshNrOK(const GuiFixtureT& GF, unsigned int PointNr) const
{
    return GF.Points[PointNr].MeshNr < m_Meshes.Size();
}


bool CafuModelT::IsVertexNrOK(const GuiFixtureT& GF, unsigned int PointNr) const
{
    return IsMeshNrOK(GF, PointNr) &&
           GF.Points[PointNr].VertexNr < m_Meshes[GF.Points[PointNr].MeshNr].Vertices.Size();
}


AnimPoseT* CafuModelT::GetSharedPose(IntrusivePtrT<AnimExpressionT> AE) const
{
    m_TEMP_Pose->SetAnimExpr(AE);

    return m_TEMP_Pose;
}


void CafuModelT::Print() const
{
    std::cout << "\nThis is a cmdl Cafu model. FileName: \"" << m_FileName << "\"\n";

    for (unsigned long MeshNr=0; MeshNr<m_Meshes.Size(); MeshNr++)
    {
        const MeshT& Mesh=m_Meshes[MeshNr];

        std::cout << "\n### Mesh " << MeshNr << " ####\n";

        std::cout << "Triangles:\n";
        for (unsigned long TriangleNr=0; TriangleNr<Mesh.Triangles.Size(); TriangleNr++)
        {
            const MeshT::TriangleT& Tri=Mesh.Triangles[TriangleNr];

            std::cout << TriangleNr << ": ";
            std::cout << "Vertices ("   << Tri.VertexIdx[0] << " " << Tri.VertexIdx[1] << " " << Tri.VertexIdx[2] << "), ";
            std::cout << "Neighbours (" << Tri.NeighbIdx[0] << " " << Tri.NeighbIdx[1] << " " << Tri.NeighbIdx[2] << "), ";
            std::cout << "Polarity " << (Tri.Polarity ? '+' : '-');
            std::cout << "\n";
        }

        std::cout << "Vertices:\n";
        for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
        {
            const MeshT::VertexT& Vertex=Mesh.Vertices[VertexNr];

            std::cout << VertexNr << ": ";
            std::cout << "uv (" << Vertex.u << ", " << Vertex.v << "), ";
            std::cout << "wgt: " << Vertex.FirstWeightIdx << " " << Vertex.NumWeights << ", ";
            std::cout << "geodups [";
            for (unsigned long DupNr=0; DupNr<Vertex.GeoDups.Size(); DupNr++)
                std::cout << " " << Vertex.GeoDups[DupNr];
            std::cout << "], ";
            std::cout << "Polarity " << (Vertex.Polarity ? '+' : '-');
            std::cout << "\n";
        }
    }
}
