/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_HL2_MDL_MODEL_LOADER_HPP_INCLUDED
#define CAFU_HL2_MDL_MODEL_LOADER_HPP_INCLUDED

#include "Loader.hpp"


namespace HL2mdl
{
    class VertexHeaderT;
    class StripsHeaderT;
    class StudioHeaderT;
}


/// This class imports a HL2 (.mdl) model file into a new Cafu model.
class LoaderHL2mdlT : public ModelLoaderT
{
    public:

    /// The constructor for importing a HL2 (.mdl) model file into a new Cafu model.
    /// @param FileName   The name of the .mdl file to import.
    /// @param Flags      The flags to load the model with. See ModelLoaderT::FlagsT for details.
    LoaderHL2mdlT(const std::string& FileName, int Flags = NONE);

    void Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan);
    void Load(ArrayT<CafuModelT::SkinT>& Skins, const MaterialManagerImplT& MaterialMan);
    void Load(ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures) { }
    void Load(ArrayT<CafuModelT::ChannelT>& Channels) { }
    bool Load(unsigned int Level, CafuModelT*& DlodModel, float& DlodDist) { return false; }


    private:

    void Load(MaterialManagerImplT& MaterialMan);
    void Load(ArrayT<CafuModelT::JointT>& Joints) const;
    void Load(ArrayT<CafuModelT::MeshT>& Meshes) const;

    ArrayT<uint8_t>    m_VertexData;
    ArrayT<uint8_t>    m_StripsData;
    ArrayT<uint8_t>    m_ModelData;
    ArrayT<MaterialT*> m_Materials;     ///< The MatSys materials used in the model.

    // Pointers into the first bytes of the above data arrays.
    const HL2mdl::VertexHeaderT* VertexHeader;
    const HL2mdl::StripsHeaderT* StripsHeader;
    const HL2mdl::StudioHeaderT* StudioHeader;
};

#endif
