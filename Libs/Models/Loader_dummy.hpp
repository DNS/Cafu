/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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

    void Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan);
    void Load(ArrayT<CafuModelT::SkinT>& Skins, const MaterialManagerImplT& MaterialMan);
    void Load(ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures);
    void Load(ArrayT<CafuModelT::ChannelT>& Channels);
    bool Load(unsigned int Level, CafuModelT*& DlodModel, float& DlodDist);
};

#endif
