/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#include "Loader_fbx.hpp"
#ifdef HAVE_FBX_SDK
// #include "MaterialSystem/Renderer.hpp"
// #include "wx/textdlg.h"


LoaderFbxT::LoaderFbxT(const std::string& FileName) /*throw (ModelT::LoadError)*/
    : ModelLoaderT(FileName),
      m_SdkManager(KFbxSdkManager::Create()),
      m_Scene(KFbxScene::Create(m_SdkManager, "")),
      m_Importer(KFbxImporter::Create(m_SdkManager, ""))
{
    // Initialize the importer by providing a filename.
    if (!m_Importer->Initialize(m_FileName.c_str(), -1, m_SdkManager->GetIOSettings()))
    {
        std::string ErrorMsg=m_Importer->GetLastErrorString();

        CleanUp();
        throw LoadErrorT(ErrorMsg);
    }

    bool Result=m_Importer->Import(m_Scene);

    if (!Result && m_Importer->GetLastErrorID()==KFbxIO::ePASSWORD_ERROR)
    {
     // wxString Password=wxGetPasswordFromUser("Please enter password:", m_FileName);
        KString  KPwd("TODO!");

        m_SdkManager->GetIOSettings()->SetStringProp(IMP_FBX_PASSWORD, KPwd);
        m_SdkManager->GetIOSettings()->SetBoolProp(IMP_FBX_PASSWORD_ENABLE, true);

        Result=m_Importer->Import(m_Scene);
    }

    if (!Result)
    {
        std::string ErrorMsg=m_Importer->GetLastErrorString();

        CleanUp();
        throw LoadErrorT(ErrorMsg);
    }
}


LoaderFbxT::~LoaderFbxT()
{
    CleanUp();
}


void LoaderFbxT::CleanUp()
{
    m_Importer->Destroy();
    m_Scene->Destroy();
    m_SdkManager->Destroy();
}


bool LoaderFbxT::UseGivenTS() const
{
    // TODO...!
    return false;
}


void LoaderFbxT::Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims)
{
}


void LoaderFbxT::Load(ArrayT<CafuModelT::GuiLocT>& GuiLocs)
{
}


#else   // HAVE_FBX_SDK

LoaderFbxT::LoaderFbxT(const std::string& FileName) /*throw (ModelT::LoadError)*/
    : ModelLoaderT(FileName)
{
    throw LoadErrorT("This edition of the program was built without the Autodesk"
                     "FBX library that is required to import this file format.");
}

#endif  // HAVE_FBX_SDK
