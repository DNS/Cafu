/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_HL2_MDL_FILE_FORMAT_HPP_INCLUDED
#define CAFU_HL2_MDL_FILE_FORMAT_HPP_INCLUDED

#include "Loader_mdl_hl2_vvd.hpp"
#include "Loader_mdl_hl2_vtx.hpp"
#include "Math3D/Angles.hpp"
#include "Math3D/Quaternion.hpp"


namespace HL2mdl
{
    using namespace cf::math;
    class StudioModelT;


    /// This class describes a mesh of a model.
    struct StudioMeshT
    {
        uint32_t  Material;         ///< See LoaderHL2mdlT::Load(Skins, ...) for details!
        int32_t   ModelOffset;      ///< Points back to the parent StudioModelT. Usually negative.

        uint32_t  NumVertices;      ///< The number of unique vertices (along with normals, tangents and texcoords) in this mesh.
        uint32_t  VerticesOffset;   ///< The first vertex of the mesh, relative to the first vertex of the model.

        uint32_t  NumFlexes;        ///< Flexes are for animating vertices (unrelated to / independent from bones).
        uint32_t  FlexesOffset;

        uint32_t  MaterialType;
        uint32_t  MaterialParam;

        uint32_t  MeshID;           ///< A unique ID for this mesh.
        Vector3fT Center;

        uint32_t  Unused1;
        uint32_t  NumLODVertices[HL2mdl_MAX_NUM_LODS];
        uint32_t  Unused2[8];


        public:

        const StudioModelT*  GetModel() const { return (StudioModelT*)(((uint8_t*)this) + ModelOffset); }
        const StudioVertexT* GetVertices() const;
        const Vector4D*      GetTangents() const;

        std::ostream& print(std::ostream& os, const char* indent) const;
    };


    /// This class describes a model of a body part.
    class StudioModelT
    {
        public:

        char     Name[64];
        uint32_t Type;
        float    BoundingRadius;

        uint32_t NumMeshes;         ///< LOD is implemented at mesh level, not at model level.
        uint32_t MeshesOffset;

        uint32_t NumVertices;       ///< The number of unique vertices (along with normals, tangents and texcoords) in this mesh.
        uint32_t VertOffset;        ///< The byte-offset into the VVD file to the first vertex. See GetVertices() for details.
        uint32_t TangentsOffset;    ///< As VertOffset, but for tangents.

        uint32_t NumAttachments;
        uint32_t AttachmentsOffset;

        uint32_t NumEyeballs;
        uint32_t EyeballsOffset;

        const StudioVertexT* m_Vertices;    ///< = VertexHeader->GetVertices();
        const Vector4D*      m_Tangents;    ///< = VertexHeader->GetTangents();

        #if UINTPTR_MAX == UINT32_MAX
        uint32_t Unused[8];         ///< Pointers are 32 bits wide.
        #else
        uint32_t Unused[6];         ///< Pointers are 64 bits wide.
        #endif


        public:

        const StudioMeshT* GetMeshes() const
        {
            return (StudioMeshT*)(((uint8_t*)this) + MeshesOffset);
        }

        const StudioVertexT* GetVertices() const
        {
            assert(VertOffset % sizeof(StudioVertexT) == 0);

            return m_Vertices ? m_Vertices + (VertOffset / sizeof(StudioVertexT)) : NULL;
        }

        const Vector4D* GetTangents() const
        {
            assert(TangentsOffset % sizeof(Vector4D) == 0);

            return m_Tangents ? m_Tangents + (TangentsOffset / sizeof(Vector4D)) : NULL;
        }

        std::ostream& print(std::ostream& os, const char* indent) const;
    };


    inline const StudioVertexT* StudioMeshT::GetVertices() const
    {
        return GetModel()->GetVertices() + VerticesOffset;
    }

    inline const Vector4D* StudioMeshT::GetTangents() const
    {
        return GetModel()->GetTangents() + VerticesOffset;
    }


    /// This class describes a body part.
    /// A body part contains one or several models that are mutually exclusive, e.g.
    /// "empty holster", "holster with gun", "walkie-talkie", "empty/nothing".
    /// Bodies ("body variants") can be constructed from all such possible combinations.
    /// The related hierarchy is:
    ///
    ///     body variant    // a result of the combinations below
    ///       body part     // all needed
    ///         model       // mutually exclusive
    ///           meshes    // all needed
    ///
    struct StudioBodyPartT
    {
        uint32_t NameOffset;
        uint32_t NumModels;
        uint32_t Base;          ///< For the i-th body ("body variant"), use from this body part the `(i / Base) % NumModels`-th model.
        uint32_t ModelsOffset;


        public:

        const char*   GetName() const { return ((char*)this) + NameOffset; }
        StudioModelT* GetModels() const { return (StudioModelT*)(((uint8_t*)this) + ModelsOffset); }

        std::ostream& print(std::ostream& os, const char* indent) const;
    };


    /// Used in StudioBoneT.
    class Matrix3x4fT
    {
        public:

        Matrix3x4fT() { }

        /// Computes M*v, where M is this matrix.
        /// The w-component of v is assumed to be 1 (v being a point, not a direction vector).
        /// The fourth, missing row of M is assumed to be (0 0 0 1).
        /// That means that both the rotation (and scale) *and* the translation of M is applied to v.
        /// @param  v    A point.
        /// @return M*v. The w-component of the returned vector is implied to be 1.
        Vector3fT Mul1(const Vector3fT& v) const
        {
            return Vector3fT(m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z + m[0][3],
                             m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z + m[1][3],
                             m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z + m[2][3]);
        }


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

        Matrix3x4fT  PoseToBone;        ///< Transforms a (default pose's) mesh vertex from Model Space to Bone Space.
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


    struct StudioTextureT
    {
        uint32_t NameOffset;
        uint32_t Flags;
        uint32_t Unknown;
        uint32_t Unused[13];


        public:

        const char* GetName() const { return ((char*)this) + NameOffset; }

        std::ostream& print(std::ostream& os, const char* indent) const;
    };


    /// This is the header of an MDL model file.
    class StudioHeaderT
    {
        public:

        static const uint32_t FILE_ID_IDST;     ///< The ID of a MDL studio model file.
        static const uint32_t FILE_VERSION;     ///< The currently supported file format version.

        uint32_t       ID;                      ///< Must be FILE_ID_IDST.
        uint32_t       Version;                 ///< Must be FILE_VERSION.
        uint32_t       Checksum;                ///< Must match the checksum in VertexHeaderT.
        char           Name[64];                ///< The (relative) file name of this model.
        uint32_t       FileSize;                ///< The size in bytes of the MDL file.

        Vector3fT      EyePos;                  ///< Ideal eye position.
        Vector3fT      IllumPos;                ///< Illumination center.
        BoundingBox3fT HullBB;                  ///< The ideal movement hull size.
        BoundingBox3fT ViewBB;                  ///< The bounding box for visual clipping, possibly empty (0, 0, 0) (0, 0, 0).
        uint32_t       Flags;

        uint32_t       NumBones;                ///< The number of StudioBoneT instances.
        uint32_t       BonesOffset;             ///< The byte offset to the first bone, relative to "this".

        uint32_t       NumBoneControllers;      ///< The number of StudioBoneControllerT instances.
        uint32_t       BoneControllersOffset;   ///< The byte offset to the first bone controller, relative to "this".

        uint32_t       NumHitboxSets;
        uint32_t       HitboxSetsOffset;

        uint32_t       NumLocalAnims;
        uint32_t       LocalAnimsOffset;

        uint32_t       NumLocalSeqs;
        uint32_t       LocalSeqsOffset;

        uint32_t       ActivityListVersion;
        uint32_t       EventsIndexed;

        uint32_t       NumTextures;
        uint32_t       TexturesOffset;

        uint32_t       NumTexturePaths;
        uint32_t       TexturePathsOffset;

        uint32_t       NumSkinRefs;
        uint32_t       NumSkinFamilies;
        uint32_t       SkinsOffset;

        uint32_t       NumBodyParts;            ///< The number of StudioBodyPartT instances.
        uint32_t       BodyPartsOffset;         ///< The byte offset to the first body part, relative to "this".

        uint32_t       NumLocalAttachments;
        uint32_t       LocalAttachmentsOffset;


        public:

        const StudioBoneT*     GetBones() const { return (StudioBoneT*)(((uint8_t*)this) + BonesOffset); }
        const StudioTextureT*  GetTextures() const { return (StudioTextureT*)(((uint8_t*)this) + TexturesOffset); }
        const char*            GetTexturePath(uint32_t i) const { return ((char*)this) + *((int*)(((uint8_t*)this) + TexturePathsOffset) + i); }
        const uint16_t*        GetSkinRefs() const { return (uint16_t*)(((uint8_t*)this) + SkinsOffset); }
        const StudioBodyPartT* GetBodyParts() const { return (StudioBodyPartT*)(((uint8_t*)this) + BodyPartsOffset); }
    };
}

#endif
