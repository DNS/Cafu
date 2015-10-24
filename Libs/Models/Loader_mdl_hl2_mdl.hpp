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

#ifndef CAFU_HL2_MDL_FILE_FORMAT_HPP_INCLUDED
#define CAFU_HL2_MDL_FILE_FORMAT_HPP_INCLUDED

#include "Loader_mdl_hl2_vvd.hpp"
#include "Math3D/Angles.hpp"
#include "Math3D/Quaternion.hpp"


namespace HL2mdl
{
    using namespace cf::math;


    class Matrix3x4fT
    {
        public:

        Matrix3x4fT() { }


        private:

        float m[3][4];
    };


    /// This class describes a bone.
    /// The entirety of the bones define the skeleton of the model.
    struct StudioBoneT
    {
        uint32_t     NameOffset;
        int32_t      Parent;
        int32_t      BoneController[6]; ///< Indices to the bone controllers of this bone, -1 means "none".

        Vector3fT    Pos;               ///< The bone's position in the default pose.
        QuaternionfT Quat;              ///< The bone's orientation in the default pose.
        AnglesfT     Angles;            ///< The bone's rotation in the default pose (Euler angles in radians, not degrees).

        Vector3fT    PosScale;          ///< Compression scale.
        Vector3fT    RotScale;

        Matrix3x4fT  PoseToBone;        // Does this transform a mesh vertex from Model Space to Bone Space?
        QuaternionfT Alignment;
        uint32_t     Flags;
        uint32_t     ProcType;
        uint32_t     ProcOffset;
        uint32_t     PhysicsBone;
        uint32_t     SurfacePropNameOffset;
        uint32_t     Contents;
        uint32_t     Unused[8];


        public:

        const char* GetName() const { return ((char*)this) + NameOffset; }
        const char* GetSurfacePropName() const { return ((char*)this) + SurfacePropNameOffset; }

        std::ostream& print(std::ostream& os, const char* indent) const;
    };


    /// This is the header of an MDL model file.
    class StudioHeaderT
    {
        public:

        static const uint32_t FILE_ID_IDST;     ///< The ID of a MDL studio model file.
        static const uint32_t FILE_VERSION;     ///< The currently supported file format version.

        uint32_t       ID;                  ///< Must be FILE_ID_IDST.
        uint32_t       Version;             ///< Must be FILE_VERSION.
        uint32_t       Checksum;            ///< Must match the checksum in VertexHeaderT.
        char           Name[64];            ///< The (relative) file name of this model.
        uint32_t       FileSize;            ///< The size in bytes of the MDL file.

        Vector3fT      EyePos;              ///< Ideal eye position.
        Vector3fT      IllumPos;            ///< Illumination center.
        BoundingBox3fT HullBB;              ///< The ideal movement hull size.
        BoundingBox3fT ViewBB;              ///< The bounding box for visual clipping, possibly empty (0, 0, 0) (0, 0, 0).
        uint32_t       Flags;

        uint32_t       NumBones;            ///< The number of StudioBoneT instances.
        uint32_t       BonesOffset;         ///< The byte offset to the first bone, relative to "this".


        public:

        const StudioBoneT* GetBones() const { return (StudioBoneT*)(((uint8_t*)this) + BonesOffset); }
    };
}

#endif
