/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

// Initialisiert die PatchMeshesPVS-Matrix, für die nach Ausführung dieser Funktion folgende Eigenschaften gelten:
//
//     1) PatchMeshesPVS[i][j]==     NO_VISIBILITY   iff   PatchMeshes[i] can  NOT        see PatchMeshes[j]
//        PatchMeshesPVS[i][j]==PARTIAL_VISIBILITY   iff   PatchMeshes[i] can (PARTIALLY) see PatchMeshes[j]
//        PatchMeshesPVS[i][j]==   FULL_VISIBILITY   iff   PatchMeshes[i] can  FULLY      see PatchMeshes[j]
//
//     2) PatchMeshesPVS[i][i]==NO_VISIBILITY für planare PatchMeshes, d.h. kein planares Patch Mesh kann sich selbst sehen!
//
//     3) PatchMeshesPVS[i][j]==PatchMeshesPVS[j][i], d.h. die PatchMeshesPVS-Matrix ist symmetrisch
//
void InitializePatchMeshesPVSMatrix(const CaLightWorldT& CaLightWorld)
{
    printf("\n%-50s %s\n", "*** Initialize PatchMeshesPVS matrix ***", GetTimeSinceProgramStart());


    // 1. Allokiere neuen Speicher für die PatchMeshesPVS-Matrix.
    PatchMeshesPVS.SetSize(PatchMeshes.Size());

    printf("PatchMeshes:                                %10lu\n", PatchMeshes.Size());
    printf("Bytes allocated for PatchMeshesPVS matrix:  %10lu\n", PatchMeshesPVS.GetBytesAlloced());


    // 2. Initialisiere alle Elemente mit NO_VISIBILITY.
    for (unsigned long pm1Nr=0; pm1Nr<PatchMeshes.Size(); pm1Nr++)
        for (unsigned long pm2Nr=0; pm2Nr<PatchMeshes.Size(); pm2Nr++)
            PatchMeshesPVS.SetValue(pm1Nr, pm2Nr, NO_VISIBILITY);


    // 3. Setze nun die aus dem BspTree.PVS abgeleiteten Werte ein.
    //    Für jedes Leaf L, markiere also alle Patch Meshes aller Leaves im PVS von L als sichtbar von allen PatchMeshes von L aus.
    //    Wichtig: Dies handhabt auch diejenigen PatchMeshes richtig, die größer sind als die Abmessungen ihres Leafs!
    const cf::SceneGraph::BspTreeNodeT& BspTree=CaLightWorld.GetBspTree();

    for (unsigned long Leaf1Nr=0; Leaf1Nr<BspTree.Leaves.Size(); Leaf1Nr++)
    {
        const cf::SceneGraph::BspTreeNodeT::LeafT& L1=BspTree.Leaves[Leaf1Nr];

        // First of all, determine all the patch meshes (index numbers into the PatchMeshes array) of L1.
        ArrayT<unsigned long> PatchMeshesInLeaf1;

        for (unsigned long SetNr=0; SetNr<L1.FaceChildrenSet.Size(); SetNr++)
        {
            const ArrayT<unsigned long>& PMIndices=NodePtrToPMIndices[BspTree.FaceChildren[L1.FaceChildrenSet[SetNr]]];

            PatchMeshesInLeaf1.PushBack(PMIndices);
        }

        for (unsigned long SetNr=0; SetNr<L1.OtherChildrenSet.Size(); SetNr++)
        {
            const ArrayT<unsigned long>& PMIndices=NodePtrToPMIndices[BspTree.OtherChildren[L1.OtherChildrenSet[SetNr]]];

            PatchMeshesInLeaf1.PushBack(PMIndices);
        }


        // Wichtig: Auch Leaf1Nr liegt im (eigenen) PVS von Leaf1Nr!
        ArrayT<unsigned long> FacesVisible;
        ArrayT<unsigned long> OthersVisible;

        for (unsigned long LeafNr=0; LeafNr<BspTree.Leaves.Size(); LeafNr++)
        {
            const cf::SceneGraph::BspTreeNodeT::LeafT& L=BspTree.Leaves[LeafNr];

            if (!BspTree.IsInPVS(LeafNr, Leaf1Nr)) continue;

            // ACHTUNG: FacesVisible und OthersVisible können einen Index *mehrfach* enthalten!
            // Für die folgende Anwendung ist das allerdings nicht schlimm.
            FacesVisible.PushBack(L.FaceChildrenSet);
            OthersVisible.PushBack(L.OtherChildrenSet);
        }


        // Wir wissen jetzt, daß von Leaf Leaf1Nr aus die Faces in FacesVisible und die anderen Nodes in OthersVisible sichtbar sind.
        // Bestimme nun, welchen PatchMeshes dies entspricht, d.h. von Leaf Leaf1Nr aus sind welche PatchMeshes sichtbar?
        ArrayT<unsigned long> PatchMeshesVisibleFromLeaf1;

        for (unsigned long VisNr=0; VisNr<FacesVisible.Size(); VisNr++)
        {
            const ArrayT<unsigned long>& PMIndices=NodePtrToPMIndices[BspTree.FaceChildren[FacesVisible[VisNr]]];

            PatchMeshesVisibleFromLeaf1.PushBack(PMIndices);
        }

        for (unsigned long VisNr=0; VisNr<OthersVisible.Size(); VisNr++)
        {
            const ArrayT<unsigned long>& PMIndices=NodePtrToPMIndices[BspTree.OtherChildren[OthersVisible[VisNr]]];

            PatchMeshesVisibleFromLeaf1.PushBack(PMIndices);
        }


        // Für alle Patch Meshes in L1 (PatchMeshesInLeaf1) halte nun fest / markiere, daß man sie von all denjenigen Patch Meshes
        // aus sehen kann, die man von L1 aus sehen kann (PatchMeshesVisibleFromLeaf1), bzw. umgekehrt, daß sie alle die von L1
        // aus sichtbaren PatchMeshes sehen können (Symmetrie!).
        for (unsigned long inNr=0; inNr<PatchMeshesInLeaf1.Size(); inNr++)
        {
            for (unsigned long fromNr=0; fromNr<PatchMeshesVisibleFromLeaf1.Size(); fromNr++)
            {
                PatchMeshesPVS.SetValue(PatchMeshesInLeaf1[inNr], PatchMeshesVisibleFromLeaf1[fromNr], PARTIAL_VISIBILITY);
                PatchMeshesPVS.SetValue(PatchMeshesVisibleFromLeaf1[fromNr], PatchMeshesInLeaf1[inNr], PARTIAL_VISIBILITY);
            }
        }
    }


    // 4. Für die Statistik: Zähle die Anzahl der PARTIAL_VISIBILITY-Elemente in der PatchMeshesPVS-Matrix.
    unsigned long VisCount=0;

    for (unsigned long pm1Nr=0; pm1Nr<PatchMeshes.Size(); pm1Nr++)
        for (unsigned long pm2Nr=0; pm2Nr<PatchMeshes.Size(); pm2Nr++)
            if (PatchMeshesPVS.GetValue(pm1Nr, pm2Nr)==PARTIAL_VISIBILITY) VisCount++;

    if (VisCount==PatchMeshes.Size()*PatchMeshes.Size())
    {
        printf("\nWARNING: NON-TRIVIAL PVS INFORMATION REQUIRED FOR SENSIBLE LIGHTING!\n");
        printf("   [You may choose to ignore this warning, but lighting will take much longer.]\n\n");
    }
    printf("# matrix elements set to 'PARTIAL':          %10lu  (%7.3f%%)\n", VisCount, 100.0*double(VisCount)/double(PatchMeshes.Size()*PatchMeshes.Size()));


    // 5. Optimierung der PatchMeshesPVS-Matrix, Teil 1:
    //    Beim BspTree.PVS handelt es sich um den Leaf-Allgemeinfall. Nutze hier den Spezialfall aus,
    //    daß alles unterhalb oder in der Ebene eines planaren PatchMeshes PM nicht von PM aus sichtbar sein kann!
    //    (Das heißt insbesondere auch, daß PM sich nicht selbst sehen kann!)
    for (unsigned long pm1Nr=0; pm1Nr<PatchMeshes.Size(); pm1Nr++)
    {
        // Zuerst (wegen oben) muß jedes Patch Mesh sich noch selbst sehen können.
        if (PatchMeshesPVS.GetValue(pm1Nr, pm1Nr)==NO_VISIBILITY) printf("WARNING: PatchMeshesPVS[%lu][%lu]==NO_VISIBILITY\n", pm1Nr, pm1Nr);

        // TODO: Das hier könnte man echt verallgemeinern, mit einer PatchMeshT::IsPlanar() Methode.
        const cf::SceneGraph::FaceNodeT* FaceNode1=dynamic_cast<const cf::SceneGraph::FaceNodeT*>(PatchMeshes[pm1Nr].Node);
        if (FaceNode1==NULL) continue;

        PatchMeshesPVS.SetValue(pm1Nr, pm1Nr, NO_VISIBILITY);

        for (unsigned long pm2Nr=pm1Nr+1; pm2Nr<PatchMeshes.Size(); pm2Nr++)
            if (PatchMeshesPVS.GetValue(pm1Nr, pm2Nr)==PARTIAL_VISIBILITY || PatchMeshesPVS.GetValue(pm2Nr, pm1Nr)==PARTIAL_VISIBILITY)
            {
                if (PatchMeshesPVS.GetValue(pm1Nr, pm2Nr)!=PatchMeshesPVS.GetValue(pm2Nr, pm1Nr)) printf("WARNING: PatchMeshesPVS[i][j]!=PatchMeshesPVS[j][i]\n");

                const cf::SceneGraph::FaceNodeT* FaceNode2=dynamic_cast<const cf::SceneGraph::FaceNodeT*>(PatchMeshes[pm2Nr].Node);
                if (FaceNode2==NULL) continue;

                const Polygon3T<double>::SideT Side1=FaceNode2->Polygon.WhatSideSimple(FaceNode1->Polygon.Plane, MapT::RoundEpsilon);
                const Polygon3T<double>::SideT Side2=FaceNode1->Polygon.WhatSideSimple(FaceNode2->Polygon.Plane, MapT::RoundEpsilon);

                if (Side1!=Polygon3T<double>::Front && Side1!=Polygon3T<double>::Both  &&
                    Side2!=Polygon3T<double>::Front && Side2!=Polygon3T<double>::Both )
                {
                    PatchMeshesPVS.SetValue(pm1Nr, pm2Nr, NO_VISIBILITY);
                    PatchMeshesPVS.SetValue(pm2Nr, pm1Nr, NO_VISIBILITY);
                }
            }
    }


    // 6. Zähle die Anzahl der PARTIAL_VISIBILITY-Elemente in der PatchMeshesPVS-Matrix nach erster Optimierung.
    VisCount=0;

    for (unsigned long pm1Nr=0; pm1Nr<PatchMeshes.Size(); pm1Nr++)
        for (unsigned long pm2Nr=0; pm2Nr<PatchMeshes.Size(); pm2Nr++)
            if (PatchMeshesPVS.GetValue(pm1Nr, pm2Nr)==PARTIAL_VISIBILITY) VisCount++;

    printf("Still 'PARTIAL' after 1st optimization pass: %10lu  (%7.3f%%)\n", VisCount, 100.0*double(VisCount)/double(PatchMeshes.Size()*PatchMeshes.Size()));


    // 7. Optimierung der PatchMeshesPVS-Matrix, Teil 2:
    //    PatchMeshes ohne Lightmap (z.B. mit Sky-Materials) emitieren kein Licht und reflektieren auch keins.
    //    Deshalb können sie als "unsichtbar" aus dem PatchMeshesPVS entfernt werden!
    //    Genaugenommen sollten solche patch meshes gar nicht erst erzeugt werden!
    for (unsigned long pm1Nr=0; pm1Nr<PatchMeshes.Size(); pm1Nr++)
    {
        if (!PatchMeshes[pm1Nr].Material->UsesGeneratedLightMap())
        {
            printf("WARNING: Got a patch mesh that doesn't use a generated lightmap! Material name is \"%s\".\n", PatchMeshes[pm1Nr].Material->Name.c_str());

            for (unsigned long pm2Nr=0; pm2Nr<PatchMeshes.Size(); pm2Nr++)
            {
                PatchMeshesPVS.SetValue(pm1Nr, pm2Nr, NO_VISIBILITY);
                PatchMeshesPVS.SetValue(pm2Nr, pm1Nr, NO_VISIBILITY);
            }
        }
    }


/*  // 8. Zähle die Anzahl der PARTIAL_VISIBILITY-Elemente in der PatchMeshesPVS-Matrix nach zweiter Optimierung.
    VisCount=0;

    for (unsigned long pm1Nr=0; pm1Nr<PatchMeshes.Size(); pm1Nr++)
        for (unsigned long pm2Nr=0; pm2Nr<PatchMeshes.Size(); pm2Nr++)
            if (PatchMeshesPVS.GetValue(pm1Nr, pm2Nr)==PARTIAL_VISIBILITY) VisCount++;

    printf("Still 'PARTIAL' after 2nd optimization pass: %10lu  (%7.3f%%)\n", VisCount, 100.0*double(VisCount)/double(PatchMeshes.Size()*PatchMeshes.Size()));
    printf("    (%.2f patch meshes visible in average from each patch mesh.)\n", double(VisCount)/double(PatchMeshes.Size())); */


// THIS HAS BEEN REMOVED, BECAUSE:
// Precomputing "full" visibility is quasi impossible with leaves having arbitrary contents (bezier patches, terrains, detail faces, etc.)
// Also note that the number of mutual "full" visibility used to be less than 5% in quasi all cases anyway.
#if 0
    // 9. BIS JETZT: Nur NO_VISIBILITY- und PARTIAL_VISIBILITY-Einträge.
    //    JETZT NEU: Bestimme, welche der PARTIAL_VISIBILITY-Einträge sogar FULL_VISIBILITY-Einträge sind.
    ArrayT< BoundingBox3T<double> > FaceBBs;

    // Zuerst mal Bounding-Boxes für alle Faces erstellen.
    for (unsigned long Face1Nr=0; Face1Nr<Map.FaceChildren.Size(); Face1Nr++)
        FaceBBs.PushBack(BoundingBox3T<double>(Map.FaceChildren[Face1Nr]->Polygon.Vertices));

    for (Face1Nr=UseFullVis ? 0 : Map.FaceChildren.Size(); Face1Nr<Map.FaceChildren.Size(); Face1Nr++)
    {
        printf("%5.1f%%\r", (double)Face1Nr/Map.FaceChildren.Size()*100.0);
        fflush(stdout);

        for (unsigned long Face2Nr=Face1Nr+1; Face2Nr<Map.FaceChildren.Size(); Face2Nr++)
        {
            // Faces, die sich nicht mal teilweise sehen können, können sich erst recht nicht komplett sehen.
            if (FacePVS.GetValue(Face1Nr, Face2Nr)==NO_VISIBILITY) continue;

            // Faces, die sich "schon aus sich heraus" (T-artige Anordnung) nur teilweise sehen können, können sich nicht komplett sehen.
            if (Map.FaceChildren[Face1Nr]->Polygon.WhatSideSimple(Map.FaceChildren[Face2Nr]->Polygon.Plane, MapT::RoundEpsilon)!=Polygon3T<double>::Front) continue;
            if (Map.FaceChildren[Face2Nr]->Polygon.WhatSideSimple(Map.FaceChildren[Face1Nr]->Polygon.Plane, MapT::RoundEpsilon)!=Polygon3T<double>::Front) continue;

            // Faces, zwischen denen möglicherweise ein Terrain liegt, können sich nicht komplett sehen.
            BoundingBox3T<double> ConvexHullBB(Map.FaceChildren[Face1Nr]->Polygon.Vertices);
            ConvexHullBB.Insert(Map.FaceChildren[Face2Nr]->Polygon.Vertices);
            ConvexHullBB=ConvexHullBB.GetEpsilonBox(-MapT::RoundEpsilon);

            unsigned long TerrainNr;
            for (TerrainNr=0; TerrainNr<CaLightWorld.TerrainEntities.Size(); TerrainNr++)
                if (ConvexHullBB.Intersects(CaLightWorld.TerrainEntities[TerrainNr].BB)) break;

            if (TerrainNr<CaLightWorld.TerrainEntities.Size()) continue;

            // Bilde die ConvexHull über die Faces Face1Nr und Face2Nr,
            // wobei alle Normalenvektoren dieser ConvexHull nach *INNEN* zeigen!
            ArrayT< Plane3T<double> > ConvexHull;

            // Auf diese HullPlanes können wir nicht verzichten -- im Occluders-Array werden wir Occluder haben,
            // die von Face[Face1Nr] ODER Face[Face2Nr] aus mindestens PARTIAL sichbar sind!
            ConvexHull.PushBack(Map.FaceChildren[Face1Nr]->Polygon.Plane);
            ConvexHull.PushBack(Map.FaceChildren[Face2Nr]->Polygon.Plane);

            // Teil 1: Planes von der einen Seite aus anlegen
            for (unsigned long Vertex1Nr=0; Vertex1Nr<Map.FaceChildren[Face1Nr]->Polygon.Vertices.Size(); Vertex1Nr++)
            {
                // Beuge degenerierten und mehrfach vorkommenden HullPlanes vor
                if (fabs(Map.FaceChildren[Face2Nr]->Polygon.Plane.GetDistance(Map.FaceChildren[Face1Nr]->Polygon.Vertices[Vertex1Nr]))<0.1) continue;

                for (unsigned long Vertex2Nr=0; Vertex2Nr<Map.FaceChildren[Face2Nr]->Polygon.Vertices.Size(); Vertex2Nr++)
                {
                    // Dieser try/catch-Block sollte trotz obigem Test nicht entfernt werden, denn es gibt keine Garantieen!
                    try
                    {
                        Plane3T<double> HullPlane(Map.FaceChildren[Face2Nr]->Polygon.Vertices[Vertex2Nr], Map.FaceChildren[Face1Nr]->Polygon.Vertices[Vertex1Nr],
                                                  Map.FaceChildren[Face2Nr]->Polygon.Vertices[Vertex2Nr+1<Map.FaceChildren[Face2Nr]->Polygon.Vertices.Size() ? Vertex2Nr+1 : 0], MapT::RoundEpsilon);

                        if (Map.FaceChildren[Face1Nr]->Polygon.WhatSideSimple(HullPlane, MapT::RoundEpsilon)!=Polygon3T<double>::Front) continue;
                        if (Map.FaceChildren[Face2Nr]->Polygon.WhatSideSimple(HullPlane, MapT::RoundEpsilon)!=Polygon3T<double>::Front) continue;

                        ConvexHull.PushBack(HullPlane);
                    }
                    catch (const DivisionByZeroE&) {}
                }
            }

            // Teil 2: Planes von der anderen Seite aus anlegen (Notwendig! Betrachte gegenüberstehendes Dreieck und Quadrat!)
            for (unsigned long Vertex2Nr=0; Vertex2Nr<Map.FaceChildren[Face2Nr]->Polygon.Vertices.Size(); Vertex2Nr++)
            {
                // Beuge degenerierten und mehrfach vorkommenden HullPlanes vor
                if (fabs(Map.FaceChildren[Face1Nr]->Polygon.Plane.GetDistance(Map.FaceChildren[Face2Nr]->Polygon.Vertices[Vertex2Nr]))<0.1) continue;

                for (unsigned long Vertex1Nr=0; Vertex1Nr<Map.FaceChildren[Face1Nr]->Polygon.Vertices.Size(); Vertex1Nr++)
                {
                    // Dieser try/catch-Block sollte trotz obigem Test nicht entfernt werden, denn es gibt keine Garantieen!
                    try
                    {
                        Plane3T<double> HullPlane(Map.FaceChildren[Face1Nr]->Polygon.Vertices[Vertex1Nr], Map.FaceChildren[Face2Nr]->Polygon.Vertices[Vertex2Nr],
                                                  Map.FaceChildren[Face1Nr]->Polygon.Vertices[Vertex1Nr+1<Map.FaceChildren[Face1Nr]->Polygon.Vertices.Size() ? Vertex1Nr+1 : 0], MapT::RoundEpsilon);

                        if (Map.FaceChildren[Face2Nr]->Polygon.WhatSideSimple(HullPlane, MapT::RoundEpsilon)!=Polygon3T<double>::Front) continue;
                        if (Map.FaceChildren[Face1Nr]->Polygon.WhatSideSimple(HullPlane, MapT::RoundEpsilon)!=Polygon3T<double>::Front) continue;

                        ConvexHull.PushBack(HullPlane);
                    }
                    catch (const DivisionByZeroE&) {}
                }
            }

            // Bilde ein Array der in Frage kommenden Occluder.
            ArrayT< Polygon3T<double> > Occluders;

            for (unsigned long FaceNr=0; FaceNr<Map.FaceChildren.Size(); FaceNr++)
            {
                if (!ConvexHullBB.Intersects(FaceBBs[FaceNr])) continue;

                // TODO: if (Face[FaceNr].Alpha<255 || Face[FaceNr].RenderMode==Masked) continue;

                if ((FacePVS.GetValue(Face1Nr, FaceNr)!=NO_VISIBILITY) ||
                    (FacePVS.GetValue(Face2Nr, FaceNr)!=NO_VISIBILITY)) Occluders.PushBack(Map.FaceChildren[FaceNr]->Polygon);
            }

            // Clippe die Occluder gegen die ConvexHull.
            for (unsigned long HullPlaneNr=0; HullPlaneNr<ConvexHull.Size(); HullPlaneNr++)
                for (unsigned long OccluderNr=0; OccluderNr<Occluders.Size(); OccluderNr++)
                    switch (Occluders[OccluderNr].WhatSideSimple(ConvexHull[HullPlaneNr], MapT::RoundEpsilon))
                    {
                        case Polygon3T<double>::Both:
                            // Occluder splitten und Front-Teil behalten.
                            Occluders[OccluderNr]=Occluders[OccluderNr].GetSplits(ConvexHull[HullPlaneNr], MapT::RoundEpsilon)[0];
                            break;

                        case Polygon3T<double>::Front:
                            // Occluder bleibt unverändert in der Liste.
                            break;

                        default:
                            // Diesen Occluder aus der Liste löschen.
                            Occluders[OccluderNr]=Occluders[Occluders.Size()-1];
                            Occluders.DeleteBack();
                            OccluderNr--;
                            break;
                    }

            // Es sind noch Occluder in der ConvexHull, keine gegenseitige komplette Sichtbarkeit.
            if (Occluders.Size()) continue;

            // Die Faces Face1Nr und Face2Nr können sich gegenseitig komplett sehen!
            FacePVS.SetValue(Face1Nr, Face2Nr, FULL_VISIBILITY);
            FacePVS.SetValue(Face2Nr, Face1Nr, FULL_VISIBILITY);
        }
    }


    // 10. Zähle die Anzahl der FULL_VISIBILITY-Elemente in der FacePVS-Matrix.
    unsigned long PartialVisCount=VisCount;
    VisCount=0;

    for (unsigned long Face1Nr=0; Face1Nr<Map.FaceChildren.Size(); Face1Nr++)
        for (unsigned long Face2Nr=0; Face2Nr<Map.FaceChildren.Size(); Face2Nr++)
            if (FacePVS.GetValue(Face1Nr, Face2Nr)==FULL_VISIBILITY) VisCount++;

    printf("Number of 'FULL' visibility entries:         %10lu  (%7.3f%%)\n", VisCount, 100.0*double(VisCount)/double(Map.FaceChildren.Size()*Map.FaceChildren.Size()));
    printf("    (These are %.2f%% of the 'PARTIAL' entries!)\n", 100.0*double(VisCount)/double(PartialVisCount));
#endif
}
