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

/****************************/
/*** SHLMap related stuff ***/
/****************************/


void BspTreeBuilderT::ChopUpForMaxSHLMapSize()
{
    Console->Print(cf::va("\n%-50s %s\n", "*** Chop up for max SHLMap size ***", GetTimeSinceProgramStart()));

    for (unsigned long FaceNr=0; FaceNr<FaceChildren.Size(); FaceNr++)
    {
        // Faces whose materials specify that they don't want a lightmap generated can be skipped.
        // (We assume that "no lightmap!" implies "no SHL-map!".)
        if (!FaceChildren[FaceNr]->Material->UsesGeneratedSHLMap()) continue;

        // Bestimme die Gr��e der SHLMap.
        VectorT U;
        VectorT V;

        FaceChildren[FaceNr]->Polygon.Plane.GetSpanVectors(U, V);

        // Bestimme MinU, MinV, MaxU, MaxV.
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

        // Um hier Rundungsfehler zu vermeiden (die definitive Gr��e wird erst in CreateZeroBandSHLMaps() ermittelt)
        // bestimmen wir SizeS und SizeT mit einem Sicherheitszuschlag von 1. So wird sichergestellt, da� auch im Falle einer
        // fehlerhaften Rundung in CreateZeroBandSHLMaps() keine SHLMap die zul�ssige Gr��e �berschreitet!
        // Die +0.5 sind nur da, um sicherzustellen, da� die Konversion von double nach unsigned short wie erwartet funktioniert.
        // Die +2 sind f�r den Rahmen (links und rechts bzw. oben und unten).
        // Die +1 sind der Sicherheitszuschlag.
        unsigned long SizeS=(unsigned long)(ceil(MaxU/cf::SceneGraph::FaceNodeT::SHLMapInfoT::PatchSize)-floor(MinU/cf::SceneGraph::FaceNodeT::SHLMapInfoT::PatchSize)+0.5)+2+1;
        unsigned long SizeT=(unsigned long)(ceil(MaxV/cf::SceneGraph::FaceNodeT::SHLMapInfoT::PatchSize)-floor(MinV/cf::SceneGraph::FaceNodeT::SHLMapInfoT::PatchSize)+0.5)+2+1;

        // If the face would generate really many splits for really many shl-maps, the user probably forgot
        // to apply a material with the meta_noSHLMap property to it, or was not aware lacking it.
        if ((SizeS/cf::SceneGraph::SHLMapManT::SIZE_S+1) * (SizeT/cf::SceneGraph::SHLMapManT::SIZE_T+1) > 50)
        {
            Console->Warning(cf::va("\nFace %lu with material \"", FaceNr)+FaceChildren[FaceNr]->Material->Name+"\" seems really big.\n"+
                             "    Maybe you wanted to use a material with the \"meta_noLightMap\" property on that face instead?\n\n");
        }

        // SHLMaps d�rfen maximal SIZE_S*SIZE_T gro� sein, andernfalls teile die Face in der Mitte bzgl. der betreffenden Achse.
        if (SizeS>cf::SceneGraph::SHLMapManT::SIZE_S)
        {
            const Plane3T<double> SplitPlane(U, 0.5*(MinU+MaxU));

            if (FaceChildren[FaceNr]->Polygon.WhatSideSimple(SplitPlane, MapT::RoundEpsilon)!=Polygon3T<double>::Both) Error("SplitPlane doesn't split plane!");

            ArrayT< Polygon3T<double> > SplitResult=FaceChildren[FaceNr]->Polygon.GetSplits(SplitPlane, MapT::RoundEpsilon);

            // Hier sollte niemals ein Fehler erkannt werden. Falls doch: Programm so erweitern, da� die Coords
            // der betroffenen Face ausgegeben werden, soda� der User im Map-Editor das Problem selbst beheben kann!
            if (!SplitResult[0].IsValid(MapT::RoundEpsilon, MapT::MinVertexDist) ||
                !SplitResult[1].IsValid(MapT::RoundEpsilon, MapT::MinVertexDist)) Error("Split caused bad polygon!");

            FaceChildren.PushBack(new cf::SceneGraph::FaceNodeT(*FaceChildren[FaceNr]));

            FaceChildren[FaceNr               ]->Polygon=SplitResult[0];
            FaceChildren[FaceChildren.Size()-1]->Polygon=SplitResult[1];

            FaceNr--;
            continue;
        }

        if (SizeT>cf::SceneGraph::SHLMapManT::SIZE_T)
        {
            const Plane3T<double> SplitPlane(V, 0.5*(MinV+MaxV));

            if (FaceChildren[FaceNr]->Polygon.WhatSideSimple(SplitPlane, MapT::RoundEpsilon)!=Polygon3T<double>::Both) Error("SplitPlane doesn't split plane!");

            ArrayT< Polygon3T<double> > SplitResult=FaceChildren[FaceNr]->Polygon.GetSplits(SplitPlane, MapT::RoundEpsilon);

            // Hier sollte niemals ein Fehler erkannt werden. Falls doch: Programm so erweitern, da� die Coords
            // der betroffenen Face ausgegeben werden, soda� der User im Map-Editor das Problem selbst beheben kann!
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


// Der Code dieser Funktion ist sehr �hnlich zu ChopUpForMaxSHLMapSize().
void BspTreeBuilderT::CreateZeroBandSHLMaps()
{
    Console->Print(cf::va("\n%-50s %s\n", "*** Create zero-band SHL maps ***", GetTimeSinceProgramStart()));

    for (unsigned long FaceNr=0; FaceNr<FaceChildren.Size(); FaceNr++)
    {
        // Einige Faces wollen keine SHL-map generiert haben.
        if (!FaceChildren[FaceNr]->Material->UsesGeneratedSHLMap())
        {
            FaceChildren[FaceNr]->SHLMapInfo.SizeS=0;
            FaceChildren[FaceNr]->SHLMapInfo.SizeT=0;
            continue;
        }

        // Bestimme die Gr��e der SHLMap.
        VectorT U;
        VectorT V;

        FaceChildren[FaceNr]->Polygon.Plane.GetSpanVectors(U, V);

        // Bestimme MinU, MinV, MaxU, MaxV.
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

        // Dies ist das einzige Mal, da� die Gr��en der SHLMaps rechnerisch bestimmt werden.
        // Fortan werden sie von allen Tools und der Engine direkt aus dem CW-File geladen,
        // um Rundungsfehler, die beim Speichern durch Zusammenfassung entstehen, zu vermeiden.
        cf::SceneGraph::FaceNodeT::SHLMapInfoT& SMI=FaceChildren[FaceNr]->SHLMapInfo;

        SMI.SizeS=(unsigned short)(ceil(MaxU/cf::SceneGraph::FaceNodeT::SHLMapInfoT::PatchSize)-floor(MinU/cf::SceneGraph::FaceNodeT::SHLMapInfoT::PatchSize)+0.5)+2;
        SMI.SizeT=(unsigned short)(ceil(MaxV/cf::SceneGraph::FaceNodeT::SHLMapInfoT::PatchSize)-floor(MinV/cf::SceneGraph::FaceNodeT::SHLMapInfoT::PatchSize)+0.5)+2;

        if (SMI.SizeS>cf::SceneGraph::SHLMapManT::SIZE_S) Error("SMI.SizeS exceeds maximum!");
        if (SMI.SizeT>cf::SceneGraph::SHLMapManT::SIZE_T) Error("SMI.SizeT exceeds maximum!");

        // Allocate the sub-SHLMap in the larger SHLMap.
        unsigned long shlNr;
        unsigned long shlPosS;
        unsigned long shlPosT;

        if (!FaceChildren[FaceNr]->GetSHLMapMan().Allocate(SMI.SizeS, SMI.SizeT, shlNr, shlPosS, shlPosT)) Error("AllocateSHLMap() failed.");

        SMI.SHLMapNr=(unsigned short)shlNr;
        SMI.PosS    =(unsigned short)shlPosS;
        SMI.PosT    =(unsigned short)shlPosT;


        // As we are dealing with an empty, zero-band SHLMap here, there is no need to write anything into 'SHLMaps[SMI.SHLMapNr].Coeff'.
        // In fact, that would even be a bug, so just make sure that the coefficient storage is really empty.
        if (FaceChildren[FaceNr]->GetSHLMapMan().SHLMaps[SMI.SHLMapNr]->Coeffs .Size()!=0) Error("Zero-band SHLMap is not empty.");
        if (FaceChildren[FaceNr]->GetSHLMapMan().SHLMaps[SMI.SHLMapNr]->Indices.Size()!=0) Error("Should not have indices here.");
    }

    Console->Print("done\n");
}
