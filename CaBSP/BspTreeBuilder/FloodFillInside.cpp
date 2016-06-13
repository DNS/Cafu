/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/********************************/
/*** Flood Fill Inside (Code) ***/
/********************************/


ArrayT<bool> IsDefinitivelyOuterLeaf;


#if 0
unsigned int PrintCount = 0;

void OutPoly(const Polygon3T<double>& P)
{
    const int sigdigits = std::numeric_limits<double>::digits10;

    std::cout << std::setprecision(sigdigits);

    std::cout << "    " << P.Plane << "\n";

    for (unsigned int i = 0; i < P.Vertices.Size(); i++)
    {
        std::cout << "    " << P.Vertices[i] << "\n";
    }

    std::cout << "\n";
}
#endif


void BspTreeBuilderT::FloodFillInsideRecursive(unsigned long Leaf1Nr)
{
    ArrayT<cf::SceneGraph::BspTreeNodeT::LeafT>& Leaves=BspTree->Leaves;

    // Dieses Leaf ist ein inneres Leaf.
    Leaves[Leaf1Nr].IsInnerLeaf=true;

    // Sämtliche Nachbarn finden - ohne Adjacency Graph.
    for (unsigned long Leaf2Nr=0; Leaf2Nr<Leaves.Size(); Leaf2Nr++)
    {
        if (Leaves[Leaf2Nr].IsInnerLeaf) continue;                          // Already flooded into 'Leaf2Nr' by another path.
        if (!Leaves[Leaf1Nr].BB.GetEpsilonBox(MapT::RoundEpsilon).Intersects(Leaves[Leaf2Nr].BB)) continue; // Leaves have no chance to touch.
     // if (IsDefinitivelyOuterLeaf[Leaf2Nr]) continue;                     // Leaf was classified by other means - no further tests required.

        for (unsigned long Portal1Nr=0; Portal1Nr<Leaves[Leaf1Nr].Portals.Size(); Portal1Nr++)
            for (unsigned long Portal2Nr=0; Portal2Nr<Leaves[Leaf2Nr].Portals.Size(); Portal2Nr++)
                if (Leaves[Leaf1Nr].Portals[Portal1Nr].Overlaps(Leaves[Leaf2Nr].Portals[Portal2Nr], false, MapT::RoundEpsilon))
                {
                    if (IsDefinitivelyOuterLeaf[Leaf2Nr])
                    {
                        // TODO: More extensive diagnostics would be nice, in order to better understand the problem.
                        // For example, examine the shape and size of the involved portals, and why they are there in the first place.
                        //
                        // Possible options to visualize the related data:
                        //   - graphically in CaSanity, directly in the Cafu Engine, augment pts pointfile format for CaWE,
                        //   - structurally via graphviz (dot) graphs, create collapsible tree representation in HTML, ...
                        //
                        // I strongly suspect that we experience cases of "degenerate leaves" here, see the diagram in
                        // file `DegenLeaves.dia` for details. These in turn are nothing else but plain, valid leaks,
                        // stopped and "patched" here early by our IsDefinitivelyOuterLeaf[] check.
                        //
                        Console->Warning(cf::va("Found a way from inner leaf %lu to outer leaf %lu!\n", Leaf1Nr, Leaf2Nr));

#if 0
                        if (PrintCount < 3)
                        {
                            PrintCount++;
                            std::cout << "******************************************************************\n";

                            const Polygon3T<double>& P1 = Leaves[Leaf1Nr].Portals[Portal1Nr];
                            const Polygon3T<double>& P2 = Leaves[Leaf2Nr].Portals[Portal2Nr];

                            std::cout << "Polygon 1:\n"; OutPoly(P1);
                            std::cout << "Polygon 2:\n"; OutPoly(P2);
                        }
#endif
                        continue;
                    }

                    FloodFillInsideRecursive(Leaf2Nr);
                }
    }
}


void BspTreeBuilderT::FloodFillInside(const ArrayT<VectorT>& FloodFillOrigins, const ArrayT<VectorT>& WorldOutsidePointSamples)
{
    const ArrayT<cf::SceneGraph::BspTreeNodeT::LeafT>& Leaves=BspTree->Leaves;

    if (BspTree->Nodes.Size()==0 || BspTree->Leaves.Size()==0)
    {
        // There is no point in filling anything if the BSP tree has no faces and thus no nodes and no leaves.
        // Such BSP trees can occur with many entity classes ("point entities").
        assert(FaceChildren.Size()==0);
        assert(BspTree->Nodes.Size()==0);
        assert(BspTree->Leaves.Size()==0);
        return;
    }

    Console->Print(cf::va("\n%-50s %s\n", "*** Fill Inside ***", GetTimeSinceProgramStart()));

    // During map load time ("LoadWorld.cpp"), we gathered a collection of point samples that are guaranteed to be outside the world.
    // Now, for the purposes of fighting rounding errors, we use those point samples to determine leaves which are definitively
    // outer leaves. This helps because otherwise, after rounding errors, portals might actually lead into outer leaves,
    // turning them into inner leaves - and all sorts of problems occur.
    // In other words, the only purpose of this code is dealing with self-made leaks due to rounding errors.
    IsDefinitivelyOuterLeaf.Clear();

    for (unsigned long LeafNr=0; LeafNr<Leaves.Size(); LeafNr++) IsDefinitivelyOuterLeaf.PushBack(false);

    for (unsigned long SampleNr=0; SampleNr<WorldOutsidePointSamples.Size(); SampleNr++)
        IsDefinitivelyOuterLeaf[BspTree->WhatLeaf(WorldOutsidePointSamples[SampleNr])]=true;


    // All leaves in a newly created tree are already marked as "outer" leaves by default.
    for (unsigned long OriginNr=0; OriginNr<FloodFillOrigins.Size(); OriginNr++)
        FloodFillInsideRecursive(BspTree->WhatLeaf(FloodFillOrigins[OriginNr]));


    // Print some stats.
    unsigned long InnerLeaves=0;

    for (unsigned long LeafNr=0; LeafNr<Leaves.Size(); LeafNr++)
        if (Leaves[LeafNr].IsInnerLeaf) InnerLeaves++;

    Console->Print(cf::va("Inner Leaves        : %10lu\n", InnerLeaves));
}
