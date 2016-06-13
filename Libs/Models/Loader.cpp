/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Loader.hpp"
#include "MaterialSystem/Material.hpp"
#include "Math3D/Quaternion.hpp"


ModelLoaderT::LoadErrorT::LoadErrorT(const std::string& Message)
    : std::runtime_error(Message)
{
}


ModelLoaderT::ModelLoaderT(const std::string& FileName, int Flags)
    : m_FileName(FileName),
      m_Flags(Flags)
{
}


void ModelLoaderT::Postprocess(ArrayT<CafuModelT::MeshT>& Meshes)
{
    for (unsigned long MeshNr=0; MeshNr<Meshes.Size(); MeshNr++)
    {
        if (m_Flags & REMOVE_DEGEN_TRIANGLES   ) RemoveDegenTriangles   (Meshes[MeshNr]);
        if (m_Flags & REMOVE_UNUSED_VERTICES   ) RemoveUnusedVertices   (Meshes[MeshNr]);
     // if (m_Flags & ABANDON_DUPLICATE_WEIGHTS) AbandonDuplicateWeights(Meshes[MeshNr]);
        if (m_Flags & REMOVE_UNUSED_WEIGHTS    ) RemoveUnusedWeights    (Meshes[MeshNr]);
    }
}


void ModelLoaderT::RemoveDegenTriangles(CafuModelT::MeshT& Mesh)
{
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


void ModelLoaderT::RemoveUnusedVertices(CafuModelT::MeshT& Mesh)
{
    ArrayT<CafuModelT::MeshT::TriangleT>& Triangles=Mesh.Triangles;
    ArrayT<CafuModelT::MeshT::VertexT>&   Vertices =Mesh.Vertices;

    ArrayT<bool> Used;
    Used.PushBackEmptyExact(Vertices.Size());
    for (unsigned long VNr=0; VNr<Vertices.Size(); VNr++) Used[VNr]=false;

    for (unsigned long TriNr=0; TriNr<Triangles.Size(); TriNr++)
        for (unsigned int i=0; i<3; i++)
            Used[Triangles[TriNr].VertexIdx[i]]=true;

    for (unsigned long VNr=0; VNr<Vertices.Size(); VNr++)
    {
        if (Used[VNr]) continue;

        // The vertex at VNr is not used: Fix references and remove it from the array.
        for (unsigned long TriNr=0; TriNr<Triangles.Size(); TriNr++)
            for (unsigned int i=0; i<3; i++)
                if (Triangles[TriNr].VertexIdx[i] > VNr)
                    Triangles[TriNr].VertexIdx[i]--;

        Vertices.RemoveAtAndKeepOrder(VNr);
        Used.RemoveAtAndKeepOrder(VNr);
        VNr--;
    }
}


void ModelLoaderT::AbandonDuplicateWeights(CafuModelT::MeshT& Mesh)
{
    ArrayT<CafuModelT::MeshT::VertexT>& Vertices=Mesh.Vertices;

    for (unsigned long V1Nr=0; V1Nr<Vertices.Size(); V1Nr++)
        for (unsigned long V2Nr=V1Nr+1; V2Nr<Vertices.Size(); V2Nr++)
            if (Mesh.AreGeoDups(V1Nr, V2Nr))
                Vertices[V2Nr].FirstWeightIdx=Vertices[V1Nr].FirstWeightIdx;
}


void ModelLoaderT::RemoveUnusedWeights(CafuModelT::MeshT& Mesh)
{
    ArrayT<CafuModelT::MeshT::VertexT>& Vertices=Mesh.Vertices;
    ArrayT<CafuModelT::MeshT::WeightT>& Weights =Mesh.Weights;

    ArrayT<bool> Used;
    Used.PushBackEmptyExact(Weights.Size());
    for (unsigned long WNr=0; WNr<Weights.Size(); WNr++) Used[WNr]=false;

    for (unsigned long VNr=0; VNr<Vertices.Size(); VNr++)
        for (unsigned int i=0; i<Vertices[VNr].NumWeights; i++)
            Used[Vertices[VNr].FirstWeightIdx + i]=true;

    for (unsigned long WNr=0; WNr<Weights.Size(); WNr++)
    {
        if (Used[WNr]) continue;

        // The weight at WNr is not used: Fix references and remove it from the array.
        for (unsigned long VNr=0; VNr<Vertices.Size(); VNr++)
        {
            // We can safely assume that the unused weight is outside the span of weights referenced by our vertices.
            if (Vertices[VNr].FirstWeightIdx > WNr)
                Vertices[VNr].FirstWeightIdx--;
        }

        Weights.RemoveAtAndKeepOrder(WNr);
        Used.RemoveAtAndKeepOrder(WNr);
        WNr--;
    }
}


MaterialT ModelLoaderT::CreateDefaultMaterial(const std::string& MatName, bool EditorSave) const
{
    MaterialT Mat;

    Mat.Name           =MatName;
    Mat.PolygonMode    =MaterialT::Wireframe;
 // Mat.DepthOffset    =-1.0f;
    Mat.DiffMapComp    =MapCompositionT("file-not-found", "./");
    Mat.RedGen         =ExpressionT(ExpressionT::SymbolALRed);      // Modulate the model texture with the ambient light color.
    Mat.GreenGen       =ExpressionT(ExpressionT::SymbolALGreen);
    Mat.BlueGen        =ExpressionT(ExpressionT::SymbolALBlue);
 // Mat.UseMeshColors  =true;   // Mesh colors aren't used for rendering models, but unfortunately required for rendering wire-frame without diffuse-map texture image.
    Mat.TwoSided       =true;   // For wire-frame, render the backsides as well.
    Mat.meta_EditorSave=EditorSave;

    return Mat;
}


AnimImporterT::AnimImporterT(const std::string& FileName)
    : m_FileName(FileName)
{
}
