/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

/**************************/
/*** CaPVS World (Code) ***/
/**************************/

#include <stdio.h>
#include "CaPVSWorld.hpp"
#include "SceneGraph/Node.hpp"
#include "SceneGraph/BspTreeNode.hpp"
#include "SceneGraph/FaceNode.hpp"


CaPVSWorldT::CaPVSWorldT(const char* FileName, ModelManagerT& ModelMan, unsigned long SLC_MaxRecursionDepth_, double SLC_MinSubTreeFacesArea_)
    : World(FileName, ModelMan),
      SLC_MaxRecursionDepth(SLC_MaxRecursionDepth_),
      SLC_MinSubTreeFacesArea(SLC_MinSubTreeFacesArea_)
{
}


double CaPVSWorldT::SubTreeFacesArea(unsigned long NodeNr) const
{
    ArrayT<cf::SceneGraph::BspTreeNodeT::NodeT>& Nodes =World.BspTree->Nodes;
    ArrayT<cf::SceneGraph::BspTreeNodeT::LeafT>& Leaves=World.BspTree->Leaves;

    // TODO: This method counts faces that are in multiple leaves multiply!
    // This is a BUG that should be fixed!!!
    double Area=0.0;

    if (Nodes[NodeNr].FrontIsLeaf)
    {
        const cf::SceneGraph::BspTreeNodeT::LeafT& L=Leaves[Nodes[NodeNr].FrontChild];

        for (unsigned long SetNr=0; SetNr<L.FaceChildrenSet.Size(); SetNr++)
            Area+=World.BspTree->FaceChildren[L.FaceChildrenSet[SetNr]]->Polygon.GetArea();
    }
    else Area+=SubTreeFacesArea(Nodes[NodeNr].FrontChild);

    if (Nodes[NodeNr].BackIsLeaf)
    {
        const cf::SceneGraph::BspTreeNodeT::LeafT& L=Leaves[Nodes[NodeNr].BackChild];

        for (unsigned long SetNr=0; SetNr<L.FaceChildrenSet.Size(); SetNr++)
            Area+=World.BspTree->FaceChildren[L.FaceChildrenSet[SetNr]]->Polygon.GetArea();
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
    ArrayT<cf::SceneGraph::BspTreeNodeT::NodeT>& Nodes =World.BspTree->Nodes;
    ArrayT<cf::SceneGraph::BspTreeNodeT::LeafT>& Leaves=World.BspTree->Leaves;

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
    ArrayT<cf::SceneGraph::BspTreeNodeT::NodeT>& Nodes =World.BspTree->Nodes;
    ArrayT<cf::SceneGraph::BspTreeNodeT::LeafT>& Leaves=World.BspTree->Leaves;

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
    ArrayT<cf::SceneGraph::BspTreeNodeT::LeafT>& Leaves=World.BspTree->Leaves;

    printf("\n*** Create SuperLeaves ***\n");

    CreateSuperLeavesRecursive(0, SuperLeaves, 0);

    printf("%5lu SuperLeaves created from\n", SuperLeaves.Size());
    printf("%5lu world leaves in CW file.\n", Leaves.Size());
}


unsigned long CaPVSWorldT::WhatLeaf(const VectorT& Position) const
{
    return World.BspTree->WhatLeaf(Position);
}


double CaPVSWorldT::ClipLine(const VectorT& P, const VectorT& U) const
{
    return World.BspTree->ClipLine(P, U, 0.0, 1.0);
}


void CaPVSWorldT::StorePVS(const ArrayT<SuperLeafT>& SuperLeaves, const ArrayT<unsigned long>& SuperLeavesPVS)
{
    ArrayT<cf::SceneGraph::BspTreeNodeT::LeafT>& Leaves=World.BspTree->Leaves;
    ArrayT<uint32_t>&                            PVS   =World.BspTree->PVS;

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
    ArrayT<uint32_t>& PVS=World.BspTree->PVS;

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
    World.SaveToDisk(FileName);
}
