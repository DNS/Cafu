/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2014 Carsten Fuchs Software.

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


namespace
{
    void FixPortal(Polygon3T<double>& Portal)
    {
        for (unsigned long VertexNr = 0; VertexNr < Portal.Vertices.Size(); VertexNr++)
        {
            const unsigned long NextNr = (VertexNr + 1) % Portal.Vertices.Size();

            if (length(Portal.Vertices[VertexNr] - Portal.Vertices[NextNr]) < MapT::RoundEpsilon * 1.5)
            {
                Portal.Vertices.RemoveAtAndKeepOrder(NextNr);
                VertexNr--;
            }
        }
    }


    bool IsAcceptable(const Polygon3T<double>& Portal)
    {
        if (Portal.Vertices.Size() < 3) return false;
        if (Portal.GetArea() < 1.0) return false;

     // assert(Portal.IsValid(MapT::RoundEpsilon, MapT::MinVertexDist));
        return true;
    }
}


void BspTreeBuilderT::CreateLeafPortals(unsigned long LeafNr, const ArrayT< Plane3T<double> >& NodeList)
{
    ArrayT<cf::SceneGraph::BspTreeNodeT::LeafT>& Leaves=BspTree->Leaves;

    Console->Print(cf::va("%5.1f%%\r", (double)LeafNr/Leaves.Size()*100.0));
    // fflush(stdout);      // The stdout console auto-flushes the output.

    const BoundingBox3T<double> LeafBB=Leaves[LeafNr].BB.GetEpsilonBox(MapT::RoundEpsilon);

    ArrayT< Polygon3T<double> > NewPortals;

    for (unsigned long Nr=0; Nr<NodeList.Size(); Nr++)
        if (LeafBB.WhatSide(NodeList[Nr])==BoundingBox3T<double>::Both)
        {
            NewPortals.PushBackEmpty();
            NewPortals[NewPortals.Size()-1].Plane=NodeList[Nr];
        }

    // Hier ist auch denkbar, daß Portals mit 0 Vertices zurückkommen (outer leaves)!
    // (Auch ganz normale gültige Portale in outer leaves sind denkbar!)
    Polygon3T<double>::Complete(NewPortals, MapT::RoundEpsilon);

    for (unsigned long PortalNr=0; PortalNr<NewPortals.Size(); PortalNr++)
    {
        Polygon3T<double>& Portal = NewPortals[PortalNr];

        // Portals must be crafted very carefully, and we must carefully consider which ones
        // we keep and which ones we reject:
        //
        //   - Rejecting portals too easily may tear big holes into the world:
        //     It is well possible that Portal has vertices that are closer to each other than
        //     MapT::MinVertexDist, or has more than two vertices that are (almost) on a
        //     straight line. That is, Portal.IsValid(MapT::RoundEpsilon, MapT::MinVertexDist)
        //     might return false for a portal that is otherwise proper and geometrically
        //     important (consider any door-like portal as an example). If such a portal was
        //     rejected, the subsequent flood-fill would not pass through it. Anything beyond
        //     it would subsequently be removed, causing a huge leak.
        //
        //   - On the other hand, portals that are only thin slivers or generally very small
        //     (but otherwise valid) should probably not be kept: Such portals are likely the
        //     result of rounding errors elsewhere and not expected to be useful.
        FixPortal(Portal);

        if (IsAcceptable(Portal))
            Leaves[LeafNr].Portals.PushBack(Portal.GetMirror());
    }
}


void BspTreeBuilderT::BuildBSPPortals(unsigned long NodeNr, ArrayT< Plane3T<double> >& NodeList)
{
    const ArrayT<cf::SceneGraph::BspTreeNodeT::NodeT>& Nodes=BspTree->Nodes;

    NodeList.PushBack(Nodes[NodeNr].Plane.GetMirror());

    if (Nodes[NodeNr].FrontIsLeaf) CreateLeafPortals(Nodes[NodeNr].FrontChild, NodeList);
                              else BuildBSPPortals(Nodes[NodeNr].FrontChild, NodeList);

    NodeList[NodeList.Size()-1]=Nodes[NodeNr].Plane;

    if (Nodes[NodeNr].BackIsLeaf) CreateLeafPortals(Nodes[NodeNr].BackChild, NodeList);
                             else BuildBSPPortals(Nodes[NodeNr].BackChild, NodeList);

    NodeList.DeleteBack();
}


void BspTreeBuilderT::Portalize()
{
    ArrayT<cf::SceneGraph::BspTreeNodeT::LeafT>& Leaves=BspTree->Leaves;

    if (FaceChildren.Size()==0)
    {
        // BSP trees with zero faces (and thus no nodes and no leaves) can occur with many entity classes ("point entities").
        assert(BspTree->Nodes.Size()==0);
        assert(BspTree->Leaves.Size()==0);
        return;
    }

    Console->Print(cf::va("\n%-50s %s\n", "*** Portalization ***", GetTimeSinceProgramStart()));

    unsigned long TotalNrOfPortals=0;

    ArrayT<Plane3dT> NodeList;

    BuildBSPPortals(0, NodeList);
    Console->Print("Portalization       :       done\n");

    for (unsigned long LeafNr=0; LeafNr<Leaves.Size(); LeafNr++)
    {
        Console->Print(cf::va("%5.1f%%\r", (double)LeafNr/Leaves.Size()*100.0));
        // fflush(stdout);      // The stdout console auto-flushes the output.

        for (unsigned long PortalNr=0; PortalNr<Leaves[LeafNr].Portals.Size(); PortalNr++)
            for (unsigned long FaceNr=0; FaceNr<Leaves[LeafNr].FaceChildrenSet.Size(); FaceNr++)
            {
                const cf::SceneGraph::FaceNodeT* CurrentFace  =FaceChildren[Leaves[LeafNr].FaceChildrenSet[FaceNr]];
                const Polygon3T<double>&         CurrentPortal=Leaves[LeafNr].Portals[PortalNr];

                // Wenn das Material der CurrentFace "durchsichtig" ist, d.h. BSP Portale nicht clippt
                // bzw. nicht solid für sie ist, mache weiter (und lasse das CurrentPortal unberührt).
                if ((CurrentFace->Material->ClipFlags & MaterialT::Clip_BspPortals)==0) continue;

                // Wenn die CurrentFace das CurrentPortal nicht überlappt, mache weiter.
                if (!CurrentFace->Polygon.Overlaps(CurrentPortal, false, MapT::RoundEpsilon)) continue;

                // Zerschneide das CurrentPortal entlang der Edges der CurrentFace.
                ArrayT< Polygon3T<double> > NewPortals;
                CurrentPortal.GetChoppedUpAlong(CurrentFace->Polygon, MapT::RoundEpsilon, NewPortals);

                // Das letzte der NewPortals überdeckt sich mit der Face, daher löschen wir es.
                NewPortals.DeleteBack();

                // Das alte Portal wird nicht mehr gebraucht.
                Leaves[LeafNr].Portals[PortalNr]=Leaves[LeafNr].Portals[Leaves[LeafNr].Portals.Size()-1];
                Leaves[LeafNr].Portals.DeleteBack();

                // Dafür die Splitter anhängen.
                for (unsigned long PNr=0; PNr<NewPortals.Size(); PNr++)
                {
                    Polygon3T<double>& Portal = NewPortals[PNr];

                    // See the comment in CreateLeafPortals() for more details.
                    FixPortal(Portal);

                    if (IsAcceptable(Portal))
                        Leaves[LeafNr].Portals.PushBack(Portal);
                }

                PortalNr--;
                break;
            }

        TotalNrOfPortals+=Leaves[LeafNr].Portals.Size();
    }

    Console->Print(cf::va("Portals             : %10lu\n", TotalNrOfPortals));
}
