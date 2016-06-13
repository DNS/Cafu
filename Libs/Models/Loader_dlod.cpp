/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Loader_dlod.hpp"
#include "Loader_ase.hpp"
#include "Loader_cmdl.hpp"
#include "Loader_lwo.hpp"
#include "Loader_md5.hpp"
#include "Loader_mdl_hl1.hpp"
#include "TextParser/TextParser.hpp"
#include "String.hpp"


LoaderDlodT::LoaderDlodT(const std::string& FileName, int Flags)
    : ModelLoaderT(FileName, Flags)
{
    TextParserT TP(FileName.c_str());

    try
    {
        while (!TP.IsAtEOF())
        {
            m_ModelNames .PushBack(TP.GetNextToken());
            m_EndRanges  .PushBack(TP.GetNextTokenAsFloat());
        }
    }
    catch (const TextParserT::ParseError&)
    {
        throw LoadErrorT("Could not parse the dlod file.");
    }

    if (m_EndRanges.Size() < m_ModelNames.Size())
    {
        // The last end-range number is optional, as it is ignored:
        // The last model is always used up to infinity.
        m_EndRanges.PushBack(0.0f);
    }

    if (m_ModelNames.Size()==0 ||
        m_ModelNames.Size()!=m_EndRanges.Size()) throw LoadErrorT("Invalid dlod file.");

    // Acquire loader instances for each model in the dlod chain.
    try
    {
        for (unsigned long ModelNr=0; ModelNr<m_ModelNames.Size(); ModelNr++)
        {
            // The m_ModelNames are specified relative to the parent file name,
            // thus extract the path portion from FileName and prepend it to the m_ModelNames.
            const std::string fn=cf::String::GetPath(FileName) + "/" + m_ModelNames[ModelNr];

                 if (cf::String::EndsWith(fn, "ase"    )) m_ModelLoaders.PushBack(new LoaderAseT   (fn, m_Flags));
            else if (cf::String::EndsWith(fn, "cmdl"   )) m_ModelLoaders.PushBack(new LoaderCafuT  (fn, m_Flags));
         // else if (cf::String::EndsWith(fn, "3ds"    )) m_ModelLoaders.PushBack(new LoaderFbxT   (fn, m_Flags));
         // else if (cf::String::EndsWith(fn, "dae"    )) m_ModelLoaders.PushBack(new LoaderFbxT   (fn, m_Flags));
         // else if (cf::String::EndsWith(fn, "dxf"    )) m_ModelLoaders.PushBack(new LoaderFbxT   (fn, m_Flags));
         // else if (cf::String::EndsWith(fn, "fbx"    )) m_ModelLoaders.PushBack(new LoaderFbxT   (fn, m_Flags));
         // else if (cf::String::EndsWith(fn, "dlod"   )) m_ModelLoaders.PushBack(new LoaderDlodT  (fn, m_Flags));  // Don't recurse.
            else if (cf::String::EndsWith(fn, "lwo"    )) m_ModelLoaders.PushBack(new LoaderLwoT   (fn, m_Flags));
            else if (cf::String::EndsWith(fn, "mdl"    )) m_ModelLoaders.PushBack(new LoaderHL1mdlT(fn, m_Flags));
            else if (cf::String::EndsWith(fn, "md5"    )) m_ModelLoaders.PushBack(new LoaderMd5T   (fn, m_Flags));
            else if (cf::String::EndsWith(fn, "md5mesh")) m_ModelLoaders.PushBack(new LoaderMd5T   (fn, m_Flags));
         // else if (cf::String::EndsWith(fn, "obj"    )) m_ModelLoaders.PushBack(new LoaderFbxT   (fn, m_Flags));
            else throw LoadErrorT("Model file format not supported as dlod model.");
        }
    }
    catch (const LoadErrorT&)
    {
        for (unsigned long ModelNr=0; ModelNr<m_ModelLoaders.Size(); ModelNr++)
            delete m_ModelLoaders[ModelNr];

        throw;
    }
}


LoaderDlodT::~LoaderDlodT()
{
    for (unsigned long ModelNr=0; ModelNr<m_ModelLoaders.Size(); ModelNr++)
        delete m_ModelLoaders[ModelNr];
}


const std::string& LoaderDlodT::GetFileName() const
{
    // The model will (attempt to) register a wrong .cmat file
    // with its material manager if we return m_FileName here.
    return m_ModelLoaders[0]->GetFileName();
}


void LoaderDlodT::Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan)
{
    m_ModelLoaders[0]->Load(Joints, Meshes, Anims, MaterialMan);
}


void LoaderDlodT::Load(ArrayT<CafuModelT::SkinT>& Skins, const MaterialManagerImplT& MaterialMan)
{
    m_ModelLoaders[0]->Load(Skins, MaterialMan);
}


void LoaderDlodT::Load(ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures)
{
    m_ModelLoaders[0]->Load(GuiFixtures);
}


void LoaderDlodT::Load(ArrayT<CafuModelT::ChannelT>& Channels)
{
    m_ModelLoaders[0]->Load(Channels);
}


bool LoaderDlodT::Load(unsigned int Level, CafuModelT*& DlodModel, float& DlodDist)
{
    if (Level >= m_ModelLoaders.Size()) return false;

    delete DlodModel;
    DlodModel=NULL;

    try
    {
        DlodModel=new CafuModelT(*m_ModelLoaders[Level]);
    }
    catch (const LoadErrorT&)
    {
        return false;
    }

    DlodDist=m_EndRanges[Level-1];
    return true;
}
