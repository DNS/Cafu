/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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


namespace
{
#if 0
    std::ofstream Log("fbx-loader.log");
#else
    struct NullStreamT
    {
        void flush() { }
    };

    template<typename T> NullStreamT& operator << (NullStreamT& ns, const T& x) { return ns; }

    NullStreamT Log;
#endif

#if 0
    std::ostream& operator << (std::ostream& os, const FbxVector4& A)
    {
        return os << "fbxVec4(" << A[0] << ", " << A[1] << ", " << A[2] << ", " << A[3] << ")";
    }

    std::ostream& operator << (std::ostream& os, const FbxQuaternion& A)
    {
        return os << "fbxQuat(" << A[0] << ", " << A[1] << ", " << A[2] << ", " << A[3] << ")";
    }
#endif

    Vector3fT conv(const FbxVector4& V)
    {
        return Vector3dT(V[0], V[1], V[2]).AsVectorOfFloat();
    }

    CafuModelT::MeshT::WeightT CreateWeight(unsigned int JointIdx, float w, const Vector3fT& Pos)
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
}


class LoaderFbxT::FbxSceneT
{
    public:

    FbxSceneT(const LoaderFbxT& MainClass, UserCallbacksI& UserCallbacks, const std::string& FileName);
    ~FbxSceneT();

    /// Returns the root node of this scene.
    const FbxNode* GetRootNode() const { return m_Scene->GetRootNode(); }

    /// Recursively loads the joints, beginning at the given FbxNode instance and the given parent index.
    void Load(ArrayT<CafuModelT::JointT>& Joints, int ParentIndex, const FbxNode* Node) const;

    /// Loads the meshes.
    void Load(ArrayT<CafuModelT::MeshT>& Meshes, MaterialManagerImplT& MaterialMan) const;

    /// Loads the animations.
    void Load(ArrayT<CafuModelT::AnimT>& Anims) const;


    private:

    void CleanUp();
    void GetNodes(const FbxNode* Node);
    void ConvertNurbsAndPatches(FbxNode* Node);
    void GetWeights(const FbxMesh* Mesh, const unsigned long MeshNodeNr, ArrayT< ArrayT<CafuModelT::MeshT::WeightT> >& Weights) const;
    bool LoadMaterial(MaterialT& Mat, const FbxSurfaceMaterial* FbxMaterial) const;
    ArrayT< ArrayT<PosQtrScaleT> > GetSequData(FbxAnimStack* AnimStack, const ArrayT<FbxTime>& FrameTimes) const;

    const LoaderFbxT&      m_MainClass;
    UserCallbacksI&        m_UserCallbacks;    ///< Interface to get the password from the user.
    FbxManager*            m_SdkManager;
    FbxScene*              m_Scene;
    FbxImporter*           m_Importer;
    ArrayT<const FbxNode*> m_Nodes;
};


LoaderFbxT::FbxSceneT::FbxSceneT(const LoaderFbxT& MainClass, UserCallbacksI& UserCallbacks, const std::string& FileName)
    : m_MainClass(MainClass),
      m_UserCallbacks(UserCallbacks),
      m_SdkManager(FbxManager::Create()),
      m_Scene(FbxScene::Create(m_SdkManager, "")),
      m_Importer(FbxImporter::Create(m_SdkManager, ""))
{
    Log << "\nBeginning import of \"" << FileName << "\" ...\n";

    // Initialize the importer by providing a filename.
    if (!m_Importer->Initialize(FileName.c_str(), -1, m_SdkManager->GetIOSettings()))
    {
        std::string ErrorMsg=m_Importer->GetStatus().GetErrorString();

        CleanUp();
        throw LoadErrorT(ErrorMsg);
    }

    bool Result=m_Importer->Import(m_Scene);

    if (!Result && m_Importer->GetStatus().GetCode() == FbxStatus::ePasswordError)
    {
        const FbxString Password=UserCallbacks.GetPasswordFromUser("Please enter the password to open file\n" + FileName).c_str();

        m_SdkManager->GetIOSettings()->SetStringProp(IMP_FBX_PASSWORD, Password);
        m_SdkManager->GetIOSettings()->SetBoolProp(IMP_FBX_PASSWORD_ENABLE, true);

        Result=m_Importer->Import(m_Scene);
    }

    if (!Result)
    {
        std::string ErrorMsg=m_Importer->GetStatus().GetErrorString();

        CleanUp();
        throw LoadErrorT(ErrorMsg);
    }


    // The coordinate system of 3D Studio Max happens to be the same as Cafu's (x: right, y: into screen, z: up),
    // thus make sure that the scene is converted to this coordinate system if necessary.
    if (m_Scene->GetGlobalSettings().GetAxisSystem()!=FbxAxisSystem::Max)
    {
        Log << "Converting coordinate system to Cafu's...\n";
        FbxAxisSystem::Max.ConvertScene(m_Scene);
    }


    // Let the user know how many centimeters one scene unit is.
    const FbxSystemUnit SysUnit=m_Scene->GetGlobalSettings().GetSystemUnit();

    Log << "Scene unit info:\n"
        << "    scale factor: one scene unit is " << SysUnit.GetScaleFactor() << " cm (" << (SysUnit.GetScaleFactor()/2.54) << " inches)\n"
        << "    multiplier: " << SysUnit.GetMultiplier() << "\n";

    // if (SysUnit.GetScaleFactor()!=1.0)
    // {
    //     FbxSystemUnit CafuSystemUnit(1.0);
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
        const FbxPose* Pose=m_Scene->GetPose(PoseNr);

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
        const FbxNode*           Node          =m_Nodes[NodeNr];
        const FbxLayerContainer* LayerContainer=FbxCast<FbxLayerContainer>(Node->GetNodeAttribute());    // Must use FbxCast<T> instead of dynamic_cast<T*> for classes of the FBX SDK.

        if (LayerContainer)   // Typically a mesh, but can also be a Nurb, a patch, etc.
        {
            Log << "    Node " << NodeNr << " has " << Node->GetSrcObjectCount(FbxSurfaceMaterial::ClassId) << " material(s)\n";
            const int lNbMat=Node->GetSrcObjectCount(FbxSurfaceMaterial::ClassId);

            for (int lMaterialIndex=0; lMaterialIndex<lNbMat; lMaterialIndex++)
            {
                FbxSurfaceMaterial* lMaterial=FbxCast<FbxSurfaceMaterial>(Node->GetSrcObject(FbxSurfaceMaterial::ClassId, lMaterialIndex));

                if (lMaterial)
                {
                    FbxProperty lProperty=lMaterial->FindProperty(FbxSurfaceMaterial::sDiffuse);

                    if (lProperty.IsValid())
                    {
                        const int lNbTex = lProperty.GetSrcObjectCount(FbxFileTexture::ClassId);

                        for (int lTextureIndex=0; lTextureIndex<lNbTex; lTextureIndex++)
                        {
                            FbxFileTexture* lTexture=FbxCast<FbxFileTexture>(lProperty.GetSrcObject(FbxFileTexture::ClassId, lTextureIndex));

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


void LoaderFbxT::FbxSceneT::GetNodes(const FbxNode* Node)
{
    m_Nodes.PushBack(Node);

    for (int ChildNr=0; ChildNr<Node->GetChildCount(); ChildNr++)
        GetNodes(Node->GetChild(ChildNr));
}


void LoaderFbxT::FbxSceneT::ConvertNurbsAndPatches(FbxNode* Node)
{
    FbxNodeAttribute* NodeAttribute=Node->GetNodeAttribute();

    if (NodeAttribute)
    {
        if (NodeAttribute->GetAttributeType()==FbxNodeAttribute::eNurbs ||
            NodeAttribute->GetAttributeType()==FbxNodeAttribute::ePatch)
        {
            FbxGeometryConverter Converter(m_SdkManager);

            Log << "Node \"" << Node->GetName() << "\" is a " << (NodeAttribute->GetAttributeType()==FbxNodeAttribute::eNurbs ? "Nurbs" : "Patch") << ", triangulating...\n";
            Converter.Triangulate(NodeAttribute, true /*in place?*/);
        }
    }

    for (int ChildNr=0; ChildNr<Node->GetChildCount(); ChildNr++)
        ConvertNurbsAndPatches(Node->GetChild(ChildNr));
}


void LoaderFbxT::FbxSceneT::Load(ArrayT<CafuModelT::JointT>& Joints, int ParentIndex, const FbxNode* Node) const
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
            case FbxNodeAttribute::eUnknown         : Log << "        eUnknown         \n"; break;
            case FbxNodeAttribute::eNull            : Log << "        eNull            \n"; break;
            case FbxNodeAttribute::eMarker          : Log << "        eMarker          \n"; break;
            case FbxNodeAttribute::eSkeleton        : Log << "        eSkeleton        \n"; break;
            case FbxNodeAttribute::eMesh            : Log << "        eMesh            \n"; break;
            case FbxNodeAttribute::eNurbs           : Log << "        eNurbs           \n"; break;
            case FbxNodeAttribute::ePatch           : Log << "        ePatch           \n"; break;
            case FbxNodeAttribute::eCamera          : Log << "        eCamera          \n"; break;
            case FbxNodeAttribute::eCameraStereo    : Log << "        eCameraStereo    \n"; break;
            case FbxNodeAttribute::eCameraSwitcher  : Log << "        eCameraSwitcher  \n"; break;
            case FbxNodeAttribute::eLight           : Log << "        eLight           \n"; break;
            case FbxNodeAttribute::eOpticalReference: Log << "        eOpticalReference\n"; break;
            case FbxNodeAttribute::eOpticalMarker   : Log << "        eOpticalMarker   \n"; break;
            case FbxNodeAttribute::eNurbsCurve      : Log << "        eNurbsCurve      \n"; break;
            case FbxNodeAttribute::eTrimNurbsSurface: Log << "        eTrimNurbsSurface\n"; break;
            case FbxNodeAttribute::eBoundary        : Log << "        eBoundary        \n"; break;
            case FbxNodeAttribute::eNurbsSurface    : Log << "        eNurbsSurface    \n"; break;
            case FbxNodeAttribute::eShape           : Log << "        eShape           \n"; break;
            case FbxNodeAttribute::eLODGroup        : Log << "        eLODGroup        \n"; break;
            case FbxNodeAttribute::eSubDiv          : Log << "        eSubDiv          \n"; break;
            case FbxNodeAttribute::eCachedEffect    : Log << "        eCachedEffect    \n"; break;
            case FbxNodeAttribute::eLine            : Log << "        eLine            \n"; break;
            default:                                  Log << "        [UNKNOWN ATTRIB] \n"; break;
        }
    }

    // Note that FbxAnimEvaluator::GetNodeLocalTransform() is the proper method to call here:
    // It returns the node's default transformation matrix or the node's actual transformation
    // matrix at a specified point in time (depending on its second parameter pTime).
    // See FBX SDK Programmer's Guide pages 89 and 90 and its API method documentation for details.
    // Also see FbxAnimEvaluator::SetContext(): as we set no anim stack, it automatically uses the first in the scene.
    const FbxAMatrix& Transform  =m_Scene->GetAnimationEvaluator()->GetNodeLocalTransform(const_cast<FbxNode*>(Node));
    FbxVector4        Translation=Transform.GetT();
    FbxQuaternion     Quaternion =Transform.GetQ();
    FbxVector4        Scale      =Transform.GetS();

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


namespace
{
    // This function is from file GetPosition.cxx of the Autodesk SDK ViewScene example:
    // Get the geometry deformation local to a node. It is never inherited by the children.
    FbxAMatrix GetGeometry(const FbxNode* pNode)
    {
        FbxAMatrix GeometryMat;

        GeometryMat.SetT(pNode->GetGeometricTranslation(FbxNode::eSourcePivot));
        GeometryMat.SetR(pNode->GetGeometricRotation   (FbxNode::eSourcePivot));
        GeometryMat.SetS(pNode->GetGeometricScaling    (FbxNode::eSourcePivot));

        return GeometryMat;
    }
}


/// Returns, for each vertex in the mesh, the list of weights that affect it.
void LoaderFbxT::FbxSceneT::GetWeights(const FbxMesh* Mesh, const unsigned long MeshNodeNr, ArrayT< ArrayT<CafuModelT::MeshT::WeightT> >& Weights) const
{
    // All the links must have the same link mode.
    FbxCluster::ELinkMode ClusterMode=FbxCluster::eNormalize;     // Updated below.

    Weights.PushBackEmptyExact(Mesh->GetControlPointsCount());

    for (int DeformerNr=0; DeformerNr < Mesh->GetDeformerCount(); DeformerNr++)
    {
        Log << "    " << DeformerNr << ", ";

        switch (Mesh->GetDeformer(DeformerNr)->GetDeformerType())
        {
            case FbxDeformer::eSkin:        Log << "eSkin\n"; break;
            case FbxDeformer::eVertexCache: Log << "eVertexCache\n"; break;
            default:                        Log << "[unknown deformer type]\n"; break;
        }

        FbxSkin* Skin=FbxCast<FbxSkin>(Mesh->GetDeformer(DeformerNr));   // Must use FbxCast<T> instead of dynamic_cast<T*> for classes of the FBX SDK.

        if (Skin && Skin->GetClusterCount()>0)
        {
            ArrayT<FbxVector4> GlobalBindPose;
            FbxAMatrix         MeshToGlobalBindPose;

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
                const FbxCluster*  Cluster =Skin->GetCluster(ClusterNr);
                const unsigned int JointIdx=m_Nodes.Find(Cluster->GetLink());

                Log << "        cluster " << ClusterNr << ", link mode: " << Cluster->GetLinkMode() << "\n";

                if (JointIdx>=m_Nodes.Size()) continue;

                // Update the ClusterMode (it should/must be the same for all links).
                ClusterMode=Cluster->GetLinkMode();

                // Get the matrix that transforms the vertices from global space to local bone space in bind pose.
                FbxAMatrix BoneBindingMatrix;
                Cluster->GetTransformLinkMatrix(BoneBindingMatrix);
                FbxAMatrix GlobalToLocalBoneBindPose=BoneBindingMatrix.Inverse();

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
                case FbxCluster::eNormalize:
                    for (unsigned long WNr=0; WNr<Weights[VertexNr].Size(); WNr++)
                        Weights[VertexNr][WNr].Weight/=wSum;
                    break;

                case FbxCluster::eTotalOne:
                    if (wSum!=1.0f)
                        Weights[VertexNr].PushBack(CreateWeight(MeshNodeNr, 1.0f-wSum, conv(Mesh->GetControlPoints()[VertexNr])));
                    break;

                case FbxCluster::eAdditive:
                    // This case is not implemented/supported yet.
                    break;
            }
        }
    }
}


namespace
{
    std::string GetTexFileName(const FbxSurfaceMaterial* FbxMaterial, const char* PropName)
    {
        FbxProperty Property=FbxMaterial->FindProperty(PropName);
        if (!Property.IsValid()) return "";

        const int       TextureIndex=0;
        FbxFileTexture* Texture=Property.GetSrcObject<FbxFileTexture>(TextureIndex);
        if (!Texture) return "";

        const char* FileName=Texture->GetRelativeFileName();
        if (FileName!=NULL && FileName[0]!=0) return cf::String::Replace(FileName, "\\", "/");

        FileName=Texture->GetFileName();
        if (FileName!=NULL && FileName[0]!=0) return cf::String::Replace(FileName, "\\", "/");

        return "";
    }
}


/// Attempts to load a MaterialT material from a FbxSurfaceMaterial.
/// @param Mat           The material to initialize.
/// @param FbxMaterial   The source material.
/// @returns \c true on success (\c Mat could be successfully/meaningfully initialized from \c FbxMaterial), \c false otherwise.
bool LoaderFbxT::FbxSceneT::LoadMaterial(MaterialT& Mat, const FbxSurfaceMaterial* FbxMaterial) const
{
    const std::string BaseDir=cf::String::GetPath(m_MainClass.GetFileName())+"/";

    if (!FbxMaterial) return false;

    std::string fn=GetTexFileName(FbxMaterial, FbxSurfaceMaterial::sDiffuse);
    if (fn!="") Mat.DiffMapComp=MapCompositionT(fn, BaseDir); else return false;

    fn=GetTexFileName(FbxMaterial, FbxSurfaceMaterial::sNormalMap);
    if (fn!="") Mat.NormMapComp=MapCompositionT(fn, BaseDir);

    fn=GetTexFileName(FbxMaterial, FbxSurfaceMaterial::sBump);
    if (fn!="") Mat.NormMapComp=MapCompositionT(std::string("hm2nm(")+fn+", 1)", BaseDir);

    fn=GetTexFileName(FbxMaterial, FbxSurfaceMaterial::sSpecular);
    if (fn!="") Mat.SpecMapComp=MapCompositionT(fn, BaseDir);

    fn=GetTexFileName(FbxMaterial, FbxSurfaceMaterial::sEmissive);
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
        const FbxMesh* Mesh=FbxCast<FbxMesh>(m_Nodes[NodeNr]->GetNodeAttribute());   // Must use FbxCast<T> instead of dynamic_cast<T*> for classes of the FBX SDK.

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
        const FbxLayerElement::EMappingMode       MappingMode=(Mesh->GetLayer(0) && Mesh->GetLayer(0)->GetUVs()) ? Mesh->GetLayer(0)->GetUVs()->GetMappingMode() : FbxLayerElement::eNone;
        FbxLayerElementArrayTemplate<FbxVector2>* UVArray    =NULL;
        const FbxLayerElementSmoothing*           SmoothingLE=NULL;
        std::map<uint64_t, unsigned int>          UniqueVertices;  // Maps tuples of (Mesh->GetPolygonVertex(), Mesh->GetTextureUVIndex()) to indices into CafuMesh.Vertices.

        Mesh->GetTextureUV(&UVArray, FbxLayerElement::eTextureDiffuse);

        if (Mesh->GetLayer(0))
        {
            SmoothingLE=Mesh->GetLayer(0)->GetSmoothing();

            if (SmoothingLE)
                Log << "    The mesh has a smoothing layer element with mapping mode "
                    << SmoothingLE->GetMappingMode() << " and reference mode "
                    << SmoothingLE->GetReferenceMode() << ".\n";
            else
                Log << "    The mesh has no smoothing layer element.\n";
        }

        for (int PolyNr=0; PolyNr<Mesh->GetPolygonCount(); PolyNr++)
        {
            uint32_t SmoothingGroups=0x01;    // Per default, all triangles are in a common smoothing group.

            if (SmoothingLE)
            {
                if (SmoothingLE->GetMappingMode() == FbxGeometryElement::eByPolygon)
                {
                    switch (SmoothingLE->GetReferenceMode())
                    {
                        case FbxGeometryElement::eDirect:
                            SmoothingGroups = SmoothingLE->GetDirectArray().GetAt(PolyNr);
                            break;

                        case FbxGeometryElement::eIndexToDirect:
                            SmoothingGroups = SmoothingLE->GetDirectArray().GetAt(SmoothingLE->GetIndexArray().GetAt(PolyNr));
                            break;

                        default:
                            // Ignore any unknown reference mode.
                            break;
                    }
                }
                else if (SmoothingLE->GetMappingMode() == FbxGeometryElement::eByEdge)
                {
                    // Unfortunately, we cannot deal with Maya-style smoothing info (hard vs. soft edges).
                }
            }

            for (int PolyTriNr=0; PolyTriNr < Mesh->GetPolygonSize(PolyNr)-2; PolyTriNr++)
            {
                CafuModelT::MeshT::TriangleT Tri;
                const int                    TriVIs[3]={ 0, PolyTriNr+2, PolyTriNr+1 };

                for (unsigned int i=0; i<3; i++)
                {
                    const int VertexIdx=Mesh->GetPolygonVertex(PolyNr, TriVIs[i]);
                    const int TexUV_Idx=(MappingMode==FbxLayerElement::eByPolygonVertex) ? const_cast<FbxMesh*>(Mesh)->GetTextureUVIndex(PolyNr, TriVIs[i])
                                       /*MappingMode==FbxLayerElement::eByControlPoint*/ : VertexIdx;

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

                        CafuMesh.Vertices[VertexNr].u=UVArray ?        float(UVArray->GetAt(TexUV_Idx)[0]) : 0.0f;
                        CafuMesh.Vertices[VertexNr].v=UVArray ? 1.0f - float(UVArray->GetAt(TexUV_Idx)[1]) : 0.0f;
                        CafuMesh.Vertices[VertexNr].FirstWeightIdx=VertexFirstWeightIdx[VertexIdx];
                        CafuMesh.Vertices[VertexNr].NumWeights=VertexWeights[VertexIdx].Size();

                        Tri.VertexIdx[i]=VertexNr;
                        UniqueVertices[Tuple]=VertexNr;
                    }
                }

                Tri.SmoothGroups=SmoothingGroups;
                CafuMesh.Triangles.PushBack(Tri);
            }
        }

        // Set the material.
        const int                 FbxMaterialIndex=0;
        const FbxSurfaceMaterial* FbxMaterial=m_Nodes[NodeNr]->GetMaterial(FbxMaterialIndex);
        const std::string         MatName=FbxMaterial ? FbxMaterial->GetName() : "wire-frame";

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


namespace
{
    ArrayT<FbxTime> GetFrameTimes(const FbxAnimStack* AnimStack, const FbxTime& TimeStep)
    {
        ArrayT<FbxTime> FrameTimes;

        const FbxTime TimeStart=AnimStack->GetLocalTimeSpan().GetStart();
        const FbxTime TimeStop =AnimStack->GetLocalTimeSpan().GetStop();

        Log << "    " << "start: " << TimeStart.Get() << " (" << TimeStart.GetSecondDouble() << ")";
        Log <<    " " << "stop: "  << TimeStop.Get()  << " (" << TimeStop.GetSecondDouble()  << ")";
        Log <<    " " << "step: "  << TimeStep.Get()  << " (" << TimeStep.GetSecondDouble()  << ")" << "\n";

        for (FbxTime TimeNow=TimeStart; TimeNow<=TimeStop; TimeNow+=TimeStep)
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
}


/// Gathers all animation data of a sequence (AnimStack) in uncompressed form in an array of the form SequData[NodeNr][FrameNr].
/// That is, for each frame and for each node, we store the position, quaternion and scale.
ArrayT< ArrayT<PosQtrScaleT> > LoaderFbxT::FbxSceneT::GetSequData(FbxAnimStack* AnimStack, const ArrayT<FbxTime>& FrameTimes) const
{
    ArrayT< ArrayT<PosQtrScaleT> > SequData;

    m_Scene->SetCurrentAnimationStack(AnimStack);
    SequData.PushBackEmptyExact(m_Nodes.Size());

    for (unsigned long NodeNr=0; NodeNr<m_Nodes.Size(); NodeNr++)
    {
        SequData[NodeNr].PushBackEmptyExact(FrameTimes.Size());

        for (unsigned long FrameNr=0; FrameNr<FrameTimes.Size(); FrameNr++)
        {
            const FbxAMatrix& Transform  =m_Scene->GetAnimationEvaluator()->GetNodeLocalTransform(const_cast<FbxNode*>(m_Nodes[NodeNr]), FrameTimes[FrameNr]);
            FbxVector4        Translation=Transform.GetT();
            FbxQuaternion     Quaternion =Transform.GetQ();
            FbxVector4        Scale      =Transform.GetS();

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
    FbxTime              TimeStep;
    FbxArray<FbxString*> AnimStackNames;

    TimeStep.SetTime(0, 0, 0, 1, 0, m_Scene->GetGlobalSettings().GetTimeMode());
    m_Scene->FillAnimStackNameArray(AnimStackNames);

    Log << "\n";
    Log << "Global scene FPS: " << 1.0/TimeStep.GetSecondDouble() << "\n";
    Log << "Anim stacks in scene: " << AnimStackNames.GetCount() << "\n";
    Log << "Animated nodes: " << m_Nodes.Size() << "\n";

    for (int NameNr=0; NameNr<AnimStackNames.GetCount(); NameNr++)
    {
        FbxAnimStack* AnimStack=m_Scene->FindMember<FbxAnimStack>(AnimStackNames[NameNr]->Buffer());

        // Make sure that the animation stack was found in the scene (it always should).
        if (AnimStack==NULL) continue;

        const ArrayT<FbxTime>                FrameTimes=GetFrameTimes(AnimStack, TimeStep);
        const ArrayT< ArrayT<PosQtrScaleT> > SequData  =GetSequData(AnimStack, FrameTimes);

        Log << "    \"" << AnimStackNames[NameNr]->Buffer() << "\"\n";
        // if (AnimStackNames[NameNr]->Compare(FbxGet<FbxString>(m_Scene->ActiveAnimStackName))==0) Log << "    (active)";   // Why does this corrupt the stack under (x86_64) Linux?
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

    FbxArrayDelete(AnimStackNames);
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


void LoaderFbxT::Load(ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures)
{
}


#else   // HAVE_FBX_SDK

// This is a stub implementation for use whenever the Autodesk FBX SDK is not available.
LoaderFbxT::LoaderFbxT(const std::string& FileName, UserCallbacksI& /*UserCallbacks*/, int Flags)
    : ModelLoaderT(FileName, Flags)
{
    throw LoadErrorT("This edition of the program was built without the Autodesk FBX "
                     "library that is required to import files in this file format.");
}

LoaderFbxT::~LoaderFbxT() { }
void LoaderFbxT::Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan) { }
void LoaderFbxT::Load(ArrayT<CafuModelT::SkinT>& Skins, const MaterialManagerImplT& MaterialMan) { }
void LoaderFbxT::Load(ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures) { }

#endif  // HAVE_FBX_SDK
