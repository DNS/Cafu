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
#include "MaterialSystem/Material.hpp"
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

const uint32_t StripsHeaderT::FILE_VERSION = 7;

const uint32_t StudioHeaderT::FILE_ID_IDST = 0x54534449;
const uint32_t StudioHeaderT::FILE_VERSION = 48;


LoaderHL2mdlT::LoaderHL2mdlT(const std::string& FileName, int Flags)
    : ModelLoaderT(FileName, Flags |
          REMOVE_DEGEN_TRIANGLES |                          // Need this flag in order to pass all assertions in the CafuModelT code.
          REMOVE_UNUSED_VERTICES | REMOVE_UNUSED_WEIGHTS),  // The code below relies on postprocessing removing unused vertices and weights.
      VertexHeader(NULL),
      StripsHeader(NULL),
      StudioHeader(NULL)
{
    if (!cf::String::EndsWith(m_FileName, ".mdl"))
        throw LoadErrorT("HL2 model file name doesn't end with .mdl");

    const std::string BaseName(m_FileName.c_str(), m_FileName.length() - 4);


    // Load the vertex data
    // ********************

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


    // Load the strips data
    // ********************

    GetRawBytes(BaseName, "sw.vtx", m_StripsData);

    StripsHeader = (StripsHeaderT*)(&m_StripsData[0]);
    // StripsHeader->print(std::cout, "    ");

    if (m_StripsData.Size() < sizeof(StripsHeaderT))
        throw LoadErrorT("Invalid .vtx file header.");

    if (StripsHeader->Version != StripsHeaderT::FILE_VERSION)
        throw LoadErrorT("Strips file (.vtx) version is not 7.");

    assert(sizeof(StripsHeaderT)  == 36);
    assert(sizeof(vtxBodyPartT)   ==  8);
    assert(sizeof(vtxModelT)      ==  8);
    assert(sizeof(vtxModelLODT)   == 12);
    assert(sizeof(vtxMeshT)       ==  9);
    assert(sizeof(vtxStripGroupT) == 25);
    assert(sizeof(vtxStripT)      == 27);
    assert(sizeof(vtxVertexT)     ==  9);


    // Load the model data
    // *******************

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

 // assert(sizeof(StudioHeaderT)   == 408);
    assert(sizeof(StudioBoneT)     == 216);
    assert(sizeof(StudioBodyPartT) ==  16);
    assert(sizeof(StudioModelT)    == 148);
    assert(sizeof(StudioMeshT)     == 116);
    assert(sizeof(StudioTextureT)  ==  64);


    // More checks and inits
    // *********************

    if (StudioHeader->Checksum != VertexHeader->Checksum)
        throw LoadErrorT("The .mdl and .vvd checksums don't match.");

    if (StudioHeader->Checksum != StripsHeader->Checksum)
        throw LoadErrorT("The .mdl and .vtx checksums don't match.");

    // VertexHeader->FixData();   but with a non-const version of VertexHeader.
    ((VertexHeaderT*)(&m_VertexData[0]))->FixData();


    if (StudioHeader->NumBodyParts != StripsHeader->NumBodyParts)
        throw LoadErrorT("Mismatching number of body parts in the .mdl and .vtx files.");

    if (StripsHeader->NumLODs == 0)     // Compared to vtxModel.NumLODs below.
        throw LoadErrorT("No LODs in the vtx file.");

    for (uint32_t i = 0; i < StudioHeader->NumBodyParts; i++)
    {
        const StudioBodyPartT& BodyPart    = StudioHeader->GetBodyParts()[i];
        const vtxBodyPartT&    vtxBodyPart = StripsHeader->GetBodyParts()[i];

        if (BodyPart.NumModels != vtxBodyPart.NumModels)
            throw LoadErrorT("Mismatching number of models in the .mdl and .vtx files.");

        for (uint32_t j = 0; j < BodyPart.NumModels; j++)
        {
            StudioModelT&    Model    = BodyPart.GetModels()[j];
            const vtxModelT& vtxModel = vtxBodyPart.GetModels()[j];

            if (StripsHeader->NumLODs != vtxModel.NumLODs)    // Checked for != 0 above.
                throw LoadErrorT("Mismatching number of LODs in the vtx file.");

            if (Model.NumMeshes != vtxModel.GetLODs()[0].NumMeshes)
                throw LoadErrorT("Mismatching number of meshes in the .mdl and .vtx files.");

            // Finally no more checks, but some inits here.
            Model.m_Vertices = VertexHeader->GetVertices();
            Model.m_Tangents = VertexHeader->GetTangents();
        }
    }
}


void LoaderHL2mdlT::Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan)
{
    // Load the materials, create/convert them as required.
    const std::string BaseDir(cf::String::GetPath(m_FileName) + "/");
    const std::string BaseName(m_FileName.c_str(), m_FileName.length() - 4);

    m_Materials.Overwrite();

    for (uint32_t TexNr = 0; TexNr < StudioHeader->NumTextures; TexNr++)
    {
        const StudioTextureT& StudioTexture = StudioHeader->GetTextures()[TexNr];

        const char* RelFileName1 = strstr(BaseName.c_str(), "Models");        // Strip the leading "Games/DeathMatch/", if present.
        std::string RelFileName2 = RelFileName1 ? RelFileName1 : BaseName;
        std::string MaterialName = cf::String::Replace(cf::String::StripExt(RelFileName2 + "/" + StudioTexture.GetName()), "\\", "/");

        MaterialT* Material = MaterialMan.GetMaterial(MaterialName);

        if (!Material)
        {
            // If there isn't an appropriately prepared .cmat file (so that MatName is found in MaterialMan),
            // create a substitute using the diffuse texture available in StudioTexture.
            std::string fn = cf::String::StripExt(StudioTexture.GetName());
            fn = cf::String::Replace(fn, "\\", "_");
            fn = cf::String::Replace(fn, "/",  "_");
            fn += std::string("_") + char('a' + (TexNr % 26)) + std::string(".png");  // Make sure that fn is non-empty and (up to a certain extent) unique.

            MaterialT Mat;

            Mat.Name            = MaterialName;
            Mat.DiffMapComp     = MapCompositionT(fn, BaseDir);
            Mat.RedGen          = ExpressionT(ExpressionT::SymbolALRed);
            Mat.GreenGen        = ExpressionT(ExpressionT::SymbolALGreen);
            Mat.BlueGen         = ExpressionT(ExpressionT::SymbolALBlue);
            Mat.meta_EditorSave = true;

            Material = MaterialMan.RegisterMaterial(Mat);

            // If it does not exist yet, create and save the related bitmap.
            FILE* TestFile = fopen((BaseDir + fn).c_str(), "rb");

            if (TestFile == NULL)
            {
                // TODO ...
            }
            else
            {
                fclose(TestFile);
            }
        }

        m_Materials.PushBack(Material);
    }

    // For clarity and better readability, break the loading into three separate functions.
    Load(Joints);
    Load(Meshes);
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


void LoaderHL2mdlT::Load(ArrayT<CafuModelT::MeshT>& Meshes) const
{
    for (uint32_t BodyPartNr = 0; BodyPartNr < StudioHeader->NumBodyParts; BodyPartNr++)
    {
        const StudioBodyPartT& BodyPart    = StudioHeader->GetBodyParts()[BodyPartNr];
        const vtxBodyPartT&    vtxBodyPart = StripsHeader->GetBodyParts()[BodyPartNr];

        // At this time, we only load the first non-empty model of each BodyPart
        // (see end of loop), because the models in a BodyPart are mutually exclusive.
        // See the class documentation of StudioBodyPartT for details.
        for (uint32_t ModelNr = 0; ModelNr < BodyPart.NumModels; ModelNr++)
        {
            const StudioModelT& Model    = BodyPart.GetModels()[ModelNr];
            const vtxModelT&    vtxModel = vtxBodyPart.GetModels()[ModelNr];

            for (uint32_t MeshNr = 0; MeshNr < Model.NumMeshes; MeshNr++)
            {
                Meshes.PushBackEmpty();

                const StudioMeshT& StudioMesh = Model.GetMeshes()[MeshNr];
                const vtxMeshT&    vtxMesh    = vtxModel.GetLODs()[0].GetMeshes()[MeshNr];
                CafuModelT::MeshT& CafuMesh   = Meshes[Meshes.Size() - 1];

                // About StudioMesh.Material, see LoaderHL2mdlT::Load(Skins, ...) for details!
                assert(StudioHeader->NumTextures == StudioHeader->NumSkinRefs);
                assert(StudioHeader->NumTextures == m_Materials.Size());
                assert(StudioMesh.Material < m_Materials.Size());

                CafuMesh.Name = std::string(BodyPart.GetName()) + "/" + Model.Name;
                CafuMesh.Material = m_Materials[StudioMesh.Material];

                for (uint32_t VertexNr = 0; VertexNr < StudioMesh.NumVertices; VertexNr++)
                {
                    const StudioVertexT&       StudioVertex = StudioMesh.GetVertices()[VertexNr];
                    CafuModelT::MeshT::VertexT CafuVertex;

                    CafuVertex.u              = StudioVertex.TexCoord[0];
                    CafuVertex.v              = StudioVertex.TexCoord[1];
                    CafuVertex.FirstWeightIdx = CafuMesh.Weights.Size();
                    CafuVertex.NumWeights     = StudioVertex.BoneWeights.NumBones;
                    CafuVertex.Polarity       = false;  // Don't leave this uninitialized now, it is re-initialized in the CafuModelT code later.

                    CafuMesh.Vertices.PushBack(CafuVertex);

                    for (uint32_t WeightNr = 0; WeightNr < StudioVertex.BoneWeights.NumBones; WeightNr++)
                    {
                        CafuModelT::MeshT::WeightT Weight;

                        Weight.JointIdx = StudioVertex.BoneWeights.Bone[WeightNr];
                        Weight.Weight   = StudioVertex.BoneWeights.Weight[WeightNr];
                        Weight.Pos      = StudioHeader->GetBones()[Weight.JointIdx].PoseToBone.Mul1(StudioVertex.Pos);

                        CafuMesh.Weights.PushBack(Weight);
                    }
                }

                for (uint32_t SGNr = 0; SGNr < vtxMesh.NumStripGroups; SGNr++)
                {
                    const vtxStripGroupT& vtxSG = vtxMesh.GetStripGroups()[SGNr];

                    for (uint32_t StripNr = 0; StripNr < vtxSG.NumStrips; StripNr++)
                    {
                        const vtxStripT& vtxStrip = vtxSG.GetStrips()[StripNr];

                        const uint16_t*   Indices  = vtxSG.GetIndices()  + vtxStrip.IndicesOffset;
                        const vtxVertexT* Vertices = vtxSG.GetVertices() + vtxStrip.VertsOffset;

                        assert(vtxStrip.NumBoneStateChanges == 0);  // What is the meaning of "bone state changes"?

#ifdef DEBUG
                        // Make sure that our assumptions about vtxVertexT instances hold.
                        for (uint32_t VNr = 0; VNr < vtxSG.NumVerts; VNr++)
                        {
                            const vtxVertexT&    vtxVert = vtxSG.GetVertices()[VNr];
                            const StudioVertexT& stVert  = StudioMesh.GetVertices()[vtxVert.Sibling];

                            assert(vtxVert.NumBones == stVert.BoneWeights.NumBones);

                            for (uint32_t i = 0; i < vtxVert.NumBones; i++)
                            {
                                assert(vtxVert.Bone[i] == stVert.BoneWeights.Bone[i]);
                                assert(vtxVert.BoneWeightIndex[i] == i);
                            }
                        }
#endif

                        if (vtxStrip.Flags == vtxStripT::IS_TRILIST)
                        {
                            // A list of triangles.
                            assert(vtxStrip.NumIndices % 3 == 0);

                            for (uint32_t TriOffset = 0; TriOffset+2 < vtxStrip.NumIndices; TriOffset += 3)
                            {
                                CafuModelT::MeshT::TriangleT CafuTri;

                                for (uint32_t i = 0; i < 3; i++)
                                {
                                    const vtxVertexT&    vtxVert = Vertices[Indices[TriOffset + i]];
                                 // const StudioVertexT& stVert  = StudioMesh.GetVertices()[vtxVert.Sibling];

                                    CafuTri.VertexIdx[i] = vtxVert.Sibling;
                                }

                                CafuMesh.Triangles.PushBack(CafuTri);
                            }
                        }
                        else
                        {
                            // A triangles strip.
                            assert(false);  // Well, this has never been tested!

                            for (uint32_t TriOffset = 0; TriOffset+2 < vtxStrip.NumIndices; TriOffset++)
                            {
                                CafuModelT::MeshT::TriangleT CafuTri;

                                for (uint32_t i = 0; i < 3; i++)
                                {
                                    const vtxVertexT&    vtxVert = Vertices[Indices[TriOffset + i]];
                                 // const StudioVertexT& stVert  = StudioMesh.GetVertices()[vtxVert.Sibling];

                                    CafuTri.VertexIdx[i] = vtxVert.Sibling;
                                }

                                if (TriOffset & 1)
                                {
                                    // TriOffset is odd, swap two vertices in order to fix the orientation.
                                    unsigned int swap = CafuTri.VertexIdx[0];

                                    CafuTri.VertexIdx[0] = CafuTri.VertexIdx[1];
                                    CafuTri.VertexIdx[1] = swap;
                                }

                                CafuMesh.Triangles.PushBack(CafuTri);
                            }
                        }
                    }
                }
            }

            // This model was non-empty, so load no further models from the same BodyPart.
            if (Model.NumMeshes) break;
        }
    }
}


/*
 * This big question is what StudioMeshT::Material indexes into. Candidates are:
 *
 *   - StudioHeader->GetTextures()
 *   - StudioHeader->GetSkinRefs()   (offset by skin family)
 *
 * It is conceivable that it only indexes into the SkinRefs, and only those index into the
 * Textures. In this case, the existence of at least one skin family is mandatory.
 *
 * Alternatively, it may index both into the Textures (for the first, default skin) as well
 * as into the SkinRefs (for optional, additional skins). In this case, NumTextures and
 * NumSkinRefs must be equal. This is what we assume in this loader. This implies that we
 * also assume that the default skin is directly stored in the mesh definitions.
 */
void LoaderHL2mdlT::Load(ArrayT<CafuModelT::SkinT>& Skins, const MaterialManagerImplT& MaterialMan)
{
    if (StudioHeader->NumSkinFamilies == 0) return;
    assert(StudioHeader->NumTextures == StudioHeader->NumSkinRefs);

    const uint16_t* SkinRefs0 = StudioHeader->GetSkinRefs();
    uint32_t        FirstSkin = 1;

    // Is skin 0 the identity? This affects the first skin to consider in the loop below.
    for (uint32_t RefNr = 0; RefNr < StudioHeader->NumSkinRefs; RefNr++)
        if (SkinRefs0[RefNr] != RefNr)
            FirstSkin = 0;

    // If skin 0 is the identity, it is the same as the default skin that is directly kept
    // in the mesh definitions and thus can be skipped here. Otherwise, take it as well.
    Skins.PushBackEmptyExact(StudioHeader->NumSkinFamilies - FirstSkin);

    for (uint32_t SkinNr = FirstSkin; SkinNr < StudioHeader->NumSkinFamilies; SkinNr++)
    {
        const uint16_t*    SkinRefs = StudioHeader->GetSkinRefs() + SkinNr * StudioHeader->NumSkinRefs;
        CafuModelT::SkinT& Skin     = Skins[SkinNr - 1];

        Skin.Name = "Skin";

        for (uint32_t RefNr = 0; RefNr < StudioHeader->NumSkinRefs; RefNr++)
        {
            Skin.Materials.PushBack(SkinRefs[RefNr] == RefNr ? NULL : m_Materials[SkinRefs[RefNr]]);
            Skin.RenderMaterials.PushBack(NULL);
        }
    }
}
