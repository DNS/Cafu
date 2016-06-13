/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CMDL_MODEL_LOADER_HPP_INCLUDED
#define CAFU_CMDL_MODEL_LOADER_HPP_INCLUDED

#include "Loader.hpp"


struct lua_State;


/// This class loads a native Cafu (.cmdl) model file into a new Cafu model.
class LoaderCafuT : public ModelLoaderT
{
    public:

    /// The constructor for loading a native Cafu (.cmdl) model file into a new Cafu model.
    /// @param FileName   The name of the .cmdl file to load.
    /// @param Flags      The flags to load the model with. See ModelLoaderT::FlagsT for details.
    LoaderCafuT(const std::string& FileName, int Flags=NONE);

    /// The destructor.
    ~LoaderCafuT();

    void Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan);
    void Load(ArrayT<CafuModelT::SkinT>& Skins, const MaterialManagerImplT& MaterialMan);
    void Load(ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures);
    void Load(ArrayT<CafuModelT::ChannelT>& Channels);
    bool Load(unsigned int Level, CafuModelT*& DlodModel, float& DlodDist) { return false; }


    private:

    static int SetVersion(lua_State* LuaState);

    lua_State*   m_LuaState;
    unsigned int m_Version;
};

#endif
