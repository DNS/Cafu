/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

Cafu is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

Cafu is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Cafu. If not, see <http://www.gnu.org/licenses/>.

For support and more information about Cafu, visit us at <http://www.cafu.de>.
=================================================================================
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


const CafuModelT* ModelManagerT::GetModel(const std::string& FileName, std::string& ErrorMsg) const
{
    ErrorMsg="";
    std::map<std::string, CafuModelT*>::const_iterator It=m_Models.find(FileName);

    if (It!=m_Models.end()) return It->second;

    // Load the model at FileName and add it to m_Models.
    CafuModelT* NewModel=NULL;

    try
    {
             if (cf::String::EndsWith(FileName, "ase"    )) { LoaderAseT    Loader(FileName); NewModel=new CafuModelT(Loader); }
        else if (cf::String::EndsWith(FileName, "cmdl"   )) { LoaderCafuT   Loader(FileName); NewModel=new CafuModelT(Loader); }
        else if (cf::String::EndsWith(FileName, "dlod"   )) { LoaderDlodT   Loader(FileName); NewModel=new CafuModelT(Loader); }
        else if (cf::String::EndsWith(FileName, "lwo"    )) { LoaderLwoT    Loader(FileName); NewModel=new CafuModelT(Loader); }
        else if (cf::String::EndsWith(FileName, "mdl"    )) { LoaderHL1mdlT Loader(FileName); NewModel=new CafuModelT(Loader); }
        else if (cf::String::EndsWith(FileName, "md5"    )) { LoaderMd5T    Loader(FileName); NewModel=new CafuModelT(Loader); }
        else if (cf::String::EndsWith(FileName, "md5mesh")) { LoaderMd5T    Loader(FileName); NewModel=new CafuModelT(Loader); }
     // else if (cf::String::EndsWith(FileName, "3ds"    )) { LoaderFbxT    Loader(FileName); NewModel=new CafuModelT(Loader); }
     // else if (cf::String::EndsWith(FileName, "dae"    )) { LoaderFbxT    Loader(FileName); NewModel=new CafuModelT(Loader); }
     // else if (cf::String::EndsWith(FileName, "dxf"    )) { LoaderFbxT    Loader(FileName); NewModel=new CafuModelT(Loader); }
     // else if (cf::String::EndsWith(FileName, "fbx"    )) { LoaderFbxT    Loader(FileName); NewModel=new CafuModelT(Loader); }
     // else if (cf::String::EndsWith(FileName, "obj"    )) { LoaderFbxT    Loader(FileName); NewModel=new CafuModelT(Loader); }
        else throw ModelLoaderT::LoadErrorT(
            "No loader is available for model files of this type.\n"
            "Use CaWE in order to convert this model into a cmdl model, "
            "then load the cmdl model in place of this.");
    }
    catch (const ModelT::LoadError&)
    {
        LoaderDummyT Loader(FileName);

        NewModel=new CafuModelT(Loader);
        ErrorMsg="The file could not be loaded (unknown error).";
    }
    catch (const ModelLoaderT::LoadErrorT& LE)
    {
        LoaderDummyT Loader(FileName);

        NewModel=new CafuModelT(Loader);
        ErrorMsg=LE.what();
    }

    m_Models[FileName]=NewModel;
    return NewModel;
}
