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

    if (Warn) Console->Warning("Outer Leaf has Faces! Use \"CaSanity\" for more extensive diagnostics!\n");     // Sollte niemals vorkommen!

    Console->Print(cf::va("Inner Leaves        : %10lu\n", NrOfInnerLeaves));
    Console->Print(cf::va("Outer Leaves        : %10lu\n", Leaves.Size()-NrOfInnerLeaves));
}
