/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/******************************/
/*** LightMap related stuff ***/
/******************************/


void BspTreeBuilderT::ChopUpForMaxLightMapSize()
{
    Console->Print(cf::va("\n%-50s %s\n", "*** Chop up for max LightMap size ***", GetTimeSinceProgramStart()));

    for (unsigned long FaceNr=0; FaceNr<FaceChildren.Size(); FaceNr++)
    {
        // FaceChildren whose materials specify that they don't want a lightmap generated can be skipped.
        if (!FaceChildren[FaceNr]->Material->UsesGeneratedLightMap()) continue;

        // Bestimme die Größe der LightMap
        VectorT U;
        VectorT V;

        FaceChildren[FaceNr]->Polygon.Plane.GetSpanVectors(U, V);

        // Bestimme MinU, MinV, MaxU, MaxV
        if (FaceChildren[FaceNr]->Polygon.Vertices.Size()==0) Error("Found a Face with 0 vertices!");

        double MinU=dot(FaceChildren[FaceNr]->Polygon.Vertices[0], U); double MaxU=MinU;
        double MinV=dot(FaceChildren[FaceNr]->Polygon.Vertices[0], V); double MaxV=MinV;

        for (unsigned long VertexNr=1; VertexNr<FaceChildren[FaceNr]->Polygon.Vertices.Size(); VertexNr++)
        {
            double u=dot(FaceChildren[FaceNr]->Polygon.Vertices[VertexNr], U);
            double v=dot(FaceChildren[FaceNr]->Polygon.Vertices[VertexNr], V);

            if (u<MinU) MinU=u;
            if (v<MinV) MinV=v;

            if (u>MaxU) MaxU=u;
            if (v>MaxV) MaxV=v;
        }

        // KLEINER TIP: Sollte die Berechnung der LightMap-Koordinaten jemals der der SHLMap-Koordinaten angepaßt werden,
        // sodaß Patches sich immer integral überlappen (wie es auch in der TODO-Liste steht), kann man (alle) Code-Stellen,
        // die einer Änderung bedürfen, mit dem Durchsuchen des D:\Projects Verzeichnisses nach "SmallestU" finden.
        // Ich bin mir nicht 100% sicher, ob das wirklich ALLE Stellen findet, das ist aber ein sehr guter Anfang!

        // Um hier Rundungsfehler zu vermeiden (die definitive Größe wird erst in CalculateFullBrightLightMaps() ermittelt)
        // bestimmen wir SizeS und SizeT mit einem Sicherheitszuschlag von 1. So wird sichergestellt, daß auch im Falle einer
        // fehlerhaften Rundung in CalculateFullBrightLightMaps() keine LightMap die zulässige Größe überschreitet!
        unsigned long SizeS=(unsigned long)ceil((MaxU-MinU) / BspTree->GetLightMapPatchSize())+2+1;
        unsigned long SizeT=(unsigned long)ceil((MaxV-MinV) / BspTree->GetLightMapPatchSize())+2+1;

        // If the face would generate really many splits for really many lightmaps, the user probably forgot
        // to apply a material with the meta_noLightMap property to it, or was not aware lacking it.
        if ((SizeS/cf::SceneGraph::LightMapManT::SIZE_S+1) * (SizeT/cf::SceneGraph::LightMapManT::SIZE_T+1) > 50)
        {
            Console->Warning(cf::va("Face %lu with material \"", FaceNr)+FaceChildren[FaceNr]->Material->Name+"\" seems really big.\n"+
                             "    Maybe you wanted to use a material with a lightmap other than \"$lightmap\" (or no lightmap at all)?\n\n");
        }

        // LightMaps dürfen maximal SIZE_S*SIZE_T groß sein, andernfalls teile die Face in der Mitte bzgl. der betreffenden Achse.
        if (SizeS>cf::SceneGraph::LightMapManT::SIZE_S)
        {
            const Plane3T<double> SplitPlane(U, 0.5*(MinU+MaxU));

            if (FaceChildren[FaceNr]->Polygon.WhatSideSimple(SplitPlane, MapT::RoundEpsilon)!=Polygon3T<double>::Both) Error("SplitPlane doesn't split plane!");

            ArrayT< Polygon3T<double> > SplitResult=FaceChildren[FaceNr]->Polygon.GetSplits(SplitPlane, MapT::RoundEpsilon);

            // Hier sollte niemals ein Fehler erkannt werden. Falls doch: Programm so erweitern, daß die Coords
            // der betroffenen Face ausgegeben werden, sodaß der User im Map-Editor das Problem selbst beheben kann!
            if (!SplitResult[0].IsValid(MapT::RoundEpsilon, MapT::MinVertexDist) ||
                !SplitResult[1].IsValid(MapT::RoundEpsilon, MapT::MinVertexDist)) Error("Split caused bad polygon!");

            FaceChildren.PushBack(new cf::SceneGraph::FaceNodeT(*FaceChildren[FaceNr]));

            FaceChildren[FaceNr               ]->Polygon=SplitResult[0];
            FaceChildren[FaceChildren.Size()-1]->Polygon=SplitResult[1];

            FaceNr--;
            continue;
        }

        if (SizeT>cf::SceneGraph::LightMapManT::SIZE_T)
        {
            const Plane3T<double> SplitPlane(V, 0.5*(MinV+MaxV));

            if (FaceChildren[FaceNr]->Polygon.WhatSideSimple(SplitPlane, MapT::RoundEpsilon)!=Polygon3T<double>::Both) Error("SplitPlane doesn't split plane!");

            ArrayT< Polygon3T<double> > SplitResult=FaceChildren[FaceNr]->Polygon.GetSplits(SplitPlane, MapT::RoundEpsilon);

            // Hier sollte niemals ein Fehler erkannt werden. Falls doch: Programm so erweitern, daß die Coords
            // der betroffenen Face ausgegeben werden, sodaß der User im Map-Editor das Problem selbst beheben kann!
            if (!SplitResult[0].IsValid(MapT::RoundEpsilon, MapT::MinVertexDist) ||
                !SplitResult[1].IsValid(MapT::RoundEpsilon, MapT::MinVertexDist)) Error("Split caused bad polygon!");

            FaceChildren.PushBack(new cf::SceneGraph::FaceNodeT(*FaceChildren[FaceNr]));

            FaceChildren[FaceNr               ]->Polygon=SplitResult[0];
            FaceChildren[FaceChildren.Size()-1]->Polygon=SplitResult[1];

            FaceNr--;
            continue;
        }
    }

    Console->Print(cf::va("Faces               : %10lu\n", FaceChildren.Size()));
}


// Der Code dieser Funktion ist sehr ähnlich zu ChopUpForMaxLightMapSize().
void BspTreeBuilderT::CreateFullBrightLightMaps()
{
    Console->Print(cf::va("\n%-50s %s\n", "*** Create Default LightMaps ***", GetTimeSinceProgramStart()));

    for (unsigned long FaceNr=0; FaceNr<FaceChildren.Size(); FaceNr++)
        FaceChildren[FaceNr]->InitDefaultLightMaps(BspTree->GetLightMapPatchSize());

    for (unsigned long OtherNr=0; OtherNr<OtherChildren.Size(); OtherNr++)
        OtherChildren[OtherNr]->InitDefaultLightMaps(BspTree->GetLightMapPatchSize());

    Console->Print("done\n");
}
