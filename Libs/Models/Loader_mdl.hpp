/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2013 Carsten Fuchs Software.

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

#ifndef CAFU_MDL_MODEL_LOADER_HPP_INCLUDED
#define CAFU_MDL_MODEL_LOADER_HPP_INCLUDED

#include "Loader.hpp"


/// This class imports a HL1 (version 10) or a HL2 (version 48) model file into a new Cafu model.
class LoaderMdlT : public ModelLoaderT
{
    public:

    /// The constructor for importing a HL1 (version 10) or a HL2 (version 48) model file into a new Cafu model.
    /// @param FileName   The name of the .mdl file to import.
    /// @param Flags      The flags to load the model with. See ModelLoaderT::FlagsT for details.
    LoaderMdlT(const std::string& FileName, int Flags=NONE);

    /// The destructor.
    ~LoaderMdlT();

    void Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan);
    void Load(ArrayT<CafuModelT::SkinT>& Skins, const MaterialManagerImplT& MaterialMan);
    void Load(ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures);
    void Load(ArrayT<CafuModelT::ChannelT>& Channels);
    bool Load(unsigned int Level, CafuModelT*& DlodModel, float& DlodDist);
    void Postprocess(ArrayT<CafuModelT::MeshT>& Meshes);


    private:

    ModelLoaderT* m_Loader;
};

#endif
