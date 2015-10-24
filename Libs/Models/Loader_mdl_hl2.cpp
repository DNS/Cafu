/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2014 Carsten Fuchs Software.

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

#include "Loader_mdl_hl2.hpp"
#include "Loader_mdl_hl2_mdl.hpp"
#include "String.hpp"


using namespace HL2mdl;


namespace
{
    uint8_t* GetRawBytes(const std::string& BaseName, const std::string& Ext, ArrayT<uint8_t>& RawBytes)
    {
        const std::string Name   = BaseName + "." + Ext;
        FILE*             InFile = fopen(Name.c_str(), "rb");

        if (!InFile) throw ModelLoaderT::LoadErrorT("Could not open the " + Name + " file.");

        fseek(InFile, 0, SEEK_END); RawBytes.PushBackEmptyExact(ftell(InFile));
        fseek(InFile, 0, SEEK_SET);

        if (fread(&RawBytes[0], RawBytes.Size(), 1, InFile) == 0) { }   // Must check the return value of fread() with GCC 4.3...

        fclose(InFile);
        InFile = NULL;

        return &RawBytes[0];
    }
}


const uint32_t VertexHeaderT::FILE_ID_IDSV = 0x56534449;
const uint32_t VertexHeaderT::FILE_ID_IDCV = 0x56434449;
const uint32_t VertexHeaderT::FILE_VERSION = 4;

const uint32_t StudioHeaderT::FILE_ID_IDST = 0x54534449;
const uint32_t StudioHeaderT::FILE_VERSION = 48;


LoaderHL2mdlT::LoaderHL2mdlT(const std::string& FileName, int Flags)
    : ModelLoaderT(FileName, Flags |
          REMOVE_DEGEN_TRIANGLES |                          // Need this flag in order to pass all assertions in the CafuModelT code.
          REMOVE_UNUSED_VERTICES | REMOVE_UNUSED_WEIGHTS),  // The code below relies on postprocessing removing unused vertices and weights.
      VertexHeader(NULL),
      StudioHeader(NULL)
{
    // 1. Initialize auxiliary variables.
    // **********************************

    if (!cf::String::EndsWith(m_FileName, ".mdl")) throw LoadErrorT("HL2 model file name doesn't end with .mdl");

    const std::string BaseName(m_FileName.c_str(), m_FileName.length() - 4);


    // 2a. Load the vertex data.
    // *************************

    GetRawBytes(BaseName, "vvd", m_VertexData);

    VertexHeader = (VertexHeaderT*)(&m_VertexData[0]);
    // VertexHeader->print(std::cout, "    ");

    if (m_VertexData.Size() < sizeof(VertexHeaderT))
        throw LoadErrorT("Invalid .vvd file header.");

    if (VertexHeader->ID != VertexHeaderT::FILE_ID_IDSV)
        throw LoadErrorT("Vertex file (.vvd) ID is not \"IDSV\".");

    if (VertexHeader->Version != VertexHeaderT::FILE_VERSION)
        throw LoadErrorT("Vertex file (.vvd) version is not 4.");

    assert(sizeof(VertexHeaderT)     == 64);
    assert(sizeof(StudioBoneWeightT) == 16);
    assert(sizeof(StudioVertexT)     == 48);


    // 2b. Load the model data.
    // ************************

    GetRawBytes(BaseName, "mdl", m_ModelData);

    StudioHeader = (StudioHeaderT*)(&m_ModelData[0]);
    // StudioHeader->print(std::cout, "    ");

    if (m_ModelData.Size() < sizeof(StudioHeaderT))
        throw LoadErrorT("Invalid .mdl file header.");

    if (StudioHeader->ID != StudioHeaderT::FILE_ID_IDST)
        throw LoadErrorT("Model file (.mdl) ID is not \"IDST\".");

    if (StudioHeader->Version != StudioHeaderT::FILE_VERSION)
        throw LoadErrorT("Model file (.mdl) version is not 48.");

    if (StudioHeader->FileSize != m_ModelData.Size())
        throw LoadErrorT("Wrong file size indicated in .mdl file header.");   // This helps with detecting wrong data alignment.

    if (StudioHeader->Checksum != VertexHeader->Checksum)
        throw LoadErrorT("The .mdl and .vvd checksums don't match.");
}


void LoaderHL2mdlT::Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan)
{
    // For clarity and better readability, break the loading into three separate functions.
    Load(Joints);
    // Load(Meshes, m_MeshSkinRef);
    // Load(Anims);
}


void LoaderHL2mdlT::Load(ArrayT<CafuModelT::JointT>& Joints) const
{
    Joints.PushBackEmptyExact(StudioHeader->NumBones);

    for (uint32_t BoneNr = 0; BoneNr < StudioHeader->NumBones; BoneNr++)
    {
        const StudioBoneT&  Bone  = StudioHeader->GetBones()[BoneNr];
        CafuModelT::JointT& Joint = Joints[BoneNr];

        // Wow, this is simple!  :-)
        Joint.Name   = Bone.GetName();
        Joint.Parent = Bone.Parent;
        Joint.Pos    = Bone.Pos;
        Joint.Qtr    = Bone.Quat.GetXYZ();
        Joint.Scale  = Vector3fT(1.0f, 1.0f, 1.0f);
    }
}


void LoaderHL2mdlT::Load(ArrayT<CafuModelT::SkinT>& Skins, const MaterialManagerImplT& MaterialMan)
{
}
