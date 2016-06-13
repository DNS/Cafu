/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Loader_dummy.hpp"
#include "MaterialSystem/Material.hpp"
#include "Math3D/Angles.hpp"


LoaderDummyT::LoaderDummyT(const std::string& FileName, int Flags)
    : ModelLoaderT(FileName, Flags)
{
}


namespace
{
    void AddVertex(CafuModelT::MeshT& Mesh, const Vector3fT& Pos, float u, float v)
    {
        Mesh.Weights.PushBackEmpty();
        CafuModelT::MeshT::WeightT& Weight=Mesh.Weights[Mesh.Weights.Size()-1];

        Weight.JointIdx=0;
        Weight.Weight  =0.0f;
        Weight.Pos     =Pos;

        Mesh.Vertices.PushBackEmpty();
        CafuModelT::MeshT::VertexT& Vertex=Mesh.Vertices[Mesh.Vertices.Size()-1];

        Vertex.u             =u;
        Vertex.v             =v;
        Vertex.FirstWeightIdx=Mesh.Weights.Size()-1;
        Vertex.NumWeights    =1;
    }
}


void LoaderDummyT::Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan)
{
    // Create a default "identity" joint.
    // That single joint is used for (shared by) all weights of all meshes.
    Joints.PushBackEmpty();

    Joints[0].Name  ="root";
    Joints[0].Parent=-1;
 // Joints[0].Pos   =Vector3fT();
 // Joints[0].Qtr   =Vector3fT();   // Identity quaternion...
    Joints[0].Scale =Vector3fT(1.0f, 1.0f, 1.0f);

    Meshes.PushBackEmpty();
    CafuModelT::MeshT& Mesh=Meshes[0];

    Mesh.Name    ="cone";
    Mesh.Material=MaterialMan.RegisterMaterial(CreateDefaultMaterial("dummy"));

    const float        Height=64.0f;
    const float        Radius=16.0f;
    const unsigned int Facets=5;

    AddVertex(Mesh, Vector3fT(0, 0, Height), 0, 0);

    for (unsigned int i=0; i<Facets; i++)
    {
        const float f=float(i) / Facets;
        const float a=float(2.0 * cf::math::AnglesT<double>::PI) * f;

        AddVertex(Mesh, Vector3fT(cos(a)*Radius, sin(a)*Radius, 0), f, 1.0f);
    }

    for (unsigned int i=0; i<Facets; i++)
    {
        Mesh.Triangles.PushBack(CafuModelT::MeshT::TriangleT(0, ((i+1) % Facets)+1, i+1));
        Mesh.Triangles[i].SmoothGroups=0x01;
    }

    for (unsigned int i=2; i<Facets; i++)
    {
        Mesh.Triangles.PushBack(CafuModelT::MeshT::TriangleT(1, i, i+1));
    }
}


void LoaderDummyT::Load(ArrayT<CafuModelT::SkinT>& Skins, const MaterialManagerImplT& MaterialMan)
{
}


void LoaderDummyT::Load(ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures)
{
}


void LoaderDummyT::Load(ArrayT<CafuModelT::ChannelT>& Channels)
{
}


bool LoaderDummyT::Load(unsigned int Level, CafuModelT*& DlodModel, float& DlodDist)
{
    return false;
}
