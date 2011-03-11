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

#include "Loader.hpp"
#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "Math3D/Quaternion.hpp"


ModelLoaderT::LoadErrorT::LoadErrorT(const std::string& Message)
    : std::runtime_error(Message)
{
}


ModelLoaderT::ModelLoaderT(const std::string& FileName)
    : m_FileName(FileName)
{
}


MaterialT* ModelLoaderT::GetMaterialByName(const std::string& MaterialName) const
{
    MaterialT* Material=MaterialManager->GetMaterial(MaterialName);

    if (Material==NULL)
    {
        static ConVarT ModelReplacementMaterial("modelReplaceMat", "meta/model_replacement", 0, "Replacement for unknown materials of models.", NULL);

        Console->Warning("Model \""+m_FileName+"\" refers to unknown material \""+MaterialName+"\". "
            "Replacing the material with \""+ModelReplacementMaterial.GetValueString()+"\".\n");

        Material=MaterialManager->GetMaterial(ModelReplacementMaterial.GetValueString());
    }

    if (Material==NULL)
    {
        Console->Warning("The replacement material is also unknown - the model will NOT render!\n");
    }

    if (Material!=NULL && !Material->LightMapComp.IsEmpty())
    {
        Console->Warning("Model \""+m_FileName+"\" uses material \""+MaterialName+"\", which in turn has lightmaps defined.\n"
            "It will work in the ModelViewer, but for other applications like Cafu itself you should use a material without lightmaps.\n");
            // It works in the ModelViewer because the ModelViewer is kind enough to provide a default lightmap...
    }

    return Material;
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
