/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/**************************/
/*** Remove Outer Stuff ***/
/**************************/


void BspTreeBuilderT::RemoveOuterFaces()
{
    ArrayT<cf::SceneGraph::BspTreeNodeT::NodeT>& Nodes =BspTree->Nodes;
    ArrayT<cf::SceneGraph::BspTreeNodeT::LeafT>& Leaves=BspTree->Leaves;

    ArrayT<bool> FaceVis;

    for (unsigned long FaceNr=0; FaceNr<FaceChildren.Size(); FaceNr++) FaceVis.PushBack(false);

    // Alle Faces aller inneren Leaves als sichtbar bzw. benutzt markieren.
    for (unsigned long LeafNr=0; LeafNr<Leaves.Size(); LeafNr++)
        if (Leaves[LeafNr].IsInnerLeaf)
            for (unsigned long FaceNr=0; FaceNr<Leaves[LeafNr].FaceChildrenSet.Size(); FaceNr++)
                FaceVis[Leaves[LeafNr].FaceChildrenSet[FaceNr]]=true;

    // Faces bereinigen. Macht den BSP-Baum (Nodes und Leaves) nutzlos!
    for (unsigned long FaceNr=0; FaceNr<FaceChildren.Size(); FaceNr++)
        if (!FaceVis[FaceNr])
        {
            delete FaceChildren[FaceNr];

            FaceChildren[FaceNr]=FaceChildren[FaceChildren.Size()-1]; FaceChildren.DeleteBack();
            FaceVis     [FaceNr]=FaceVis     [FaceVis     .Size()-1]; FaceVis     .DeleteBack();
            FaceNr--;
        }

    // Lösche den nun ohnehin unbrauchbaren BSP-Tree.
    Nodes .Clear();
    Leaves.Clear();

    Console->Print(cf::va("Faces               : %10lu\n", FaceChildren.Size()));
}


void BspTreeBuilderT::RemoveOuterPortals()
{
    ArrayT<cf::SceneGraph::BspTreeNodeT::LeafT>& Leaves=BspTree->Leaves;

    unsigned long NrOfInnerLeaves=0;
    bool          Warn           =false;

    for (unsigned long LeafNr=0; LeafNr<Leaves.Size(); LeafNr++)
        if (!Leaves[LeafNr].IsInnerLeaf)
        {
            if (Leaves[LeafNr].FaceChildrenSet.Size()) Warn=true;
            Leaves[LeafNr].Portals.Clear();
        }
        else NrOfInnerLeaves++;

    if (Warn)
    {
        // This warning is no longer useful, because it assumes that we carefully
        // (in fact, aggressively) split all faces along intersecting node planes.
        // As we today rather avoid splitting faces in many cases, it has become
        // quite normal that portions of inner faces also extend into outer leaves.
        // Console->Warning("Outer Leaf has Faces! Use \"CaSanity\" for more extensive diagnostics!\n");
    }

    Console->Print(cf::va("Inner Leaves        : %10lu\n", NrOfInnerLeaves));
    Console->Print(cf::va("Outer Leaves        : %10lu\n", Leaves.Size()-NrOfInnerLeaves));
}
