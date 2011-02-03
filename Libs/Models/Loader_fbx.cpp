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

#include <fstream>


static std::ofstream Log("fbx-loader.log");
// static std::ostream& Log=std::cout;


static std::ostream& operator << (std::ostream& os, const KFbxVector4& A)
{
    return os << "fbxVec4(" << A[0] << ", " << A[1] << ", " << A[2] << ", " << A[3] << ")";
}

static std::ostream& operator << (std::ostream& os, const KFbxQuaternion& A)
{
    return os << "fbxQuat(" << A[0] << ", " << A[1] << ", " << A[2] << ", " << A[3] << ")";
}


class LoaderFbxT::FbxSceneT
{
    public:

    FbxSceneT(const std::string& FileName);
    ~FbxSceneT();

    /// Returns the root node of this scene.
    /*const*/ KFbxNode* GetRootNode() const { return m_Scene->GetRootNode(); }

    /// Recursively loads the joints, beginning at the given KFbxNode instance and the given parent index.
    void Load(ArrayT<CafuModelT::JointT>& Joints, int ParentIndex, /*const*/ KFbxNode* Node) const;

    /// Loads the animations.
    void Load(ArrayT<CafuModelT::AnimT>& Anims) const;


    private:

    void CleanUp();
    void ConvertNurbsAndPatches(KFbxNode* Node);

    KFbxSdkManager* m_SdkManager;
    KFbxScene*      m_Scene;
    KFbxImporter*   m_Importer;
};


LoaderFbxT::FbxSceneT::FbxSceneT(const std::string& FileName)
    : m_SdkManager(KFbxSdkManager::Create()),
      m_Scene(KFbxScene::Create(m_SdkManager, "")),
      m_Importer(KFbxImporter::Create(m_SdkManager, ""))
{
    Log << "\nBeginning import of \"" << FileName << "\" ...\n";

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


    // The coordinate system of 3D Studio Max happens to be the same as Cafu's (x: right, y: into screen, z: up),
    // thus make sure that the scene is converted to this coordinate system if necessary.
    if (m_Scene->GetGlobalSettings().GetAxisSystem()!=KFbxAxisSystem::Max)
    {
        Log << "Converting coordinate system to Cafu's...\n";
        KFbxAxisSystem::Max.ConvertScene(m_Scene);
    }


    // Let the user know how many centimeters one scene unit is.
    const KFbxSystemUnit SysUnit=m_Scene->GetGlobalSettings().GetSystemUnit();

    Log << "Scene unit info:\n"
        << "    scale factor: one scene unit is " << SysUnit.GetScaleFactor() << " cm (" << (SysUnit.GetScaleFactor()/2.54) << " inches)\n"
        << "    multiplier: " << SysUnit.GetMultiplier() << "\n";

    // if (SysUnit.GetScaleFactor()!=1.0)
    // {
    //     KFbxSystemUnit CafuSystemUnit(1.0);
    //     CafuSystemUnit.ConvertScene(gScene);
    // }


    // We cannot directly deal with NURBS and Patches, so convert them into mesh node attributes.
    ConvertNurbsAndPatches(m_Scene->GetRootNode());

    // Convert any .PC2 point cache data into the .MC format for
    // vertex cache deformer playback.
    // PreparePointCacheData(gScene);

    // Get the list of all the cameras in the scene.
    // FillCameraArray(gScene, gCameraArray);


    // Get the list of poses in the scene.
    Log << "Poses in scene: " << m_Scene->GetPoseCount() << "\n";

    for (int PoseNr=0; PoseNr<m_Scene->GetPoseCount(); PoseNr++)
    {
        const KFbxPose* Pose=m_Scene->GetPose(PoseNr);

        Log << "    " << PoseNr;
        if (Pose->IsBindPose()) Log << ", bind pose";
        if (Pose->IsRestPose()) Log << ", rest pose";
        Log << "\n";
    }


    // Load the texture data in memory (for supported formats)
    // LoadSupportedTextures(gScene, gTextureArray);
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


void LoaderFbxT::FbxSceneT::ConvertNurbsAndPatches(KFbxNode* Node)
{
    KFbxNodeAttribute* NodeAttribute=Node->GetNodeAttribute();

    if (NodeAttribute)
    {
        if (NodeAttribute->GetAttributeType()==KFbxNodeAttribute::eNURB ||
            NodeAttribute->GetAttributeType()==KFbxNodeAttribute::ePATCH)
        {
            KFbxGeometryConverter Converter(m_SdkManager);

            Log << "Node \"" << Node->GetName() << "\" is a " << (NodeAttribute->GetAttributeType()==KFbxNodeAttribute::eNURB ? "NURB" : "Patch") << ", triangulating...\n";
            Converter.TriangulateInPlace(Node);
        }
    }

    for (int ChildNr=0; ChildNr<Node->GetChildCount(); ChildNr++)
        ConvertNurbsAndPatches(Node->GetChild(ChildNr));
}


void LoaderFbxT::FbxSceneT::Load(ArrayT<CafuModelT::JointT>& Joints, int ParentIndex, /*const*/ KFbxNode* Node) const
{
    Log << "Loading node " << ParentIndex << ", \"" << Node->GetName() << "\"\n";

    // Note that KFbxAnimEvaluator::GetNodeGlobalTransform() is the proper method to call here:
    // It returns the node's default transformation matrix or the node's actual transformation
    // matrix at a specified point in time (depending on its second parameter pTime).
    // See FBX SDK Programmer's Guide pages 89 and 90 and its API method documentation for details.
    // Also see KFbxAnimEvaluator::SetContext(): as we set no anim stack, it automatically uses the first in the scene.
    const KFbxXMatrix& Transform  =m_Scene->GetEvaluator()->GetNodeGlobalTransform(Node);
    KFbxVector4        Translation=Transform.GetT();
    KFbxQuaternion     Quaternion =Transform.GetQ();
    KFbxVector4        Scale      =Transform.GetS();

    // TODO: If Scaling.x|y|z < 0.99 or > 1.01 then log warning.
    Quaternion.Normalize();
    if (Quaternion[3]>0) Quaternion=-Quaternion;

    Log << "    trans: " << Translation << "\n";
    Log << "    quat:  " << Quaternion << "\n";
    Log << "    scale: " << Scale << "\n";

    CafuModelT::JointT Joint;

    Joint.Name  =Node->GetName();
    Joint.Parent=ParentIndex;
    Joint.Pos   =Vector3dT(Translation[0], Translation[1], Translation[2]).AsVectorOfFloat();
    Joint.Qtr   =Vector3dT(Quaternion[0], Quaternion[1], Quaternion[2]).AsVectorOfFloat();

    Joints.PushBack(Joint);

    for (int ChildNr=0; ChildNr<Node->GetChildCount(); ChildNr++)
        Load(Joints, Joints.Size()-1, Node->GetChild(ChildNr));
}


void LoaderFbxT::FbxSceneT::Load(ArrayT<CafuModelT::AnimT>& Anims) const
{
    KTime FrameTime;
    KArrayTemplate<KString*> AnimStackNames;

    FrameTime.SetTime(0, 0, 0, 1, 0, m_Scene->GetGlobalSettings().GetTimeMode());
    Log << "Global scene FPS: " << 1.0/FrameTime.GetSecondDouble() << "\n";

    m_Scene->FillAnimStackNameArray(AnimStackNames);
    Log << "Anim stacks in scene: " << AnimStackNames.GetCount() << "\n";

    for (int NameNr=0; NameNr<AnimStackNames.GetCount(); NameNr++)
    {
        KFbxAnimStack* AnimStack=m_Scene->FindMember(FBX_TYPE(KFbxAnimStack), AnimStackNames[NameNr]->Buffer());

        // Make sure that the animation stack was found in the scene (it always should).
        if (AnimStack==NULL) continue;

        const bool  IsActive =AnimStackNames[NameNr]->Compare(KFbxGet<KString>(m_Scene->ActiveAnimStackName))==0;
        const KTime TimeStart=AnimStack->GetLocalTimeSpan().GetStart();
        const KTime TimeStop =AnimStack->GetLocalTimeSpan().GetStop();   // + 0.1*FrameTime;     // For roundoff error.

        m_Scene->GetEvaluator()->SetContext(AnimStack);
        Log << "    \"" << AnimStackNames[NameNr]->Buffer() << "\"" << (IsActive ? "    (active)" : "") << "\n";

        unsigned int FrameNr=0;
        for (KTime TimeNow=TimeStart; TimeNow<=TimeStop; TimeNow+=FrameTime)
        {
            FrameNr++;
        }

        Log << "        " << FrameNr << " frames\n";
    }

    FbxSdkDeleteAndClear(AnimStackNames);
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
    Log.flush();
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

    // Load the animations.
    m_FbxScene->Load(Anims);
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
