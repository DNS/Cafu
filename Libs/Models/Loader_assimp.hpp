/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#ifndef CAFU_ASSIMP_MODEL_LOADER_HPP_INCLUDED
#define CAFU_ASSIMP_MODEL_LOADER_HPP_INCLUDED

#include "Loader.hpp"


struct aiAnimation;
struct aiNode;
struct aiScene;


/// This class imports a Doom3 (.md5) model file into a new Cafu model.
class LoaderAssimpT : public ModelLoaderT
{
    public:

    /// The constructor for importing a Doom3 (.md5) model file into a new Cafu model.
    /// @param FileName   The name of the .md5 file to import.
    /// @param Flags      The flags to load the model with. See ModelLoaderT::FlagsT for details.
    /// If FileName ends with "md5", it is assumed that the file has a white-space separated list of one md5mesh and arbitrarily many md5anim files.
    /// If FileName ends with "md5mesh", this file is loaded without any animation information (e.g. for static detail models).
    LoaderAssimpT(const std::string& FileName, int Flags=NONE);

    bool UseGivenTS() const;
    void Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan);
    void Load(ArrayT<CafuModelT::SkinT>& Skins, const MaterialManagerImplT& MaterialMan) { }
    void Load(ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures, ArrayT<CafuModelT::GuiLocT>& GuiLocs) { }
    void Load(ArrayT<CafuModelT::ChannelT>& Channels) { }
    bool Load(unsigned int Level, CafuModelT*& DlodModel, float& DlodDist) { return false; }


    private:

    void Load(ArrayT<CafuModelT::JointT>& Joints, int ParentIndex, const aiNode* Node);
    void Load(ArrayT<CafuModelT::MeshT>& Meshes, const aiNode* Node, const ArrayT<CafuModelT::JointT>& Joints);
    void Load(CafuModelT::AnimT& CafuAnim, const aiAnimation* AiAnim, const ArrayT<CafuModelT::JointT>& Joints);

    const aiScene* m_Scene;
};

#endif
