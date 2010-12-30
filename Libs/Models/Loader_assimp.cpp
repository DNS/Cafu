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

#include "Loader_assimp.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/Misc.hpp"

#include "assimp.hpp"       // C++ importer interface
#include "aiScene.h"        // Output data structure
#include "aiPostProcess.h"  // Post processing flags

#include <map>


LoaderAssimpT::LoaderAssimpT(const std::string& FileName) /*throw (ModelT::LoadError)*/
    : ModelLoaderT(FileName),
      m_Scene(NULL)
{
}


bool LoaderAssimpT::UseGivenTS() const
{
    // TODO...
    // Return true, if the mesh comes with predefined normals (and has no animations?),
    // and false otherwise (no normals given in original model file, or there are animations that require per-frame recomputing of tangent-space).
    return false;
}


void LoaderAssimpT::Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims)
{
    Assimp::Importer Importer;

    m_Scene=Importer.ReadFile(m_FileName,
        aiProcess_CalcTangentSpace       |      // TODO: I DON'T THINK THAT WE REALLY WANT THIS...
        aiProcess_Triangulate            |      // Decompose polygons into triangles.
        aiProcess_JoinIdenticalVertices  |
        aiProcess_SortByPType);                 // Only one type of face primitives (points, lines, triangles or polygons) per mesh.

    // TODO: Auto-drop point and line primitives? See http://assimp.sourceforge.net/lib_html/structai_face.html

    if (!m_Scene)
        throw LoadErrorT(std::string("Asset Import: ") + Importer.GetErrorString());


    Load(Joints, -1, m_Scene->mRootNode);
    Load(Meshes, m_Scene->mRootNode, Joints);

    for (unsigned int MeshNr=0; MeshNr<m_Scene->mNumMeshes; MeshNr++)
    {
        // const aiMesh* Mesh=m_Scene->mMeshes[MeshNr];
    }

    Anims.PushBackEmptyExact(m_Scene->mNumAnimations);
    for (unsigned int AnimNr=0; AnimNr<m_Scene->mNumAnimations; AnimNr++)
        Load(Anims[AnimNr], m_Scene->mAnimations[AnimNr], Joints);

    m_Scene=NULL;
}


/// Recursively loads the joints, beginning at the given aiNode instance and the given parent index.
void LoaderAssimpT::Load(ArrayT<CafuModelT::JointT>& Joints, int ParentIndex, const aiNode* Node)
{
    aiVector3D   Scaling;
    aiQuaternion Quaternion;
    aiVector3D   Translation;

	Node->mTransformation.Decompose(Scaling, Quaternion, Translation);

    // TODO: If Scaling.x|y|z < 0.99 or > 1.01 then log warning.
    // TODO: If Quaternion.magnitude != 1.0 then log warning.
    // TODO: Do we need the conjugate?
    Quaternion.Normalize();

    CafuModelT::JointT Joint;

    Joint.Name  =Node->mName.data;
    Joint.Parent=ParentIndex;
    Joint.Pos   =Vector3fT(Translation.x, Translation.y, Translation.z);
    Joint.Qtr   =Vector3fT(Quaternion.x, Quaternion.y, Quaternion.z);

    Joints.PushBack(Joint);

    for (unsigned int ChildNr=0; ChildNr<Node->mNumChildren; ChildNr++)
        Load(Joints, Joints.Size()-1, Node->mChildren[ChildNr]);
}


/// Recursively loads the static, non-animated meshes as mentioned in the m_Scene->mRootNode hierarchy.
void LoaderAssimpT::Load(ArrayT<CafuModelT::MeshT>& Meshes, const aiNode* AiNode, const ArrayT<CafuModelT::JointT>& Joints)
{
    unsigned long JointNr;

    // Find the joint for AiNode.
    for (JointNr=0; JointNr<Joints.Size(); JointNr++)
        if (Joints[JointNr].Name==AiNode->mName.data)
            break;

    assert(JointNr<Joints.Size());
    if (JointNr<Joints.Size())
    {
        for (unsigned int MeshNr=0; MeshNr<AiNode->mNumMeshes; MeshNr++)
        {
            const aiMesh* AiMesh=m_Scene->mMeshes[AiNode->mMeshes[MeshNr]];
            const bool    HaveUV=(AiMesh->mTextureCoords[0]!=NULL);

            // Meshes that are (bone-)animated are loaded elsewhere.
            // (We could instantiate the bind pose in the coordinate system of AiNode here, but that's barely meaningful.)
            if (AiMesh->HasBones()) continue;

            // Ignore meshes that are not made (only) of triangles.
            // This should never trigger though, as per the Assimp postprocessing steps.
            if (!AiMesh->HasPositions() || AiMesh->mPrimitiveTypes!=aiPrimitiveType_TRIANGLE) continue;

            CafuModelT::MeshT Mesh;

            Mesh.Material=NULL;             // TODO!
            Mesh.RenderMaterial=NULL;       // TODO!
            Mesh.Weights.PushBackEmptyExact(AiMesh->mNumVertices);
            Mesh.Vertices.PushBackEmptyExact(AiMesh->mNumVertices);
            Mesh.Triangles.PushBackEmptyExact(AiMesh->mNumFaces);

            for (unsigned int VertexNr=0; VertexNr<AiMesh->mNumVertices; VertexNr++)
            {
                const aiVector3D& aiVec=AiMesh->mVertices[VertexNr];

                Mesh.Weights[VertexNr].JointIdx=JointNr;
                Mesh.Weights[VertexNr].Weight  =1.0f;
                Mesh.Weights[VertexNr].Pos     =Vector3fT(aiVec.x, aiVec.y, aiVec.z);

                Mesh.Vertices[VertexNr].u=HaveUV ? AiMesh->mTextureCoords[0][VertexNr].x : 0.0f;
                Mesh.Vertices[VertexNr].v=HaveUV ? AiMesh->mTextureCoords[0][VertexNr].y : 0.0f;
                Mesh.Vertices[VertexNr].FirstWeightIdx=VertexNr;
                Mesh.Vertices[VertexNr].NumWeights    =1;
            }

            for (unsigned int TriNr=0; TriNr<AiMesh->mNumFaces; TriNr++)
            {
                assert(AiMesh->mFaces[TriNr].mNumIndices==3);

                for (unsigned int i=0; i<3; i++)
                    Mesh.Triangles[TriNr].VertexIdx[i]=AiMesh->mFaces[TriNr].mIndices[i];
            }

            Meshes.PushBack(Mesh);
        }
    }

    for (unsigned int ChildNr=0; ChildNr<AiNode->mNumChildren; ChildNr++)
        Load(Meshes, AiNode->mChildren[ChildNr], Joints);
}


/// Returns all "ticks" for the given aiAnimation as a std::map of the form (t, u),
/// where t is the time (in ticks), and u is the number of "uses" of this tick.
/// Note that our focus is on the (sorted) t values, whereas u is mostly waiting for future use.
/* static std::map<double, unsigned int> GetAllTicks(const aiAnimation* AiAnim)
{
    std::map<double, unsigned int> AllTicks;

    for (unsigned int ChannelNr=0; ChannelNr<AiAnim->mNumChannels; ChannelNr++)
    {
        const aiNodeAnim* AiAnimNode=AiAnim->mChannels[ChannelNr];

        for (unsigned int KeyNr=0; KeyNr<AiAnimNode->mNumPositionKeys; KeyNr++)
            AllTicks[cf::math::round(AiAnimNode->mPositionKeys[KeyNr].mTime*100.0)/100.0]++;

        for (unsigned int KeyNr=0; KeyNr<AiAnimNode->mNumRotationKeys; KeyNr++)
            AllTicks[cf::math::round(AiAnimNode->mRotationKeys[KeyNr].mTime*100.0)/100.0]++;

        for (unsigned int KeyNr=0; KeyNr<AiAnimNode->mNumScalingKeys; KeyNr++)
            AllTicks[cf::math::round(AiAnimNode->mScalingKeys[KeyNr].mTime*100.0)/100.0]++;
    }

    return AllTicks;
} */


void LoaderAssimpT::Load(CafuModelT::AnimT& CafuAnim, const aiAnimation* AiAnim, const ArrayT<CafuModelT::JointT>& Joints)
{
    // CafuAnim.Name=AiAnim.mName.data;
    CafuAnim.FPS=1.2345f;       // TODO!

    CafuAnim.AnimJoints.PushBackEmptyExact(Joints.Size());

    for (unsigned long JointNr=0; JointNr<Joints.Size(); JointNr++)
    {
      //CafuModelT::AnimT::AnimJointT& AnimJoint =CafuAnim.AnimJoints[JointNr];
        const aiNodeAnim*              AiAnimNode=NULL;

        // Find the related aiNodeAnim in AiAnim.mChannels:
        for (unsigned int ChannelNr=0; ChannelNr<AiAnim->mNumChannels; ChannelNr++)
            if (Joints[JointNr].Name == AiAnim->mChannels[ChannelNr]->mNodeName.data)
            {
                AiAnimNode=AiAnim->mChannels[ChannelNr];
                break;
            }
    }
}
