/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "BspTreeNode.hpp"
#include "_aux.hpp"
#include "FaceNode.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/Renderer.hpp"

#include <cassert>

using namespace cf::SceneGraph;


static ConVarT usePVS("usePVS", true, ConVarT::FLAG_MAIN_EXE, "Toggles whether the PVS is used for rendering (recommended!).");


BspTreeNodeT::BspTreeNodeT(float LightMapPatchSize, float SHLMapPatchSize)
    : m_LightMapPatchSize(LightMapPatchSize),
      m_SHLMapPatchSize(SHLMapPatchSize),
      NextLightNeedsInit(true)
{
}


BspTreeNodeT* BspTreeNodeT::CreateFromFile_cw(std::istream& InFile, aux::PoolT& Pool,
    LightMapManT& LMM, SHLMapManT& SMM, PlantDescrManT& PDM, const ArrayT<const TerrainT*>& ShTe, ModelManagerT& ModelMan)
{
    const float LightMapPatchSize = aux::ReadFloat(InFile);
    const float SHLMapPatchSize   = aux::ReadFloat(InFile);

    BspTreeNodeT* BspTree = new BspTreeNodeT(LightMapPatchSize, SHLMapPatchSize);

    // Read the FaceChildren.
    for (unsigned long Count=aux::ReadUInt32(InFile); Count>0; Count--)
    {
        // Don't call cf::SceneGraph::FaceNodeT::CreateFromFile_cw() directly, because it would expect
        // that the "Face" string that states the identity of this NodeT has already been read!
        FaceNodeT* FaceNode=dynamic_cast<FaceNodeT*>(cf::SceneGraph::GenericNodeT::CreateFromFile_cw(InFile, Pool, LMM, SMM, PDM, ShTe, ModelMan));

        if (FaceNode==NULL)
        {
            printf("Could not read scene graph face node!\n");
            delete BspTree;
            return NULL;
        }

        BspTree->FaceChildren.PushBack(FaceNode);
    }

    // Read the OtherChildren.
    for (unsigned long Count=aux::ReadUInt32(InFile); Count>0; Count--)
    {
        cf::SceneGraph::GenericNodeT* Node=cf::SceneGraph::GenericNodeT::CreateFromFile_cw(InFile, Pool, LMM, SMM, PDM, ShTe, ModelMan);

        if (Node==NULL)
        {
            printf("Could not read scene graph node!");
            delete BspTree;
            return NULL;
        }

        BspTree->OtherChildren.PushBack(Node);
    }

    // Read GlobalDrawVertices.
    for (unsigned long Count=aux::ReadUInt32(InFile); Count>0; Count--)
    {
        BspTree->GlobalDrawVertices.PushBack(Pool.ReadVector3d(InFile));
    }

    // Read the BSP nodes.
    for (unsigned long Count=aux::ReadUInt32(InFile); Count>0; Count--)
    {
        NodeT N;

        N.Plane.Normal=Pool.ReadVector3d(InFile);
        InFile.read((char*)&N.Plane.Dist, sizeof(N.Plane.Dist));

        N.FrontChild=aux::ReadUInt32(InFile);
        N.BackChild =aux::ReadUInt32(InFile);
        InFile.read((char*)&N.FrontIsLeaf, sizeof(N.FrontIsLeaf)); assert(sizeof(N.FrontIsLeaf)==1);
        InFile.read((char*)&N. BackIsLeaf, sizeof(N. BackIsLeaf)); assert(sizeof(N. BackIsLeaf)==1);

        BspTree->Nodes.PushBack(N);
    }

    // Read the BSP leaves.
    BspTree->Leaves.PushBackEmpty(aux::ReadUInt32(InFile));

    for (unsigned long LeafNr=0; LeafNr<BspTree->Leaves.Size(); LeafNr++)
    {
        LeafT& L=BspTree->Leaves[LeafNr];

        for (unsigned long Count=aux::ReadUInt32(InFile); Count>0; Count--)
            L.FaceChildrenSet.PushBack(aux::ReadUInt32(InFile));

        for (unsigned long Count=aux::ReadUInt32(InFile); Count>0; Count--)
            L.OtherChildrenSet.PushBack(aux::ReadUInt32(InFile));

        L.Portals.PushBackEmpty(aux::ReadUInt32(InFile));
        for (unsigned long PortalNr=0; PortalNr<L.Portals.Size(); PortalNr++)
        {
            L.Portals[PortalNr].Plane.Normal=Pool.ReadVector3d(InFile);
            InFile.read((char*)&L.Portals[PortalNr].Plane.Dist, sizeof(L.Portals[PortalNr].Plane.Dist));

            for (unsigned long Count=aux::ReadUInt32(InFile); Count>0; Count--)
            {
                L.Portals[PortalNr].Vertices.PushBack(Pool.ReadVector3d(InFile));
            }
        }

        const Vector3dT MinBB=aux::ReadVector3d(InFile);
        const Vector3dT MaxBB=aux::ReadVector3d(InFile);

        L.BB=BoundingBox3T<double>(MinBB, MaxBB);

        InFile.read((char*)&L.IsInnerLeaf, sizeof(L.IsInnerLeaf));

        if (LeafNr==0) BspTree->BB=L.BB;
                  else { BspTree->BB.Insert(L.BB.Min); BspTree->BB.Insert(L.BB.Max); }
    }

    // Read the PVS.
    for (unsigned long Nr=0; Nr<(BspTree->Leaves.Size()*BspTree->Leaves.Size()+31)/32; Nr++)
    {
        BspTree->PVS.PushBack(aux::ReadUInt32(InFile));
    }

    return BspTree;
}


BspTreeNodeT::~BspTreeNodeT()
{
    for (unsigned long ChildNr=0; ChildNr<FaceChildren.Size(); ChildNr++)
        delete FaceChildren[ChildNr];

    for (unsigned long ChildNr=0; ChildNr<OtherChildren.Size(); ChildNr++)
        delete OtherChildren[ChildNr];
}


void BspTreeNodeT::InitDrawing()
{
    FaceChildIsInViewerPVS      .PushBackEmpty(FaceChildren .Size());
    OtherChildIsInViewerPVS     .PushBackEmpty(OtherChildren.Size());
    FaceChildIsInLightSourcePVS .PushBackEmpty(FaceChildren .Size());
    OtherChildIsInLightSourcePVS.PushBackEmpty(OtherChildren.Size());

    // TODO: Need something like this for the OtherChildren, too?
    for (unsigned long ChildNr=0; ChildNr<FaceChildren.Size(); ChildNr++)
        FaceChildren[ChildNr]->InitRenderMeshesAndMats(GlobalDrawVertices, m_LightMapPatchSize);

    // TODO: see above.
    // for (unsigned long ChildNr=0; ChildNr<OtherChildren.Size(); ChildNr++)
    //     OtherChildren[ChildNr]->InitRenderMeshesAndMats(GlobalDrawVertices, m_LightMapPatchSize);
}


double BspTreeNodeT::ClipLine(const VectorT& P, const VectorT& U, double Min, double Max, unsigned long NodeNr, bool NodeIsLeaf) const
{
    if (NodeIsLeaf)
    {
        return Leaves[NodeNr].IsInnerLeaf ? Max : Min;
    }

    const NodeT& N    =Nodes[NodeNr];
    const double DistP=N.Plane.GetDistance(P);
    const double Div  =dot(N.Plane.Normal, U);
    const double Dist1=DistP+Min*Div;   // Dist1=N.Plane.GetDistance(P+scale(U, Min));
    const double Dist2=DistP+Max*Div;   // Dist2=N.Plane.GetDistance(P+scale(U, Max));

    if (Dist1>0)
    {
        if (Dist2>0) return ClipLine(P, U, Min, Max, N.FrontChild, N.FrontIsLeaf);

        const double Range=-DistP/Div;
        const double Hit  =ClipLine(P, U, Min, Range, N.FrontChild, N.FrontIsLeaf);

        return Hit<Range ? Hit : ClipLine(P, U, Range, Max, N.BackChild, N.BackIsLeaf);
    }
    else
    {
        if (Dist2<=0) return ClipLine(P, U, Min, Max, N.BackChild, N.BackIsLeaf);

        const double Range=-DistP/Div;
        const double Hit  =ClipLine(P, U, Min, Range, N.BackChild, N.BackIsLeaf);

        return Hit<Range ? Hit : ClipLine(P, U, Range, Max, N.FrontChild, N.FrontIsLeaf);
    }
}


void BspTreeNodeT::WriteTo(std::ostream& OutFile, aux::PoolT& Pool) const
{
    aux::Write(OutFile, "BspTree");

    aux::Write(OutFile, m_LightMapPatchSize);
    aux::Write(OutFile, m_SHLMapPatchSize);

    // Write the FaceChildren.
    aux::Write(OutFile, aux::cnc_ui32(FaceChildren.Size()));
    for (unsigned long ChildNr=0; ChildNr<FaceChildren.Size(); ChildNr++)
        FaceChildren[ChildNr]->WriteTo(OutFile, Pool);

    // Write the OtherChildren.
    aux::Write(OutFile, aux::cnc_ui32(OtherChildren.Size()));
    for (unsigned long ChildNr=0; ChildNr<OtherChildren.Size(); ChildNr++)
        OtherChildren[ChildNr]->WriteTo(OutFile, Pool);

    // Write GlobalDrawVertices.
    aux::Write(OutFile, aux::cnc_ui32(GlobalDrawVertices.Size()));
    for (unsigned long VertexNr=0; VertexNr<GlobalDrawVertices.Size(); VertexNr++)
        Pool.Write(OutFile, GlobalDrawVertices[VertexNr]);

    // Write the BSP nodes.
    aux::Write(OutFile, aux::cnc_ui32(Nodes.Size()));
    for (unsigned long NodeNr=0; NodeNr<Nodes.Size(); NodeNr++)
    {
        Pool.Write(OutFile, Nodes[NodeNr].Plane.Normal);
        OutFile.write((char*)&Nodes[NodeNr].Plane.Dist, sizeof(Nodes[NodeNr].Plane.Dist));

        aux::Write(OutFile, aux::cnc_ui32(Nodes[NodeNr].FrontChild));
        aux::Write(OutFile, aux::cnc_ui32(Nodes[NodeNr]. BackChild));
        OutFile.write((char*)&Nodes[NodeNr].FrontIsLeaf, sizeof(Nodes[NodeNr].FrontIsLeaf));
        OutFile.write((char*)&Nodes[NodeNr]. BackIsLeaf, sizeof(Nodes[NodeNr]. BackIsLeaf));
    }

    // Write the BSP leaves.
    aux::Write(OutFile, aux::cnc_ui32(Leaves.Size()));
    for (unsigned long LeafNr=0; LeafNr<Leaves.Size(); LeafNr++)
    {
        const LeafT& L=Leaves[LeafNr];

        aux::Write(OutFile, aux::cnc_ui32(L.FaceChildrenSet.Size()));
        for (unsigned long ChildNr=0; ChildNr<L.FaceChildrenSet.Size(); ChildNr++)
            aux::Write(OutFile, aux::cnc_ui32(L.FaceChildrenSet[ChildNr]));

        aux::Write(OutFile, aux::cnc_ui32(L.OtherChildrenSet.Size()));
        for (unsigned long ChildNr=0; ChildNr<L.OtherChildrenSet.Size(); ChildNr++)
            aux::Write(OutFile, aux::cnc_ui32(L.OtherChildrenSet[ChildNr]));

        aux::Write(OutFile, aux::cnc_ui32(L.Portals.Size()));
        for (unsigned long PortalNr=0; PortalNr<L.Portals.Size(); PortalNr++)
        {
            Pool.Write(OutFile, L.Portals[PortalNr].Plane.Normal);
            OutFile.write((char*)&L.Portals[PortalNr].Plane.Dist, sizeof(L.Portals[PortalNr].Plane.Dist));

            aux::Write(OutFile, aux::cnc_ui32(L.Portals[PortalNr].Vertices.Size()));
            for (unsigned long VertexNr=0; VertexNr<L.Portals[PortalNr].Vertices.Size(); VertexNr++)
                Pool.Write(OutFile, L.Portals[PortalNr].Vertices[VertexNr]);
        }

        aux::Write(OutFile, L.BB.Min);
        aux::Write(OutFile, L.BB.Max);

        OutFile.write((char*)&L.IsInnerLeaf, sizeof(L.IsInnerLeaf));
    }

    // Write the PVS.
    for (unsigned long Nr=0; Nr<PVS.Size(); Nr++)
        aux::Write(OutFile, PVS[Nr]);
}


const BoundingBox3T<double>& BspTreeNodeT::GetBoundingBox() const
{
    return BB;
}


bool BspTreeNodeT::IsOpaque() const
{
    return true;
}


void BspTreeNodeT::DrawAmbientContrib(const Vector3dT& ViewerPos) const
{
    assert(MatSys::Renderer!=NULL);
    assert(MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::AMBIENT);

    // This method takes the ViewerPos as input.
    // It outputs:
    // - The ChildIsInViewerPVS bitfield, which has "true" for all children in the PVS of the leaf of the ViewerPos.
    // - The BackToFrontList* arrays, which have the same contents BUT: They are subdivided into Opaque and Translucent, sorted back-to-front and
    //   do *NOT* contain faces that the viewer cannot see (because he is "behind" the plane and so cannot see them).

    // TODO: Re-use the information both in 'OrderedLeaves' as well as 'FaceIsInViewerPVS' IF the 'ViewerPos' is still in the same leaf
    // as in the previous call to this function.
    const unsigned long          ViewerLeafNr=WhatLeaf(ViewerPos);
    static ArrayT<unsigned long> OrderedLeaves;

    GetLeavesOrderedBackToFront(OrderedLeaves, ViewerPos);

    for (unsigned long ChildNr=0; ChildNr<FaceChildren.Size();  ChildNr++) FaceChildIsInViewerPVS [ChildNr]=false;
    for (unsigned long ChildNr=0; ChildNr<OtherChildren.Size(); ChildNr++) OtherChildIsInViewerPVS[ChildNr]=false;

    // Nun erstelle aus OrderedLeaves und FaceIsInViewerPVS zwei Listen:
    // a) Eine Liste der relevanten (sichtbaren) Faces, die fully-opaque sind, und front-to-back gezeichnet werden (für beste Z-Buffer Effizienz!).
    // b) Eine Liste der relevanten, translucent Faces, die anschließend back-to-front gezeichnet werden (für korrektes Blending).
    BackToFrontListOpaque.Overwrite();
    BackToFrontListTranslucent.Overwrite();

    for (unsigned long OrderNr=0; OrderNr<OrderedLeaves.Size(); OrderNr++)
    {
        const unsigned long LeafNr=OrderedLeaves[OrderNr];
        const LeafT&        L     =Leaves[LeafNr];

        if (usePVS.GetValueBool() && !IsInPVS(LeafNr, ViewerLeafNr)) continue;

        for (unsigned long SetNr=0; SetNr<L.FaceChildrenSet.Size(); SetNr++)
        {
            const unsigned long ChildNr=L.FaceChildrenSet[SetNr];

            if (FaceChildIsInViewerPVS[ChildNr]) continue;

            FaceNodeT* FaceNode=FaceChildren[ChildNr];

            //REMOVE:   if (FaceNode->TI.Alpha==0) continue;       // Insb. für Sky-Texturen haben wir auch Alpha==0 gesetzt (Konstruktor)!
            if (FaceNode->Polygon.Plane.GetDistance(ViewerPos)<0) continue;

            FaceChildIsInViewerPVS[ChildNr]=true;

            if (FaceNode->IsOpaque()) BackToFrontListOpaque     .PushBack(FaceNode);
                                 else BackToFrontListTranslucent.PushBack(FaceNode);
        }

        // Sort the "regular" children more in front than the faces - it likely gives a better overall back-to-front order.
        for (unsigned long SetNr=0; SetNr<L.OtherChildrenSet.Size(); SetNr++)
        {
            const unsigned long ChildNr=L.OtherChildrenSet[SetNr];

            if (OtherChildIsInViewerPVS[ChildNr]) continue;
            OtherChildIsInViewerPVS[ChildNr]=true;

            cf::SceneGraph::GenericNodeT* Child=OtherChildren[ChildNr];

            // Alternative 1 (worst):
         // BackToFrontListOpaque.PushBack(Child);

            // Alternative 2 (better):
            if (Child->IsOpaque()) BackToFrontListOpaque     .PushBack(Child);
                              else BackToFrontListTranslucent.PushBack(Child);

            // TODO: Alternative 3 (best, how it should be!):
         // Child->RegisterForAmbientDrawing(BackToFrontListOpaque, BackToFrontListTranslucent);
        }
    }


    for (unsigned long Count=BackToFrontListOpaque.Size(); Count>0;)
    {
        Count--;    // Must do the decrement early.
        BackToFrontListOpaque[Count]->DrawAmbientContrib(ViewerPos);
    }

    NextLightNeedsInit=true;
}


void BspTreeNodeT::InitForNextLight() const
{
    const Vector3dT             LightPos(MatSys::Renderer->GetCurrentLightSourcePosition());
    const float                 LightRadius=MatSys::Renderer->GetCurrentLightSourceRadius();
    const unsigned long         LightLeafNr=WhatLeaf(LightPos);
    const VectorT               TempVec(LightRadius, LightRadius, LightRadius);
    const BoundingBox3T<double> LightBB(LightPos+TempVec, LightPos-TempVec);

    for (unsigned long ChildNr=0; ChildNr<FaceChildren .Size(); ChildNr++) FaceChildIsInLightSourcePVS [ChildNr]=false;
    for (unsigned long ChildNr=0; ChildNr<OtherChildren.Size(); ChildNr++) OtherChildIsInLightSourcePVS[ChildNr]=false;

    // Now create two sets of faces from 'FaceIsInLightSourcePVS':
    // a) A list of faces that the light source can see (i.e. those who are potentially lit by the light).
    //    These are the front-facing faces wrt. the light.
    // b) A list of (potential) occluders, i.e. faces that may cast shadows on the faces of set a).
    //    These are the back-facing faces wrt. the light.
    // TODO: The list of occluders is incomplete - this is a BUG. Consider the JrBaseHq scenario... Ideas?
    FrontFacingList.Overwrite();
    BackFacingList.Overwrite();

    for (unsigned long LeafNr=0; LeafNr<Leaves.Size(); LeafNr++)
    {
        const LeafT& L=Leaves[LeafNr];

        // Children that cannot be seen by the light source, are trivially and fully in the light sources shadow.
        if (usePVS.GetValueBool() && !IsInPVS(LeafNr, LightLeafNr)) continue;

        for (unsigned long SetNr=0; SetNr<L.FaceChildrenSet.Size(); SetNr++)
        {
            const unsigned long ChildNr=L.FaceChildrenSet[SetNr];

            if (FaceChildIsInLightSourcePVS[ChildNr]) continue;
            FaceChildIsInLightSourcePVS[ChildNr]=true;

            FaceNodeT* FaceNode=FaceChildren[ChildNr];

            // If the face is outside the lights radius (bounding sphere/box),
            // there is no need to light it (if it is front-facing),
            // and no need to consider it as a shadow caster (if it is back-facing, everything behind it is even farther away anyway).
            const double DistOfLightToFacePlane=FaceNode->Polygon.Plane.GetDistance(LightPos);

            if (fabs(DistOfLightToFacePlane)>LightRadius) continue;
            if (!LightBB.Intersects(FaceNode->GetBoundingBox())) continue;

            // No need to do anything for this face if its material has not been found.
            if (FaceNode->Material==NULL) continue;

            if (DistOfLightToFacePlane>0)
            {
                // Front-facing face wrt. the light (LIGHT RECEIVER).
                // Here we have to apply essentially the same rules as in 'Draw_Prepare()'.
                if (!FaceChildIsInViewerPVS[ChildNr]) continue;             // If the viewer cannot see the face, there is no need to light it.
             // if (Map.TexInfos[FaceNode->TexInfoNr].Alpha==0) continue;   // Insb. für Sky-Texturen haben wir auch Alpha==0 gesetzt (Konstruktor)!
             // if (FaceNode->TI.Alpha!=255) continue;                      // Only fully opaque surfaces receive light (even if FLAG_ISWATER is set).
             // if (FaceNode->Material->NoDynLight) continue;               // [This is not really needed here, just for symmetry. This is no meta keyword as NoShadows should be.]

                FrontFacingList.PushBack(FaceNode);
            }
            else
            {
                // Back-facing face wrt. the light (SHADOW CASTER).
                if (FaceNode->Material->NoShadows) continue;                // Faces with materials that have NoShadows set don't cast shadows.

                BackFacingList.PushBack(FaceNode);
            }
        }

        for (unsigned long SetNr=0; SetNr<L.OtherChildrenSet.Size(); SetNr++)
        {
            const unsigned long ChildNr=L.OtherChildrenSet[SetNr];

            if (OtherChildIsInLightSourcePVS[ChildNr]) continue;
            OtherChildIsInLightSourcePVS[ChildNr]=true;

            cf::SceneGraph::GenericNodeT* Child=OtherChildren[ChildNr];

            if (!LightBB.Intersects(Child->GetBoundingBox())) continue;

            // Alternative 1:
            if (OtherChildIsInViewerPVS[ChildNr])
            {
                FrontFacingList.PushBack(Child);
            }

            BackFacingList.PushBack(Child);  // The name should actually be "ShadowCasterList"!

            // TODO: Alternative 2 (better):
         // Child->RegisterForLightDrawing(FrontFacingList, BackFacingList);
        }
    }

    NextLightNeedsInit=false;
}


void BspTreeNodeT::DrawStencilShadowVolumes(const Vector3dT& LightPos, const float LightRadius) const
{
    assert(MatSys::Renderer!=NULL);
    assert(MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::STENCILSHADOW);

    if (NextLightNeedsInit) InitForNextLight();

    for (unsigned long Count=0; Count<BackFacingList.Size(); Count++)
        BackFacingList[Count]->DrawStencilShadowVolumes(LightPos, LightRadius);
}


void BspTreeNodeT::DrawLightSourceContrib(const Vector3dT& ViewerPos, const Vector3dT& LightPos) const
{
    assert(MatSys::Renderer!=NULL);
    assert(MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::LIGHTING);

    if (NextLightNeedsInit) InitForNextLight();

    for (unsigned long Count=0; Count<FrontFacingList.Size(); Count++)
        FrontFacingList[Count]->DrawLightSourceContrib(ViewerPos, LightPos);

    NextLightNeedsInit=true;
}


void BspTreeNodeT::DrawTranslucentContrib(const Vector3dT& ViewerPos) const
{
    assert(MatSys::Renderer!=NULL);
    assert(MatSys::Renderer->GetCurrentRenderAction()==MatSys::RendererI::AMBIENT);

    // Render translucent nodes back-to-front.
    for (unsigned long Count=0; Count<BackToFrontListTranslucent.Size(); Count++)
        BackToFrontListTranslucent[Count]->DrawTranslucentContrib(ViewerPos);
}


unsigned long BspTreeNodeT::WhatLeaf(const VectorT& Position) const
{
#if 0
    // Recursive implementation.
    if (Nodes[NodeNr].Plane.GetDistance(Position)>0)
        return Nodes[NodeNr].FrontIsLeaf ? Nodes[NodeNr].FrontChild : WhatLeaf(Position, Nodes[NodeNr].FrontChild);
    else
        return Nodes[NodeNr]. BackIsLeaf ? Nodes[NodeNr]. BackChild : WhatLeaf(Position, Nodes[NodeNr]. BackChild);
#else
    // Iterative implementation.
    unsigned long NodeNr=0;

    while (true)
    {
        const NodeT& Node=Nodes[NodeNr];

        if (Node.Plane.GetDistance(Position)>0)
        {
            NodeNr=Node.FrontChild;
            if (Node.FrontIsLeaf) break;
        }
        else
        {
            NodeNr=Node.BackChild;
            if (Node.BackIsLeaf) break;
        }
    }

    return NodeNr;
#endif
}


void BspTreeNodeT::WhatLeaves(ArrayT<unsigned long>& ResultLeaves, const BoundingBox3T<double>& BoundingBox, unsigned long NodeNr) const
{
    BoundingBox3T<double>::SideT Side=BoundingBox.WhatSide(Nodes[NodeNr].Plane);

    if (Side==BoundingBox3T<double>::Front || Side==BoundingBox3T<double>::Both)
    {
        if (Nodes[NodeNr].FrontIsLeaf) ResultLeaves.PushBack(Nodes[NodeNr].FrontChild);
                                  else WhatLeaves(ResultLeaves, BoundingBox, Nodes[NodeNr].FrontChild);
    }

    if (Side==BoundingBox3T<double>::Back  || Side==BoundingBox3T<double>::Both)
    {
        if (Nodes[NodeNr]. BackIsLeaf) ResultLeaves.PushBack(Nodes[NodeNr]. BackChild);
                                  else WhatLeaves(ResultLeaves, BoundingBox, Nodes[NodeNr]. BackChild);
    }
}


#if 0
const BoundingBox3T<double>* TBB_BB_Helper    =NULL;
const VectorT*               TBB_Origin_Helper=NULL;
const VectorT*               TBB_Dir_Helper   =NULL;
VB_Trace3T<double>           TBB_Trace(1.0);
ArrayT<unsigned long>        TBB_BrushCheckCounts;
ArrayT<unsigned long>        TBB_OtherChildrenCheckCounts;
unsigned long                TBB_CheckCount=0;
cf::SceneGraph::ConsiderT    TBB_Consider;
MaterialT::ClipFlagsT        TBB_ClipMask;


double BspTreeNodeT::ClipLine(const VectorT& P, const VectorT& U, double Min, double Max, const ConsiderT& Consider, MaterialT::ClipFlagsT ClipMask, unsigned long NodeNr, bool NodeIsLeaf) const
{
    if (NodeNr==0)
    {
        while (TBB_OtherChildrenCheckCounts.Size()<OtherChildren.Size()) TBB_OtherChildrenCheckCounts.PushBack(0);
        TBB_CheckCount++;
    }

    if (NodeIsLeaf)
    {
        const LeafT& Leaf=Leaves[NodeNr];   // Der Name sollte hier eigentlich LeafNr sein.

        if (!Leaf.IsInnerLeaf) return Min;  // For outer solid leaves, return Min.

        // Leaf is a non-solid, inner leaf, and we have to take the TerrainEntities into account.
        const VectorT Origin=P+scale(U, Min);
        const VectorT Dir   =scale(U, Max-Min);

        VB_Trace3T<double> OtherChildrenTrace(1.0);

        for (unsigned long SetNr=0; SetNr<Leaf.OtherChildrenSet.Size(); SetNr++)
        {
            const unsigned long ChildNr=Leaf.OtherChildrenSet[SetNr];

            if (TBB_OtherChildrenCheckCounts[ChildNr]==TBB_CheckCount) continue;

            OtherChildren[ChildNr]->TraceBoundingBox(Consider, BoundingBox3T<double>(Vector3dT()), Origin, Dir, ClipMask, OtherChildrenTrace);
            TBB_OtherChildrenCheckCounts[ChildNr]=TBB_CheckCount;
        }

        return (Max-Min)*OtherChildrenTrace.Fraction+Min;
    }

    const NodeT& N=Nodes[NodeNr];

    // double Dist1=N.Plane.GetDistance(P+scale(U, Min));
    // double Dist2=N.Plane.GetDistance(P+scale(U, Max));

    double DistP=N.Plane.GetDistance(P);
    double Div  =dot(N.Plane.Normal, U);
    double Dist1=DistP+Min*Div;
    double Dist2=DistP+Max*Div;

    if (Dist1>0)
    {
        if (Dist2>0) return ClipLine(P, U, Min, Max, Consider, ClipMask, N.FrontChild, N.FrontIsLeaf);

        double Range=-DistP/Div;
        double Hit  =ClipLine(P, U, Min, Range, Consider, ClipMask, N.FrontChild, N.FrontIsLeaf);
        return Hit<Range ? Hit : ClipLine(P, U, Range, Max, Consider, ClipMask, N.BackChild, N.BackIsLeaf);
    }
    else
    {
        if (Dist2<=0) return ClipLine(P, U, Min, Max, Consider, ClipMask, N.BackChild, N.BackIsLeaf);

        double Range=-DistP/Div;
        double Hit  =ClipLine(P, U, Min, Range, Consider, ClipMask, N.BackChild, N.BackIsLeaf);
        return Hit<Range ? Hit : ClipLine(P, U, Range, Max, Consider, ClipMask, N.FrontChild, N.FrontIsLeaf);
    }
}


void BspTreeNodeT::TraceBoundingBoxHelper(float Min, float Max, unsigned long NodeNr, bool NodeIsLeaf) const
{
    // Schonmal etwas getroffen, das näher gelegen hat?
    if (TBB_Trace.Fraction<=Min) return;

    // Ist dieser Node ein Leaf?
    if (NodeIsLeaf)
    {
     // if ((Consider & ConsiderFaces)>0)
        for (unsigned long SetNr=0; SetNr<Leaves[NodeNr].BrushSet.Size(); SetNr++)
        {
            const unsigned long BrushNr=Leaves[NodeNr].BrushSet[SetNr];

            // This works because the trace works progressively from its origin to its destination.
            if (TBB_BrushCheckCounts[BrushNr]==TBB_CheckCount) continue;

            // Each brush includes all necessary bevel planes already,
            // and the TraceBoundingBox() method handles the testing, including the bloating and unbloating.
            ClipBrushes[BrushNr].TraceBoundingBox(*TBB_BB_Helper, *TBB_Origin_Helper, *TBB_Dir_Helper, TBB_Trace);
            TBB_BrushCheckCounts[BrushNr]=TBB_CheckCount;
        }

        for (unsigned long SetNr=0; SetNr<Leaves[NodeNr].OtherChildrenSet.Size(); SetNr++)
        {
            const unsigned long ChildNr=Leaves[NodeNr].OtherChildrenSet[SetNr];

            if (TBB_OtherChildrenCheckCounts[ChildNr]==TBB_CheckCount) continue;

            OtherChildren[ChildNr]->TraceBoundingBox(TBB_Consider, *TBB_BB_Helper, *TBB_Origin_Helper, *TBB_Dir_Helper, TBB_ClipMask, TBB_Trace);
            TBB_OtherChildrenCheckCounts[ChildNr]=TBB_CheckCount;
        }

        return;
    }


    const NodeT&   N     =Nodes[NodeNr];
    const VectorT& Normal=N.Plane.Normal;
    const float    DistP =float(N.Plane.GetDistance(*TBB_Origin_Helper));
    const float    Div   =float(dot(Normal, *TBB_Dir_Helper));
    const float    Dist1 =DistP+Min*Div;    // Dist1=N.Plane.GetDistance(Origin+scale(Dir, Min));
    const float    Dist2 =DistP+Max*Div;    // Dist2=N.Plane.GetDistance(Origin+scale(Dir, Max));

    // Hinweise zu den Bloat-Offsets:
    // a) Die Berechnung des 'BloatOffsetFront' entspricht dem Bloaten von Brushes ("<" Vergleich, nicht ">").
    // b) Bei der Berechnung des 'BloatOffsetBack' müßte das Minuszeichen eigentlich wie folgt stehen: "...=dot(-Normal, VectorT(...))".
    // c) Auf die Epsilons kann nicht verzichtet werden, ansonsten kann man plötzlich feststecken.
    const float    BloatOffsetFront=float(-dot(Normal, VectorT(Normal.x<0 ? TBB_BB_Helper->Max.x : TBB_BB_Helper->Min.x,
                                                               Normal.y<0 ? TBB_BB_Helper->Max.y : TBB_BB_Helper->Min.y,
                                                               Normal.z<0 ? TBB_BB_Helper->Max.z : TBB_BB_Helper->Min.z))+0.04 /*Epsilon*/);
    const float    BloatOffsetBack =float(-dot(Normal, VectorT(Normal.x>0 ? TBB_BB_Helper->Max.x : TBB_BB_Helper->Min.x,
                                                               Normal.y>0 ? TBB_BB_Helper->Max.y : TBB_BB_Helper->Min.y,
                                                               Normal.z>0 ? TBB_BB_Helper->Max.z : TBB_BB_Helper->Min.z))+0.04 /*Epsilon*/);

    // Wir müssen sehr konservativ vorgehen.
    // Prüfe zuerst, ob die trivialen Fälle vorliegen und erledige sie gegebenenfalls.
    if (Dist1>BloatOffsetFront && Dist2>BloatOffsetFront) { TraceBoundingBoxHelper(Min, Max, N.FrontChild, N.FrontIsLeaf); return; }
    if (Dist1<BloatOffsetBack  && Dist2<BloatOffsetBack ) { TraceBoundingBoxHelper(Min, Max, N. BackChild, N. BackIsLeaf); return; }

    if (Dist1<Dist2)
    {
        // Die Bewegung geht grob in die Richtung des Normalenvektors,
        // d.h. von der Rückseite der Node-Plane hin zur Vorderseite.
        // Es ist also auch 'Div>0'.

        // Teste den Bewegungsabschnitt von 'Min' bis zum Schnittpunkt mit der nach *vorne* gebloateten Node-Plane gegen die *Rückseite*.
        float Range=-(DistP-BloatOffsetFront)/Div;

        if (Range<Min) Range=Min;
        if (Range>Max) Range=Max;

        TraceBoundingBoxHelper(Min, Range, N.BackChild, N.BackIsLeaf);

        // Teste den Bewegungsabschnitt vom Schnittpunkt mit der nach *hinten* gebloateten Node-Plane bis 'Max' gegen die *Vorderseite*.
        Range=-(DistP-BloatOffsetBack)/Div;

        if (Range<Min) Range=Min;
        if (Range>Max) Range=Max;

        TraceBoundingBoxHelper(Range, Max, N.FrontChild, N.FrontIsLeaf);
    }
    else if (Dist1>Dist2)
    {
        // Die Bewegung geht grob in die Gegenrichtung des Normalenvektors,
        // d.h. von der Vorderseite der Node-Plane hin zur Rückseite.
        // Es ist also auch 'Div<0'.

        // Teste den Bewegungsabschnitt von 'Min' bis zum Schnittpunkt mit der nach *hinten* gebloateten Node-Plane gegen die *Vorderseite*.
        float Range=-(DistP-BloatOffsetBack)/Div;

        if (Range<Min) Range=Min;
        if (Range>Max) Range=Max;

        TraceBoundingBoxHelper(Min, Range, N.FrontChild, N.FrontIsLeaf);

        // Teste den Bewegungsabschnitt vom Schnittpunkt mit der nach *vorne* gebloateten Node-Plane bis 'Max' gegen die *Rückseite*.
        Range=-(DistP-BloatOffsetFront)/Div;

        if (Range<Min) Range=Min;
        if (Range>Max) Range=Max;

        TraceBoundingBoxHelper(Range, Max, N.BackChild, N.BackIsLeaf);
    }
    else
    {
        // Die Bewegung geht parallel zur Node-Plane. Es ist also auch 'Div==0'.
        // Teste daher beide Seiten komplett.
        TraceBoundingBoxHelper(Min, 1.0, N.FrontChild, N.FrontIsLeaf);
        TraceBoundingBoxHelper(0.0, Max, N. BackChild, N. BackIsLeaf);
    }
}


VB_Trace3T<double> BspTreeNodeT::TraceBoundingBox(const BoundingBox3T<double>& BB, const VectorT& Origin, const VectorT& Dir, const ConsiderT& Consider, MaterialT::ClipFlagsT ClipMask) const
{
    TBB_BB_Helper    =&BB;
    TBB_Origin_Helper=&Origin;
    TBB_Dir_Helper   =&Dir;
    TBB_Trace        =VB_Trace3T<double>(1.0);
    TBB_Consider     =Consider;
    TBB_ClipMask     =ClipMask;

    while (TBB_BrushCheckCounts.Size()<ClipBrushes.Size()) TBB_BrushCheckCounts.PushBack(0);
    while (TBB_OtherChildrenCheckCounts.Size()<OtherChildren.Size()) TBB_OtherChildrenCheckCounts.PushBack(0);
    TBB_CheckCount++;

    TraceBoundingBoxHelper(0.0, 1.0, 0, false);

    return TBB_Trace;
}
#endif


ArrayT<unsigned long>* OrderedLeavesHelper=NULL;
const VectorT*         OriginHelper       =NULL;

void BspTreeNodeT::GetLeavesOrderedBackToFrontHelper(unsigned long NodeNr) const
{
    const NodeT& N=Nodes[NodeNr];

    if (N.Plane.GetDistance(*OriginHelper)>0)
    {
        if (N.BackIsLeaf) OrderedLeavesHelper->PushBack(N.BackChild);
                     else GetLeavesOrderedBackToFrontHelper(N.BackChild);

        if (N.FrontIsLeaf) OrderedLeavesHelper->PushBack(N.FrontChild);
                      else GetLeavesOrderedBackToFrontHelper(N.FrontChild);
    }
    else
    {
        if (N.FrontIsLeaf) OrderedLeavesHelper->PushBack(N.FrontChild);
                      else GetLeavesOrderedBackToFrontHelper(N.FrontChild);

        if (N.BackIsLeaf) OrderedLeavesHelper->PushBack(N.BackChild);
                     else GetLeavesOrderedBackToFrontHelper(N.BackChild);
    }
}


void BspTreeNodeT::GetLeavesOrderedBackToFront(ArrayT<unsigned long>& OrderedLeaves, const VectorT& Origin) const
{
    OrderedLeavesHelper=&OrderedLeaves;
    OriginHelper       =&Origin;

    OrderedLeavesHelper->Overwrite();
    GetLeavesOrderedBackToFrontHelper(0);
}


bool BspTreeNodeT::IsInPVS(unsigned long QueryLeafNr, unsigned long LeafNr) const
{
    // Assume that the PVS is symmetric: If A can see B, then B can see A.
    const unsigned long PVSTotalBitNr=LeafNr*Leaves.Size()+QueryLeafNr;

    return bool((PVS[PVSTotalBitNr >> 5] >> (PVSTotalBitNr & 31)) & 1);
}


bool BspTreeNodeT::IsInPVS(const VectorT& Position, unsigned long LeafNr) const
{
    // Assume that the PVS is symmetric: If A can see B, then B can see A.
    const unsigned long PVSTotalBitNr=LeafNr*Leaves.Size()+WhatLeaf(Position);

    return bool((PVS[PVSTotalBitNr >> 5] >> (PVSTotalBitNr & 31)) & 1);
}


bool BspTreeNodeT::IsInPVS(const BoundingBox3T<double>& BoundingBox, unsigned long LeafNr) const
{
    ArrayT<unsigned long> LeavesTouchedByBB;

    WhatLeaves(LeavesTouchedByBB, BoundingBox);

    for (unsigned long LeafIndexNr=0; LeafIndexNr<LeavesTouchedByBB.Size(); LeafIndexNr++)
    {
        const unsigned long PVSTotalBitNr=LeafNr*Leaves.Size()+LeavesTouchedByBB[LeafIndexNr];

        if ((PVS[PVSTotalBitNr >> 5] >> (PVSTotalBitNr & 31)) & 1) return true;
    }

    return false;
}
