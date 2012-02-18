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

#ifndef CAFU_DUMMY_MODEL_LOADER_HPP_INCLUDED
#define CAFU_DUMMY_MODEL_LOADER_HPP_INCLUDED

#include "Loader.hpp"


/// This class loads a "dummy model" into a new Cafu model.
/// A dummy model consists of a single, fixed mesh in the shape of a cone.
///
/// While the given string \c FileName is recorded as the filename of the model,
/// no attempt is made to actually load the model from the specified file.
/// Instead, the "loaded" data is hard-wired in the code.
/// As a result, this loader cannot fail and never throws a LoadErrorT exception.
///
/// This is usually used when the proper loader for the file could not load the model
/// (e.g. due to file not found or an error inside the file), and the caller wishes to
/// deal with the problem by substituting a visual replacement that is guaranteed to work.
class LoaderDummyT : public ModelLoaderT
{
    public:

    /// The constructor for loading a "dummy model" into a new Cafu model.
    /// @param FileName   The name of the dummy file to load. See however the description of the LoaderDummyT class for details!
    /// @param Flags      This parameter is ignored. It just exists for consistency with the other loaders.
    LoaderDummyT(const std::string& FileName, int Flags=NONE);

    bool UseGivenTS() const;
    void Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan);
    void Load(ArrayT<CafuModelT::SkinT>& Skins, const MaterialManagerImplT& MaterialMan);
    void Load(ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures);
    void Load(ArrayT<CafuModelT::ChannelT>& Channels);
    bool Load(unsigned int Level, CafuModelT*& DlodModel, float& DlodDist);
};

#endif
