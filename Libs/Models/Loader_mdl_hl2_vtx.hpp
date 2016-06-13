/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_HL2_MDL_VTX_STRIPS_DATA_HPP_INCLUDED
#define CAFU_HL2_MDL_VTX_STRIPS_DATA_HPP_INCLUDED

#if defined(_MSC_VER)
    #pragma pack(push, 1)
    #define GCC_PACKED
#elif defined(__GNUG__)
    #define GCC_PACKED __attribute__ ((packed))
#endif


namespace HL2mdl
{
    struct vtxBoneStateChangeT
    {
        uint32_t HardwareID;
        uint32_t NewBoneID;
    } GCC_PACKED;


    /// This class corresponds to StudioVertexT in the vvd file.
    /// It works like this: If we have these instances:
    ///
    ///     vtxVertexT  vtxVertex;
    ///     vtxMeshT    vtxMesh;    // The mesh related to vtxVertex.
    ///     StudioMeshT StudioMesh; // The mesh sibling related to vtxMesh.
    ///
    /// then the StudioVertexT related to vtxVertex is found like this:
    ///
    ///     StudioVertexT sv = StudioMesh.GetVertices()[vtxVert.Sibling];
    ///
    struct vtxVertexT
    {
        uint8_t  BoneWeightIndex[HL2mdl_MAX_NUM_BONES_PER_VERT];  ///< These index into `sv`'s bones. Can this ever be different from `[0, 1, 2]`? If so, why?
        uint8_t  NumBones;      ///< Same as `sv.BoneWeights.NumBones`.
        uint16_t Sibling;       ///< See class description for usage.

        /// For software-skinned vertices, these are indices into the mdl file's list of
        /// bones, that is, same as `sv.BoneWeights.Bone[i]` for each `i < NumBones`.
        /// For hardware-skinned vertices, these are hardware bone indices (different from
        /// software-skinned vertices if the hardware limits the max number of bones??).
        uint8_t  Bone[HL2mdl_MAX_NUM_BONES_PER_VERT];


        std::ostream& print(std::ostream& os, const char* indent) const;
    } GCC_PACKED;


    /// A strip is a portion of a strip group.
    struct vtxStripT
    {
        enum FlagsT
        {
            IS_TRILIST  = 1,
            IS_TRISTRIP = 2     ///< May contain degenerate triangles.
        };

        uint32_t NumIndices;
        uint32_t IndicesOffset;

        uint32_t NumVerts;
        uint32_t VertsOffset;

        uint16_t NumBones;      ///< What's this? The max. number of bones to be used, clamping/overriding the value in vtxVertexT?
        uint8_t  Flags;

        uint32_t NumBoneStateChanges;
        uint32_t BoneStateChangesOffset;


        vtxBoneStateChangeT* GetBoneStateChanges() const
        {
            return (vtxBoneStateChangeT*)(((uint8_t*)this) + BoneStateChangesOffset);
        }

        std::ostream& print(std::ostream& os, const char* indent) const;
    } GCC_PACKED;


    /// A strip group is a locking group, representing a single vertex buffer and a single index buffer.
    struct vtxStripGroupT
    {
        enum FlagsT { IS_FLEXED = 1, IS_HWSKINNED = 2, IS_DELTA_FLEXED = 4 };

        uint32_t NumVerts;
        uint32_t VertsOffset;

        uint32_t NumIndices;
        uint32_t IndicesOffset;

        uint32_t NumStrips;
        uint32_t StripsOffset;

        uint8_t  Flags;


        vtxVertexT* GetVertices() const
        {
            return (vtxVertexT*)(((uint8_t*)this) + VertsOffset);
        }

        uint16_t* GetIndices() const
        {
            return (uint16_t*)(((uint8_t*)this) + IndicesOffset);
        }

        vtxStripT* GetStrips() const
        {
            return (vtxStripT*)(((uint8_t*)this) + StripsOffset);
        }

        std::ostream& print(std::ostream& os, const char* indent) const;
    } GCC_PACKED;


    /// This class corresponds to StudioMeshT in the mdl file. A mesh has exactly one material
    /// associated with it and contains up to four locking groups (see vtxStripGroupT::FlagsT):
    /// flexed / non-flexed, hardware skinned / software skinned.
    struct vtxMeshT
    {
        enum FlagsT { IS_TEETH = 1, IS_EYES  = 2 };

        uint32_t NumStripGroups;
        uint32_t StripGroupsOffset;
        uint8_t  Flags;


        vtxStripGroupT* GetStripGroups() const
        {
            return (vtxStripGroupT*)(((uint8_t*)this) + StripGroupsOffset);
        }

        std::ostream& print(std::ostream& os, const char* indent) const;
    } GCC_PACKED;


    struct vtxModelLODT
    {
        uint32_t NumMeshes;     ///< One vtxMeshT for each StudioMeshT.
        uint32_t MeshesOffset;
        float    SwitchPoint;


        vtxMeshT* GetMeshes() const
        {
            return (vtxMeshT*)(((uint8_t*)this) + MeshesOffset);
        }

        std::ostream& print(std::ostream& os, const char* indent) const;
    } GCC_PACKED;


    /// This class corresponds to StudioModelT in the mdl file.
    struct vtxModelT
    {
        uint32_t NumLODs;       ///< Must match the NumLODs value in StripsHeaderT.
        uint32_t LODsOffset;


        vtxModelLODT* GetLODs() const
        {
            return (vtxModelLODT*)(((uint8_t*)this) + LODsOffset);
        }

        std::ostream& print(std::ostream& os, const char* indent) const;
    } GCC_PACKED;


    /// This class corresponds to StudioBodyPartT in the mdl file.
    struct vtxBodyPartT
    {
        uint32_t NumModels;
        uint32_t ModelsOffset;


        vtxModelT* GetModels() const
        {
            return (vtxModelT*)(((uint8_t*)this) + ModelsOffset);
        }

        std::ostream& print(std::ostream& os, const char* indent) const;
    } GCC_PACKED;


    struct vtxMaterialReplacementT
    {
        uint16_t MatID;
        uint32_t ReplacementMatNameOffset;


        const char* GetMaterialReplacementName() const
        {
            return (const char*)(((uint8_t*)this) + ReplacementMatNameOffset);
        }

        // std::ostream& print(std::ostream& os, const char* indent) const;
    } GCC_PACKED;


    struct vtxMaterialReplacementListT
    {
        uint32_t NumMatReplacements;
        uint32_t MatReplacementsOffset;


        vtxMaterialReplacementT* GetMaterialReplacements() const
        {
            return (vtxMaterialReplacementT*)(((uint8_t*)this) + MatReplacementsOffset);
        }

        // std::ostream& print(std::ostream& os, const char* indent) const;
    } GCC_PACKED;


    /// This is the header of a VTX strips data file.
    class StripsHeaderT
    {
        public:

        static const uint32_t FILE_VERSION;     ///< The currently supported file format version.

        uint32_t Version;                       ///< Must be FILE_VERSION.

        uint32_t VertCacheSize;                 ///< These four are hardware-specific parameters that affect how the model is to be optimized.
        uint16_t MaxBonesPerStrip;
        uint16_t MaxBonesPerTri;
        uint32_t MaxBonesPerVert;

        uint32_t Checksum;                      ///< Must match the checksum in StudioHeaderT.

        uint32_t NumLODs;                       ///< The same value is repeated in each vtxModelT.
        uint32_t MatReplacementListsOffset;     ///< One per LOD.

        uint32_t NumBodyParts;
        uint32_t BodyPartsOffset;


        public:

        vtxMaterialReplacementListT* GetMaterialReplacementLists() const
        {
            return (vtxMaterialReplacementListT*)(((uint8_t*)this) + MatReplacementListsOffset);
        }

        vtxBodyPartT* GetBodyParts() const
        {
            return (vtxBodyPartT*)(((uint8_t*)this) + BodyPartsOffset);
        }

        std::ostream& print(std::ostream& os, const char* indent) const;
    } GCC_PACKED;
}


#if defined(_MSC_VER)
    #pragma pack(pop)
#endif
#undef GCC_PACKED

#endif
