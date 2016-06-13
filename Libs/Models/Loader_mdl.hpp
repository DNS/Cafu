/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
