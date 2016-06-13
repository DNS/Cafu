/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

// Initialisiert die FacePVS-Matrix, für die nach Ausführung dieser Funktion folgende Eigenschaften gelten:
//
//     1) FacePVS[i][j]==     NO_VISIBILITY   iff   Face[i] can  NOT        see Face[j]
//        FacePVS[i][j]==PARTIAL_VISIBILITY   iff   Face[i] can (PARTIALLY) see Face[j]
//        FacePVS[i][j]==   FULL_VISIBILITY   iff   Face[i] can  FULLY      see Face[j]
//
//     2) FacePVS[i][i]==NO_VISIBILITY für alle i (NO_VISIBILITY auf der Diagonalen), d.h. keine Face kann sich selbst sehen!
//
//     3) FacePVS[i][j]==FacePVS[j][i], d.h. die FacePVS-Matrix ist symmetrisch
//
void InitializeFacePVSMatrix(const CaSHLWorldT& CaSHLWorld, const bool UseFullVis)
{
    const cf::SceneGraph::BspTreeNodeT& Map=CaSHLWorld.GetBspTree();

    printf("\n%-50s %s\n", "*** Initialize FacePVS matrix ***", GetTimeSinceProgramStart());


    // 1. Allokiere neuen Speicher für die FacePVS-Matrix
    FacePVS.Clear();
    FacePVS.PushBackEmpty(Map.FaceChildren.Size());

    for (unsigned long Face1Nr=0; Face1Nr<Map.FaceChildren.Size(); Face1Nr++) FacePVS[Face1Nr].PushBackEmpty(Map.FaceChildren.Size());

    printf("Faces (n):                                   %10lu\n", Map.FaceChildren.Size());
    printf("Bytes allocated for FacePVS matrix (n^2):    %10lu\n", Map.FaceChildren.Size()*Map.FaceChildren.Size());


    // 2. Initialisiere alle Elemente mit NO_VISIBILITY
    for (unsigned long Face1Nr=0; Face1Nr<Map.FaceChildren.Size(); Face1Nr++)
        for (unsigned long Face2Nr=0; Face2Nr<Map.FaceChildren.Size(); Face2Nr++)
            FacePVS[Face1Nr][Face2Nr]=NO_VISIBILITY;


    // 3. Setze nun die aus dem Map.PVS abgeleiteten Werte ein.
    //    Für alle Leaves L, markiere also alle Faces aller Leaves im PVS von L als sichtbar von allen Faces von L aus.
    //    Wichtig: Dies handhabt auch diejenigen Faces richtig, die größer sind als ein Leaf, also mehrere Leaves begrenzen!
    ArrayT<bool> FaceSetBool;
    FaceSetBool.PushBackEmpty(Map.FaceChildren.Size());

    for (unsigned long Leaf1Nr=0; Leaf1Nr<Map.Leaves.Size(); Leaf1Nr++)
    {
        for (unsigned long Face1Nr=0; Face1Nr<Map.FaceChildren.Size(); Face1Nr++) FaceSetBool[Face1Nr]=false;

        // Setze für alle Faces F aller Leaves im PVS von Leaf1Nr FaceSetBool[F] auf true.
        // Wichtig: Auch Leaf1Nr liegt im (eigenen) PVS von Leaf1Nr!
        for (unsigned long Leaf2Nr=0; Leaf2Nr<Map.Leaves.Size(); Leaf2Nr++)
        {
            unsigned long PVSTotalBitNr=Leaf1Nr*Map.Leaves.Size()+Leaf2Nr;
            unsigned long PVS_W32_Nr   =PVSTotalBitNr >> 5;

            if ((Map.PVS[PVS_W32_Nr] >> (PVSTotalBitNr & 31)) & 1)
                for (unsigned long Face2Nr=0; Face2Nr<Map.Leaves[Leaf2Nr].FaceChildrenSet.Size(); Face2Nr++)
                    FaceSetBool[Map.Leaves[Leaf2Nr].FaceChildrenSet[Face2Nr]]=true;
        }

        ArrayT<unsigned long> FaceSetList;
        for (unsigned long Face1Nr=0; Face1Nr<Map.FaceChildren.Size(); Face1Nr++) if (FaceSetBool[Face1Nr]) FaceSetList.PushBack(Face1Nr);

        // Markiere für alle Faces F im Leaf1Nr, daß alle Faces F' aller Leaves im PVS von Leaf1Nr von F aus sichtbar sind,
        // aber auch die umgekehrte Sichtbarkeit, also daß F von F' aus sichtbar ist, um Symmetrie zu garantieren.
        for (unsigned long Face1Nr=0; Face1Nr<Map.Leaves[Leaf1Nr].FaceChildrenSet.Size(); Face1Nr++)
            for (unsigned long Face2Nr=0; Face2Nr<FaceSetList.Size(); Face2Nr++)
            {
                FacePVS[Map.Leaves[Leaf1Nr].FaceChildrenSet[Face1Nr]][FaceSetList[Face2Nr]]=PARTIAL_VISIBILITY;
                FacePVS[FaceSetList[Face2Nr]][Map.Leaves[Leaf1Nr].FaceChildrenSet[Face1Nr]]=PARTIAL_VISIBILITY;
            }
    }


    // 4. Für die Statistik: Zähle die Anzahl der PARTIAL_VISIBILITY-Elemente in der FacePVS-Matrix
    unsigned long VisCount=0;

    for (unsigned long Face1Nr=0; Face1Nr<Map.FaceChildren.Size(); Face1Nr++)
        for (unsigned long Face2Nr=0; Face2Nr<Map.FaceChildren.Size(); Face2Nr++)
            if (FacePVS[Face1Nr][Face2Nr]==PARTIAL_VISIBILITY) VisCount++;

    if (VisCount==Map.FaceChildren.Size()*Map.FaceChildren.Size())
    {
        printf("\nWARNING: NON-TRIVIAL PVS INFORMATION REQUIRED FOR SENSIBLE LIGHTING!\n");
        printf("   [You may choose to ignore this warning, but lighting will take much longer.]\n\n");
    }
    printf("# matrix elements set to 'PARTIAL':          %10lu  (%7.3f%%)\n", VisCount, 100.0*double(VisCount)/double(Map.FaceChildren.Size()*Map.FaceChildren.Size()));


    // 5. Optimierung der FacePVS-Matrix, Teil 1:
    //    Beim Map.PVS handelt es sich um den Leaf-Allgemeinfall. Nutze hier den Spezialfall aus, daß wir nur die Sichtbarkeit
    //    von einer *Face* aus bestimmen wollen und die Tatsache, daß alles unterhalb oder in der Ebene einer Face F nicht von
    //    F aus sichtbar sein kann! (Das heißt insbesondere auch, daß F sich nicht selbst sehen kann!)
    for (unsigned long Face1Nr=0; Face1Nr<Map.FaceChildren.Size(); Face1Nr++)
    {
        // Zuerst (wegen oben) muß eine Face sich noch selbst sehen können
        if (FacePVS[Face1Nr][Face1Nr]==NO_VISIBILITY) printf("WARNING: FacePVS[i][i]==NO_VISIBILITY\n");
        FacePVS[Face1Nr][Face1Nr]=NO_VISIBILITY;

        for (unsigned long Face2Nr=Face1Nr+1; Face2Nr<Map.FaceChildren.Size(); Face2Nr++)
            if (FacePVS[Face1Nr][Face2Nr]==PARTIAL_VISIBILITY || FacePVS[Face2Nr][Face1Nr]==PARTIAL_VISIBILITY)
            {
                Polygon3T<double>::SideT Side1=Map.FaceChildren[Face2Nr]->Polygon.WhatSideSimple(Map.FaceChildren[Face1Nr]->Polygon.Plane, MapT::RoundEpsilon);
                Polygon3T<double>::SideT Side2=Map.FaceChildren[Face1Nr]->Polygon.WhatSideSimple(Map.FaceChildren[Face2Nr]->Polygon.Plane, MapT::RoundEpsilon);

                if (FacePVS[Face1Nr][Face2Nr]!=FacePVS[Face2Nr][Face1Nr]) printf("WARNING: FacePVS[i][j]!=FacePVS[j][i]\n");

                if (Side1!=Polygon3T<double>::Front && Side1!=Polygon3T<double>::Both &&
                    Side2!=Polygon3T<double>::Front && Side2!=Polygon3T<double>::Both)
                {
                    FacePVS[Face1Nr][Face2Nr]=NO_VISIBILITY;
                    FacePVS[Face2Nr][Face1Nr]=NO_VISIBILITY;
                }
            }
    }


    // 6. Zähle die Anzahl der PARTIAL_VISIBILITY-Elemente in der FacePVS-Matrix nach erster Optimierung
    VisCount=0;

    for (unsigned long Face1Nr=0; Face1Nr<Map.FaceChildren.Size(); Face1Nr++)
        for (unsigned long Face2Nr=0; Face2Nr<Map.FaceChildren.Size(); Face2Nr++)
            if (FacePVS[Face1Nr][Face2Nr]==PARTIAL_VISIBILITY) VisCount++;

    printf("Still 'PARTIAL' after 1st optimization pass: %10lu  (%7.3f%%)\n", VisCount, 100.0*double(VisCount)/double(Map.FaceChildren.Size()*Map.FaceChildren.Size()));


    // 7. Optimierung der FacePVS-Matrix, Teil 2:
    //    Faces ohne Lightmap (z.B. mit Sky-Materials) emitieren kein Licht (Initialisierung des Sonnenlichts folgt unten)
    //    und reflektieren auch keins. Deshalb können sie als "unsichtbar" aus dem FacePVS entfernt werden!
    for (unsigned long Face1Nr=0; Face1Nr<Map.FaceChildren.Size(); Face1Nr++)
    {
        if (!Map.FaceChildren[Face1Nr]->Material->UsesGeneratedSHLMap())
        {
            printf("WARNING: Got a patch mesh that doesn't use a generated SHL map! Material name is \"%s\".\n", Map.FaceChildren[Face1Nr]->Material->Name.c_str());

            for (unsigned long Face2Nr=0; Face2Nr<Map.FaceChildren.Size(); Face2Nr++)
            {
                FacePVS[Face1Nr][Face2Nr]=NO_VISIBILITY;
                FacePVS[Face2Nr][Face1Nr]=NO_VISIBILITY;
            }
        }
    }


    // 8. Zähle die Anzahl der PARTIAL_VISIBILITY-Elemente in der FacePVS-Matrix nach zweiter Optimierung
    VisCount=0;

    for (unsigned long Face1Nr=0; Face1Nr<Map.FaceChildren.Size(); Face1Nr++)
        for (unsigned long Face2Nr=0; Face2Nr<Map.FaceChildren.Size(); Face2Nr++)
            if (FacePVS[Face1Nr][Face2Nr]==PARTIAL_VISIBILITY) VisCount++;

    printf("Still 'PARTIAL' after 2nd optimization pass: %10lu  (%7.3f%%)\n", VisCount, 100.0*double(VisCount)/double(Map.FaceChildren.Size()*Map.FaceChildren.Size()));
    printf("    (%.2f faces visible in average from each face.)\n", double(VisCount)/double(Map.FaceChildren.Size()));


    // 9. BIS JETZT: Nur NO_VISIBILITY- und PARTIAL_VISIBILITY-Einträge.
    //    JETZT NEU: Bestimme, welche der PARTIAL_VISIBILITY-Einträge sogar FULL_VISIBILITY-Einträge sind.
    ArrayT< BoundingBox3T<double> > FaceBBs;

    // Zuerst mal Bounding-Boxes für alle Faces erstellen.
    for (unsigned long Face1Nr=0; Face1Nr<Map.FaceChildren.Size(); Face1Nr++)
        FaceBBs.PushBack(BoundingBox3T<double>(Map.FaceChildren[Face1Nr]->Polygon.Vertices));

    for (unsigned long Face1Nr=UseFullVis ? 0 : Map.FaceChildren.Size(); Face1Nr<Map.FaceChildren.Size(); Face1Nr++)
    {
        printf("%5.1f%%\r", (double)Face1Nr/Map.FaceChildren.Size()*100.0);
        fflush(stdout);

        for (unsigned long Face2Nr=Face1Nr+1; Face2Nr<Map.FaceChildren.Size(); Face2Nr++)
        {
            // Faces, die sich nicht mal teilweise sehen können, können sich erst recht nicht komplett sehen.
            if (FacePVS[Face1Nr][Face2Nr]==NO_VISIBILITY) continue;

            // Faces, die sich "schon aus sich heraus" (T-artige Anordnung) nur teilweise sehen können, können sich nicht komplett sehen.
            if (Map.FaceChildren[Face1Nr]->Polygon.WhatSideSimple(Map.FaceChildren[Face2Nr]->Polygon.Plane, MapT::RoundEpsilon)!=Polygon3T<double>::Front) continue;
            if (Map.FaceChildren[Face2Nr]->Polygon.WhatSideSimple(Map.FaceChildren[Face1Nr]->Polygon.Plane, MapT::RoundEpsilon)!=Polygon3T<double>::Front) continue;

            // Faces, zwischen denen möglicherweise ein Terrain liegt, können sich nicht komplett sehen.
            BoundingBox3T<double> ConvexHullBB(Map.FaceChildren[Face1Nr]->Polygon.Vertices);
            ConvexHullBB.Insert(Map.FaceChildren[Face2Nr]->Polygon.Vertices);
            ConvexHullBB=ConvexHullBB.GetEpsilonBox(-MapT::RoundEpsilon);

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

                if ((FacePVS[Face1Nr][FaceNr]!=NO_VISIBILITY) ||
                    (FacePVS[Face2Nr][FaceNr]!=NO_VISIBILITY)) Occluders.PushBack(Map.FaceChildren[FaceNr]->Polygon);
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
            FacePVS[Face1Nr][Face2Nr]=FULL_VISIBILITY;
            FacePVS[Face2Nr][Face1Nr]=FULL_VISIBILITY;
        }
    }


    // 10. Zähle die Anzahl der FULL_VISIBILITY-Elemente in der FacePVS-Matrix.
    unsigned long PartialVisCount=VisCount;
    VisCount=0;

    for (unsigned long Face1Nr=0; Face1Nr<Map.FaceChildren.Size(); Face1Nr++)
        for (unsigned long Face2Nr=0; Face2Nr<Map.FaceChildren.Size(); Face2Nr++)
            if (FacePVS[Face1Nr][Face2Nr]==FULL_VISIBILITY) VisCount++;

    printf("Number of 'FULL' visibility entries:         %10lu  (%7.3f%%)\n", VisCount, 100.0*double(VisCount)/double(Map.FaceChildren.Size()*Map.FaceChildren.Size()));
    printf("    (These are %.2f%% of the 'PARTIAL' entries!)\n", 100.0*double(VisCount)/double(PartialVisCount));
}
