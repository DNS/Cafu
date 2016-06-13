/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MD5_MODEL_LOADER_HPP_INCLUDED
#define CAFU_MD5_MODEL_LOADER_HPP_INCLUDED

#include "Loader.hpp"


/// This class imports a Doom3 (.md5) model file into a new Cafu model.
class LoaderMd5T : public ModelLoaderT
{
    public:

    /// The constructor for importing a Doom3 (.md5) model file into a new Cafu model.
    /// @param FileName   The name of the .md5 file to import.
    /// @param Flags      The flags to load the model with. See ModelLoaderT::FlagsT for details.
    /// If FileName ends with "md5", it is assumed that the file has a white-space separated list of one md5mesh and arbitrarily many md5anim files.
    /// If FileName ends with "md5mesh", this file is loaded without any animation information (e.g. for static detail models).
    LoaderMd5T(const std::string& FileName, int Flags=NONE);

    void Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan);
    void Load(ArrayT<CafuModelT::SkinT>& Skins, const MaterialManagerImplT& MaterialMan) { }
    void Load(ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures) { }
    void Load(ArrayT<CafuModelT::ChannelT>& Channels) { }
    bool Load(unsigned int Level, CafuModelT*& DlodModel, float& DlodDist) { return false; }
};


/// This class imports animations from Doom3 .md5anim files for addition to an existing CafuModelT.
class ImporterMd5AnimT : public AnimImporterT
{
    public:

    ImporterMd5AnimT(const std::string& FileName);

    ArrayT<CafuModelT::AnimT> Import(const ArrayT<CafuModelT::JointT>& Joints, const ArrayT<CafuModelT::MeshT>& Meshes);
};

#endif
