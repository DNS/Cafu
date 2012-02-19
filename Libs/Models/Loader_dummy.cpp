/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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
