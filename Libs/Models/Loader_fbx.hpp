/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_FBX_MODEL_LOADER_HPP_INCLUDED
#define CAFU_FBX_MODEL_LOADER_HPP_INCLUDED

#include "Loader.hpp"


/// This class uses the Autodesk FBX SDK in order to load a model file into a new Cafu model.
class LoaderFbxT : public ModelLoaderT
{
    public:

    /// The constructor for loading an Autodesk FBX (.fbx) model file into a new Cafu model.
    /// @param FileName   The name of the file to load.
    /// @param UserCallbacks   An implementation of the UserCallbacksI interface.
    /// @param Flags      The flags to load the model with. See ModelLoaderT::FlagsT for details.
    LoaderFbxT(const std::string& FileName, UserCallbacksI& UserCallbacks, int Flags=NONE);

    /// The destructor.
    ~LoaderFbxT();

    void Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan);
    void Load(ArrayT<CafuModelT::SkinT>& Skins, const MaterialManagerImplT& MaterialMan);
    void Load(ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures);
    void Load(ArrayT<CafuModelT::ChannelT>& Channels) { }
    bool Load(unsigned int Level, CafuModelT*& DlodModel, float& DlodDist) { return false; }


    private:

    LoaderFbxT(const LoaderFbxT&);          ///< Use of the Copy Constructor    is not allowed.
    void operator = (const LoaderFbxT&);    ///< Use of the Assignment Operator is not allowed.

    class FbxSceneT;
    FbxSceneT* m_FbxScene;    ///< We use the PIMPL idiom because we cannot forward-declare the FBX SDK classes without hard-wiring the version dependent name of their namespace.
};

#endif
