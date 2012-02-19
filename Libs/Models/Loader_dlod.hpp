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

#ifndef CAFU_DLOD_MODEL_LOADER_HPP_INCLUDED
#define CAFU_DLOD_MODEL_LOADER_HPP_INCLUDED

#include "Loader.hpp"


/// This class loads a discrete-level-of-detail (.dlod) model file into a new Cafu model.
class LoaderDlodT : public ModelLoaderT
{
    public:

    /// The constructor for loading a discrete-level-of-detail (.dlod) model file into a new Cafu model.
    /// @param FileName   The name of the .dlod file to load.
    /// @param Flags      The flags to load the model with. See ModelLoaderT::FlagsT for details.
    LoaderDlodT(const std::string& FileName, int Flags=NONE);

    /// The destructor.
    ~LoaderDlodT();

    const std::string& GetFileName() const;
    void Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan);
    void Load(ArrayT<CafuModelT::SkinT>& Skins, const MaterialManagerImplT& MaterialMan);
    void Load(ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures);
    void Load(ArrayT<CafuModelT::ChannelT>& Channels);
    bool Load(unsigned int Level, CafuModelT*& DlodModel, float& DlodDist);


    private:

    LoaderDlodT(const LoaderDlodT&);        ///< Use of the Copy    Constructor is not allowed.
    void operator = (const LoaderDlodT&);   ///< Use of the Assignment Operator is not allowed.

    ArrayT<std::string>   m_ModelNames;     ///< The names of the concrete models in the dlod chain.
    ArrayT<float>         m_StartRanges;
    ArrayT<float>         m_EndRanges;
    ArrayT<ModelLoaderT*> m_ModelLoaders;
};

#endif
