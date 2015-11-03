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

#ifndef CAFU_HL2_MDL_MODEL_LOADER_HPP_INCLUDED
#define CAFU_HL2_MDL_MODEL_LOADER_HPP_INCLUDED

#include "Loader.hpp"


namespace HL2mdl
{
    class VertexHeaderT;
    class StripsHeaderT;
    class StudioHeaderT;
}


/// This class imports a HL2 (.mdl) model file into a new Cafu model.
class LoaderHL2mdlT : public ModelLoaderT
{
    public:

    /// The constructor for importing a HL2 (.mdl) model file into a new Cafu model.
    /// @param FileName   The name of the .mdl file to import.
    /// @param Flags      The flags to load the model with. See ModelLoaderT::FlagsT for details.
    LoaderHL2mdlT(const std::string& FileName, int Flags = NONE);

    void Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan);
    void Load(ArrayT<CafuModelT::SkinT>& Skins, const MaterialManagerImplT& MaterialMan);
    void Load(ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures) { }
    void Load(ArrayT<CafuModelT::ChannelT>& Channels) { }
    bool Load(unsigned int Level, CafuModelT*& DlodModel, float& DlodDist) { return false; }


    private:

    void Load(ArrayT<CafuModelT::JointT>& Joints) const;
    void Load(ArrayT<CafuModelT::MeshT>& Meshes) const;

    ArrayT<uint8_t> m_VertexData;
    ArrayT<uint8_t> m_StripsData;
    ArrayT<uint8_t> m_ModelData;    ///< Basic model data.

    // Pointers into the first bytes of the above data arrays.
    const HL2mdl::VertexHeaderT* VertexHeader;
    const HL2mdl::StripsHeaderT* StripsHeader;
    const HL2mdl::StudioHeaderT* StudioHeader;
};

#endif
