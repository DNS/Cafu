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

#include "Loader.hpp"
#include "Math3D/Quaternion.hpp"


ModelLoaderT::LoadErrorT::LoadErrorT(const std::string& Message)
    : std::runtime_error(Message)
{
}


ModelLoaderT::ModelLoaderT(const std::string& FileName)
    : m_FileName(FileName)
{
}


/// An auxiliary function that computes the bounding box for the model with the given
/// joints and meshes at the given anim sequence at the given frame number.
BoundingBox3fT ModelLoaderT::GetBB(const ArrayT<CafuModelT::JointT>& Joints, const ArrayT<CafuModelT::MeshT>& Meshes, const CafuModelT::AnimT& Anim, unsigned long FrameNr) const
{
    BoundingBox3fT         BB;
    static ArrayT<MatrixT> JointMatrices;

    JointMatrices.Overwrite();
    JointMatrices.PushBackEmpty(Joints.Size());

    // This is a modified version of the loop in CafuModelT::UpdateCachedDrawData().
    for (unsigned long JointNr=0; JointNr<Joints.Size(); JointNr++)
    {
        const CafuModelT::AnimT::AnimJointT& AJ=Anim.AnimJoints[JointNr];
        Vector3fT Data_0[3]={ AJ.DefaultPos, AJ.DefaultQtr, AJ.DefaultScale };

        // Determine the position, quaternion and scale at FrameNr.
        unsigned int FlagCount=0;

        for (int i=0; i<9; i++)
        {
            if ((AJ.Flags >> i) & 1)
            {
                Data_0[i/3][i % 3]=Anim.Frames[FrameNr].AnimData[AJ.FirstDataIdx+FlagCount];

                FlagCount++;
            }
        }

        // Compute the matrix that is relative to the parent bone, and finally obtain the absolute matrix for that bone!
        const MatrixT RelMatrix(Data_0[0], cf::math::QuaternionfT::FromXYZ(Data_0[1]), Data_0[2]);
        const CafuModelT::JointT& J=Joints[JointNr];

        JointMatrices[JointNr]=(J.Parent==-1) ? RelMatrix : JointMatrices[J.Parent]*RelMatrix;
    }

    // This is a modified version of the loop in CafuModelT::InitMeshes().
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

            BB+=OutVert;
        }
    }

    return BB;
}


/// An auxiliary function that cleans loaded meshes.
void ModelLoaderT::Clean(ArrayT<CafuModelT::MeshT>& Meshes)
{
    // Remove triangles with zero-length edges.
    // This is especially important because such triangles "connect" two vertices that the CafuModelT code
    // considers as "geometrical duplicates" of each other. That is, a single triangle refers to the same
    // vertex coordinate twice, which triggers related assertions in debug builds.
    for (unsigned long MeshNr=0; MeshNr<Meshes.Size(); MeshNr++)
    {
        CafuModelT::MeshT& Mesh=Meshes[MeshNr];

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
