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

/**********************/
/*** Leak Detection ***/
/**********************/


void BspTreeBuilderT::PrepareLeakDetection(const ArrayT<Vector3dT>& FloodFillSources, MaterialT* LeakDetectMat)
{
    // Determine a world bounding box.
    BoundingBox3T<double> WorldBB(FaceChildren[0]->Polygon.Vertices);

    for (unsigned long FaceNr=1; FaceNr<FaceChildren.Size(); FaceNr++)
        WorldBB.Insert(FaceChildren[FaceNr]->Polygon.Vertices);

    // The flood-fill source points are usually obtained from the origins of the info_player_start entities.
    for (unsigned long SourceNr=0; SourceNr<FloodFillSources.Size(); SourceNr++)
        WorldBB.Insert(FloodFillSources[SourceNr]);

    const double d=10.0*MapT::MinVertexDist;

    WorldBB.Min-=VectorT(d, d, d);  // Kleiner Sicherheitszuschlag,
    WorldBB.Max+=VectorT(d, d, d);  // um es korrekter und sicherer zu machen.

    // Add special bounding faces into the world.
    ArrayT< Polygon3T<double> > BoundingPolys;
    BoundingPolys.PushBackEmpty(6);

    BoundingPolys[0].Plane=Plane3T<double>(VectorT( 1.0,  0.0,  0.0),  WorldBB.Max.x);
    BoundingPolys[1].Plane=Plane3T<double>(VectorT( 0.0,  1.0,  0.0),  WorldBB.Max.y);
    BoundingPolys[2].Plane=Plane3T<double>(VectorT( 0.0,  0.0,  1.0),  WorldBB.Max.z);
    BoundingPolys[3].Plane=Plane3T<double>(VectorT(-1.0,  0.0,  0.0), -WorldBB.Min.x);
    BoundingPolys[4].Plane=Plane3T<double>(VectorT( 0.0, -1.0,  0.0), -WorldBB.Min.y);
    BoundingPolys[5].Plane=Plane3T<double>(VectorT( 0.0,  0.0, -1.0), -WorldBB.Min.z);

    Polygon3T<double>::Complete(BoundingPolys, MapT::RoundEpsilon);

    for (unsigned long PolyNr=0; PolyNr<BoundingPolys.Size(); PolyNr++)
    {
        if (!BoundingPolys[PolyNr].IsValid(MapT::RoundEpsilon, MapT::MinVertexDist)) Error("Preparing leak detection: bounding polygon is invalid!");

        BoundingPolys[PolyNr]=BoundingPolys[PolyNr].GetMirror();

        cf::SceneGraph::FaceNodeT::TexInfoT TI;

        FaceChildren.PushBack(new cf::SceneGraph::FaceNodeT(FaceChildren[0]->GetLightMapMan(), FaceChildren[0]->GetSHLMapMan(), BoundingPolys[PolyNr], LeakDetectMat, TI));
    }
}


ArrayT<unsigned long> BFS_Tree;
ArrayT<VectorT>       BFS_TreePoints;


// Computes a BFS_Tree, that contains for each leaf the shortest path to the leaf that contains 'Start'.
// Note that there can also exist leaves for which no path to 'Start' exists at all.
void BspTreeBuilderT::ComputeLeakPathByBFS(const VectorT& Start) const
{
    ArrayT<cf::SceneGraph::BspTreeNodeT::LeafT>& Leaves=BspTree->Leaves;

    BFS_Tree.Clear();         for (unsigned long LeafNr=0; LeafNr<Leaves.Size(); LeafNr++) BFS_Tree.PushBack((unsigned long)-1);
    BFS_TreePoints.Clear();   for (unsigned long LeafNr=0; LeafNr<Leaves.Size(); LeafNr++) BFS_TreePoints.PushBack(VectorT());
    ArrayT<bool> BFS_Visited; for (unsigned long LeafNr=0; LeafNr<Leaves.Size(); LeafNr++) BFS_Visited.PushBack(false);

    ArrayT<unsigned long> ToDoList;
    ToDoList.PushBack(BspTree->WhatLeaf(Start));

    BFS_Visited   [ToDoList[0]]=true;
 // BFS_Tree      [ToDoList[0]]=(unsigned long)-1;
    BFS_TreePoints[ToDoList[0]]=Start;

    while (ToDoList.Size())
    {
        // Nimm das erste Element aus der ToDoList...
        unsigned long CurrentLeafNr=ToDoList[0];

        // ...und rücke alles eins runter
        for (unsigned long LeafNr=0; LeafNr+1<ToDoList.Size(); LeafNr++) ToDoList[LeafNr]=ToDoList[LeafNr+1];
        ToDoList.DeleteBack();

        // Alle Nachbarn betrachten
        // OPTIMIZE: Das geht natürlich besser, wenn man einen Adjaceny-Graph hat!
        for (unsigned long LeafNr=0; LeafNr<Leaves.Size(); LeafNr++)
        {
            if (BFS_Visited[LeafNr] || !Leaves[CurrentLeafNr].BB.GetEpsilonBox(MapT::RoundEpsilon).Intersects(Leaves[LeafNr].BB)) continue;

            for (unsigned long Portal1Nr=0; Portal1Nr<Leaves[CurrentLeafNr].Portals.Size(); Portal1Nr++)
                for (unsigned long Portal2Nr=0; Portal2Nr<Leaves[LeafNr].Portals.Size(); Portal2Nr++)
                    if (Leaves[CurrentLeafNr].Portals[Portal1Nr].Overlaps(Leaves[LeafNr].Portals[Portal2Nr], false, MapT::RoundEpsilon))
                    {
                        BFS_Visited[LeafNr]=true;           // Nachbarn 'markieren',
                        BFS_Tree   [LeafNr]=CurrentLeafNr;  // Vorgänger von -1 auf CurrentLeaf setzen
                        ToDoList.PushBack(LeafNr);          // und in ToDoList aufnehmen.

                        // Als Zugabe wollen wir noch den Eintrittspunkt festhalten
                        ArrayT< Polygon3T<double> > NewPolys;
                        Leaves[LeafNr].Portals[Portal2Nr].GetChoppedUpAlong(Leaves[CurrentLeafNr].Portals[Portal1Nr], MapT::RoundEpsilon, NewPolys);
                        // if (!NewPolys[NewPolys.Size()-1].IsValid(MapT::RoundEpsilon, MapT::MinVertexDist)) Console->Warning("Invalid NewPoly!\n");

                        VectorT Center;
                        for (unsigned long VertexNr=0; VertexNr<NewPolys[NewPolys.Size()-1].Vertices.Size(); VertexNr++)
                            Center=Center+NewPolys[NewPolys.Size()-1].Vertices[VertexNr];
                        BFS_TreePoints[LeafNr]=scale(Center, 1.0/double(NewPolys[NewPolys.Size()-1].Vertices.Size()));

                        // Es wäre nicht schlimm, wenn ein Leaf mehrfach in der ToDoListe landet, aber sinnvoll ist es auch nicht
                        Portal1Nr=Leaves[CurrentLeafNr].Portals.Size();
                        break;
                    }
        }
    }
}


void BspTreeBuilderT::LeakDetected(const VectorT& InfoPlayerStartOrigin, const std::string& PointFileName, const unsigned long LeafNr) const
{
    Console->Print("\n### LEAK DETECTED! ###\n\n");
    Console->Print("A leak is a hole in the world, where the inside of it is exposed to the\n");
    Console->Print("(unwanted) outside region. Thus, I generated a leak pointfile.\n");
    Console->Print("Load this file into your world editor, and find the beginning of the line.\n");
    Console->Print("Hint: The beginning is always near one of the \"info_player_start\" entities.\n");
    Console->Print("Then find and fix the leak by tracing the line until you reach the outside.\n");
    Console->Print("(I always take the shortest path, so this should be easy.)\n");
    Console->Print("\nNotes:\n");
    Console->Print("- Leaks can be *very* small. Use a close-up view, if necessary.\n");
    Console->Print("- Use the grid. The grid is useful to fix leaks + to avoid them from the start.\n");
    Console->Print("- Make sure that *all* \"info_player_start\" entities are inside the world!\n");
    Console->Print("- The pointfile assumes that WcMap2Ca.exe scaled the world by factor 25.4.\n");
    Console->Print("- Be aware that both the clip hull and the draw hull must be sealed.\n");
    Console->Print("- Please refer to the documentation for additional information.\n");


    // Find path from 'InfoPlayerStartOrigin' to this leaf, and write it into the pointfile.
    ComputeLeakPathByBFS(InfoPlayerStartOrigin);

    FILE* PointFile=fopen(PointFileName.c_str(), "w");
    if (!PointFile) Error("Could not open pointfile \"%s\" for writing!", PointFileName.c_str());

    // The following is also possible in a single big for-loop that does not require the 'Points' array.
    // However, it would then be much more complicated to consider all the special cases when something goes wrong (see below).
    // Thus, for clarity, I keep it simple, even at the cost of sub-optimal efficiency.
    ArrayT<VectorT> Points;

    // Let "S" be the start leaf that contains the 'InfoPlayerStartOrigin'.
    // The following loop puts all the path points into the 'Points' array, including the 'InfoPlayerStartOrigin' of leaf "S".
    // Three cases are possible, and all of them are gracefully handled (that is much harder with a big loop!):
    // a) If "S" is NOT reachable from LeafNr, something is wrong (this is a bug!).
    //    The predecessor of LeafNr is -1, and therefore only a single point (0, 0, 0) gets written.
    // b) if "S"==LeafNr, something else is wrong, but not necessarily a bug.
    //    The predecessor is -1, and a single point 'InfoPlayerStartOrigin' gets written.
    // c) In the normal case, as least two points are written into the 'Points' array.
    for (unsigned long CurrentLeaf=LeafNr; CurrentLeaf!=(unsigned long)-1; CurrentLeaf=BFS_Tree[CurrentLeaf])
        Points.PushBack(scale(BFS_TreePoints[CurrentLeaf], 1.0/25.4));

    for (unsigned long PointNr=0; PointNr+1<Points.Size(); PointNr++)
    {
        VectorT V1=Points[PointNr  ];
        VectorT d =Points[PointNr+1]-V1;
        double  l =length(d);

        // Console->Print(cf::va("%5u %5u    %f %f %f   -   %f %f %f\n", CurrentLeaf, NextLeaf, V1.x, V1.y, V1.z, V2.x, V2.y, V2.z));
        if (l<1.0) continue;
        d=scale(normalize(d, 0.0), 2.0);

        while (l>=0.0)
        {
            fprintf(PointFile, "%f %f %f\n", V1.x, V1.y, V1.z);
            V1=V1+d;
            l-=2.0;
        }
    }

    fclose(PointFile);

    if (Points.Size()<2)
    {
        Console->Print("\nSorry, I have ANOTHER WARNING:\n");
        Console->Print("I was *not* able to generate a reasonable pointfile, and I'm not sure why.\n");
        Console->Print(">> Did you really place all your \"info_player_start\" entities INSIDE the world?\n");
        Console->Print("If you did, and you still see this message, please send an email to\n");
        Console->Print("CarstenFuchs@T-Online.de\n");
        Console->Print("Tell him about this message (best is to use copy & paste, or a screen-shot),\n");
        Console->Print("and ideally, include this map as an attachment.\n");
    }
    else Console->Print("\nPointfile written to "+PointFileName+"\n");

    Error("Stopped by leak.");
}


void BspTreeBuilderT::DetectLeaks(const ArrayT<VectorT>& FloodFillOrigins, const std::string& MapFileName, MaterialT* LeakDetectMat)
{
    ArrayT<cf::SceneGraph::BspTreeNodeT::LeafT>& Leaves=BspTree->Leaves;

    // Generate the 'PointFileName' from the given 'WorldFileName' by replacing its extension.
    std::string PointFileName=MapFileName;
    size_t      i=MapFileName.rfind(".cmap");

    if (i==std::string::npos) PointFileName+=".pts";
                         else PointFileName.replace(i, 5, ".pts");


    // See if any inner leaf has one of our special bounding faces.
    for (unsigned long LeafNr=0; LeafNr<Leaves.Size(); LeafNr++)
    {
        if (!Leaves[LeafNr].IsInnerLeaf) continue;

        for (unsigned long SetNr=0; SetNr<Leaves[LeafNr].FaceChildrenSet.Size(); SetNr++)
            if (FaceChildren[Leaves[LeafNr].FaceChildrenSet[SetNr]]->Material==LeakDetectMat)
                LeakDetected(FloodFillOrigins.Size()>0 ? FloodFillOrigins[0] : Vector3dT(), PointFileName, LeafNr);
    }
}
