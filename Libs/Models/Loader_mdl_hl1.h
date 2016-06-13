/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_HL1_MDL_FILE_FORMAT_HPP_INCLUDED
#define CAFU_HL1_MDL_FILE_FORMAT_HPP_INCLUDED

#if defined(_WIN32)
    // We are on the Win32 platform.
    #if defined(__WATCOMC__)
        // Using the OpenWatcom C/C++ compiler.
        #pragma off(unreferenced);
        #define WATCOM_PACKED _Packed
        #define GCC_PACKED
    #elif defined(_MSC_VER)
        // Using the Microsoft Visual C++ compiler.
        #pragma pack(push, 1)
        #define WATCOM_PACKED
        #define GCC_PACKED
    #else
        #error Using unknown compiler on the Win32 platform.
    #endif
#elif __linux__
    // We are on the Linux platform.
    #if __GNUG__    // This is equivalent to testing (__GNUC__ && __cplusplus).
        // Using the g++ compiler.
        // See http://www.delorie.com/gnu/docs/gcc/cpp_toc.html for documentation about the C preprocessor.
        #define WATCOM_PACKED
        #define GCC_PACKED __attribute__ ((packed))
    #else
        #error Using unknown compiler on the Linux platform.
    #endif
#else
    #error Compiling on unknown platform.
#endif


typedef float Vec3T[3];


WATCOM_PACKED struct StudioHeaderT
{
    int   ID;                   // 4 Byte File-ID
    int   Version;              // File-Version

    char  Name[64];             // Model name as given by author(?), with file-path(?)
    int   Length;               // File-Size

    Vec3T EyePosition;          // ideal eye position
    Vec3T Min;                  // ideal movement hull size
    Vec3T Max;

    Vec3T BBMin;                // clipping bounding box
    Vec3T BBMax;

    int   Flags;

    int   NumBones;             // StudioBoneT
    int   BoneIndex;

    int   NumBoneControllers;   // StudioBoneControllerT
    int   BoneControllerIndex;

    int   NumHitBoxes;          // complex bounding boxes
    int   HitBoxIndex;

    int   NumSeq;               // StudioSequenceT, animation sequence descriptions
    int   SeqIndex;

    int   NumSeqGroups;         // StudioSeqGroupT, NumSeqGroups>1 == demand loaded sequences
    int   SeqGroupIndex;

    int   NumTextures;          // StudioTextureT, raw textures
    int   TextureIndex;
    int   TextureDataIndex;

    int   NumSkinRef;           // replaceable textures
    int   NumSkinFamilies;
    int   SkinIndex;

    int   NumBodyParts;         // StudioBodyPartT
    int   BodyPartIndex;

    int   NumAttachments;       // queryable attachable points
    int   AttachmentIndex;      // z.B. Haltepunkt für Waffe, Sprite o.ä.

    int   SoundTable;
    int   SoundIndex;
    int   SoundGroups;
    int   SoundGroupIndex;

    int   NumTransitions;       // animation node to animation node transition graph
    int   TransitionIndex;      // Zeigt auf eine NumTransitions*NumTransitions-Matrix von Bytes (chars)
} GCC_PACKED;


WATCOM_PACKED struct StudioBoneT
{
    char  Name[32];             // bone name for symbolic links
    int   Parent;               // parent bone
    int   Flags;
    int   BoneController[6];    // bone controller index, -1 == none
    float Value[6];             // default DoF values
    float Scale[6];             // scale for delta DoF values
} GCC_PACKED;


WATCOM_PACKED struct StudioBoneControllerT
{
    int   Bone;                 // -1 == none
    int   Type;                 // X, Y, Z, X rotation, Y rotation, Z rotation, M
    float Start;
    float End;
    int   Rest;                 // byte index value at rest
    int   Index;                // 0-3 user set controller, 4 mouth
} GCC_PACKED;


WATCOM_PACKED struct StudioSequenceT
{
    char  Label[32];            // sequence label

    float FPS;                  // frames per second
    int   Flags;                // looping/non-looping flags

    int   Activity;
    int   ActWeight;

    int   NumEvents;
    int   EventIndex;

    int   NumFrames;            // number of frames per sequence

    int   NumPivots;            // number of foot pivots
    int   PivotIndex;

    int   MotionType;
    int   MotionBone;
    Vec3T LinearMovement;
    int   AutoMovePosIndex;
    int   AutoMoveAngleIndex;

    Vec3T BBMin;                // per sequence bounding box
    Vec3T BBMax;

    int   NumBlends;
    int   AnimIndex;            // StudioAnimT pointer relative to start of sequence group data ([blend][bone][X, Y, Z, XR, YR, ZR] (?))

    int   BlendType[2];         // X, Y, Z, XR, YR, ZR
    float BlendStart[2];        // starting value
    float BlendEnd[2];          // ending value
    int   BlendParent;

    int   SeqGroup;             // sequence group for demand loading

    int   EntryNode;            // transition node at entry
    int   ExitNode;             // transition node at exit
    int   NodeFlags;            // transition rules

    int   NextSeq;              // auto advancing sequences
} GCC_PACKED;


WATCOM_PACKED struct StudioSequenceGroupT
{
    char Label[32];     // textual name
    char Name[64];      // file name
    char Dummy[4];
    int  Data;          // hack for group 0
} GCC_PACKED;


WATCOM_PACKED struct StudioTextureT
{
    char Name[64];
    int  Flags;
    int  Width;
    int  Height;
    int  Index;
} GCC_PACKED;


WATCOM_PACKED struct StudioBodyPartT
{
    char Name[64];
    int  NumModels;
    int  Base;
    int  ModelIndex;    // index into models array
} GCC_PACKED;


WATCOM_PACKED struct StudioModelT
{
    char  Name[64];
    int   Type;
    float BoundingRadius;

    int   NumMesh;
    int   MeshIndex;

    int   NumVerts;         // number of unique vertices
    int   VertInfoIndex;    // vertex bone info
    int   VertIndex;        // vertex Vec3T
    int   NumNorms;         // number of unique surface normals
    int   NormInfoIndex;    // normal bone info
    int   NormIndex;        // normal Vec3T

    int   NumGroups;        // deformation groups
    int   GroupIndex;
} GCC_PACKED;


WATCOM_PACKED struct StudioAnimT
{
    unsigned short Offset[6];   // relativer Offset ab "this" zu den AnimValues
} GCC_PACKED;


// The following union/struct describes a sequence of values, using RLE (run-length encoding).
// Let SAV be a properly set pointer to a StudioAnimValueT, as in "StudioAnimValueT* SAV=...;".
// Then use/interprete SAV as follows:
// 'SAV[0].Num.Total' is the total number of values that are being described.
// The first 'SAV[0].Num.Valid' of those values are actually present (see example below),
// the remaining ones are obtained by repeating the last valid value.
// Example: If SAV[0].Num.Total==8, SAV[0].Num.Valid==3, SAV[1].Value==33, SAV[2].Value==77 and SAV[3].Value==123,
// then the resulting sequence of values is "33, 77, 123, 123, 123, 123, 123, 123".
WATCOM_PACKED union StudioAnimValueT
{
    WATCOM_PACKED struct
    {
        char Valid;
        char Total;
    } Num;
    short Value;
} GCC_PACKED;


WATCOM_PACKED struct StudioMeshT
{
    int NumTris;
    int TriIndex;
    int SkinRef;
    int NumNorms_UNUSED;    // per mesh normals (this was used in the very old "pseudo-hemispherical" OpenGL 1.1 lighting code).
    int NormIndex_UNUSED;   // normal Vec3T     (never seen this one being used).
} GCC_PACKED;


WATCOM_PACKED struct StudioAttachmentT
{
    char  Name[32];
    int   Type;
    int   Bone;
    Vec3T Org;          // attachment point
    Vec3T Vectors[3];
} GCC_PACKED;


#if defined(_WIN32) && defined (_MSC_VER)
    #pragma pack(pop)
#endif

#endif
