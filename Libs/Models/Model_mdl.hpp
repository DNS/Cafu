/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

/**************************/
/*** mdl Model (Header) ***/
/**************************/

#ifndef _MODEL_MDL_HPP_
#define _MODEL_MDL_HPP_

#include "Model.hpp"
#include "MaterialSystem/MaterialManagerImpl.hpp"


class  MaterialT;
struct StudioHeaderT;
struct StudioBoneT;
struct StudioBoneControllerT;
struct StudioSequenceT;
struct StudioSequenceGroupT;
struct StudioHeaderT;
struct StudioTextureT;
struct StudioBodyPartT;
struct StudioAttachmentT;
struct StudioAnimT;


/// This class represents a model that is imported from a HL1 mdl file.
class ModelMdlT : public ModelT
{
    public:

    typedef float Vec3T[3];     ///< x, y, z
    typedef float Vec4T[4];     ///< x, y, z, w


    private:

    struct TriangleInfoT
    {
        const signed short* Vertices  [3];
        int                 Neighbours[3];      ///< The array indices of the three neighbouring triangles. -1 indicates no neighbour, -2 indicates more than one neighbour.
        unsigned long       MaterialIndex;
    };


    const std::string                FileName;                  ///< The file name.
    ArrayT<char>                     ModelData;                 ///< Basic model data.
    ArrayT<char>                     TextureData;               ///< Texture data (if not already present in ModelData).
    ArrayT< ArrayT<char> >           AnimationData;             ///< Animation data ("demand loaded sequences").
    ArrayT<MaterialT*>               Materials;                 ///< The MatSys materials (only needed for the ugly optimization below, in order to see whether the Normal-Map is empty or not).
    ArrayT<MatSys::RenderMaterialT*> RenderMaterials;           ///< The MatSys render materials.
    ArrayT< ArrayT<TriangleInfoT> >  TriangleInfos;             ///< For the first model of each body part, this contains information about each triangle across all meshes of that model.
    ArrayT< ArrayT< ArrayT<int> > >  TrianglesReferingToNormal; ///< For each normal of the first model of each body part, this is a list of triangle indices into TriangleInfos of triangles that refer to this normal (with one vertex).
    MaterialManagerImplT             m_MaterialMan;             ///< The material manager for the materials that are used with the meshes of this model.

    // Convenient abbreviations into the above data arrays
    const StudioHeaderT*            StudioHeader;
    const StudioBoneT*              StudioBones;
    const StudioBoneControllerT*    StudioBoneControllers;
 // const StudioHitBoxT*            StudioHitBoxes;
    const StudioSequenceT*          StudioSequences;
    const StudioSequenceGroupT*     StudioSequenceGroups;
    const StudioHeaderT*            StudioTextureHeader;
    const StudioTextureT*           StudioTextures;
    const StudioBodyPartT*          StudioBodyParts;
    const StudioAttachmentT*        StudioAttachments;
 // const StudioTransitionT*        StudioTransitions;

    // Helper data for Draw().
    mutable int    SequenceNrOfTBUpdate; mutable float FrameNrOfTBUpdate;
    mutable int    SequenceNrOfTVUpdate; mutable float FrameNrOfTVUpdate;
    mutable int    SequenceNrOfTNUpdate; mutable float FrameNrOfTNUpdate;
    mutable int    SequenceNrOfTTUpdate; mutable float FrameNrOfTTUpdate;
    mutable float  TransformedBones[/*MAXSTUDIOBONES*/ 128][3][4];  ///< Transformation matrices for which the last trivial line [ 0 0 0 1 ] has been omitted.
    Vec3T*         TransformedVerticesOfAllBP_;     ///< Global array of transformed vertices.
    Vec3T*         TransformedNormalsOfAllBP_;
    Vec3T*         TransformedTangentsOfAllBP_;
    Vec3T*         TransformedBiNormalsOfAllBP_;
    ArrayT<Vec3T*> TransformedVerticesOfAllBP;      ///< For the first model of each body part, this points into the TransformedVerticesOfAllBP_ array.
    ArrayT<Vec3T*> TransformedNormalsOfAllBP;
    ArrayT<Vec3T*> TransformedTangentsOfAllBP;
    ArrayT<Vec3T*> TransformedBiNormalsOfAllBP;

    // Helper functions for Draw().
    inline void CalcRelativeBoneTransformations(Vec3T* Positions, Vec4T* Quaternions, const StudioSequenceT& CurrentSequence, const StudioAnimT* Animations, const float FrameNr) const;
    void        DoDraw(const int SequenceNr, const float FrameNr, const ModelMdlT* SubModel,
                       const int SuperModel_NumBones, const StudioBoneT* SuperModel_StudioBones, float SuperModel_TransformedBones[][3][4]) const;

    ModelMdlT(const ModelMdlT&);            // Use of the Copy Constructor    is not allowed.
    void operator = (const ModelMdlT&);     // Use of the Assignment Operator is not allowed.


    public:

    /// The constructor.
    ModelMdlT(const std::string& FileName_) /*throw (ModelT::LoadError)*/;

    /// The destructor.
    ~ModelMdlT();

    // The ModelT interface.
    const std::string& GetFileName() const;
    void               Draw(int SequenceNr, float FrameNr, float LodDist, const ModelT* SubModel=NULL) const;
    bool               GetGuiPlane(int SequenceNr, float FrameNr, float LodDist, Vector3fT& GuiOrigin, Vector3fT& GuiAxisX, Vector3fT& GuiAxisY) const;
    void               Print() const;
    unsigned int       GetNrOfSequences() const;
    BoundingBox3fT     GetBB(int SequenceNr, float FrameNr) const;
    bool               TraceRay(int SequenceNr, float FrameNr, int SkinNr, const Vector3fT& RayOrigin, const Vector3fT& RayDir, TraceResultT& Result) const;
 // float              GetNrOfFrames(int SequenceNr) const;
    float              AdvanceFrameNr(int SequenceNr, float FrameNr, float DeltaTime, bool Loop=true) const;
};

#endif
