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

#ifndef _HL1_MDL_MODEL_LOADER_HPP_
#define _HL1_MDL_MODEL_LOADER_HPP_

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
    LoaderHL1mdlT(const std::string& FileName, int Flags=NONE) /*throw (ModelT::LoadError)*/;

    bool UseGivenTS() const;
    void Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan);
    void Load(ArrayT<CafuModelT::SkinT>& Skins, const MaterialManagerImplT& MaterialMan);
    void Load(ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures, ArrayT<CafuModelT::GuiLocT>& GuiLocs) { }


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
