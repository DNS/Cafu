/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_HL2_MDL_VVD_VERTEX_DATA_HPP_INCLUDED
#define CAFU_HL2_MDL_VVD_VERTEX_DATA_HPP_INCLUDED

#include "Math3D/Vector3.hpp"
#include "Templates/Array.hpp"

#if defined(_WIN32) && _MSC_VER<1600
#include "pstdint.h"            // Paul Hsieh's portable implementation of the stdint.h header.
#else
#include <stdint.h>
#endif


#define HL2mdl_MAX_NUM_BONES_PER_VERT 3
#define HL2mdl_MAX_NUM_LODS 8


namespace HL2mdl
{
    typedef float Vector2D[2];


    struct Vector4D
    {
        float v[4];
    };


    struct StudioBoneWeightT
    {
        float   Weight[HL2mdl_MAX_NUM_BONES_PER_VERT];
        uint8_t Bone[HL2mdl_MAX_NUM_BONES_PER_VERT];
        uint8_t NumBones;
    };


    struct StudioVertexT
    {
        StudioBoneWeightT BoneWeights;
        Vector3fT         Pos;
        Vector3fT         Normal;
        Vector2D          TexCoord;

        std::ostream& print(std::ostream& os, const char* indent) const;
    };


    struct FixupT
    {
        uint32_t LOD;           ///< Can use this to skip higher LODs.
        uint32_t StartVertex;   ///< Begin  of the "fixup strip".
        uint32_t NumVertices;   ///< Length of the "fixup strip".

        std::ostream& print(std::ostream& os, const char* indent) const;
    };


    /// This is the header of a VVD vertex data file.
    class VertexHeaderT
    {
        public:

        static const uint32_t FILE_ID_IDSV;     ///< The ID of a VVD file with normal vertex data.
        static const uint32_t FILE_ID_IDCV;     ///< The ID of a VVD file with compressed ("thin") vertex data.
        static const uint32_t FILE_VERSION;     ///< The currently supported file format version.

        uint32_t ID;                            ///< Must be FILE_ID_IDSV or FILE_ID_IDCV.
        uint32_t Version;                       ///< Must be FILE_VERSION.
        uint32_t Checksum;                      ///< Must match the checksum in StudioHeaderT.
        uint32_t NumLODs;                       ///< The number of available detail levels.
        uint32_t NumLODVertices[HL2mdl_MAX_NUM_LODS];  ///< The cumulated number of vertices for the given and all lower detail levels.
        uint32_t NumFixups;                     ///< The number of FixupT instances.
        uint32_t FixupsOffset;                  ///< The byte offset to the first fixup, relative to "this".
        uint32_t VerticesOffset;                ///< The byte offset to the first vertex, relative to "this".
        uint32_t TangentsOffset;                ///< The byte offset to the first tangent, relative to "this".


        public:

        StudioVertexT* GetVertices() const
        {
            return ID == FILE_ID_IDSV && VerticesOffset != 0 ? (StudioVertexT*)((uint8_t*)this + VerticesOffset) : NULL;
        }

        Vector4D* GetTangents() const
        {
            return ID == FILE_ID_IDSV && TangentsOffset != 0 ? (Vector4D*)((uint8_t*)this + TangentsOffset) : NULL;
        }

        const FixupT* GetFixups() const
        {
            return (FixupT*)((uint8_t*)this + FixupsOffset);
        }

        void FixData()
        {
            // This is not efficient, but this is just model importer code anyway.
            ArrayT<StudioVertexT> NewVertices;
            ArrayT<Vector4D>      NewTangents;

            for (uint32_t i = 0; i < NumFixups; i++)
            {
                const FixupT& Fixup = GetFixups()[i];

                if (GetVertices())
                {
                    const StudioVertexT* Vertices = GetVertices() + Fixup.StartVertex;

                    for (uint32_t VertNr = 0; VertNr < Fixup.NumVertices; VertNr++)
                        NewVertices.PushBack(Vertices[VertNr]);
                }

                if (GetTangents())
                {
                    const Vector4D* Tangents = GetTangents() + Fixup.StartVertex;

                    for (uint32_t VertNr = 0; VertNr < Fixup.NumVertices; VertNr++)
                        NewTangents.PushBack(Tangents[VertNr]);
                }
            }

            for (uint32_t VertNr = 0; VertNr < NewVertices.Size(); VertNr++)
                GetVertices()[VertNr] = NewVertices[VertNr];

            for (uint32_t VertNr = 0; VertNr < NewTangents.Size(); VertNr++)
                GetTangents()[VertNr] = NewTangents[VertNr];

            NumFixups = 0;
        }

        std::ostream& print(std::ostream& os, const char* indent) const;
    };
}

#endif
