/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_HL1_MDL_MODEL_LOADER_HPP_INCLUDED
#define CAFU_HL1_MDL_MODEL_LOADER_HPP_INCLUDED

#include "Loader.hpp"


struct StudioHeaderT;
struct StudioBoneT;
struct StudioSequenceT;
struct StudioSequenceGroupT;
struct StudioTextureT;
struct StudioBodyPartT;


/// This class imports a HL1 (.mdl) model file into a new Cafu model.
class LoaderHL1mdlT : public ModelLoaderT
{
    public:

    /// The constructor for importing a HL1 (.mdl) model file into a new Cafu model.
    /// @param FileName   The name of the .mdl file to import.
    /// @param Flags      The flags to load the model with. See ModelLoaderT::FlagsT for details.
    LoaderHL1mdlT(const std::string& FileName, int Flags=NONE);

    void Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan);
    void Load(ArrayT<CafuModelT::SkinT>& Skins, const MaterialManagerImplT& MaterialMan);
    void Load(ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures) { }
    void Load(ArrayT<CafuModelT::ChannelT>& Channels) { }
    bool Load(unsigned int Level, CafuModelT*& DlodModel, float& DlodDist) { return false; }


    private:

    void Load(ArrayT<CafuModelT::JointT>& Joints) const;
    void Load(ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<int>& MeshSkinRef) const;
    void Load(ArrayT<CafuModelT::AnimT>& Anims) const;

    ArrayT<char>           ModelData;       ///< Basic model data.
    ArrayT<char>           TextureData;     ///< Texture data (if not already present in ModelData).
    ArrayT< ArrayT<char> > AnimationData;   ///< Animation data ("demand loaded sequences").
    ArrayT<MaterialT*>     m_Materials;     ///< The MatSys materials used in the model.
    ArrayT<int>            m_MeshSkinRef;   ///< For each mesh, the original StudioMeshT::SkinRef number.

    // Convenient abbreviations into the above data arrays.
    const StudioHeaderT*         StudioHeader;
    const StudioBoneT*           StudioBones;
 // const StudioBoneControllerT* StudioBoneControllers;
 // const StudioHitBoxT*         StudioHitBoxes;
    const StudioSequenceT*       StudioSequences;
    const StudioSequenceGroupT*  StudioSequenceGroups;
    const StudioHeaderT*         StudioTextureHeader;
    const StudioTextureT*        StudioTextures;
    const StudioBodyPartT*       StudioBodyParts;
 // const StudioAttachmentT*     StudioAttachments;
 // const StudioTransitionT*     StudioTransitions;
};

#endif
