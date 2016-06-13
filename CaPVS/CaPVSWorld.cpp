/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/**************************/
/*** CaPVS World (Code) ***/
/**************************/

#include <stdio.h>
#include "CaPVSWorld.hpp"
#include "SceneGraph/Node.hpp"
#include "SceneGraph/BspTreeNode.hpp"
#include "SceneGraph/FaceNode.hpp"


CaPVSWorldT::CaPVSWorldT(const char* FileName, ModelManagerT& ModelMan, cf::GuiSys::GuiResourcesT& GuiRes, unsigned long SLC_MaxRecursionDepth_, double SLC_MinSubTreeFacesArea_)
    : m_World(FileName, ModelMan, GuiRes),
      m_BspTree(m_World.m_StaticEntityData[0]->m_BspTree),
      SLC_MaxRecursionDepth(SLC_MaxRecursionDepth_),
      SLC_MinSubTreeFacesArea(SLC_MinSubTreeFacesArea_)
{
}


double CaPVSWorldT::SubTreeFacesArea(unsigned long NodeNr) const
{
    const ArrayT<cf::SceneGraph::BspTreeNodeT::NodeT>& Nodes  = m_BspTree->Nodes;
    const ArrayT<cf::SceneGraph::BspTreeNodeT::LeafT>& Leaves = m_BspTree->Leaves;

    // TODO: This method counts faces that are in multiple leaves multiply!
    // This is a BUG that should be fixed!!!
    double Area=0.0;

    if (Nodes[NodeNr].FrontIsLeaf)
    {
        const cf::SceneGraph::BspTreeNodeT::LeafT& L=Leaves[Nodes[NodeNr].FrontChild];

        for (unsigned long SetNr=0; SetNr<L.FaceChildrenSet.Size(); SetNr++)
            Area += m_BspTree->FaceChildren[L.FaceChildrenSet[SetNr]]->Polygon.GetArea();
    }
    else Area+=SubTreeFacesArea(Nodes[NodeNr].FrontChild);

    if (Nodes[NodeNr].BackIsLeaf)
    {
        const cf::SceneGraph::BspTreeNodeT::LeafT& L=Leaves[Nodes[NodeNr].BackChild];

        for (unsigned long SetNr=0; SetNr<L.FaceChildrenSet.Size(); SetNr++)
            Area += m_BspTree->FaceChildren[L.FaceChildrenSet[SetNr]]->Polygon.GetArea();
    }
    else Area+=SubTreeFacesArea(Nodes[NodeNr].BackChild);

    return Area;
}


// Considers the sub-tree beginning at node 'NodeNr', and determines whether a SuperLeaf should be constructed from it or not.
// Returns 'true' when a SuperLeaf should be constructed, 'false' otherwise.
bool CaPVSWorldT::SuperLeafConditionIsMet(unsigned long NodeNr, unsigned long RecursionDepth) const
{
    if (RecursionDepth>SLC_MaxRecursionDepth) return true;
    if (SubTreeFacesArea(NodeNr)<SLC_MinSubTreeFacesArea) return true;

    return false;
}


void CaPVSWorldT::CreateSuperLeafFromSubTreeRecursive(unsigned long NodeNr, SuperLeafT& SuperLeaf) const
{
    const ArrayT<cf::SceneGraph::BspTreeNodeT::NodeT>& Nodes  = m_BspTree->Nodes;
    const ArrayT<cf::SceneGraph::BspTreeNodeT::LeafT>& Leaves = m_BspTree->Leaves;

    if (Nodes[NodeNr].FrontIsLeaf)
    {
        const unsigned long LeafNr=Nodes[NodeNr].FrontChild;

        SuperLeaf.LeafSet.PushBack(LeafNr);

        for (unsigned long PortalNr=0; PortalNr<Leaves[LeafNr].Portals.Size(); PortalNr++)
            SuperLeaf.Portals.PushBack(Leaves[LeafNr].Portals[PortalNr]);

        if (SuperLeaf.LeafSet.Size()>1)
        {
            SuperLeaf.BB.Insert(Leaves[LeafNr].BB.Min);
            SuperLeaf.BB.Insert(Leaves[LeafNr].BB.Max);
        }
        else SuperLeaf.BB=Leaves[LeafNr].BB;
    }
    else CreateSuperLeafFromSubTreeRecursive(Nodes[NodeNr].FrontChild, SuperLeaf);

    if (Nodes[NodeNr].BackIsLeaf)
    {
        const unsigned long LeafNr=Nodes[NodeNr].BackChild;

        SuperLeaf.LeafSet.PushBack(LeafNr);

        for (unsigned long PortalNr=0; PortalNr<Leaves[LeafNr].Portals.Size(); PortalNr++)
            SuperLeaf.Portals.PushBack(Leaves[LeafNr].Portals[PortalNr]);

        if (SuperLeaf.LeafSet.Size()>1)
        {
            SuperLeaf.BB.Insert(Leaves[LeafNr].BB.Min);
            SuperLeaf.BB.Insert(Leaves[LeafNr].BB.Max);
        }
        else SuperLeaf.BB=Leaves[LeafNr].BB;
    }
    else CreateSuperLeafFromSubTreeRecursive(Nodes[NodeNr].BackChild, SuperLeaf);
}


// Traverses the sub-tree beginning at node 'NodeNr' and constructs a SuperLeaf from it.
// The resulting SuperLeaf is returned.
SuperLeafT CaPVSWorldT::CreateSuperLeafFromSubTree(unsigned long NodeNr) const
{
    SuperLeafT SuperLeaf;

    CreateSuperLeafFromSubTreeRecursive(NodeNr, SuperLeaf);

    return SuperLeaf;
}


void CaPVSWorldT::CreateSuperLeavesRecursive(unsigned long NodeNr, ArrayT<SuperLeafT>& SuperLeaves, unsigned long RecursionDepth) const
{
    const ArrayT<cf::SceneGraph::BspTreeNodeT::NodeT>& Nodes  = m_BspTree->Nodes;
    const ArrayT<cf::SceneGraph::BspTreeNodeT::LeafT>& Leaves = m_BspTree->Leaves;

    if (Nodes[NodeNr].FrontIsLeaf)
    {
        // Create SuperLeaf from single leaf child.
        const unsigned long LeafNr=Nodes[NodeNr].FrontChild;

        SuperLeaves.PushBackEmpty();
        SuperLeaves[SuperLeaves.Size()-1].LeafSet.PushBack(LeafNr);
        SuperLeaves[SuperLeaves.Size()-1].Portals=Leaves[LeafNr].Portals;
        SuperLeaves[SuperLeaves.Size()-1].BB     =Leaves[LeafNr].BB;
    }
    else
    {
        if (SuperLeafConditionIsMet(Nodes[NodeNr].FrontChild, RecursionDepth+1))
        {
            // Create SuperLeaf from the remaining front sub-tree.
            SuperLeaves.PushBack(CreateSuperLeafFromSubTree(Nodes[NodeNr].FrontChild));
        }
        else CreateSuperLeavesRecursive(Nodes[NodeNr].FrontChild, SuperLeaves, RecursionDepth+1);
    }

    if (Nodes[NodeNr].BackIsLeaf)
    {
        // Create SuperLeaf from single leaf child.
        const unsigned long LeafNr=Nodes[NodeNr].BackChild;

        SuperLeaves.PushBackEmpty();
        SuperLeaves[SuperLeaves.Size()-1].LeafSet.PushBack(LeafNr);
        SuperLeaves[SuperLeaves.Size()-1].Portals=Leaves[LeafNr].Portals;
        SuperLeaves[SuperLeaves.Size()-1].BB     =Leaves[LeafNr].BB;
    }
    else
    {
        if (SuperLeafConditionIsMet(Nodes[NodeNr].BackChild, RecursionDepth+1))
        {
            // Create SuperLeaf from the remaining back sub-tree.
            SuperLeaves.PushBack(CreateSuperLeafFromSubTree(Nodes[NodeNr].BackChild));
        }
        else CreateSuperLeavesRecursive(Nodes[NodeNr].BackChild, SuperLeaves, RecursionDepth+1);
    }
}


void CaPVSWorldT::CreateSuperLeaves(ArrayT<SuperLeafT>& SuperLeaves) const
{
    const ArrayT<cf::SceneGraph::BspTreeNodeT::LeafT>& Leaves = m_BspTree->Leaves;

    printf("\n*** Create SuperLeaves ***\n");

    CreateSuperLeavesRecursive(0, SuperLeaves, 0);

    printf("%5lu SuperLeaves created from\n", SuperLeaves.Size());
    printf("%5lu world leaves in CW file.\n", Leaves.Size());
}


unsigned long CaPVSWorldT::WhatLeaf(const VectorT& Position) const
{
    return m_BspTree->WhatLeaf(Position);
}


double CaPVSWorldT::ClipLine(const VectorT& P, const VectorT& U) const
{
    return m_BspTree->ClipLine(P, U, 0.0, 1.0);
}


void CaPVSWorldT::StorePVS(const ArrayT<SuperLeafT>& SuperLeaves, const ArrayT<unsigned long>& SuperLeavesPVS)
{
    const ArrayT<cf::SceneGraph::BspTreeNodeT::LeafT>& Leaves = m_BspTree->Leaves;
    ArrayT<uint32_t>&                                  PVS    = m_BspTree->PVS;

    // 'PVS' zurücksetzen (völlige Blindheit).
    for (unsigned long Vis=0; Vis<PVS.Size(); Vis++) PVS[Vis]=0;

    // 'SuperLeavesPVS' ins 'PVS' übertragen.
    for (unsigned long SL1Nr=0; SL1Nr<SuperLeaves.Size(); SL1Nr++)
        for (unsigned long SL2Nr=0; SL2Nr<SuperLeaves.Size(); SL2Nr++)
        {
            const unsigned long PVSTotalBitNr1=SL1Nr*SuperLeaves.Size()+SL2Nr;
            const unsigned long PVSTotalBitNr2=SL2Nr*SuperLeaves.Size()+SL1Nr;
            const bool          CanSeeFrom1To2=bool((SuperLeavesPVS[PVSTotalBitNr1 >> 5] >> (PVSTotalBitNr1 & 31)) & 1);
            const bool          CanSeeFrom2To1=bool((SuperLeavesPVS[PVSTotalBitNr2 >> 5] >> (PVSTotalBitNr2 & 31)) & 1);

            // 'CanSeeFrom2To1' ist nicht wirklich notwendig, nur wegen der PVS-Matrix-Symmetrie
            // (um "Can see from A to B, but not vice versa!" zu vermeiden)!
            if (CanSeeFrom1To2 && CanSeeFrom2To1)
            {
                // Kann von allen Leaves in 'SuperLeaves[SL1Nr]' alle Leaves in 'SuperLeaves[SL2Nr]' sehen.
                for (unsigned long LeafSet1Nr=0; LeafSet1Nr<SuperLeaves[SL1Nr].LeafSet.Size(); LeafSet1Nr++)
                    for (unsigned long LeafSet2Nr=0; LeafSet2Nr<SuperLeaves[SL2Nr].LeafSet.Size(); LeafSet2Nr++)
                    {
                        const unsigned long Leaf1Nr=SuperLeaves[SL1Nr].LeafSet[LeafSet1Nr];
                        const unsigned long Leaf2Nr=SuperLeaves[SL2Nr].LeafSet[LeafSet2Nr];

                        if (!Leaves[Leaf1Nr].IsInnerLeaf) continue;
                        if (!Leaves[Leaf2Nr].IsInnerLeaf) continue;

                        // Kann von 'Leaf1Nr' nach 'Leaf2Nr' sehen, markiere also 'Leaf2Nr' als von 'Leaf1Nr' aus sichtbar.
                        const unsigned long PVSTotalBitNr=Leaf1Nr*Leaves.Size()+Leaf2Nr;
                        const unsigned long PVS_W32_Nr   =PVSTotalBitNr >> 5;
                        const unsigned long PVSBitMask   =1 << (PVSTotalBitNr & 31);
                        PVS[PVS_W32_Nr]|=PVSBitMask;
                    }
            }
        }
}


unsigned long CaPVSWorldT::GetChecksumAndPrintStats() const
{
    const ArrayT<uint32_t>& PVS = m_BspTree->PVS;

    printf("\n*** Statistics ***\n");

    unsigned long CheckSum=0;

    for (unsigned long Count=0; Count<PVS.Size(); Count++)
    {
        CheckSum+=(PVS[Count] >> 24) & 0xFF;
        CheckSum+=(PVS[Count] >> 16) & 0xFF;
        CheckSum+=(PVS[Count] >>  8) & 0xFF;
        CheckSum+=(PVS[Count]      ) & 0xFF;
    }

    printf("Size (bytes)        : %10lu\n", PVS.Size()*4);
    printf("CheckSum            : %10lu\n", CheckSum    );

    return CheckSum;
}


void CaPVSWorldT::SaveToDisk(const char* FileName) const
{
    m_World.SaveToDisk(FileName);
}
