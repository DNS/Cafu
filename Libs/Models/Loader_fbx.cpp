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

#include "Loader_fbx.hpp"
#ifdef HAVE_FBX_SDK
#include "MaterialSystem/Material.hpp"
#include "String.hpp"
#include "fbxsdk.h"

#if defined(_WIN32) && _MSC_VER<1600
#include "pstdint.h"            // Paul Hsieh's portable implementation of the stdint.h header.
#else
#include <stdint.h>
#endif

#include <fstream>
#include <map>

#if defined(_WIN32) && defined(_MSC_VER)
// Turn off "warning C4355: 'this' : used in base member initializer list".
#pragma warning(disable:4355)
#endif


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

static Vector3fT conv(const KFbxVector4& V)
{
    return Vector3dT(V[0], V[1], V[2]).AsVectorOfFloat();
}

static CafuModelT::MeshT::WeightT CreateWeight(unsigned int JointIdx, float w, const Vector3fT& Pos)
{
    CafuModelT::MeshT::WeightT Weight;

    Weight.JointIdx=JointIdx;
    Weight.Weight  =w;
    Weight.Pos     =Pos;

    return Weight;
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

    FbxSceneT(const LoaderFbxT& MainClass, UserCallbacksI& UserCallbacks, const std::string& FileName);
    ~FbxSceneT();

    /// Returns the root node of this scene.
    const KFbxNode* GetRootNode() const { return m_Scene->GetRootNode(); }

    /// Recursively loads the joints, beginning at the given KFbxNode instance and the given parent index.
    void Load(ArrayT<CafuModelT::JointT>& Joints, int ParentIndex, const KFbxNode* Node) const;

    /// Loads the meshes.
    void Load(ArrayT<CafuModelT::MeshT>& Meshes, MaterialManagerImplT& MaterialMan) const;

    /// Loads the animations.
    void Load(ArrayT<CafuModelT::AnimT>& Anims) const;


    private:

    void CleanUp();
    void GetNodes(const KFbxNode* Node);
    void ConvertNurbsAndPatches(KFbxNode* Node);
    void GetWeights(const KFbxMesh* Mesh, const unsigned long MeshNodeNr, ArrayT< ArrayT<CafuModelT::MeshT::WeightT> >& Weights) const;
    bool LoadMaterial(MaterialT& Mat, const KFbxSurfaceMaterial* FbxMaterial) const;
    ArrayT< ArrayT<PosQtrScaleT> > GetSequData(KFbxAnimStack* AnimStack, const ArrayT<KTime>& FrameTimes) const;

    const LoaderFbxT&       m_MainClass;
    UserCallbacksI&         m_UserCallbacks;    ///< Interface to get the password from the user.
    KFbxSdkManager*         m_SdkManager;
    KFbxScene*              m_Scene;
    KFbxImporter*           m_Importer;
    ArrayT<const KFbxNode*> m_Nodes;
};


LoaderFbxT::FbxSceneT::FbxSceneT(const LoaderFbxT& MainClass, UserCallbacksI& UserCallbacks, const std::string& FileName)
    : m_MainClass(MainClass),
      m_UserCallbacks(UserCallbacks),
      m_SdkManager(KFbxSdkManager::Create()),
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
        const KString Password=UserCallbacks.GetPasswordFromUser("Please enter the password to open file\n" + FileName).c_str();

        m_SdkManager->GetIOSettings()->SetStringProp(IMP_FBX_PASSWORD, Password);
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


    // For future reference, create a linear record (in array m_Nodes) of all nodes in the scene.
    // It is important that the nodes are in the same order as in Load(ArrayT<CafuModelT::JointT>& Joints, ...).
    GetNodes(m_Scene->GetRootNode());


#if 0
    // Get the list of all materials in the scene.
    Log << "Materials in scene:\n";

    for (unsigned long NodeNr=0; NodeNr<m_Nodes.Size(); NodeNr++)
    {
        const KFbxNode*           Node          =m_Nodes[NodeNr];
        const KFbxLayerContainer* LayerContainer=KFbxCast<KFbxLayerContainer>(Node->GetNodeAttribute());    // Must use KFbxCast<T> instead of dynamic_cast<T*> for classes of the FBX SDK.

        if (LayerContainer)   // Typically a mesh, but can also be a Nurb, a patch, etc.
        {
            Log << "    Node " << NodeNr << " has " << Node->GetSrcObjectCount(KFbxSurfaceMaterial::ClassId) << " material(s)\n";
            const int lNbMat=Node->GetSrcObjectCount(KFbxSurfaceMaterial::ClassId);

            for (int lMaterialIndex=0; lMaterialIndex<lNbMat; lMaterialIndex++)
            {
                KFbxSurfaceMaterial* lMaterial=KFbxCast<KFbxSurfaceMaterial>(Node->GetSrcObject(KFbxSurfaceMaterial::ClassId, lMaterialIndex));

                if (lMaterial)
                {
                    KFbxProperty lProperty=lMaterial->FindProperty(KFbxSurfaceMaterial::sDiffuse);

                    if (lProperty.IsValid())
                    {
                        const int lNbTex = lProperty.GetSrcObjectCount(KFbxFileTexture::ClassId);

                        for (int lTextureIndex=0; lTextureIndex<lNbTex; lTextureIndex++)
                        {
                            KFbxFileTexture* lTexture=KFbxCast<KFbxFileTexture>(lProperty.GetSrcObject(KFbxFileTexture::ClassId, lTextureIndex));

                            //if (lTexture)
                            //    LoadTexture(lTexture, pTextureArray);
                        }
                    }
                }
            }
        }
    }
#endif
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


void LoaderFbxT::FbxSceneT::GetNodes(const KFbxNode* Node)
{
    m_Nodes.PushBack(Node);

    for (int ChildNr=0; ChildNr<Node->GetChildCount(); ChildNr++)
        GetNodes(Node->GetChild(ChildNr));
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


void LoaderFbxT::FbxSceneT::Load(ArrayT<CafuModelT::JointT>& Joints, int ParentIndex, const KFbxNode* Node) const
{
    Log << "Loading node " << Joints.Size() << ", \"" << Node->GetName() << "\", Parent == " << ParentIndex << "\n";
 // Log << "        Character Link Count: " << Node->GetCharacterLinkCount() << "\n";
    Log << "        Attribute Count: " << Node->GetNodeAttributeCount() << "\n";

    if (!Node->GetNodeAttribute())
    {
        Log << "        [No attribute assigned.]\n";
    }
    else
    {
        switch (Node->GetNodeAttribute()->GetAttributeType())
        {
            case KFbxNodeAttribute::eUNIDENTIFIED      : Log << "        eUNIDENTIFIED      \n"; break;
            case KFbxNodeAttribute::eNULL              : Log << "        eNULL              \n"; break;
            case KFbxNodeAttribute::eMARKER            : Log << "        eMARKER            \n"; break;
            case KFbxNodeAttribute::eSKELETON          : Log << "        eSKELETON          \n"; break;
            case KFbxNodeAttribute::eMESH              : Log << "        eMESH              \n"; break;
            case KFbxNodeAttribute::eNURB              : Log << "        eNURB              \n"; break;
            case KFbxNodeAttribute::ePATCH             : Log << "        ePATCH             \n"; break;
            case KFbxNodeAttribute::eCAMERA            : Log << "        eCAMERA            \n"; break;
            case KFbxNodeAttribute::eCAMERA_STEREO     : Log << "        eCAMERA_STEREO     \n"; break;
            case KFbxNodeAttribute::eCAMERA_SWITCHER   : Log << "        eCAMERA_SWITCHER   \n"; break;
            case KFbxNodeAttribute::eLIGHT             : Log << "        eLIGHT             \n"; break;
            case KFbxNodeAttribute::eOPTICAL_REFERENCE : Log << "        eOPTICAL_REFERENCE \n"; break;
            case KFbxNodeAttribute::eOPTICAL_MARKER    : Log << "        eOPTICAL_MARKER    \n"; break;
            case KFbxNodeAttribute::eNURBS_CURVE       : Log << "        eNURBS_CURVE       \n"; break;
            case KFbxNodeAttribute::eTRIM_NURBS_SURFACE: Log << "        eTRIM_NURBS_SURFACE\n"; break;
            case KFbxNodeAttribute::eBOUNDARY          : Log << "        eBOUNDARY          \n"; break;
            case KFbxNodeAttribute::eNURBS_SURFACE     : Log << "        eNURBS_SURFACE     \n"; break;
            case KFbxNodeAttribute::eSHAPE             : Log << "        eSHAPE             \n"; break;
            case KFbxNodeAttribute::eLODGROUP          : Log << "        eLODGROUP          \n"; break;
            case KFbxNodeAttribute::eSUBDIV            : Log << "        eSUBDIV            \n"; break;
            case KFbxNodeAttribute::eCACHED_EFFECT     : Log << "        eCACHED_EFFECT     \n"; break;
            default:                                     Log << "        [UNKNOWN ATTRIBUTE]\n"; break;
        }
    }

    // Note that KFbxAnimEvaluator::GetNodeLocalTransform() is the proper method to call here:
    // It returns the node's default transformation matrix or the node's actual transformation
    // matrix at a specified point in time (depending on its second parameter pTime).
    // See FBX SDK Programmer's Guide pages 89 and 90 and its API method documentation for details.
    // Also see KFbxAnimEvaluator::SetContext(): as we set no anim stack, it automatically uses the first in the scene.
    const KFbxXMatrix& Transform  =m_Scene->GetEvaluator()->GetNodeLocalTransform(const_cast<KFbxNode*>(Node));
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
    Joint.Pos   =conv(Translation);
    Joint.Qtr   =Vector3dT(Quaternion[0], Quaternion[1], Quaternion[2]).AsVectorOfFloat();
    Joint.Scale =conv(Scale);

    Joints.PushBack(Joint);
    const int ThisIndex=Joints.Size()-1;

    assert(m_Nodes[ThisIndex]==Node);   // Make sure that the m_Nodes array is in fact "parallel" to the Joints array.

    for (int ChildNr=0; ChildNr<Node->GetChildCount(); ChildNr++)
        Load(Joints, ThisIndex, Node->GetChild(ChildNr));
}


// This function is from file GetPosition.cxx of the Autodesk SDK ViewScene example:
// Get the geometry deformation local to a node. It is never inherited by the children.
static KFbxXMatrix GetGeometry(const KFbxNode* pNode)
{
    KFbxXMatrix GeometryMat;

    GeometryMat.SetT(pNode->GetGeometricTranslation(KFbxNode::eSOURCE_SET));
    GeometryMat.SetR(pNode->GetGeometricRotation   (KFbxNode::eSOURCE_SET));
    GeometryMat.SetS(pNode->GetGeometricScaling    (KFbxNode::eSOURCE_SET));

    return GeometryMat;
}


/// Returns, for each vertex in the mesh, the list of weights that affect it.
void LoaderFbxT::FbxSceneT::GetWeights(const KFbxMesh* Mesh, const unsigned long MeshNodeNr, ArrayT< ArrayT<CafuModelT::MeshT::WeightT> >& Weights) const
{
    // All the links must have the same link mode.
    KFbxCluster::ELinkMode ClusterMode=KFbxCluster::eNORMALIZE;     // Updated below.

    Weights.PushBackEmptyExact(Mesh->GetControlPointsCount());

    for (int DeformerNr=0; DeformerNr < Mesh->GetDeformerCount(); DeformerNr++)
    {
        Log << "    " << DeformerNr << ", ";

        switch (Mesh->GetDeformer(DeformerNr)->GetDeformerType())
        {
            case KFbxDeformer::eSKIN:         Log << "eSKIN\n"; break;
            case KFbxDeformer::eVERTEX_CACHE: Log << "eVERTEX_CACHE\n"; break;
            default:                          Log << "[unknown deformer type]\n"; break;
        }

        KFbxSkin* Skin=KFbxCast<KFbxSkin>(Mesh->GetDeformer(DeformerNr));   // Must use KFbxCast<T> instead of dynamic_cast<T*> for classes of the FBX SDK.

        if (Skin && Skin->GetClusterCount()>0)
        {
            ArrayT<KFbxVector4> GlobalBindPose;
            KFbxXMatrix         MeshToGlobalBindPose;

            // Transform the vertices from the local space of Mesh to bind pose in global space.
            GlobalBindPose.PushBackEmptyExact(Mesh->GetControlPointsCount());

            Skin->GetCluster(0)->GetTransformMatrix(MeshToGlobalBindPose);
            MeshToGlobalBindPose *= GetGeometry(Mesh->GetNode());

            for (unsigned long VertexNr=0; VertexNr<GlobalBindPose.Size(); VertexNr++)
                GlobalBindPose[VertexNr]=MeshToGlobalBindPose.MultT(Mesh->GetControlPoints()[VertexNr]);

            // For additional information regarding this code, besides the FBX SDK documentation and the ViewScene sample, see my post at
            // http://area.autodesk.com/forum/autodesk-fbx/fbx-sdk/getting-the-local-transformation-matrix-for-the-vertices-of-a-cluster/
            for (int ClusterNr=0; ClusterNr<Skin->GetClusterCount(); ClusterNr++)
            {
                const KFbxCluster* Cluster =Skin->GetCluster(ClusterNr);
                const unsigned int JointIdx=m_Nodes.Find(Cluster->GetLink());

                Log << "        cluster " << ClusterNr << ", link mode: " << Cluster->GetLinkMode() << "\n";

                if (JointIdx>=m_Nodes.Size()) continue;

                // Update the ClusterMode (it should/must be the same for all links).
                ClusterMode=Cluster->GetLinkMode();

                // Get the matrix that transforms the vertices from global space to local bone space in bind pose.
                KFbxXMatrix BoneBindingMatrix;
                Cluster->GetTransformLinkMatrix(BoneBindingMatrix);
                KFbxXMatrix GlobalToLocalBoneBindPose=BoneBindingMatrix.Inverse();

                for (int i=0; i<Cluster->GetControlPointIndicesCount(); i++)
                {
                    const int   VertexNr=Cluster->GetControlPointIndices()[i];
                    const float w       =float(Cluster->GetControlPointWeights()[i]);

                    if (VertexNr >= Mesh->GetControlPointsCount()) continue;
                    if (w==0.0f) continue;

                    Weights[VertexNr].PushBack(CreateWeight(JointIdx, w, conv(GlobalToLocalBoneBindPose.MultT(GlobalBindPose[VertexNr]))));
                }
            }
        }
    }

    // Renormalize, complete or correct the weights for each vertex.
    for (unsigned long VertexNr=0; VertexNr<Weights.Size(); VertexNr++)
    {
        float wSum=0.0f;

        for (unsigned long WNr=0; WNr<Weights[VertexNr].Size(); WNr++)
            wSum+=Weights[VertexNr][WNr].Weight;

        if (wSum==0.0f)
        {
            Weights[VertexNr].Overwrite();
            Weights[VertexNr].PushBack(CreateWeight(MeshNodeNr, 1.0f, conv(Mesh->GetControlPoints()[VertexNr])));
        }
        else
        {
            switch (ClusterMode)
            {
                case KFbxCluster::eNORMALIZE:
                    for (unsigned long WNr=0; WNr<Weights[VertexNr].Size(); WNr++)
                        Weights[VertexNr][WNr].Weight/=wSum;
                    break;

                case KFbxCluster::eTOTAL1:
                    if (wSum!=1.0f)
                        Weights[VertexNr].PushBack(CreateWeight(MeshNodeNr, 1.0f-wSum, conv(Mesh->GetControlPoints()[VertexNr])));
                    break;

                case KFbxCluster::eADDITIVE:
                    // This case is not implemented/supported yet.
                    break;
            }
        }
    }
}


static std::string GetTexFileName(const KFbxSurfaceMaterial* FbxMaterial, const char* PropName)
{
    KFbxProperty Property=FbxMaterial->FindProperty(PropName);
    if (!Property.IsValid()) return "";

    const int        TextureIndex=0;
    KFbxFileTexture* Texture=KFbxCast<KFbxFileTexture>(Property.GetSrcObject(KFbxFileTexture::ClassId, TextureIndex));  // Must use KFbxCast<T> instead of dynamic_cast<T*> for classes of the FBX SDK.
    if (!Texture) return "";

    const char* FileName=Texture->GetRelativeFileName();
    if (FileName!=NULL && FileName[0]!=0) return cf::String::Replace(FileName, "\\", "/");

    FileName=Texture->GetFileName();
    if (FileName!=NULL && FileName[0]!=0) return cf::String::Replace(FileName, "\\", "/");

    return "";
}


/// Attempts to load a MaterialT material from a KFbxSurfaceMaterial.
/// @param Mat           The material to initialize.
/// @param FbxMaterial   The source material.
/// @returns \c true on success (\c Mat could be successfully/meaningfully initialized from \c FbxMaterial), \c false otherwise.
bool LoaderFbxT::FbxSceneT::LoadMaterial(MaterialT& Mat, const KFbxSurfaceMaterial* FbxMaterial) const
{
    const std::string BaseDir=cf::String::GetPath(m_MainClass.GetFileName())+"/";

    if (!FbxMaterial) return false;

    std::string fn=GetTexFileName(FbxMaterial, KFbxSurfaceMaterial::sDiffuse);
    if (fn!="") Mat.DiffMapComp=MapCompositionT(fn, BaseDir); else return false;

    fn=GetTexFileName(FbxMaterial, KFbxSurfaceMaterial::sNormalMap);
    if (fn!="") Mat.NormMapComp=MapCompositionT(fn, BaseDir);

    fn=GetTexFileName(FbxMaterial, KFbxSurfaceMaterial::sBump);
    if (fn!="") Mat.NormMapComp=MapCompositionT(std::string("hm2nm(")+fn+", 1)", BaseDir);

    fn=GetTexFileName(FbxMaterial, KFbxSurfaceMaterial::sSpecular);
    if (fn!="") Mat.SpecMapComp=MapCompositionT(fn, BaseDir);

    fn=GetTexFileName(FbxMaterial, KFbxSurfaceMaterial::sEmissive);
    if (fn!="") Mat.LumaMapComp=MapCompositionT(fn, BaseDir);

    Mat.RedGen  =ExpressionT(ExpressionT::SymbolALRed);
    Mat.GreenGen=ExpressionT(ExpressionT::SymbolALGreen);
    Mat.BlueGen =ExpressionT(ExpressionT::SymbolALBlue);

    return true;
}


void LoaderFbxT::FbxSceneT::Load(ArrayT<CafuModelT::MeshT>& Meshes, MaterialManagerImplT& MaterialMan) const
{
    Log << "\n";

    for (unsigned long NodeNr=0; NodeNr<m_Nodes.Size(); NodeNr++)
    {
        const KFbxMesh* Mesh=KFbxCast<KFbxMesh>(m_Nodes[NodeNr]->GetNodeAttribute());   // Must use KFbxCast<T> instead of dynamic_cast<T*> for classes of the FBX SDK.

        if (!Mesh) continue;
        Log << "Node " << NodeNr << " is a mesh with " << Mesh->GetPolygonCount() << " polygons and " << Mesh->GetDeformerCount() << " deformers.\n";

        ArrayT< ArrayT<CafuModelT::MeshT::WeightT> > VertexWeights;   // For each vertex in the mesh, an array of weights that affect it.
        GetWeights(Mesh, NodeNr, VertexWeights);

        Meshes.PushBackEmpty();
        CafuModelT::MeshT& CafuMesh=Meshes[Meshes.Size()-1];

        CafuMesh.Name=Mesh->GetName();
        if (CafuMesh.Name=="") CafuMesh.Name=m_Nodes[NodeNr]->GetName();
        if (CafuMesh.Name=="") CafuMesh.Name="Mesh";

        // Create the "flat" list of weights, and for each vertex, record the first index into this list.
        ArrayT<unsigned long> VertexFirstWeightIdx;

        for (unsigned long VertexNr=0; VertexNr<VertexWeights.Size(); VertexNr++)
        {
            VertexFirstWeightIdx.PushBack(CafuMesh.Weights.Size());
            CafuMesh.Weights.PushBack(VertexWeights[VertexNr]);
        }

        // Create the list of triangles, also creating the list of vertices as we go.
        const KFbxLayerElement::EMappingMode        MappingMode=(Mesh->GetLayer(0) && Mesh->GetLayer(0)->GetUVs()) ? Mesh->GetLayer(0)->GetUVs()->GetMappingMode() : KFbxLayerElement::eNONE;
        KFbxLayerElementArrayTemplate<KFbxVector2>* UVArray    =NULL;
        std::map<uint64_t, unsigned int>            UniqueVertices;     // Maps tuples of (Mesh->GetPolygonVertex(), Mesh->GetTextureUVIndex()) to indices into CafuMesh.Vertices.

        Mesh->GetTextureUV(&UVArray, KFbxLayerElement::eDIFFUSE_TEXTURES);

        for (int PolyNr=0; PolyNr<Mesh->GetPolygonCount(); PolyNr++)
        {
            for (int PolyTriNr=0; PolyTriNr < Mesh->GetPolygonSize(PolyNr)-2; PolyTriNr++)
            {
                CafuModelT::MeshT::TriangleT Tri;
                const int                    TriVIs[3]={ 0, PolyTriNr+2, PolyTriNr+1 };

                for (unsigned int i=0; i<3; i++)
                {
                    const int VertexIdx=Mesh->GetPolygonVertex(PolyNr, TriVIs[i]);
                    const int TexUV_Idx=(MappingMode==KFbxLayerElement::eBY_POLYGON_VERTEX) ? const_cast<KFbxMesh*>(Mesh)->GetTextureUVIndex(PolyNr, TriVIs[i])
                                       /*MappingMode==KFbxLayerElement::eBY_CONTROL_POINT*/ : VertexIdx;

                    const uint64_t Tuple=(uint64_t(uint32_t(VertexIdx)) << 32) | uint64_t(uint32_t(TexUV_Idx));
                    const std::map<uint64_t, unsigned int>::const_iterator It=UniqueVertices.find(Tuple);

                    if (It!=UniqueVertices.end())
                    {
                        Tri.VertexIdx[i]=It->second;
                    }
                    else
                    {
                        const unsigned long VertexNr=CafuMesh.Vertices.Size();
                        CafuMesh.Vertices.PushBackEmpty();

                        CafuMesh.Vertices[VertexNr].u=UVArray ? float(UVArray->GetAt(TexUV_Idx)[0]) : 0.0f;
                        CafuMesh.Vertices[VertexNr].v=UVArray ? float(UVArray->GetAt(TexUV_Idx)[1]) : 0.0f;
                        CafuMesh.Vertices[VertexNr].FirstWeightIdx=VertexFirstWeightIdx[VertexIdx];
                        CafuMesh.Vertices[VertexNr].NumWeights=VertexWeights[VertexIdx].Size();

                        Tri.VertexIdx[i]=VertexNr;
                        UniqueVertices[Tuple]=VertexNr;
                    }
                }

                CafuMesh.Triangles.PushBack(Tri);
            }
        }

        // Set the material.
        const int                  FbxMaterialIndex=0;
        const KFbxSurfaceMaterial* FbxMaterial=m_Nodes[NodeNr]->GetMaterial(FbxMaterialIndex);
        const std::string          MatName=FbxMaterial ? FbxMaterial->GetName() : "wire-frame";

        CafuMesh.Material=MaterialMan.GetMaterial(MatName);

        if (!CafuMesh.Material)
        {
            MaterialT Mat;

            Mat.Name           =MatName;
            Mat.meta_EditorSave=true;

            CafuMesh.Material=MaterialMan.RegisterMaterial(LoadMaterial(Mat, FbxMaterial) ? Mat : m_MainClass.CreateDefaultMaterial(MatName));
        }
    }
}


static ArrayT<KTime> GetFrameTimes(const KFbxAnimStack* AnimStack, const KTime& TimeStep)
{
    ArrayT<KTime> FrameTimes;

    const KTime TimeStart=AnimStack->GetLocalTimeSpan().GetStart();
    const KTime TimeStop =AnimStack->GetLocalTimeSpan().GetStop();

    Log << "    " << "start: " << TimeStart.Get() << " (" << TimeStart.GetSecondDouble() << ")";
    Log <<    " " << "stop: "  << TimeStop.Get()  << " (" << TimeStop.GetSecondDouble()  << ")";
    Log <<    " " << "step: "  << TimeStep.Get()  << " (" << TimeStep.GetSecondDouble()  << ")" << "\n";

    for (KTime TimeNow=TimeStart; TimeNow<=TimeStop; TimeNow+=TimeStep)
    {
        FrameTimes.PushBack(TimeNow);

        if (FrameTimes.Size()>30*3600*10)
        {
            // Frame times that are worth 10 hours at 30 FPS??
            Log.flush();
            throw cfSizeOverflow();
        }
    }

    return FrameTimes;
}


/// Gathers all animation data of a sequence (AnimStack) in uncompressed form in an array of the form SequData[NodeNr][FrameNr].
/// That is, for each frame and for each node, we store the position, quaternion and scale.
ArrayT< ArrayT<PosQtrScaleT> > LoaderFbxT::FbxSceneT::GetSequData(KFbxAnimStack* AnimStack, const ArrayT<KTime>& FrameTimes) const
{
    ArrayT< ArrayT<PosQtrScaleT> > SequData;

    m_Scene->GetEvaluator()->SetContext(AnimStack);
    SequData.PushBackEmptyExact(m_Nodes.Size());

    for (unsigned long NodeNr=0; NodeNr<m_Nodes.Size(); NodeNr++)
    {
        SequData[NodeNr].PushBackEmptyExact(FrameTimes.Size());

        for (unsigned long FrameNr=0; FrameNr<FrameTimes.Size(); FrameNr++)
        {
            const KFbxXMatrix& Transform  =m_Scene->GetEvaluator()->GetNodeLocalTransform(const_cast<KFbxNode*>(m_Nodes[NodeNr]), FrameTimes[FrameNr]);
            KFbxVector4        Translation=Transform.GetT();
            KFbxQuaternion     Quaternion =Transform.GetQ();
            KFbxVector4        Scale      =Transform.GetS();

            Quaternion.Normalize();
            if (Quaternion[3]>0) Quaternion=-Quaternion;

            SequData[NodeNr][FrameNr].Pos  =conv(Translation);
            SequData[NodeNr][FrameNr].Qtr  =Vector3dT(Quaternion[0], Quaternion[1], Quaternion[2]).AsVectorOfFloat();
            SequData[NodeNr][FrameNr].Scale=conv(Scale);
        }
    }

    // Ideally, this requires no extra copy, see <http://www.parashift.com/c++-faq-lite/ctors.html#faq-10.10>.
    return SequData;
}


void LoaderFbxT::FbxSceneT::Load(ArrayT<CafuModelT::AnimT>& Anims) const
{
    KTime                    TimeStep;
    KArrayTemplate<KString*> AnimStackNames;

    TimeStep.SetTime(0, 0, 0, 1, 0, m_Scene->GetGlobalSettings().GetTimeMode());
    m_Scene->FillAnimStackNameArray(AnimStackNames);

    Log << "\n";
    Log << "Global scene FPS: " << 1.0/TimeStep.GetSecondDouble() << "\n";
    Log << "Anim stacks in scene: " << AnimStackNames.GetCount() << "\n";
    Log << "Animated nodes: " << m_Nodes.Size() << "\n";

    for (int NameNr=0; NameNr<AnimStackNames.GetCount(); NameNr++)
    {
        KFbxAnimStack* AnimStack=m_Scene->FindMember(FBX_TYPE(KFbxAnimStack), AnimStackNames[NameNr]->Buffer());

        // Make sure that the animation stack was found in the scene (it always should).
        if (AnimStack==NULL) continue;

        const ArrayT<KTime>                  FrameTimes=GetFrameTimes(AnimStack, TimeStep);
        const ArrayT< ArrayT<PosQtrScaleT> > SequData  =GetSequData(AnimStack, FrameTimes);

        Log << "    \"" << AnimStackNames[NameNr]->Buffer() << "\"\n";
        // if (AnimStackNames[NameNr]->Compare(KFbxGet<KString>(m_Scene->ActiveAnimStackName))==0) Log << "    (active)";   // Why does this corrupt the stack under (x86_64) Linux?
        Log << "        " << FrameTimes.Size() << " frames\n";


        Anims.PushBackEmpty();
        CafuModelT::AnimT& Anim=Anims[NameNr];
        unsigned int       AnimData_Size=0;   // The current common value of Anim.Frames[FrameNr].AnimData.Size(), always the same for all frames.

        Anim.Name=AnimStackNames[NameNr]->Buffer();
        Anim.FPS =float(1.0/TimeStep.GetSecondDouble());
        Anim.Next=-1;

        Anim.AnimJoints.PushBackEmptyExact(m_Nodes.Size());
        Anim.Frames.PushBackEmptyExact(FrameTimes.Size());

        for (unsigned long NodeNr=0; NodeNr<m_Nodes.Size(); NodeNr++)
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

        // The caller computes the proper bounding-box for each frame.
        for (unsigned long FrameNr=0; FrameNr<Anim.Frames.Size(); FrameNr++)
            Anim.Frames[FrameNr].BB=BoundingBox3fT();
    }

    FbxSdkDeleteAndClear(AnimStackNames);
}


/******************/
/*** LoaderFbxT ***/
/******************/

LoaderFbxT::LoaderFbxT(const std::string& FileName, UserCallbacksI& UserCallbacks, int Flags)
    : ModelLoaderT(FileName, Flags | REMOVE_DEGEN_TRIANGLES),   // Always remove triangles with zero-length edges (required for proper loading).
      m_FbxScene(new FbxSceneT(*this, UserCallbacks, FileName))
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


void LoaderFbxT::Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan)
{
    // We unconditionally import all nodes in the FBX scene as joints
    // (and leave it up to the caller to e.g. remove unused joints later).
    m_FbxScene->Load(Joints, -1, m_FbxScene->GetRootNode());

    // Load the meshes.
    m_FbxScene->Load(Meshes, MaterialMan);

    // Load the animations.
    m_FbxScene->Load(Anims);

    // Compute the proper bounding-box for each frame of each animation.
    for (unsigned long AnimNr=0; AnimNr<Anims.Size(); AnimNr++)
        for (unsigned long FrameNr=0; FrameNr<Anims[AnimNr].Frames.Size(); FrameNr++)
            Anims[AnimNr].RecomputeBB(FrameNr, Joints, Meshes);
}


void LoaderFbxT::Load(ArrayT<CafuModelT::SkinT>& Skins, const MaterialManagerImplT& MaterialMan)
{
    // TODO...
}


void LoaderFbxT::Load(ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures, ArrayT<CafuModelT::GuiLocT>& GuiLocs)
{
}


#else   // HAVE_FBX_SDK

// This is a stub implementation for use whenever the Autodesk FBX SDK is not available.
LoaderFbxT::LoaderFbxT(const std::string& FileName, UserCallbacksI& /*UserCallbacks*/, int Flags)
    : ModelLoaderT(FileName, Flags)
{
    throw LoadErrorT("This edition of the program was built without the Autodesk FBX"
                     "library that is required to import files in this file format.");
}

LoaderFbxT::~LoaderFbxT() { }
bool LoaderFbxT::UseGivenTS() const { return false; }
void LoaderFbxT::Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan) { }
void LoaderFbxT::Load(ArrayT<CafuModelT::SkinT>& Skins, const MaterialManagerImplT& MaterialMan) { }
void LoaderFbxT::Load(ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures, ArrayT<CafuModelT::GuiLocT>& GuiLocs) { }

#endif  // HAVE_FBX_SDK
