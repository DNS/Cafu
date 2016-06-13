/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_LIGHTWAVE_OBJECT_LOADER_HPP_INCLUDED
#define CAFU_LIGHTWAVE_OBJECT_LOADER_HPP_INCLUDED

#include "Loader.hpp"


/// This class imports a LightWave Object (.lwo) file into a new Cafu model.
class LoaderLwoT : public ModelLoaderT
{
    public:

    /// The constructor for importing a LightWave Object (.lwo) file into a new Cafu model.
    /// @param FileName   The name of the .lwo file to import.
    /// @param Flags      The flags to load the model with. See ModelLoaderT::FlagsT for details.
    LoaderLwoT(const std::string& FileName, int Flags=NONE);

    void Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan);
    void Load(ArrayT<CafuModelT::SkinT>& Skins, const MaterialManagerImplT& MaterialMan) { }
    void Load(ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures) { }
    void Load(ArrayT<CafuModelT::ChannelT>& Channels) { }
    bool Load(unsigned int Level, CafuModelT*& DlodModel, float& DlodDist) { return false; }
};

#endif
