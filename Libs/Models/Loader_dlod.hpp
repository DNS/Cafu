/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
    ArrayT<float>         m_EndRanges;
    ArrayT<ModelLoaderT*> m_ModelLoaders;
};

#endif
