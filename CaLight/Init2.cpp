/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

struct SunT
{
    MaterialT* Material;
    VectorT    SunIrradiance;
    VectorT    LightRevDir;
};


void InitSunlight(cf::PatchMeshT& PatchMesh, const ArrayT< ArrayT<Vector3dT> >& SampleCoords, const ArrayT<SunT>& Suns, const ArrayT<const cf::SceneGraph::FaceNodeT*>& SkyFaces, const CaLightWorldT& CaLightWorld)
{
    for (unsigned long PatchNr=0; PatchNr<PatchMesh.Patches.Size(); PatchNr++)
    {
        cf::PatchT&              Patch=PatchMesh.Patches[PatchNr];
        const ArrayT<Vector3dT>& sc   =SampleCoords[PatchNr];

        if (!Patch.InsideFace) continue;
        if (sc.Size()==0) continue;

        for (unsigned long SunNr=0; SunNr<Suns.Size(); SunNr++)
        {
            if (dot(Patch.Normal, Suns[SunNr].LightRevDir)<=0.0) continue;

            unsigned long NrOfSkyHits=0;

            for (unsigned long SampleNr=0; SampleNr<sc.Size(); SampleNr++)
            {
                // Teste, ob der Strahl sc[SampleNr]+r*(-Suns[SunNr].LightDir) eine Face mit Sky-Texture trifft.
                const VectorT Ray=Suns[SunNr].LightRevDir*9999999.9;
                const VectorT Hit=sc[SampleNr]+Ray*CaLightWorld.TraceRay(sc[SampleNr], Ray);

                // Teste, ob 'Hit' in einer Face mit Sky-Texture liegt.
                unsigned long FNr;

                for (FNr=0; FNr<SkyFaces.Size(); FNr++)
                {
                    const cf::SceneGraph::FaceNodeT* SkyFace=SkyFaces[FNr];

                    if (SkyFace->Material!=Suns[SunNr].Material) continue;              // In diesem Durchlauf tragen nur solche SkyFaces bei, die zu Suns[SunNr] gehören.
                    if (fabs(SkyFace->Polygon.Plane.GetDistance(Hit))>0.2) continue;    // Ist Hit zu weit von der SkyFace weg?

                    unsigned long VNr;

                    for (VNr=0; VNr<SkyFace->Polygon.Vertices.Size(); VNr++)
                        if (SkyFace->Polygon.GetEdgePlane(VNr, 0.0).GetDistance(Hit)<-0.1) break;

                    if (VNr==SkyFace->Polygon.Vertices.Size()) break;
                }

                if (FNr<SkyFaces.Size()) NrOfSkyHits++;
            }

            const double  Amount  =double(NrOfSkyHits)/double(sc.Size())*dot(Suns[SunNr].LightRevDir, Patch.Normal);
            const VectorT SunLight=scale(Suns[SunNr].SunIrradiance, REFLECTIVITY*Amount);

            Patch.UnradiatedEnergy+=SunLight;
            Patch.TotalEnergy     +=SunLight;
            Patch.EnergyFromDir   +=Suns[SunNr].LightRevDir*Max3(SunLight);
        }
    }
}


void InitializePatches(const CaLightWorldT& CaLightWorld)
{
    const cf::SceneGraph::BspTreeNodeT& Map=CaLightWorld.GetBspTree();

    printf("\n%-50s %s\n", "*** Initialize Patches ***", GetTimeSinceProgramStart());


    // Bilde zuerst ein LookUp-Array, das die Nummern aller Faces mit Radiosity-Sunlight Material enthält.
    ArrayT<const cf::SceneGraph::FaceNodeT*> SkyFaces;

    for (unsigned long FaceNr=0; FaceNr<Map.FaceChildren.Size(); FaceNr++)
    {
        const cf::SceneGraph::FaceNodeT* Face=Map.FaceChildren[FaceNr];

        if (length(VectorT(Face->Material->meta_SunLight_Irr))<0.1) continue;
        if (length(VectorT(Face->Material->meta_SunLight_Dir))<0.1) continue;

        SkyFaces.PushBack(Face);
    }

    // Und daraus bilde eine Liste der vorkommenden/verwendeten Materials, in der jedes Material "unique" ist, also nur ein Mal vorkommt.
    ArrayT<SunT> Suns;

    for (unsigned long FaceNr=0; FaceNr<SkyFaces.Size(); FaceNr++)
    {
        const cf::SceneGraph::FaceNodeT* SkyFace=SkyFaces[FaceNr];

        // See if the material of this SkyFace is already listed among the Suns materials.
        unsigned long SunNr;
        for (SunNr=0; SunNr<Suns.Size(); SunNr++)
            if (Suns[SunNr].Material==SkyFace->Material) break;

        if (SunNr<Suns.Size()) continue;

        // It was not listed, so add it now.
        Suns.PushBackEmpty();
        Suns[SunNr].Material     =SkyFace->Material;
        Suns[SunNr].SunIrradiance=Vector3T<double>(SkyFace->Material->meta_SunLight_Irr);
        Suns[SunNr].LightRevDir  =-normalize(VectorT(SkyFace->Material->meta_SunLight_Dir), 0.0);
    }


    PatchMeshes.Clear();

    // Create the patch meshes.
    {
        unsigned long PatchMeshNr=0;
        unsigned long PatchesCount=0;

        for (unsigned long FaceNr=0; FaceNr<Map.FaceChildren.Size(); FaceNr++)
        {
            printf("%5.1f%% (f)\r", (double)FaceNr/(Map.FaceChildren.Size()+Map.OtherChildren.Size())*100.0);
            fflush(stdout);

            ArrayT< ArrayT< ArrayT<Vector3dT> > > SampleCoords;

            Map.FaceChildren[FaceNr]->CreatePatchMeshes(PatchMeshes, SampleCoords, Map.GetLightMapPatchSize());

            for (unsigned long SampleCoordsNr=0; SampleCoordsNr<SampleCoords.Size(); SampleCoordsNr++)
            {
                InitSunlight(PatchMeshes[PatchMeshNr], SampleCoords[SampleCoordsNr], Suns, SkyFaces, CaLightWorld);

                PatchesCount+=PatchMeshes[PatchMeshNr].Patches.Size();
                PatchMeshNr++;
            }
        }

        for (unsigned long OtherNr=0; OtherNr<Map.OtherChildren.Size(); OtherNr++)
        {
            printf("%5.1f%% (o)\r", (double)(Map.FaceChildren.Size()+OtherNr)/(Map.FaceChildren.Size()+Map.OtherChildren.Size())*100.0);
            fflush(stdout);

            ArrayT< ArrayT< ArrayT<Vector3dT> > > SampleCoords;

            Map.OtherChildren[OtherNr]->CreatePatchMeshes(PatchMeshes, SampleCoords, Map.GetLightMapPatchSize());

            for (unsigned long SampleCoordsNr=0; SampleCoordsNr<SampleCoords.Size(); SampleCoordsNr++)
            {
                InitSunlight(PatchMeshes[PatchMeshNr], SampleCoords[SampleCoordsNr], Suns, SkyFaces, CaLightWorld);

                PatchesCount+=PatchMeshes[PatchMeshNr].Patches.Size();
                PatchMeshNr++;
            }
        }

// THIS WAS REMOVED, BECAUSE:
// I'm in the progress to introduce a Scene Graph for Cafu.
// This means that the contents of BSP leaves will be described by an array of cf::SceneGraph::GenericNodeT* and terrains
// are not explicitly mentioned anymore. Terrain support is temporarily removed, and will be implicitly added later again.
#if 0
        for (unsigned long TerrainNr=0; TerrainNr<CaLightWorld.TerrainEntities.Size(); TerrainNr++)
        {
            printf("%5.1f%%\r", (double)TerrainNr/CaLightWorld.TerrainEntities.Size()*100.0);
            fflush(stdout);

            const TerrainT::VertexT*     TerrainVertices=CaLightWorld.TerrainEntities[TerrainNr].Terrain.GetVertices();
            const unsigned long          TerrainSize    =CaLightWorld.TerrainEntities[TerrainNr].Terrain.GetSize();
            const BoundingBox3T<double>& TerrainBB      =CaLightWorld.TerrainEntities[TerrainNr].BB;
            const FaceT&                 TerrainFace    =TerrainFaces[TerrainNr];
            const unsigned long          Step           =(TerrainSize-1)/TerrainFace.LightMapInfo.SizeS;

            if (Step==0) Error("A terrain cell is covered by multiple lightmap elements.");


            for (unsigned long t=0; t<TerrainFace.LightMapInfo.SizeT; t++)
                for (unsigned long s=0; s<TerrainFace.LightMapInfo.SizeS; s++)
                {
                    const VectorT& V1=TerrainVertices[(s  )*Step+TerrainSize*(t  )*Step];
                    const VectorT& V2=TerrainVertices[(s  )*Step+TerrainSize*(t+1)*Step];
                    const VectorT& V3=TerrainVertices[(s+1)*Step+TerrainSize*(t+1)*Step];
                    const VectorT& V4=TerrainVertices[(s+1)*Step+TerrainSize*(t  )*Step];

                    Plane3T<double> Plane1;
                    Plane3T<double> Plane2;

                    if ((s & 1)==(t & 1))
                    {
                        // The diagonal goes from lower left to upper right ( (0, 0) to (1, 1) ), yielding triangles (V1, V2, V3) and (V3, V4, V1).
                        Plane1=Plane3T<double>(V1, V2, V3, 0.0);
                        Plane2=Plane3T<double>(V3, V4, V1, 0.0);
                    }
                    else
                    {
                        // The diagonal goes from upper left to lower right ( (0, 1) to (1, 0) ), yielding triangles (V1, V2, V4) and (V2, V3, V4).
                        Plane1=Plane3T<double>(V1, V2, V4, 0.0);
                        Plane2=Plane3T<double>(V2, V3, V4, 0.0);
                    }

                    const VectorT AvgNormal      =Plane1.Normal+Plane2.Normal;
                    const double  AvgNormalLength=length(AvgNormal);

                    // Store the patches in "image order" (y-axis pointing down / towards us),
                    // not in "world order" (y-axis pointing up / away from us).
                    // This is done because the terrains base texture is stored in the same way,
                    // and so in the engine we can use the same texture coordinates for rendering.
                    // Otherwise, a separate set of texture coordinates had to be created.
                    PatchT&  Patch       =Patches[Map.Faces.Size()+TerrainNr][(TerrainFace.LightMapInfo.SizeT-1-t)*TerrainFace.LightMapInfo.SizeS+s];
                    VectorT& Patch_Normal=TerrainPatchesNormals[TerrainNr][(TerrainFace.LightMapInfo.SizeT-1-t)*TerrainFace.LightMapInfo.SizeS+s];

                    Patch.Coord     =scale(V1+V3, 0.5);   // V1+V3 or V2+V4 doesn't matter here, due to the "safety" correction below.
                    Patch_Normal    =AvgNormalLength>0.01 ? scale(AvgNormal, 1.0/AvgNormalLength) : VectorT(0.0, 0.0, 1.0); if (AvgNormalLength<=0.01) printf("INFO: AvgNormalLength<=0.01.\n");
                    Patch.InsideFace=true;

                    // Make sure that Patch.Coord is *above* the terrain (due to the "Step", it could be below!).
                    const VectorT TraceOrigin=VectorT(Patch.Coord.x, Patch.Coord.y, TerrainBB.Max.z+100.0);
                    const VectorT TraceDir   =VectorT(0.0, 0.0, TerrainBB.Min.z-TerrainBB.Max.z-200.0);
                    VB_Trace3T<double> TraceResult(1.0);

                    CaLightWorld.TerrainEntities[TerrainNr].Terrain.TraceBoundingBox(BoundingBox3T<double>(Vector3dT()), TraceOrigin, TraceDir, TraceResult);
                    Patch.Coord.z=TraceOrigin.z+TraceDir.z*TraceResult.Fraction+0.19;
                    if (TraceResult.Fraction==1.0)
                        printf("WARNING: TraceResult.Fraction==1.0 in %s at line %u! (Terrain %lu at (%lu,%lu).)\n", __FILE__, __LINE__, TerrainNr, s, t);


                    // EINSCHUB: Betrachte bei dieser Gelegenheit auch gleich den Einfall des Sonnenlichts.
                    for (unsigned long SunNr=0; SunNr<Suns.Size(); SunNr++)
                    {
                        if (dot(Patch_Normal, Suns[SunNr].LightRevDir)<=0.0) continue;

                        // Notes:
                        // 1. It is not adviseable to add any other sample points, as their validity had to be verified as Patch.Coord had been!
                        // 2. We *could* use individual normal vectors for each sample point, but that is probably not worth the effort.
                        ArrayT<VectorT> SampleCoords;

                        SampleCoords.PushBack(V1+VectorT(0.0, 0.0, 0.19));
                        SampleCoords.PushBack(V2+VectorT(0.0, 0.0, 0.19));
                        SampleCoords.PushBack(V3+VectorT(0.0, 0.0, 0.19));
                        SampleCoords.PushBack(V4+VectorT(0.0, 0.0, 0.19));
                        SampleCoords.PushBack(Patch.Coord);

                        unsigned long NrOfSkyHits=0;
                        for (unsigned long SampleNr=0; SampleNr<SampleCoords.Size(); SampleNr++)
                        {
                            // Teste, ob der Strahl SampleCoords[SampleNr]+r*(-SunLightDir) eine Face mit Sky-Texture trifft,
                            // taking terrains into account.
                            const double  r  =CaLightWorld.Map.ClipLine(SampleCoords[SampleNr], Suns[SunNr].LightRevDir, 0.1, 9999999.9, CaLightWorld.TerrainEntities.Size()>0 ? &CaLightWorld.TerrainEntities[0] : NULL);
                            const VectorT Hit=SampleCoords[SampleNr]+scale(Suns[SunNr].LightRevDir, r);

                            // Teste, ob 'Hit' in einer Face mit Sky-Texture liegt.
                            unsigned long FNr;

                            for (FNr=0; FNr<SkyFaces.Size(); FNr++)
                            {
                                const FaceT& SkyFace=Map.Faces[SkyFaces[FNr]];

                                if (SkyFace.Material!=Suns[SunNr].Material) continue;       // In diesem Durchlauf tragen nur solche SkyFaces bei, die zu Suns[SunNr] gehören.
                                if (fabs(SkyFace.Plane.GetDistance(Hit))>0.2) continue;  // Ist Hit zu weit von der SkyFace weg?

                                unsigned long VNr;

                                for (VNr=0; VNr<SkyFace.Vertices.Size(); VNr++)
                                    if (SkyFace.GetEdgePlane(VNr, 0.0).GetDistance(Hit)<-0.1) break;

                                if (VNr==SkyFace.Vertices.Size()) break;
                            }

                            if (FNr<SkyFaces.Size()) NrOfSkyHits++;
                        }

                        const double  Amount  =double(NrOfSkyHits)/double(SampleCoords.Size())*dot(Suns[SunNr].LightRevDir, Patch_Normal);
                        const double  Ambient =0.1;     // Fraction of "artificial" ambient light.
                        const VectorT SunLight=scale(Suns[SunNr].SunIrradiance, Ambient+REFLECTIVITY*Amount*(1.0-Ambient));

                        Patch.UnradiatedEnergy=Patch.UnradiatedEnergy+SunLight;
                        Patch.TotalEnergy     =Patch.TotalEnergy     +SunLight;
                        Patch.EnergyFromDir   =Patch.EnergyFromDir   +Suns[SunNr].LightRevDir*Max3(SunLight);
                    }
                }


            // For the purpose of debugging, export the terrains normal map vectors into a normal-map image.
            /* BitmapT NormalMap;

            NormalMap.SizeX=TerrainFace.LightMapInfo.SizeS;
            NormalMap.SizeY=TerrainFace.LightMapInfo.SizeT;

            for (unsigned long NormalNr=0; NormalNr<TerrainPatchesNormals[TerrainNr].Size(); NormalNr++)
            {
                const unsigned long r=(unsigned long)((TerrainPatchesNormals[TerrainNr][NormalNr].x+1.0)*0.5*255.0+0.49); if (r>255) printf("r>255.\n");
                const unsigned long g=(unsigned long)((TerrainPatchesNormals[TerrainNr][NormalNr].y+1.0)*0.5*255.0+0.49); if (g>255) printf("g>255.\n");
                const unsigned long b=(unsigned long)((TerrainPatchesNormals[TerrainNr][NormalNr].z+1.0)*0.5*255.0+0.49); if (b>255) printf("b>255.\n");
                const unsigned long a=255;

                NormalMap.Data.PushBack((a << 24) + (b << 16) + (g << 8) + (r << 0));
            }

            NormalMap.SaveToDisk("Terrain_norm.png"); */
        }
#endif
        printf("# patches allocated:%10lu\n", PatchesCount);
        printf("Patch coords and sunlight information calculated.\n");
    }

    // 3. Trotz der vielen SamplePoints für jeden Patch kann das Sonnenlicht ziemlich eckig wirken.
    //    Deshalb filtern wir es hier vorsichtig nach, indem wir für jeden Patch das gewichtete Mittel mit den acht umliegenden Patches bilden.
    //    Die vier unmittelbar anliegenden Patches werden mit 1/4 gewichtet, die vier Ecken mit 1/16.
    for (unsigned long PatchMeshNr=0; PatchMeshNr<PatchMeshes.Size(); PatchMeshNr++)
    {
        cf::PatchMeshT& PM=PatchMeshes[PatchMeshNr];

        ArrayT<VectorT> UnradEBuffer;
        ArrayT<VectorT> TotalEBuffer;
        ArrayT<VectorT> EFrDirBuffer;

        for (unsigned long PatchNr=0; PatchNr<PM.Patches.Size(); PatchNr++)
        {
            UnradEBuffer.PushBack(PM.Patches[PatchNr].UnradiatedEnergy);
            TotalEBuffer.PushBack(PM.Patches[PatchNr].TotalEnergy);
            EFrDirBuffer.PushBack(PM.Patches[PatchNr].EnergyFromDir);
        }

        for (unsigned long t=0; t<PM.Height; t++)
            for (unsigned long s=0; s<PM.Width; s++)
            {
                cf::PatchT& Patch=PM.GetPatch(s, t);

                if (!Patch.InsideFace) continue;

                VectorT UnradAverage=Patch.UnradiatedEnergy;
                VectorT TotalAverage=Patch.TotalEnergy;
                VectorT FrDirAverage=Patch.EnergyFromDir;
                double  CoveredArea =1.0;

                // Der Patch liegt in der Mitte eines 3x3-Feldes bei Koordinate (1,1).
                // Darüber stellen wir uns ein 2x2-Feld vor, dessen Mitte mit der Mitte des mittleren 3x3-Feldes (dem betrachteten Patch) zusammenfällt.
                // Aus den Überlappungen der beiden Felder ergeben sich die Gewichte für die Felder des 3x3-Feldes: Die Mitte 100%, die Ecken 25%,
                // die anderen Felder (anliegend: oben, unten, links, rechts) 50%. Da wir nur wenig filtern wollen, verkleinern wir das 2x2-Feld, indem
                // wir die Gewichte quadrieren: Mitte 100%, Ecken 6,25%, Rest 25%.
                for (char y=0; y<=2; y++)
                    for (char x=0; x<=2; x++)
                    {
                        if (x==1 && y==1) continue;     // Patch selbst ist schon dazugezählt, nur noch die umliegenden Patches betrachten

                        int Nx=int(s+x)-1;
                        int Ny=int(t+y)-1;

                        if (PM.WrapsHorz)
                        {
                            // Patches that wrap do *not* duplicate the leftmost column at the right.
                            if (Nx<             0) Nx+=PM.Width;
                            if (Nx>=int(PM.Width)) Nx-=PM.Width;
                        }

                        if (PM.WrapsVert)
                        {
                            // Patches that wrap do *not* duplicate the topmost column at the bottom.
                            if (Ny<              0) Ny+=PM.Height;
                            if (Ny>=int(PM.Height)) Ny-=PM.Height;
                        }

                        if (Nx<              0) continue;   // Linken  Rand beachten.
                        if (Nx>= int(PM.Width)) continue;   // Rechten Rand beachten.
                        if (Ny<              0) continue;   // Oberen  Rand beachten.
                        if (Ny>=int(PM.Height)) continue;   // Unteren Rand beachten.

                        if (!PM.GetPatch(Nx, Ny).InsideFace) continue;

                        const double RelevantArea=(x!=1 && y!=1) ? 0.25*0.25 : 0.5*0.5;

                        UnradAverage=UnradAverage+scale(UnradEBuffer[Ny*PM.Width+Nx], RelevantArea);
                        TotalAverage=TotalAverage+scale(TotalEBuffer[Ny*PM.Width+Nx], RelevantArea);
                        FrDirAverage=FrDirAverage+scale(EFrDirBuffer[Ny*PM.Width+Nx], RelevantArea);
                        CoveredArea+=RelevantArea;
                    }

                Patch.UnradiatedEnergy=scale(UnradAverage, 1.0/CoveredArea);
                Patch.TotalEnergy     =scale(TotalAverage, 1.0/CoveredArea);
                Patch.EnergyFromDir   =scale(FrDirAverage, 1.0/CoveredArea);
            }
    }
    printf("Sunlight smoothed.\n");
}
