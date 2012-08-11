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

/*****************************/
/*** Build BSP Tree (Code) ***/
/*****************************/


// Für g++ darf diese Struktur nicht lokal in 'ChooseSplitPlane()' definiert sein,
// da sie ansonsten nicht als Template-Argument verwendet werden kann.
struct PlaneInfoT
{
    Plane3T<double> Plane;
    double          TotalArea;
    int             Balance;
    double          NrOfSplits;      // unsigned long
    double          AxisAligned;     // bool

    PlaneInfoT() : TotalArea(0), Balance(0), NrOfSplits(0), AxisAligned(0) { }
};


// Subroutine zu BuildBSPTree. Sucht unter den Faces des FaceSet die beste Splitplane aus.
Plane3T<double> BspTreeBuilderT::ChooseSplitPlane(const ArrayT<unsigned long>& FaceSet) const
{
    // Im FaceSet können mehrere Faces in einer Plane liegen. Daher zuerst ein PlaneSet bilden!
    ArrayT<PlaneInfoT> PlaneSet;

    for (unsigned long FaceNr=0; FaceNr<FaceSet.Size(); FaceNr++)
    {
        unsigned long PlaneNr;

        for (PlaneNr=0; PlaneNr<PlaneSet.Size(); PlaneNr++)
        {
            Polygon3T<double>::SideT Side=FaceChildren[FaceSet[FaceNr]]->Polygon.WhatSide(PlaneSet[PlaneNr].Plane, MapT::RoundEpsilon);

            if (Side==Polygon3T<double>::InIdentical || Side==Polygon3T<double>::InMirrored) break;
        }

        if (PlaneNr<PlaneSet.Size()) continue;

        PlaneSet.PushBackEmpty();
        PlaneSet[PlaneSet.Size()-1].Plane=FaceChildren[FaceSet[FaceNr]]->Polygon.Plane;
    }


    double MaxTotalArea =1; // Wenn man hier überall 0 einträgt, könnte es unten zu einer Division
    int    WorstBalance =1; // durch Null kommen. Das wird durch diese Manipulation vermieden, und
    double MaxNrOfSplits=1; // am Endergebnis ändert sich dadurch ja nichts!

    for (unsigned long PlaneNr=0; PlaneNr<PlaneSet.Size(); PlaneNr++)
    {
        for (unsigned long FaceNr=0; FaceNr<FaceSet.Size(); FaceNr++)
            switch (FaceChildren[FaceSet[FaceNr]]->Polygon.WhatSide(PlaneSet[PlaneNr].Plane, MapT::RoundEpsilon))
            {
                case Polygon3T<double>::Back:
                case Polygon3T<double>::BackAndOn:
                    PlaneSet[PlaneNr].Balance--;
                    break;

                case Polygon3T<double>::Front:
                case Polygon3T<double>::FrontAndOn:
                    PlaneSet[PlaneNr].Balance++;
                    break;

                case Polygon3T<double>::Both:
                case Polygon3T<double>::BothAndOn:
                    PlaneSet[PlaneNr].NrOfSplits++;
                    break;

                default:
                    PlaneSet[PlaneNr].TotalArea+=FaceChildren[FaceSet[FaceNr]]->Polygon.GetArea();
                    break;
            }

        PlaneSet[PlaneNr].Balance=abs(PlaneSet[PlaneNr].Balance);

        if (fabs(PlaneSet[PlaneNr].Plane.Normal.x)==1 ||
            fabs(PlaneSet[PlaneNr].Plane.Normal.y)==1 ||
            fabs(PlaneSet[PlaneNr].Plane.Normal.z)==1) PlaneSet[PlaneNr].AxisAligned=1;

        if (PlaneSet[PlaneNr].TotalArea >MaxTotalArea ) MaxTotalArea =PlaneSet[PlaneNr].TotalArea;
        if (PlaneSet[PlaneNr].Balance   >WorstBalance ) WorstBalance =PlaneSet[PlaneNr].Balance;
        if (PlaneSet[PlaneNr].NrOfSplits>MaxNrOfSplits) MaxNrOfSplits=PlaneSet[PlaneNr].NrOfSplits;
    }


    unsigned long BestNr   =0;
    double        BestScore=0;

    for (unsigned long PlaneNr=0; PlaneNr<PlaneSet.Size(); PlaneNr++)
    {
        double Score=  4*   PlaneSet[PlaneNr].TotalArea /MaxTotalArea
                     + 2*(1-PlaneSet[PlaneNr].Balance   /WorstBalance)
                     +20*(1-PlaneSet[PlaneNr].NrOfSplits/MaxNrOfSplits)
                     + 1*   PlaneSet[PlaneNr].AxisAligned;

        if (Score>BestScore)
        {
            BestNr   =PlaneNr;
            BestScore=Score;
        }
    }

    return PlaneSet[BestNr].Plane;
}


// Subroutine zu ProcessTree. Dies ist die eigentliche, rekursive und binäre Baumbildung, die die Faces des
// FaceSet in einem BSP-Baum anordnet. Dazu wird unter diesen eine Splitplane ausgesucht, bzgl. der die anderen Faces ins
// Front-, Back- bzw. NodeT::FaceSet eingeordnet werden. Rekursiv werden so auch die Bäume des Front- und BackFS bestimmt.
void BspTreeBuilderT::BuildBSPTreeRecursive(const ArrayT<unsigned long>& FaceSet)
{
    ArrayT<cf::SceneGraph::BspTreeNodeT::NodeT>& Nodes =BspTree->Nodes;
    ArrayT<cf::SceneGraph::BspTreeNodeT::LeafT>& Leaves=BspTree->Leaves;

    static unsigned long ProgressCounter;
    if (FaceSet.Size()==FaceChildren.Size()) ProgressCounter=0;

    Console->Print(cf::va("%5.1f%%\r", (double)ProgressCounter/FaceChildren.Size()*100.0));
    // fflush(stdout);      // The stdout console auto-flushes the output.

    Nodes.PushBackEmpty();
    const unsigned long NodeNr=Nodes.Size()-1;
    Nodes[NodeNr].Plane=ChooseSplitPlane(FaceSet);

    // Die Faces des FaceSet bzgl. der Nodes[NodeNr].Plane entsprechend zuordnen
    ArrayT<unsigned long> FrontFS;
    ArrayT<unsigned long> BackFS;

    for (unsigned long FaceNr=0; FaceNr<FaceSet.Size(); FaceNr++)
        switch (FaceChildren[FaceSet[FaceNr]]->Polygon.WhatSide(Nodes[NodeNr].Plane, MapT::RoundEpsilon))
        {
            case Polygon3T<double>::Front:
            case Polygon3T<double>::FrontAndOn:
                FrontFS.PushBack(FaceSet[FaceNr]);
                break;

            case Polygon3T<double>::Back:
            case Polygon3T<double>::BackAndOn:
                BackFS .PushBack(FaceSet[FaceNr]);
                break;

            case Polygon3T<double>::Both:
            case Polygon3T<double>::BothAndOn:
            {
                // Warning: Adding FaceSet[FaceNr] to both the FrontFS *and* the BackFS (that is, *without* duplicating the face in Faces),
                // will cause that face to be referenced in the NodeT::FaceSet of *two* distinct nodes! That means that the sum of all
                // NodeT::FaceSet sizes will exceed Faces.Size().
                // As of 2005-11-17, the Cafu engine can handle this situation gracefully (see MapT::GetFacesOrderedBackToFront() or svn
                // revision 140), but it still renders all affected faces *twice*!!
                if (Option_MinimizeFaceSplits)
                {
                    // Warning: Assigning the same FaceSet number to both the FrontFS *and* the BackFS is only viable
                    // because with Option_AvoidSplittingFaces we are guaranteed that the Faces array never changes
                    // while we recurse within this method!
                    FrontFS.PushBack(FaceSet[FaceNr]);
                    BackFS .PushBack(FaceSet[FaceNr]);
                }
                else
                {
                    ArrayT< Polygon3T<double> > SplitResult=FaceChildren[FaceSet[FaceNr]]->Polygon.GetSplits(Nodes[NodeNr].Plane, MapT::RoundEpsilon);

                    if (SplitResult[0].IsValid(MapT::RoundEpsilon, MapT::MinVertexDist) &&
                        SplitResult[1].IsValid(MapT::RoundEpsilon, MapT::MinVertexDist))     // Prüfe insb. auf zusammenfallende Vertices.
                    {
                        // Keine zusammenfallenden Vertices -- alles OK.
                        FaceChildren.PushBack(new cf::SceneGraph::FaceNodeT(*FaceChildren[FaceSet[FaceNr]]));

                        FaceChildren[FaceSet[FaceNr]      ]->Polygon=SplitResult[0];
                        FaceChildren[FaceChildren.Size()-1]->Polygon=SplitResult[1];
                    }
                    else
                    {
                        // Wir haben den problematischen Fall des spitzen Keils gefunden. Was tun?
                        // Es scheint am sinnvollsten zu sein, die Face zu verdoppeln und sie dann sowohl dem FrontFS als auch dem BackFS
                        // zuzuordnen. Die negativen Konsequenzen aller anderen Alternativen sind nicht überschaubar.
                        // Divergenz dürfte trotz der Verdopplung nicht eintreten können (gültige Faces vorausgesetzt)!
                        //    Haken: Die Face wird in der Engine idR zweimal gerendert werden, weil wir hier eine echte Kopie machen,
                        // und Kopie und Original danach nichts mehr voneinander wissen.
                        // Sowohl FrontFS also auch BackFS die FaceSet[FaceNr] Nummer zuzuordnen geht natürlich auch nicht,
                        // weil später in der Rekursion ja ein weiterer Split dieser Face entstehen könnte, z.B. auf der
                        // Vorderseite der Nodes[NodeNr].Plane, wodurch die weiteren Berechnungen auf der Rückseite eine
                        // "faule" Face vorfinden.
                        //    Lösung 1: Splitte niemals, genau wie Doom3. Das Faces Array bleibt dadurch konstant, und wir können doch
                        // sowohl dem FrontFS also auch BackFS die FaceSet[FaceNr] Nummer zuzuordnen. Ausprobieren!
                        //    Lösung 2: Wähle ein FS, und ordene die Face dort zu. Wenn z.B. die FrontFace valid und die BackFace
                        // invalid ist, ordne die Face dem FrontFS zu, und umgekehrt. Sind beide ungültig, wähle z.B. gemäß
                        // dem größeren Flächeninhalt, oder einfach beliebig.
                        FaceChildren.PushBack(new cf::SceneGraph::FaceNodeT(*FaceChildren[FaceSet[FaceNr]]));
                    }

                    FrontFS.PushBack(FaceSet[FaceNr]);
                    BackFS .PushBack(FaceChildren.Size()-1);
                }
                break;
            }

            default:
                // Obsolete: Nodes[NodeNr].FaceSet.PushBack(FaceSet[FaceNr]);
                ProgressCounter++;
                break;
        }


    // Make sure that we made progress.
    // if (Nodes[NodeNr].FaceSet==0 && (FrontFS.Size()==0 || BackFS.Size()==0)) { PickAnotherSplitPlane(); Repeat the above loop; }


    if (FrontFS.Size())
    {
        Nodes[NodeNr].FrontChild=Nodes.Size();
        Nodes[NodeNr].FrontIsLeaf=false;
        BuildBSPTreeRecursive(FrontFS);
    }
    else
    {
        Leaves.PushBackEmpty();
        Nodes[NodeNr].FrontChild=Leaves.Size()-1;
        Nodes[NodeNr].FrontIsLeaf=true;
    }

    if (BackFS.Size())
    {
        Nodes[NodeNr].BackChild=Nodes.Size();
        Nodes[NodeNr].BackIsLeaf=false;
        BuildBSPTreeRecursive(BackFS);
    }
    else
    {
        Leaves.PushBackEmpty();
        Nodes[NodeNr].BackChild=Leaves.Size()-1;
        Nodes[NodeNr].BackIsLeaf=true;
    }
}


// Nach dem Erstellen des Baumes sind die Leaves zunächst noch leer. Jetzt werden ihre FaceSet- und BB-Einträge ausgefüllt.
// Diese Funktion, genau wie Portalize, hätte auch direkt in BuildBSPTree integriert werden können.
// Der Klarheit halber habe ich das aber nicht gemacht.
// Das FaceChildrenSet eines Leafs besteht aus allen Faces, die beim Ablaufen des Baums zumindest teilweise auf
// der gewählten Seite eines jeden Nodes des dazugehörigen Node-Pfads liegen (die "Wände" des Leafs). Dabei können die Faces
// auch größer sein als das Leaf an sich! Die Vorgehensweise entspricht der von BuildBSPTree: Der neue Baum wird noch einmal
// abgelaufen. Dabei wird wiederum ein FaceSet mitgeführt. Diesmal werden jedoch Faces, die auf einem Node liegen, nicht mehr
// entfernt, sondern dem Front- bzw. BackFS zugeordnet! Faces, die von einem Node gesplittet werden, werden mit den ent-
// sprechenden Teilen(!) dem Front- bzw. BackFS zugeordnet (daher auch die Notwendigkeit des Face2-Arrays).
// Fehlzuordnungen wie früher wegen zu großen Poly-Teilen gibt es damit nicht mehr.
void BspTreeBuilderT::FillBSPLeaves(unsigned long NodeNr, const ArrayT<cf::SceneGraph::FaceNodeT*>& Face2, const ArrayT<unsigned long>& FaceSet, const BoundingBox3T<double>& BB)
{
    ArrayT<cf::SceneGraph::BspTreeNodeT::NodeT>& Nodes =BspTree->Nodes;
    ArrayT<cf::SceneGraph::BspTreeNodeT::LeafT>& Leaves=BspTree->Leaves;

    ArrayT<cf::SceneGraph::FaceNodeT*> FrontFaces, BackFaces, DeleteList;
    ArrayT<unsigned long>              FrontFS, BackFS;

    for (unsigned long FaceNr=0; FaceNr<Face2.Size(); FaceNr++)
        switch (Face2[FaceNr]->Polygon.WhatSide(Nodes[NodeNr].Plane, MapT::RoundEpsilon))
        {
            case Polygon3T<double>::Front:
            case Polygon3T<double>::FrontAndOn:
            case Polygon3T<double>::InIdentical:
                FrontFaces.PushBack(Face2[FaceNr]);
                FrontFS.PushBack(FaceSet[FaceNr]);
                break;

            case Polygon3T<double>::Back:
            case Polygon3T<double>::BackAndOn:
            case Polygon3T<double>::InMirrored:
                BackFaces.PushBack(Face2[FaceNr]);
                BackFS.PushBack(FaceSet[FaceNr]);
                break;

            case Polygon3T<double>::Both:
            case Polygon3T<double>::BothAndOn:
            {
                // Das Problem des spitzen Keils braucht hier nicht wirklich berücksichtigt zu werden, da FrontFace und
                // BackFace innerhalb dieser Funktion verbleiben und es keine 'gefährlichen' Operationen hierin gibt!
                ArrayT< Polygon3T<double> > SplitResult=Face2[FaceNr]->Polygon.GetSplits(Nodes[NodeNr].Plane, MapT::RoundEpsilon);

                // TexInfos usw. übernehmen.
                cf::SceneGraph::FaceNodeT* FrontFace=new cf::SceneGraph::FaceNodeT(*Face2[FaceNr]); FrontFace->Polygon=SplitResult[0];
                cf::SceneGraph::FaceNodeT* BackFace =new cf::SceneGraph::FaceNodeT(*Face2[FaceNr]); BackFace ->Polygon=SplitResult[1];

                DeleteList.PushBack(FrontFace); FrontFaces.PushBack(FrontFace); FrontFS.PushBack(FaceSet[FaceNr]);
                DeleteList.PushBack(BackFace ); BackFaces .PushBack(BackFace ); BackFS .PushBack(FaceSet[FaceNr]);
                break;
            }

            default:
                break;
        }


    // Es ist nicht möglich, eine BB aus der FrontFL bzw. BackFL zu bilden,
    // da ein Leaf nicht überall von Faces begrenzt sein muß!
    const double                    SplitEps   =0.0;    // Must use 0 here, not MapT::RoundEpsilon, or very small leaves near the split plane don't get a proper bounding-box.
    ArrayT< BoundingBox3T<double> > SplitResult=BB.GetSplits(Nodes[NodeNr].Plane, SplitEps);

#ifdef DEBUG
    // Make them easier to inspect in the debugger.
 // const cf::SceneGraph::BspTreeNodeT::NodeT& SNd=Nodes[NodeNr];
    const BoundingBox3dT&                      SR0=SplitResult[0];
    const BoundingBox3dT&                      SR1=SplitResult[1];

    assert(SR0.IsInited());
    assert(SR1.IsInited());
#endif

    if (Nodes[NodeNr].FrontIsLeaf)
    {
        Leaves[Nodes[NodeNr].FrontChild].FaceChildrenSet=FrontFS;
        Leaves[Nodes[NodeNr].FrontChild].BB             =SplitResult[0];
        Leaves[Nodes[NodeNr].FrontChild].IsInnerLeaf    =false;
    }
    else FillBSPLeaves(Nodes[NodeNr].FrontChild, FrontFaces, FrontFS, SplitResult[0]);

    if (Nodes[NodeNr].BackIsLeaf)
    {
        Leaves[Nodes[NodeNr].BackChild].FaceChildrenSet=BackFS;
        Leaves[Nodes[NodeNr].BackChild].BB             =SplitResult[1];
        Leaves[Nodes[NodeNr].BackChild].IsInnerLeaf    =false;
    }
    else FillBSPLeaves(Nodes[NodeNr].BackChild, BackFaces, BackFS, SplitResult[1]);


    for (unsigned long FaceNr=0; FaceNr<DeleteList.Size(); FaceNr++)
        delete DeleteList[FaceNr];
}


void BspTreeBuilderT::BuildBSPTree()
{
    ArrayT<cf::SceneGraph::BspTreeNodeT::NodeT>& Nodes =BspTree->Nodes;
    ArrayT<cf::SceneGraph::BspTreeNodeT::LeafT>& Leaves=BspTree->Leaves;

    Console->Print(cf::va("\n%-50s %s\n", "*** Binary Space Partitioning ***", GetTimeSinceProgramStart()));

    Nodes.Clear();
    Leaves.Clear();

    // If there are no faces at all (as can happen with many entity classes ("point entities")), the BSP tree has zero nodes.
    if (FaceChildren.Size()==0) return;

    // Start with all face numbers in a big list.
    ArrayT<unsigned long> AllFaces;

    for (unsigned long FaceNr=0; FaceNr<FaceChildren.Size(); FaceNr++) AllFaces.PushBack(FaceNr);


    // PHASE I
    BuildBSPTreeRecursive(AllFaces);


    // Complete the list, in order to account for face splits during tree creation.
    for (unsigned long FaceNr=AllFaces.Size(); FaceNr<FaceChildren.Size(); FaceNr++) AllFaces.PushBack(FaceNr);

    // Create a bounding box that contains the entire world (all 'Faces').
    BoundingBox3T<double> WorldBB(FaceChildren[0]->Polygon.Vertices);

    for (unsigned long FaceNr=1; FaceNr<FaceChildren.Size(); FaceNr++)
        WorldBB.Insert(FaceChildren[FaceNr]->Polygon.Vertices);

    // It is important to also consider the other children, as is important for non-worldspawn entities.
    for (unsigned long ChildNr=0; ChildNr<OtherChildren.Size(); ChildNr++)
    {
        const BoundingBox3T<double>& ChildBB=OtherChildren[ChildNr]->GetBoundingBox();

        WorldBB.Insert(ChildBB.Min);
        WorldBB.Insert(ChildBB.Max);
    }

    const double d=10.0*MapT::MinVertexDist;

    WorldBB.Min-=VectorT(d, d, d);  // Kleiner Sicherheitszuschlag, damit auch die äußersten
    WorldBB.Max+=VectorT(d, d, d);  // Leaves wirklich eine korrekte BB bekommen!


    // PHASE II
    FillBSPLeaves(0, FaceChildren, AllFaces, WorldBB);


    Console->Print(cf::va("Nodes  created      : %10lu\n", Nodes .Size()));
    Console->Print(cf::va("Leaves created      : %10lu\n", Leaves.Size()));
    Console->Print(cf::va("Faces               : %10lu\n", FaceChildren.Size()));
}
