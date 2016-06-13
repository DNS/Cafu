/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ModelManager.hpp"
#include "Model_cmdl.hpp"
#include "Loader_ase.hpp"
#include "Loader_cmdl.hpp"
#include "Loader_dlod.hpp"
#include "Loader_dummy.hpp"
#include "Loader_lwo.hpp"
#include "Loader_md5.hpp"
#include "Loader_mdl.hpp"
#include "String.hpp"


ModelManagerT::ModelManagerT()
{
}


ModelManagerT::~ModelManagerT()
{
    for (std::map<std::string, CafuModelT*>::const_iterator Model=m_Models.begin(); Model!=m_Models.end(); Model++)
        delete Model->second;

    m_Models.clear();
}


const CafuModelT* ModelManagerT::GetModel(const std::string& FileName, std::string* ErrorMsg) const
{
    if (ErrorMsg) (*ErrorMsg)="";
    std::map<std::string, CafuModelT*>::const_iterator It=m_Models.find(FileName);

    if (It!=m_Models.end()) return It->second;

    // Load the model at FileName and add it to m_Models.
    CafuModelT* NewModel=NULL;

    try
    {
             if (cf::String::EndsWith(FileName, "ase"    )) { LoaderAseT  Loader(FileName); NewModel = new CafuModelT(Loader); }
        else if (cf::String::EndsWith(FileName, "cmdl"   )) { LoaderCafuT Loader(FileName); NewModel = new CafuModelT(Loader); }
        else if (cf::String::EndsWith(FileName, "dlod"   )) { LoaderDlodT Loader(FileName); NewModel = new CafuModelT(Loader); }
        else if (cf::String::EndsWith(FileName, "lwo"    )) { LoaderLwoT  Loader(FileName); NewModel = new CafuModelT(Loader); }
        else if (cf::String::EndsWith(FileName, "mdl"    )) { LoaderMdlT  Loader(FileName); NewModel = new CafuModelT(Loader); }
        else if (cf::String::EndsWith(FileName, "md5"    )) { LoaderMd5T  Loader(FileName); NewModel = new CafuModelT(Loader); }
        else if (cf::String::EndsWith(FileName, "md5mesh")) { LoaderMd5T  Loader(FileName); NewModel = new CafuModelT(Loader); }
     // else if (cf::String::EndsWith(FileName, "3ds"    )) { LoaderFbxT  Loader(FileName); NewModel = new CafuModelT(Loader); }
     // else if (cf::String::EndsWith(FileName, "dae"    )) { LoaderFbxT  Loader(FileName); NewModel = new CafuModelT(Loader); }
     // else if (cf::String::EndsWith(FileName, "dxf"    )) { LoaderFbxT  Loader(FileName); NewModel = new CafuModelT(Loader); }
     // else if (cf::String::EndsWith(FileName, "fbx"    )) { LoaderFbxT  Loader(FileName); NewModel = new CafuModelT(Loader); }
     // else if (cf::String::EndsWith(FileName, "obj"    )) { LoaderFbxT  Loader(FileName); NewModel = new CafuModelT(Loader); }
        else throw ModelLoaderT::LoadErrorT(
            "No loader is available for model files of this type.\n"
            "Use CaWE in order to convert this model into a cmdl model, "
            "then load the cmdl model in place of this.");
    }
    catch (const ModelLoaderT::LoadErrorT& LE)
    {
        LoaderDummyT Loader(FileName);

        NewModel=new CafuModelT(Loader);
        if (ErrorMsg) (*ErrorMsg)=LE.what();
    }

    m_Models[FileName]=NewModel;
    return NewModel;
}
