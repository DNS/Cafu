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

#include "Loader_obj.hpp"
#include "TextParser/TextParser.hpp"


static Vector3fT ReadVec(TextParserT& TP, unsigned int NumComponents=3)
{
    Vector3fT v;

    v.x=TP.GetNextTokenAsFloat(); if (NumComponents==1) return v;
    v.y=TP.GetNextTokenAsFloat(); if (NumComponents==2) return v;
    v.z=TP.GetNextTokenAsFloat();

    return v;
}


static LoaderObjT::ObjFaceVertexT ReadFaceVert(const std::string& VertexDef)
{
    LoaderObjT::ObjFaceVertexT FaceVert;
    TextParserT                TP(VertexDef.c_str(), "/", false /*IsFileName*/, '#');

    FaceVert.VertexNr=TP.GetNextTokenAsInt();

    try
    {
        TP.AssertAndSkipToken("/");

        if (TP.PeekNextToken()!="/")
            FaceVert.TexCoordNr=TP.GetNextTokenAsInt();

        TP.AssertAndSkipToken("/");
        FaceVert.NormalNr=TP.GetNextTokenAsInt();
    }
    catch (const TextParserT::ParseError& /*PE*/) { }

    return FaceVert;
}


LoaderObjT::LoaderObjT(const std::string& FileName) /*throw (ModelT::LoadError)*/
    : ModelLoaderT(FileName)
{
    m_NumShadedFaces[0]=0;  // Counts the "fixed" shaded
    m_NumShadedFaces[1]=0;  // and the smooth shaded faces.

    // Create one default mesh. Unused meshes are generally removed again in Load().
    m_ObjMeshes.PushBackEmpty();
    m_ObjMeshes[0].MtlName="default";

    bool        UseSmoothShading=false;
    TextParserT TP(FileName.c_str(), "", true /*IsFileName*/, '#');

    while (!TP.IsAtEOF())
    {
        const std::string Type=TP.GetNextToken();

        if (Type=="v")
        {
            m_ObjVertices.PushBack(ReadVec(TP));
            TP.SkipLine();
        }
        else if (Type=="vt")
        {
            m_ObjTexCoords.PushBack(ReadVec(TP, 2));
            TP.SkipLine();
        }
        else if (Type=="vn")
        {
            m_ObjNormals.PushBack(normalizeOr0(ReadVec(TP)));
            TP.SkipLine();
        }
        else if (Type=="f")
        {
            TextParserT LineParser(TP.SkipLine().c_str(), "", false /*IsFileName*/, '#');
            ObjMeshT&   ObjMesh=m_ObjMeshes[m_ObjMeshes.Size()-1];  // There is always at least one mesh (see above).

            ObjMesh.Faces.PushBackEmpty();

            while (!LineParser.IsAtEOF())
            {
                ObjFaceVertexT FV=ReadFaceVert(LineParser.GetNextToken());

                // "Fix" negative indices according to the specs (the "+1" is because the indices are still 1-based).
                if (FV.VertexNr  <0) FV.VertexNr  =m_ObjVertices.Size() +FV.VertexNr  +1;
                if (FV.TexCoordNr<0) FV.TexCoordNr=m_ObjTexCoords.Size()+FV.TexCoordNr+1;
                if (FV.NormalNr  <0) FV.NormalNr  =m_ObjNormals.Size()  +FV.NormalNr  +1;

                ObjMesh.Faces[ObjMesh.Faces.Size()-1].PushBack(FV);
            }

            m_NumShadedFaces[UseSmoothShading ? 1 : 0]++;
        }
        else if (Type=="mtllib")
        {
            const std::string MtlLibName=TP.GetNextToken();

            // TODO: Parse the materials in the mtllib and create Cafu Materials that match them reasonably close.
        }
        else if (Type=="usemtl")
        {
            // "usemtl" is a good opportunity to start a new mesh that takes the subsequent faces:
            // Our Cafu-specific code requires a new mesh for a new material anyways,
            // and if the same material is specified with "usemtl" multiple times, we use that for naturally splitting
            // the single large mesh into smaller groups of meshes, assuming that that was what the model creator intended.
            // As a side effect, we don't have to deal with data types "o" and "g" at all, and reasonably dealing with "s"
            // (smoothing groups) becomes simpler as well.
            m_ObjMeshes.PushBackEmpty();
            m_ObjMeshes[m_ObjMeshes.Size()-1].MtlName=TP.GetNextToken();
        }
        else if (Type=="o")
        {
            const std::string ObjectName=TP.GetNextToken();
        }
        else if (Type=="g")
        {
            const std::string GroupName=TP.GetNextToken();
        }
        else if (Type=="s")
        {
            const std::string SmoothGroup=TP.GetNextToken();

            UseSmoothShading=(SmoothGroup!="0" && SmoothGroup!="off");
        }
        else
        {
            // A data type that we don't know or don't support.
            TP.SkipLine();
        }
    }
}


bool LoaderObjT::UseGivenTS() const
{
    if (m_NumShadedFaces[1] > m_NumShadedFaces[0])
    {
        // There are more faces in smoothing groups than not, so assume that smoothing is desired:
        // Don't use the given tangent-space, but have the model code compute it.
        return false;
    }

    // There are more faces in "fixed" tangent-space groups than in smoothing groups.
    return true;
}


static unsigned int FindWeight(CafuModelT::MeshT& Mesh, const Vector3fT& Pos)
{
    unsigned int WeightNr;

    for (WeightNr=0; WeightNr<Mesh.Weights.Size(); WeightNr++)
        if (Mesh.Weights[WeightNr].Pos==Pos)
            return WeightNr;

    Mesh.Weights.PushBackEmpty();
    Mesh.Weights[WeightNr].JointIdx=0;
    Mesh.Weights[WeightNr].Weight  =1.0f;
    Mesh.Weights[WeightNr].Pos     =Pos;

    return WeightNr;
}


static unsigned int FindVertex(CafuModelT::MeshT& Mesh, unsigned int WeightIdx, const Vector3fT& TexCoord, const Vector3fT& Normal)
{
    unsigned int VertexNr;

    for (VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
        if (Mesh.Vertices[VertexNr].u             ==TexCoord.x &&
            Mesh.Vertices[VertexNr].v             ==TexCoord.y &&
            Mesh.Vertices[VertexNr].FirstWeightIdx==WeightIdx &&
            Mesh.Vertices[VertexNr].Draw_Normal   ==Normal)
            return VertexNr;

    Mesh.Vertices.PushBackEmpty();
    Mesh.Vertices[VertexNr].u             =TexCoord.x;
    Mesh.Vertices[VertexNr].v             =TexCoord.y;
    Mesh.Vertices[VertexNr].FirstWeightIdx=WeightIdx;
    Mesh.Vertices[VertexNr].NumWeights    =1;
    Mesh.Vertices[VertexNr].Draw_Normal   =Normal;

    return VertexNr;
}


void LoaderObjT::Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims)
{
    // Create a default "identity" joint.
    // That single joint is used for (shared by) all weights of all meshes.
    Joints.PushBackEmpty();

    Joints[0].Name  ="root";
    Joints[0].Parent=-1;
 // Joints[0].Pos   =Vector3fT();
 // Joints[0].Qtr   =Vector3fT();   // Identity quaternion...

    // Remove empty meshes from the m_ObjMeshes array.
    for (unsigned long MeshNr=0; MeshNr<m_ObjMeshes.Size(); MeshNr++)
        if (m_ObjMeshes[MeshNr].Faces.Size()==0)
        {
            m_ObjMeshes.RemoveAt(MeshNr);
            MeshNr--;
        }

    Meshes.PushBackEmptyExact(m_ObjMeshes.Size());

    for (unsigned long MeshNr=0; MeshNr<m_ObjMeshes.Size(); MeshNr++)
    {
        const ObjMeshT&    ObjMesh=m_ObjMeshes[MeshNr];
        CafuModelT::MeshT& Mesh   =Meshes[MeshNr];

        Mesh.Material=GetMaterialByName("Models/Players/Alien/Alien" /*ObjMesh.MtlName*/);    // TODO...!

        for (unsigned long FaceNr=0; FaceNr<ObjMesh.Faces.Size(); FaceNr++)
        {
            const ArrayT<ObjFaceVertexT>& Face=ObjMesh.Faces[FaceNr];
            ArrayT<unsigned int>          CafuVertexIndices;

            // For each ObjFaceVertexT, determine the related CafuModelT::MeshT::Vertex
            // (and build the Mesh.Vertices and Mesh.Weights arrays while doing so).
            for (unsigned long FVNr=0; FVNr<Face.Size(); FVNr++)
            {
                const ObjFaceVertexT& FV=Face[FVNr];

                const unsigned int WeightIdx=FindWeight(Mesh, m_ObjVertices[FV.VertexNr-1]);
                const unsigned int VertexIdx=FindVertex(Mesh, WeightIdx,
                    FV.TexCoordNr>0 ? m_ObjTexCoords[FV.TexCoordNr-1] : Vector3fT(),
                    FV.NormalNr>0   ? m_ObjNormals  [FV.NormalNr  -1] : Vector3fT());

                CafuVertexIndices.PushBack(VertexIdx);
            }

            for (unsigned long TriNr=0; TriNr+2<Face.Size(); TriNr++)
            {
                CafuModelT::MeshT::TriangleT Tri;

                Tri.VertexIdx[0]=CafuVertexIndices[0];
                Tri.VertexIdx[1]=CafuVertexIndices[TriNr+2];
                Tri.VertexIdx[2]=CafuVertexIndices[TriNr+1];

                Mesh.Triangles.PushBack(Tri);
            }
        }

        // Remove triangles with zero-length edges.
        // This is especially important because such triangles "connect" two vertices that the CafuModelT code
        // considers as "geometrical duplicates" of each other. That is, a single triangle refers to the same
        // vertex coordinate twice, which triggers related assertions in debug builds.
        for (unsigned long TriNr=0; TriNr<Mesh.Triangles.Size(); TriNr++)
        {
            const CafuModelT::MeshT::TriangleT& Tri=Mesh.Triangles[TriNr];

            const Vector3fT& A=Mesh.Weights[Mesh.Vertices[Tri.VertexIdx[0]].FirstWeightIdx].Pos;
            const Vector3fT& B=Mesh.Weights[Mesh.Vertices[Tri.VertexIdx[1]].FirstWeightIdx].Pos;
            const Vector3fT& C=Mesh.Weights[Mesh.Vertices[Tri.VertexIdx[2]].FirstWeightIdx].Pos;

            if (length(A-B)==0 || length(B-C)==0 || length(C-A)==0)
            {
                Mesh.Triangles.RemoveAt(TriNr);
                TriNr--;
            }
        }
    }
}
