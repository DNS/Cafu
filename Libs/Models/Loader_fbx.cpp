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


struct PosQtrScaleT
{
    Vector3fT Pos;
    Vector3fT Qtr;
    Vector3fT Scale;
};


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
    ArrayT< ArrayT<PosQtrScaleT> > GetSequData(KFbxAnimStack* AnimStack, const ArrayT<KFbxNode*>& Nodes, const ArrayT<KTime>& FrameTimes) const;

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
    Log << "Loading node " << Joints.Size() << ", \"" << Node->GetName() << "\", Parent == " << ParentIndex << "\n";

    // Note that KFbxAnimEvaluator::GetNodeGlobalTransform() is the proper method to call here:
    // It returns the node's default transformation matrix or the node's actual transformation
    // matrix at a specified point in time (depending on its second parameter pTime).
    // See FBX SDK Programmer's Guide pages 89 and 90 and its API method documentation for details.
    // Also see KFbxAnimEvaluator::SetContext(): as we set no anim stack, it automatically uses the first in the scene.
    const KFbxXMatrix& Transform  =m_Scene->GetEvaluator()->GetNodeGlobalTransform(Node);
    KFbxVector4        Translation=Transform.GetT();
    KFbxQuaternion     Quaternion =Transform.GetQ();
    KFbxVector4        Scale      =Transform.GetS();

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
    Joint.Scale =Vector3dT(Scale[0], Scale[1], Scale[2]).AsVectorOfFloat();

    Joints.PushBack(Joint);
    const int ThisIndex=Joints.Size()-1;

    for (int ChildNr=0; ChildNr<Node->GetChildCount(); ChildNr++)
        Load(Joints, ThisIndex, Node->GetChild(ChildNr));
}


// Returns the nodes in an array that parallels the Joints array.
static void GetNodes(ArrayT<KFbxNode*>& Nodes, KFbxNode* Node)
{
    Nodes.PushBack(Node);

    for (int ChildNr=0; ChildNr<Node->GetChildCount(); ChildNr++)
        GetNodes(Nodes, Node->GetChild(ChildNr));
}


static ArrayT<KTime> GetFrameTimes(const KFbxAnimStack* AnimStack, const KTime& TimeStep)
{
    ArrayT<KTime> FrameTimes;

    const KTime TimeStart=AnimStack->GetLocalTimeSpan().GetStart();
    const KTime TimeStop =AnimStack->GetLocalTimeSpan().GetStop();

    for (KTime TimeNow=TimeStart; TimeNow<=TimeStop; TimeNow+=TimeStep)
    {
        FrameTimes.PushBack(TimeNow);
    }

    return FrameTimes;
}


/// Gathers all animation data of a sequence (AnimStack) in uncompressed form in an array of the form SequData[NodeNr][FrameNr].
/// That is, for each frame and for each node, we store the position, quaternion and scale.
ArrayT< ArrayT<PosQtrScaleT> > LoaderFbxT::FbxSceneT::GetSequData(
    KFbxAnimStack* AnimStack, const ArrayT<KFbxNode*>& Nodes, const ArrayT<KTime>& FrameTimes) const
{
    ArrayT< ArrayT<PosQtrScaleT> > SequData;

    m_Scene->GetEvaluator()->SetContext(AnimStack);
    SequData.PushBackEmptyExact(Nodes.Size());

    for (unsigned long NodeNr=0; NodeNr<Nodes.Size(); NodeNr++)
    {
        SequData[NodeNr].PushBackEmptyExact(FrameTimes.Size());

        for (unsigned long FrameNr=0; FrameNr<FrameTimes.Size(); FrameNr++)
        {
            const KFbxXMatrix& Transform  =m_Scene->GetEvaluator()->GetNodeLocalTransform(Nodes[NodeNr], FrameTimes[FrameNr]);
            KFbxVector4        Translation=Transform.GetT();
            KFbxQuaternion     Quaternion =Transform.GetQ();
            KFbxVector4        Scale      =Transform.GetS();

            Quaternion.Normalize();
            if (Quaternion[3]>0) Quaternion=-Quaternion;

            SequData[NodeNr][FrameNr].Pos  =Vector3dT(Translation[0], Translation[1], Translation[2]).AsVectorOfFloat();
            SequData[NodeNr][FrameNr].Qtr  =Vector3dT(Quaternion[0], Quaternion[1], Quaternion[2]).AsVectorOfFloat();
            SequData[NodeNr][FrameNr].Scale=Vector3dT(Scale[0], Scale[1], Scale[2]).AsVectorOfFloat();
        }
    }

    // Ideally, this requires no extra copy, see <http://www.parashift.com/c++-faq-lite/ctors.html#faq-10.10>.
    return SequData;
}


void LoaderFbxT::FbxSceneT::Load(ArrayT<CafuModelT::AnimT>& Anims) const
{
    KTime                    TimeStep;
    KArrayTemplate<KString*> AnimStackNames;
    ArrayT<KFbxNode*>        Nodes;

    TimeStep.SetTime(0, 0, 0, 1, 0, m_Scene->GetGlobalSettings().GetTimeMode());
    m_Scene->FillAnimStackNameArray(AnimStackNames);
    GetNodes(Nodes, m_Scene->GetRootNode());

    Log << "Global scene FPS: " << 1.0/TimeStep.GetSecondDouble() << "\n";
    Log << "Anim stacks in scene: " << AnimStackNames.GetCount() << "\n";
    Log << "Animated nodes: " << Nodes.Size() << "\n";

    for (int NameNr=0; NameNr<AnimStackNames.GetCount(); NameNr++)
    {
        KFbxAnimStack* AnimStack=m_Scene->FindMember(FBX_TYPE(KFbxAnimStack), AnimStackNames[NameNr]->Buffer());

        // Make sure that the animation stack was found in the scene (it always should).
        if (AnimStack==NULL) continue;

        const ArrayT<KTime>                  FrameTimes=GetFrameTimes(AnimStack, TimeStep);
        const ArrayT< ArrayT<PosQtrScaleT> > SequData  =GetSequData(AnimStack, Nodes, FrameTimes);

        Log << "    \"" << AnimStackNames[NameNr]->Buffer() << "\"" << (AnimStackNames[NameNr]->Compare(KFbxGet<KString>(m_Scene->ActiveAnimStackName))==0 ? "    (active)" : "") << "\n";
        Log << "        " << FrameTimes.Size() << " frames\n";


        Anims.PushBackEmpty();
        CafuModelT::AnimT& Anim=Anims[NameNr];
        unsigned int       AnimData_Size=0;   // The current common value of Anim.Frames[FrameNr].AnimData.Size(), always the same for all frames.

     // Anim.Name=AnimStackNames[NameNr]->Buffer();
        Anim.FPS =float(1.0/TimeStep.GetSecondDouble());
     // Anim.Next=-1;

        Anim.AnimJoints.PushBackEmptyExact(Nodes.Size());
        Anim.Frames.PushBackEmptyExact(FrameTimes.Size());

        for (unsigned long NodeNr=0; NodeNr<Nodes.Size(); NodeNr++)
        {
            CafuModelT::AnimT::AnimJointT& AnimJoint=Anim.AnimJoints[NodeNr];

            // For (space) efficiency, the defaults are taken from frame 0, rather than from the default pose.
            AnimJoint.DefaultPos  =SequData[NodeNr][0].Pos;
            AnimJoint.DefaultQtr  =SequData[NodeNr][0].Qtr;
            AnimJoint.DefaultScale=SequData[NodeNr][0].Scale;
            AnimJoint.Flags=0;
            AnimJoint.FirstDataIdx=AnimData_Size;

            for (unsigned long FrameNr=0; FrameNr<FrameTimes.Size(); FrameNr++)
            {
                for (unsigned int i=0; i<3; i++)
                {
                    if (SequData[NodeNr][FrameNr].Pos  [i]!=AnimJoint.DefaultPos  [i]) AnimJoint.Flags|=(1u << (i+0));
                    if (SequData[NodeNr][FrameNr].Qtr  [i]!=AnimJoint.DefaultQtr  [i]) AnimJoint.Flags|=(1u << (i+3));
                    if (SequData[NodeNr][FrameNr].Scale[i]!=AnimJoint.DefaultScale[i]) AnimJoint.Flags|=(1u << (i+6));
                }
            }

            for (unsigned int i=0; i<9; i++)
            {
                if (((AnimJoint.Flags >> i) & 1)==0) continue;

                for (unsigned long FrameNr=0; FrameNr<FrameTimes.Size(); FrameNr++)
                {
                    assert(Anim.Frames[FrameNr].AnimData.Size()==AnimData_Size);

                         if (i<3) Anim.Frames[FrameNr].AnimData.PushBack(SequData[NodeNr][FrameNr].Pos  [i  ]);
                    else if (i<6) Anim.Frames[FrameNr].AnimData.PushBack(SequData[NodeNr][FrameNr].Qtr  [i-3]);
                    else          Anim.Frames[FrameNr].AnimData.PushBack(SequData[NodeNr][FrameNr].Scale[i-6]);
                }

                AnimData_Size++;
            }
        }

        // If it is a looping sequence whose last frame is a repetition of the first,
        // remove the redundant frame (our own code automatically wrap at the end of the sequence).
        // if (Sequ.Flags & 1) Anim.Frames.DeleteBack();

        // Fill in the bounding-box for each frame from the sequence BB from the mdl file, which doesn't keep per-frame BBs.
        // It would be more accurate of course if we re-computed the proper per-frame BB ourselves...
        const Vector3fT BBMin(-24, -24, -24);
        const Vector3fT BBMax(-BBMin);

        for (unsigned long FrameNr=0; FrameNr<Anim.Frames.Size(); FrameNr++)
            Anim.Frames[FrameNr].BB=BoundingBox3fT(Vector3fT(BBMin), Vector3fT(BBMax));
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
