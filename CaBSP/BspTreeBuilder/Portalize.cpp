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

/*****************/
/*** Portalize ***/
/*****************/


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
        const Polygon3T<double>& Portal=NewPortals[PortalNr];

        // In degenerierten Grenzfällen (in Gegenwart von Splittern) können auch andere ungültige Polygone entstehen
        // (z.B. mehrere Vertices quasi auf einer Edge), sodaß wir explizit die Gültigkeit prüfen.
        // if (Portal.Vertices.Size()<3) continue;    // Ist in .IsValid() enthalten!
        if (!Portal.IsValid(MapT::RoundEpsilon, MapT::MinVertexDist)) continue;


        // Another very serious problem is the fact that we sometimes self-create leaks,
        // because nearly all operations in this program suffer from rounding errors.
        // I have *NO* idea how to best combat them (except the introduction of exact arithmetic, which I'm seriously considering).
        // But for now, lets try something simpler - enforce a "minimum area" for portals.
        // Portals that are smaller than this minimum are considered degenerate, despite they were classified as valid above.
        // Note that the same is enforced below, where portals are split along the leafs faces.
        // UPDATE: As the new Polygon3T<double>::IsValid() method now enforces the MapT::MinVertexDist,
        // I believe the problem is solved the the polygon area check not longer required.
        if (Portal.GetArea()<=100.0 /* 1 cm^2 */) continue;


        // Note that rejecting portals here (i.e. adding additional test criteria) is a dangerous idea,
        // because any omitted portal might stop the subsequent flood-fill early.
        // This in turn might easily tear big holes into the world.
        // Consider my Tech-Archive notes from 2005-11-15 for a sketch that shows a problematic (but valid!) leaf
        // whose entry portal must not be omitted so that it can be entered during the flood-fill,
        // or else the left wall will be erroneously removed by the fill.


        // Okay, the portal seems to be good, so add it to the leaf.
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

    // Kommentare des ehem. Portalize berücksichtigen!!!
    ArrayT< Plane3T<double> > NodeList;

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
                    if (NewPortals[PNr].IsValid(MapT::RoundEpsilon, MapT::MinVertexDist))
                        // Another very serious problem is the fact that we sometimes self-create leaks,
                        // because nearly all operations in this program suffer from rounding errors.
                        // I have *NO* idea how to best combat them (except the introduction of exact arithmetic, which I'm seriously considering).
                        // But for now, lets try something simpler - enforce a "minimum area" for portals.
                        // Portals that are smaller than this minimum are considered degenerate, despite they were classified as valid above.
                        // Note that the same is enforced above, where portals are first created.
                        // UPDATE: As the new Polygon3T<double>::IsValid() method now enforces the MapT::MinVertexDist,
                        // I believe the problem is solved the the polygon area check not longer required.
                        if (NewPortals[PNr].GetArea()>100.0 /* 1 cm^2 */)
                            Leaves[LeafNr].Portals.PushBack(NewPortals[PNr]);

                PortalNr--;
                break;
            }

        TotalNrOfPortals+=Leaves[LeafNr].Portals.Size();
    }

    Console->Print(cf::va("Portals             : %10lu\n", TotalNrOfPortals));
}
