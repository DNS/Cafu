/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#if USE_NORMALMAPS

// This function computes a normal vector for a patch by taking the surfaces normal-map into account.
//
// Dazu müssen wir eigentlich die Normal-Map dieser Face entlang des PatchPoly rasterizern, und dann den renomalisierten Mittelwert
// der Normal-Map-Normalenvektoren im PatchPoly bilden. Allerdings ist ein PatchPoly üblicherweise *MINDESTENS* 200.0*200.0 world units groß,
// während ein großer Normal-Map Texel idR nur (25.4*0.25)^2 world units groß ist.
// Ein PatchPoly deckt also MINDESTENS (200.0)^2/(25.4*0.25)^2 == 6299 Normal-Map Texels ab!! Neben der Tatsache, daß das Rasterizing zur
// Bestimmung dieser Normalen *sehr* aufwendig ist (und diese Normalen weiter aufwendig vom Tangent- in den Object-Space überführt werden müssen),
// ergibt sich auch nahezu IMMER ein renomalisierter Mittelwert, der F.Plane.Normal entspricht. Sollte es dennoch Abweichungen geben, sind sie so klein,
// daß man sie niemals wahrnimmt. (Die Rundungsfehler allein an anderer Stelle sind um Größenordnungen höher, z.B. die Komprimierung der Werte in "char"!!)
// Daher würde   Patch.Normal=F.Plane.Normal;   normalerweise völlig ausreichen.
void ComputePatchNormal(PatchT& Patch, const cf::SceneGraph::FaceNodeT::TexInfoT& TI, const BitmapT& NormalMap, const VectorT& SpanU, const VectorT& SpanV, const float SHLMapPatchSize)
{
    const VectorT PlaneNormal=Patch.Normal;
    const VectorT TI_U       =TI.U.AsVectorOfDouble();
    const VectorT TI_V       =TI.V.AsVectorOfDouble();

    Patch.Normal=VectorT();

    const unsigned long NrOfSamples=20;

    // Do not attempt to ACTUALLY RASTERIZE the normal-map along the dimensions of the patch: the effort is not proportional to the results.
    // Simply taking some samples serves equally well, and saves us the huge overhead of a dedicated software rasterizer.
    for (unsigned long SampleNr=0; SampleNr<NrOfSamples; SampleNr++)
    {
        const double  PatchRadius =SHLMapPatchSize / 2.0;
        const double  OffsetU     =(2.0*double(rand())/double(RAND_MAX)-1.0)*PatchRadius;
        const double  OffsetV     =(2.0*double(rand())/double(RAND_MAX)-1.0)*PatchRadius;
        const VectorT SampleOrigin=Patch.Coord+scale(SpanU, OffsetU)+scale(SpanV, OffsetV);

     // const double TexSizeX=TexDataSizeX[TI.TexDataNr];               // Texture size in X direction.
     // const double TexSizeY=TexDataSizeY[TI.TexDataNr];               // Texture size in Y direction.
        const double LengthU =length(TI_U);                       // Länge des U-Vektors (X-Richtung).
        const double LengthV =length(TI_V);                       // Länge des V-Vektors (Y-Richtung).

        double s=/*(*/ dot(SampleOrigin, TI_U)/(LengthU*LengthU)+TI.OffsetU;  // )/TexSizeX-SmallestS;
        double t=/*(*/ dot(SampleOrigin, TI_V)/(LengthV*LengthV)+TI.OffsetV;  // )/TexSizeY-SmallestT;

        // Do "modulo texsize" for s and t.
        while (s<0.0) s+=NormalMap.SizeX;
        while (s>=NormalMap.SizeX) s-=NormalMap.SizeX;
        while (t<0.0) t+=NormalMap.SizeY;
        while (t>=NormalMap.SizeY) t-=NormalMap.SizeY;

        unsigned int s_=(unsigned int)(s+0.5); if (s_>=NormalMap.SizeX) s_=0;
        unsigned int t_=(unsigned int)(t+0.5); if (t_>=NormalMap.SizeY) t_=0;

        const uint32_t NormalRC=NormalMap.Data[s_+t_*NormalMap.SizeX];
        const char     nxRC    =char(NormalRC >>  0);
        const char     nyRC    =char(NormalRC >>  8);
        const char     nzRC    =char(NormalRC >> 16);
        const VectorT  Normal  =VectorT(2.0*(nxRC/255.0-0.5), 2.0*(nyRC/255.0-0.5), 2.0*(nzRC/255.0-0.5));

        Patch.Normal=Patch.Normal+Normal;
    }

    try
      {
        Patch.Normal=normalize(Patch.Normal, 0.000001);
      }
    catch (const DivisionByZeroE& /*E*/)
      {
        printf("WARNING: Invalid patch normal occured!\n");
        Patch.Normal=VectorT(0.0, 0.0, 1.0);
      }

    // Translate (rotate) the Patch.Normal from tangent into world space.
    const VectorT R1=VectorT(SpanU.x, SpanV.x, PlaneNormal.x);
    const VectorT R2=VectorT(SpanU.y, SpanV.y, PlaneNormal.y);
    const VectorT R3=VectorT(SpanU.z, SpanV.z, PlaneNormal.z);

    Patch.Normal=VectorT(dot(R1, Patch.Normal), dot(R2, Patch.Normal), dot(R3, Patch.Normal));
}

#endif


void InitializePatches(const cf::SceneGraph::BspTreeNodeT& Map)
{
    printf("\n%-50s %s\n", "*** Initialize Patches ***", GetTimeSinceProgramStart());


    // 1. Allokiere neuen Speicher für die ganzen Patches
    Patches.Clear();
    Patches.PushBackEmpty(Map.FaceChildren.Size());

    unsigned long PatchCount=0;
    unsigned long FaceNr;

    for (FaceNr=0; FaceNr<Map.FaceChildren.Size(); FaceNr++)
    {
        const cf::SceneGraph::FaceNodeT::SHLMapInfoT& SMI            =Map.FaceChildren[FaceNr]->SHLMapInfo;
        const unsigned long                           NR_OF_SH_COEFFS=cf::SceneGraph::SHLMapManT::NrOfBands * cf::SceneGraph::SHLMapManT::NrOfBands;

        Patches[FaceNr].PushBackEmpty(SMI.SizeS*SMI.SizeT);
        PatchCount+=Patches[FaceNr].Size();

        // Wir prüfen es besser hier, denn die Engine prüft es zur Zeit nicht! Sollte aber niemals vorkommen!
        if (SMI.SizeS>cf::SceneGraph::SHLMapManT::SIZE_S || SMI.SizeT>cf::SceneGraph::SHLMapManT::SIZE_T) Error("SHLMAP OF FACE %u EXCEEDS LIMITS! ENGINE WILL DENY THIS MAP!", FaceNr);

        // Für alle Patches auch die Coeffs allokieren.
        for (unsigned long PatchNr=0; PatchNr<Patches[FaceNr].Size(); PatchNr++)
        {
            PatchT& P=Patches[FaceNr][PatchNr];

            while (P.SHCoeffs_UnradiatedTransfer.Size()<NR_OF_SH_COEFFS) P.SHCoeffs_UnradiatedTransfer.PushBack(0.0);
            while (P.SHCoeffs_TotalTransfer     .Size()<NR_OF_SH_COEFFS) P.SHCoeffs_TotalTransfer     .PushBack(0.0);
        }
    }
    printf("# patches allocated:%10lu\n", PatchCount);


    // 2. Bestimme, ob ein Patch zumindest ein bißchen innerhalb seiner Face liegt, und wenn ja, seinen Mittelpunkt.
    //    Dieser wird wegen Rundungsfehlern etwas entlang seines Normalenvektors verschoben!

    // Bilde zuerst ein LookUp-Array, das die Nummern aller Faces mit Radiosity-Sunlight Material enthält.
    ArrayT<unsigned long> SkyFaces;

    for (FaceNr=0; FaceNr<Map.FaceChildren.Size(); FaceNr++)
        if (length(Vector3T<double>(Map.FaceChildren[FaceNr]->Material->meta_SunLight_Irr))>0.1 &&
            length(Vector3T<double>(Map.FaceChildren[FaceNr]->Material->meta_SunLight_Dir))>0.1) SkyFaces.PushBack(FaceNr);


    for (FaceNr=0; FaceNr<Map.FaceChildren.Size(); FaceNr++)
    {
        printf("%5.1f%%\r", (double)FaceNr/Map.FaceChildren.Size()*100.0);
        fflush(stdout);

        const cf::SceneGraph::FaceNodeT* FN=Map.FaceChildren[FaceNr];
        const Polygon3T<double>&         F =FN->Polygon;

        // Bestimme die Spannvektoren
        VectorT U;
        VectorT V;

        F.Plane.GetSpanVectors(U, V);

        // Finde SmallestU und SmallestV
        double SmallestU=dot(F.Vertices[0], U);
        double SmallestV=dot(F.Vertices[0], V);

        for (unsigned long VertexNr=1; VertexNr<F.Vertices.Size(); VertexNr++)
        {
            double u=dot(F.Vertices[VertexNr], U);
            double v=dot(F.Vertices[VertexNr], V);

            if (u<SmallestU) SmallestU=u;
            if (v<SmallestV) SmallestV=v;
        }

        SmallestU = floor(SmallestU / Map.GetSHLMapPatchSize());
        SmallestV = floor(SmallestV / Map.GetSHLMapPatchSize());

        // Bereite folgende Schleife vor
        const VectorT UV_Origin=scale(F.Plane.Normal, F.Plane.Dist);
        const VectorT Safety   =scale(F.Plane.Normal, 0.1);

        Polygon3T<double> PatchPoly;
        PatchPoly.Plane=dot(F.Plane.Normal, cross(U, V))<0 ? F.Plane : F.Plane.GetMirror();

#if USE_NORMALMAPS
        BitmapT* NormalMapPtr=FN->Material->NormMapComp.GetBitmap();
        BitmapT  NormalMap   =*NormalMapPtr;

        delete NormalMapPtr;
        NormalMapPtr=NULL;
#endif

        // Nun betrachte alle Patches
        for (unsigned long t=0; t<FN->SHLMapInfo.SizeT; t++)
            for (unsigned long s=0; s<FN->SHLMapInfo.SizeS; s++)
            {
                const double PATCH_SIZE=Map.GetSHLMapPatchSize();
                PatchT&      Patch     =Patches[FaceNr][t*FN->SHLMapInfo.SizeS+s];

                Patch.Coord     =VectorT(0, 0, 0);
#if USE_NORMALMAPS
                Patch.Normal    =F.Plane.Normal;    // Do a fail-safe init (also required for the ComputePatchNormal() function below!).
#endif
                Patch.InsideFace=false;

                PatchPoly.Vertices.Clear();
                PatchPoly.Vertices.PushBack(UV_Origin+scale(U, (SmallestU+s-1.0)*PATCH_SIZE)+scale(V, (SmallestV+t-1.0)*PATCH_SIZE));
                PatchPoly.Vertices.PushBack(UV_Origin+scale(U, (SmallestU+s    )*PATCH_SIZE)+scale(V, (SmallestV+t-1.0)*PATCH_SIZE));
                PatchPoly.Vertices.PushBack(UV_Origin+scale(U, (SmallestU+s    )*PATCH_SIZE)+scale(V, (SmallestV+t    )*PATCH_SIZE));
                PatchPoly.Vertices.PushBack(UV_Origin+scale(U, (SmallestU+s-1.0)*PATCH_SIZE)+scale(V, (SmallestV+t    )*PATCH_SIZE));

                if (!F.Overlaps(PatchPoly, false, MapT::RoundEpsilon)) continue;

                if (!F.Encloses(PatchPoly, true, MapT::RoundEpsilon))
                {
                    ArrayT< Polygon3T<double> > NewPolygons;

                    PatchPoly.GetChoppedUpAlong(F, MapT::RoundEpsilon, NewPolygons);
                    if (NewPolygons.Size()==0) Error("PolygonChopUp failed.");

                    PatchPoly=NewPolygons[NewPolygons.Size()-1];
                    if (PatchPoly.Vertices.Size()==0) Error("PatchPoly.Vertices.Size()==0.");
                }

                for (unsigned long VertexNr=0; VertexNr<PatchPoly.Vertices.Size(); VertexNr++)
                    Patch.Coord=Patch.Coord+PatchPoly.Vertices[VertexNr];

                Patch.Coord     =scale(Patch.Coord, 1.0/double(PatchPoly.Vertices.Size()))+Safety;
                Patch.InsideFace=true;

#if USE_NORMALMAPS
                // Zuletzt noch den Patch.Normal Vektor bestimmen.
                // Siehe die Kommentare zur ComputePatchNormal Funktion für weitere Infos.
                // Außerdem ist dieses Verfahren alles andere als effizient -- sollte die Normal-Map nicht für jede Face neu laden!
                // Beachte: Die Patch.Normal muß mit der F.Plane.Normal initialisiert sein!
                ComputePatchNormal(Patch, FN->TI, NormalMap, U, V, Map.GetSHLMapPatchSize());
#endif
            }
    }
    printf("Patch coords calculated.\n");
}
