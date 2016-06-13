/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Loader_mdl.hpp"
#include "Loader_mdl_hl1.hpp"
#include "Loader_mdl_hl2.hpp"


LoaderMdlT::LoaderMdlT(const std::string& FileName, int Flags)
    : ModelLoaderT(FileName, Flags),
      m_Loader(NULL)
{
    std::string Errors;

    try
    {
        m_Loader = new LoaderHL1mdlT(FileName, Flags);
        return;
    }
    catch (const ModelLoaderT::LoadErrorT& LE)
    {
        Errors += "\nError while importing the file as HL1 mdl file:\n";
        Errors += LE.what();
        Errors += "\n";
    }

    try
    {
        m_Loader = new LoaderHL2mdlT(FileName, Flags);
        return;
    }
    catch (const ModelLoaderT::LoadErrorT& LE)
    {
        Errors += "\nError while importing the file as HL2 mdl file:\n";
        Errors += LE.what();
        Errors += "\n";
    }

    assert(m_Loader == NULL);
    throw LoadErrorT(Errors);
}


LoaderMdlT::~LoaderMdlT()
{
    delete m_Loader;
}


void LoaderMdlT::Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan)
{
    m_Loader->Load(Joints, Meshes, Anims, MaterialMan);
}


void LoaderMdlT::Load(ArrayT<CafuModelT::SkinT>& Skins, const MaterialManagerImplT& MaterialMan)
{
    m_Loader->Load(Skins, MaterialMan);
}


void LoaderMdlT::Load(ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures)
{
    m_Loader->Load(GuiFixtures);
}


void LoaderMdlT::Load(ArrayT<CafuModelT::ChannelT>& Channels)
{
    m_Loader->Load(Channels);
}


bool LoaderMdlT::Load(unsigned int Level, CafuModelT*& DlodModel, float& DlodDist)
{
    return m_Loader->Load(Level, DlodModel, DlodDist);
}


void LoaderMdlT::Postprocess(ArrayT<CafuModelT::MeshT>& Meshes)
{
    m_Loader->Postprocess(Meshes);
}
