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

#ifndef CAFU_HL2_MDL_VVD_VERTEX_DATA_HPP_INCLUDED
#define CAFU_HL2_MDL_VVD_VERTEX_DATA_HPP_INCLUDED

#include "Math3D/Vector3.hpp"

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
    typedef float Vector4D[4];


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
        uint32_t NumLODs;                       ///< The number of valid LODs.
        uint32_t NumLODVertices[HL2mdl_MAX_NUM_LODS];  ///< The number of vertices for the desired root LOD.
        uint32_t NumFixups;                     ///< The number of StudioVertexFileFixupT instances.
        uint32_t FixupsOffset;                  ///< The byte offset to the first fixup, relative to "this".
        uint32_t VerticesOffset;                ///< The byte offset to the first vertex, relative to "this".
        uint32_t TangentsOffset;                ///< The byte offset to the first tangent, relative to "this".


        public:

        const StudioVertexT* GetVertices() const
        {
            return ID == FILE_ID_IDSV && VerticesOffset != 0 ? (StudioVertexT*)((uint8_t*)this + VerticesOffset) : NULL;
        }

        const Vector4D* GetTangents() const
        {
            return ID == FILE_ID_IDSV && TangentsOffset != 0 ? (Vector4D*)((uint8_t*)this + TangentsOffset) : NULL;
        }

        std::ostream& print(std::ostream& os, const char* indent) const;
    };
}

#endif
