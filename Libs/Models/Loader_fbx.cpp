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
#include "fbxsdk.h"
// #include "MaterialSystem/Renderer.hpp"
// #include "wx/textdlg.h"


class LoaderFbxT::FbxSceneT
{
    public:

    FbxSceneT(const std::string& FileName);
    ~FbxSceneT();

    /// Returns the root node of this scene.
    /*const*/ KFbxNode* GetRootNode() const { return m_Scene->GetRootNode(); }

    /// Recursively loads the joints, beginning at the given KFbxNode instance and the given parent index.
    void Load(ArrayT<CafuModelT::JointT>& Joints, int ParentIndex, /*const*/ KFbxNode* Node) const;


    private:

    void CleanUp();

    KFbxSdkManager* m_SdkManager;
    KFbxScene*      m_Scene;
    KFbxImporter*   m_Importer;
};


LoaderFbxT::FbxSceneT::FbxSceneT(const std::string& FileName)
    : m_SdkManager(KFbxSdkManager::Create()),
      m_Scene(KFbxScene::Create(m_SdkManager, "")),
      m_Importer(KFbxImporter::Create(m_SdkManager, ""))
{
    // Initialize the importer by providing a filename.
    if (!m_Importer->Initialize(FileName.c_str(), -1, m_SdkManager->GetIOSettings()))
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


LoaderFbxT::FbxSceneT::~FbxSceneT()
{
    CleanUp();
}


void LoaderFbxT::FbxSceneT::CleanUp()
{
    m_Importer->Destroy();
    m_Scene->Destroy();
    m_SdkManager->Destroy();
}


void LoaderFbxT::FbxSceneT::Load(ArrayT<CafuModelT::JointT>& Joints, int ParentIndex, /*const*/ KFbxNode* Node) const
{
    const KFbxXMatrix& Transform  =m_Scene->GetEvaluator()->GetNodeLocalTransform(Node);
    KFbxVector4        Translation=Transform.GetT();
    KFbxQuaternion     Quaternion =Transform.GetQ();
 // KFbxVector4        Scale      =Transform.GetS();

    // TODO: If Scaling.x|y|z < 0.99 or > 1.01 then log warning.
    Quaternion.Normalize();
    if (Quaternion[3]>0) Quaternion=-Quaternion;

    CafuModelT::JointT Joint;

    Joint.Name  =Node->GetName();
    Joint.Parent=ParentIndex;
    Joint.Pos   =Vector3dT(Translation[0], Translation[1], Translation[2]).AsVectorOfFloat();
    Joint.Qtr   =Vector3dT(Quaternion[0], Quaternion[1], Quaternion[2]).AsVectorOfFloat();

    Joints.PushBack(Joint);

    for (int ChildNr=0; ChildNr<Node->GetChildCount(); ChildNr++)
        Load(Joints, Joints.Size()-1, Node->GetChild(ChildNr));
}


/******************/
/*** LoaderFbxT ***/
/******************/

LoaderFbxT::LoaderFbxT(const std::string& FileName)
    : ModelLoaderT(FileName),
      m_FbxScene(new FbxSceneT(FileName))
{
}


LoaderFbxT::~LoaderFbxT()
{
    delete m_FbxScene;
}


bool LoaderFbxT::UseGivenTS() const
{
    // TODO...!
    return false;
}


void LoaderFbxT::Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims)
{
    // We unconditionally import all nodes in the FBX scene as joints
    // (and leave it up to the caller to e.g. remove unused joints later).
    m_FbxScene->Load(Joints, -1, m_FbxScene->GetRootNode());
}


void LoaderFbxT::Load(ArrayT<CafuModelT::GuiLocT>& GuiLocs)
{
}


#else   // HAVE_FBX_SDK

// This is a stub implementation for use whenever the Autodesk FBX SDK is not available.
LoaderFbxT::LoaderFbxT(const std::string& FileName)
    : ModelLoaderT(FileName)
{
    throw LoadErrorT("This edition of the program was built without the Autodesk FBX"
                     "library that is required to import files in this file format.");
}

LoaderFbxT::~LoaderFbxT() { }
bool LoaderFbxT::UseGivenTS() const { return false; }
void LoaderFbxT::Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims) { }
void LoaderFbxT::Load(ArrayT<CafuModelT::GuiLocT>& GuiLocs) { }

#endif  // HAVE_FBX_SDK
