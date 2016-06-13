/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/


void BspTreeBuilderT::ChopUpFaces()
{
    Console->Print(cf::va("\n%-50s %s\n", "*** Chop Up Interpenetrations ***", GetTimeSinceProgramStart()));

    const double CHOP_MIN_VERTEX_DIST = std::max(3.0, MapT::MinVertexDist);

    ArrayT<BoundingBox3dT> FaceBB;

    for (unsigned long FaceNr = 0; FaceNr < FaceChildren.Size(); FaceNr++)
        FaceBB.PushBack(BoundingBox3dT(FaceChildren[FaceNr]->Polygon.Vertices));

    // Zuerst die sich echt schneidenden Faces (deren Ebenen sich in einer Geraden schneiden) choppen.
    // Die folgende Schleife lief ursprünglich in O(n^2) Zeit. Man könnte sie auf O(1/2*n^2) bringen, indem man
    // entweder eine der beiden if-Bedingungen löscht oder jede FaceNr-Kombination durch Änderung der Schleifen
    // nur genau ein Mal betrachtet. Letzteres ist geringfügig schneller und erzielt exakt das gleiche Ergebnis
    // wie der O(n^2)-Algo, der beide Face-Schleifen stets von 0..FaceCounter-1 durchlief.
    // (Die andere Methode hätte wegen anderer Reihenfolge ein geringfügig anderes Ergebnis geliefert.)
    for (unsigned long Face1Nr=0; Face1Nr+1<FaceChildren.Size(); Face1Nr++)
    {
        Console->Print(cf::va("%5.1f%%\r", (double)Face1Nr/FaceChildren.Size()*100.0));
        // fflush(stdout);      // The stdout console auto-flushes the output.

        for (unsigned long Face2Nr=Face1Nr+1; Face2Nr<FaceChildren.Size(); Face2Nr++)
        {
            // Note that   GetEpsilonBox(-MapT::RoundEpsilon)   (note the minus sign!) is not a
            // good idea here because it will miss pairs of faces that are arranged like a "T".
            if (!FaceBB[Face1Nr].GetEpsilonBox(MapT::RoundEpsilon).Intersects(FaceBB[Face2Nr])) continue;

            if (FaceChildren[Face1Nr]->Polygon.WhatSideSimple(FaceChildren[Face2Nr]->Polygon.Plane, MapT::RoundEpsilon)==Polygon3T<double>::Both)
            {
                const ArrayT< Polygon3T<double> > SplitResult = FaceChildren[Face1Nr]->Polygon.GetSplits(FaceChildren[Face2Nr]->Polygon.Plane, MapT::RoundEpsilon);

                if (SplitResult[0].IsValid(MapT::RoundEpsilon, CHOP_MIN_VERTEX_DIST) &&
                    SplitResult[1].IsValid(MapT::RoundEpsilon, CHOP_MIN_VERTEX_DIST))   // Especially check for "collapsing" vertices.
                {
                    FaceChildren.PushBack(new cf::SceneGraph::FaceNodeT(*FaceChildren[Face1Nr]));

                    Polygon3T<double>& Front=FaceChildren[Face1Nr              ]->Polygon; Front=SplitResult[0];
                    Polygon3T<double>& Back =FaceChildren[FaceChildren.Size()-1]->Polygon; Back =SplitResult[1];

                    FaceBB[Face1Nr] = BoundingBox3dT(Front.Vertices);
                    FaceBB.PushBack(BoundingBox3dT(Back.Vertices));
                }
            }

            if (FaceChildren[Face2Nr]->Polygon.WhatSideSimple(FaceChildren[Face1Nr]->Polygon.Plane, MapT::RoundEpsilon)==Polygon3T<double>::Both)
            {
                const ArrayT< Polygon3T<double> > SplitResult = FaceChildren[Face2Nr]->Polygon.GetSplits(FaceChildren[Face1Nr]->Polygon.Plane, MapT::RoundEpsilon);

                if (SplitResult[0].IsValid(MapT::RoundEpsilon, CHOP_MIN_VERTEX_DIST) &&
                    SplitResult[1].IsValid(MapT::RoundEpsilon, CHOP_MIN_VERTEX_DIST))   // Especially check for "collapsing" vertices.
                {
                    FaceChildren.PushBack(new cf::SceneGraph::FaceNodeT(*FaceChildren[Face2Nr]));

                    Polygon3T<double>& Front=FaceChildren[Face2Nr              ]->Polygon; Front=SplitResult[0];
                    Polygon3T<double>& Back =FaceChildren[FaceChildren.Size()-1]->Polygon; Back =SplitResult[1];

                    FaceBB[Face2Nr] = BoundingBox3dT(Front.Vertices);
                    FaceBB.PushBack(BoundingBox3dT(Back.Vertices));
                }
            }
        }
    }

#if 0
    // Ich #if 0'e diesen Code aus zwei Gründen:
    // a) Es ist quasi unmöglich zu entscheiden *welche* von zwei sich in einer Ebene überlappenden Faces entfernt und welche behalten werden soll,
    //    denn diese Information lässt sich aus den MaterialTs der beteiligten Faces nicht sicher ableiten.
    //    Hier ist das Wissen des Mappers gefragt, der diese Entscheidung durch die entsprechende Gestaltung seiner Geometrie treffen sollte.
    // b) Es ist langsamer, langwieriger Code für einen Spezialfall der in einer guten Map ohnehin nie auftreten sollte.
    //    CaBSP wird viel schlanker und die Maps bleiben gleich gut, wenn man diesen Code einfach weglässt.

    // Nach obiger Schleife sind jetzt fast nur noch Faces übrig, die sich im Sinne einer Überlappung (in der gleichen Ebene)
    // schneiden. Das Entfernen unnötiger TextureInfos findet übrigens erst wieder beim nächsten InsideFill() statt.
    for (unsigned long Face1Nr=0; Face1Nr+1<FaceChildren.Size(); Face1Nr++)
    {
        Console->Print(cf::va("%5.1f%%\r", (double)Face1Nr/FaceChildren.Size()*100.0));
        // fflush(stdout);      // The stdout console auto-flushes the output.

        for (unsigned long Face2Nr=Face1Nr+1; Face2Nr<FaceChildren.Size(); Face2Nr++)
        {
            // Note that   GetEpsilonBox(-MapT::RoundEpsilon)   (note the minus sign!) won't work here,
            // because it will fail the test for a pair of overlapping faces that are in an axis-aligned plane.
            if (!FaceBB[Face1Nr].GetEpsilonBox(MapT::RoundEpsilon).Intersects(FaceBB[Face2Nr])) continue;

            if (FaceChildren[Face1Nr]->Polygon.WhatSide(FaceChildren[Face2Nr]->Polygon.Plane, MapT::RoundEpsilon)!=Polygon3T<double>::InIdentical) continue;
            if (!FaceChildren[Face1Nr]->Polygon.Overlaps(FaceChildren[Face2Nr]->Polygon, false, MapT::RoundEpsilon)) continue;

            const bool Face1IsOpaque=FaceChildren[Face1Nr]->TI.RenderMode==cf::SceneGraph::FaceNodeT::TexInfoT::Normal && FaceChildren[Face1Nr]->TI.Alpha==255;
            const bool Face2IsOpaque=FaceChildren[Face2Nr]->TI.RenderMode==cf::SceneGraph::FaceNodeT::TexInfoT::Normal && FaceChildren[Face2Nr]->TI.Alpha==255;


            // Wenn Poly1 Poly2 komplett umschließt (Poly2 kann kleiner oder auch genau gleich sein wie Poly1),
            // wollen wir nicht Poly1 zerschneiden usw., sondern Poly2 einfach entfernen.
            // Dies geht nur dann NICHT, wenn Poly2 undurchsichtig (opaque) ist, und Poly1 zugleich durchsichtig ist!
            if ((Face1IsOpaque || !Face2IsOpaque) && FaceChildren[Face1Nr]->Polygon.Encloses(FaceChildren[Face2Nr]->Polygon, true, MapT::RoundEpsilon))
            {
                delete FaceChildren[Face2Nr];

                FaceChildren[Face2Nr]=FaceChildren[FaceChildren.Size()-1]; FaceChildren.DeleteBack();
                FaceBB      [Face2Nr]=FaceBB      [FaceBB      .Size()-1]; FaceBB      .DeleteBack();

                Face2Nr--;
                continue;
            }

            // Wenn Poly2 Poly1 komplett umschließt (Poly1 kann kleiner oder auch genau gleich sein wie Poly2),
            // wollen wir nicht Poly2 zerschneiden usw., sondern Poly1 einfach entfernen.
            // Dies geht nur dann NICHT, wenn Poly1 undurchsichtig (opaque) ist, und Poly2 zugleich durchsichtig ist!
            if ((Face2IsOpaque || !Face1IsOpaque) && FaceChildren[Face2Nr]->Polygon.Encloses(FaceChildren[Face1Nr]->Polygon, true, MapT::RoundEpsilon))
            {
                delete FaceChildren[Face1Nr];

                FaceChildren[Face1Nr]=FaceChildren[FaceChildren.Size()-1]; FaceChildren.DeleteBack();
                FaceBB      [Face1Nr]=FaceBB      [FaceBB      .Size()-1]; FaceBB      .DeleteBack();

                Face1Nr--;
                break;
            }


            if (Face1IsOpaque)
            {
                // Zerschneide Poly2 entlang der Kanten von Poly1.
                ArrayT< Polygon3T<double> > NewPolygons;
                FaceChildren[Face2Nr]->Polygon.GetChoppedUpAlong(FaceChildren[Face1Nr]->Polygon, MapT::RoundEpsilon, NewPolygons);
                NewPolygons.DeleteBack();   // Das letzte NewPolygon ist das überlappende, daher löschen wir es.

                // Prüfe, ob keiner der Splitter ungültig ist. Falls doch, lassen wir Face1Nr und Face2Nr lieber so, wie sie sind!
                unsigned long SplitterNr;

                for (SplitterNr=0; SplitterNr<NewPolygons.Size(); SplitterNr++)
                    if (!NewPolygons[SplitterNr].IsValid(MapT::RoundEpsilon, MapT::MinVertexDist)) break;
                if (SplitterNr<NewPolygons.Size()) continue;

                // Die Splitter nun ans FaceChildren-Array anhängen, dabei TexInfo usw. nicht vergessen und neue FaceBBs erstellen.
                // Wenn Poly1 Poly2 ganz umschlossen hat, könnte NewPolygons.Size() hier auch 0 sein!
                for (SplitterNr=0; SplitterNr<NewPolygons.Size(); SplitterNr++)
                {
                    FaceChildren.PushBack(new cf::SceneGraph::FaceNodeT(*FaceChildren[Face2Nr]));           // Erst mal alte TexInfo usw. kopieren.
                    FaceChildren[FaceChildren.Size()-1]->Polygon=NewPolygons[SplitterNr];                   // Neues Polygon einsetzen.
                    FaceBB.PushBack(BoundingBox3dT(FaceChildren[FaceChildren.Size()-1]->Polygon.Vertices)); // Neue FaceBB erstellen.
                }

                // Die ursprüngliche Face2Nr nun mit der letzten Face aus dem FaceChildren-Array überschreiben.
                delete FaceChildren[Face2Nr];

                FaceChildren[Face2Nr]=FaceChildren[FaceChildren.Size()-1]; FaceChildren.DeleteBack();
                FaceBB      [Face2Nr]=FaceBB      [FaceBB      .Size()-1]; FaceBB      .DeleteBack();

                Face2Nr--;
                continue;
            }
            else
            {
                // Zerschneide Poly1 entlang der Kanten von Poly2.
                ArrayT< Polygon3T<double> > NewPolygons;
                FaceChildren[Face1Nr]->Polygon.GetChoppedUpAlong(FaceChildren[Face2Nr]->Polygon, MapT::RoundEpsilon, NewPolygons);
                NewPolygons.DeleteBack();   // Das letzte NewPolygon ist das überlappende, daher löschen wir es.

                // Prüfe, ob keiner der Splitter ungültig ist. Falls doch, lassen wir Face2Nr und Face1Nr lieber so, wie sie sind!
                unsigned long SplitterNr;

                for (SplitterNr=0; SplitterNr<NewPolygons.Size(); SplitterNr++)
                    if (!NewPolygons[SplitterNr].IsValid(MapT::RoundEpsilon, MapT::MinVertexDist)) break;
                if (SplitterNr<NewPolygons.Size()) continue;

                // Die Splitter nun ans FaceChildren-Array anhängen, dabei TexInfo usw. nicht vergessen und neue FaceBBs erstellen.
                // Wenn Poly2 Poly1 ganz umschlossen hat, könnte NewPolygons.Size() hier auch 0 sein!
                for (SplitterNr=0; SplitterNr<NewPolygons.Size(); SplitterNr++)
                {
                    FaceChildren.PushBack(new cf::SceneGraph::FaceNodeT(*FaceChildren[Face1Nr]));           // Erst mal alte TexInfo usw. kopieren.
                    FaceChildren[FaceChildren.Size()-1]->Polygon=NewPolygons[SplitterNr];                   // Neues Polygon einsetzen,
                    FaceBB.PushBack(BoundingBox3dT(FaceChildren[FaceChildren.Size()-1]->Polygon.Vertices)); // Neue FaceBB erstellen.
                }

                // Die ursprüngliche Face1Nr nun mit der letzten Face aus dem FaceChildren-Array überschreiben.
                delete FaceChildren[Face1Nr];

                FaceChildren[Face1Nr]=FaceChildren[FaceChildren.Size()-1]; FaceChildren.DeleteBack();
                FaceBB      [Face1Nr]=FaceBB      [FaceBB      .Size()-1]; FaceBB      .DeleteBack();

                Face1Nr--;
                break;
            }
        }
    }
#endif

    Console->Print(cf::va("Faces               : %10lu\n", FaceChildren.Size()));
}
