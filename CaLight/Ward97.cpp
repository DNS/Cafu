/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

// Ward 97 Tone Reproduction Operator
// **********************************

const double DisplayLuminanceMax=80.0;


bool HistogramCeiling(ArrayT<unsigned long>& Bins, double DeltaBin)
{
    unsigned long Total=0;
    unsigned long BinNr;

    for (BinNr=0; BinNr<Bins.Size(); BinNr++) Total+=Bins[BinNr];

    unsigned long Tolerance=(unsigned long)(0.025*Total);
    unsigned long Trimmings;

    do
    {
        Trimmings=0;
        Total    =0;
        for (BinNr=0; BinNr<Bins.Size(); BinNr++) Total+=Bins[BinNr];

        if (Total<Tolerance) return false;

        for (BinNr=0; BinNr<Bins.Size(); BinNr++)
        {
            unsigned long Ceiling=(unsigned long)(Total*DeltaBin/log(DisplayLuminanceMax));

            if (Bins[BinNr]>Ceiling)
            {
                Trimmings+=Bins[BinNr]-Ceiling;
                Bins[BinNr]=Ceiling;
            }
        }
    } while (Trimmings>Tolerance);

    return true;
}


/***************************************************************************************************************
 *** TODO: Die Luminanz sollte aus RGB-Werten anhand der CIE XYZ luminous efficiency functions berechnet werden.
 *** Vgl. dazu Paper "Computational Model of Lightness Perception in High Dynamic Range Imaging" unter
 *** http://www.mpi-inf.mpg.de/resources/hdr/lightness/, Zitat aus Kapitel "4. Computational Model":
 ***    The presented model takes an image with the relative luminance values as an input. Such values can be
 *** computed from RGB channels of an HDR image according to CIE XYZ luminous efficiency functions.
 ***************************************************************************************************************/


// Diese Funktion findet einen Tone-Reproduction Operator (Funktion) nach Ward97,
// anhand dessen die Energiewerte der Patches in RGB-Tripel umgewandelt werden.
void ToneReproduction(const CaLightWorldT& CaLightWorld)
{
    printf("\n%-50s %s\n", "*** Tone Reproduction (Ward97) ***", GetTimeSinceProgramStart());

    const unsigned long   NrOfBins=300;
    ArrayT<unsigned long> Bins;
    unsigned long         BinNr;

    for (BinNr=0; BinNr<NrOfBins; BinNr++) Bins.PushBack(0);

    double MinBrightness=log(0.0001);
    double MaxBrightness=MinBrightness;

    // Suche die MaxBrightness
    for (unsigned long PatchMeshNr=0; PatchMeshNr<PatchMeshes.Size(); PatchMeshNr++)
        for (unsigned long PatchNr=0; PatchNr<PatchMeshes[PatchMeshNr].Patches.Size(); PatchNr++)
        {
            const cf::PatchT& Patch=PatchMeshes[PatchMeshNr].Patches[PatchNr];

            if (!Patch.InsideFace) continue;

            double Luminance=Max3(Patch.TotalEnergy);

            if (Luminance<0.0001) continue;
            double Brightness=log(Luminance);

            if (Brightness>MaxBrightness) MaxBrightness=Brightness;
        }

    printf("MinBrightness: %9.5f\n", MinBrightness);
    printf("MaxBrightness: %9.5f\n", MaxBrightness);

    // Bilde das Histogram
    for (unsigned long PatchMeshNr=0; PatchMeshNr<PatchMeshes.Size(); PatchMeshNr++)
        for (unsigned long PatchNr=0; PatchNr<PatchMeshes[PatchMeshNr].Patches.Size(); PatchNr++)
        {
            const cf::PatchT& Patch=PatchMeshes[PatchMeshNr].Patches[PatchNr];

            if (!Patch.InsideFace) continue;

            double Luminance=Max3(Patch.TotalEnergy);

            if (Luminance<0.0001) Luminance=0.0001;
            double Brightness=log(Luminance);

            BinNr=(unsigned long)((Brightness-MinBrightness)/(MaxBrightness-MinBrightness)*NrOfBins);
            if (BinNr>NrOfBins-1) BinNr=NrOfBins-1;
            Bins[BinNr]++;
        }

    /***********************************************************************************************
    printf("Writing histogram file...\n");
    FILE* FilePtr=fopen("histogr.dat", "wb");

    if (FilePtr!=NULL)
    {
        fwrite(&NrOfBins, sizeof(NrOfBins), 1, FilePtr);

        for (BinNr=0; BinNr<Bins.Size(); BinNr++)
            fwrite(&Bins[BinNr], sizeof(unsigned long), 1, FilePtr);

        fclose(FilePtr);
    }
    else printf("      WARNING: Unable to write histogram data file!\n");
    ***********************************************************************************************/

    // Arbeite die Ceiling in das Histogram ein
    if (HistogramCeiling(Bins, (MaxBrightness-MinBrightness)/double(NrOfBins)))
    {
        // Bilde das Integral über Bins[0..NrOfBins-1] als einfache Summe und normalisiere
        ArrayT<double> BinsNormSum;
        unsigned long  Sum=0;

        for (BinNr=0; BinNr<Bins.Size(); BinNr++)
        {
            BinsNormSum.PushBack(Sum);
            Sum+=Bins[BinNr];
        }
        for (BinNr=0; BinNr<Bins.Size(); BinNr++) BinsNormSum[BinNr]/=double(Sum);

        // Ordne nun anhand der DisplayLuminanceMax^BinsNormSum[i] Funktion RGB-Werte zu
        for (unsigned long PatchMeshNr=0; PatchMeshNr<PatchMeshes.Size(); PatchMeshNr++)
            for (unsigned long PatchNr=0; PatchNr<PatchMeshes[PatchMeshNr].Patches.Size(); PatchNr++)
            {
                cf::PatchT& Patch=PatchMeshes[PatchMeshNr].Patches[PatchNr];

                if (!Patch.InsideFace) continue;

                double Luminance=Max3(Patch.TotalEnergy);

                if (Luminance<0.0001) Luminance=0.0001;
                double Brightness=log(Luminance);

                BinNr=(unsigned long)((Brightness-MinBrightness)/(MaxBrightness-MinBrightness)*NrOfBins);
                if (BinNr>NrOfBins-1) BinNr=NrOfBins-1;

                double DisplayLuminance=pow(DisplayLuminanceMax, BinsNormSum[BinNr]);
                Patch.TotalEnergy=scale(Patch.TotalEnergy, DisplayLuminance/Luminance);
            }
    }

    // Skaliere die nun vorhandenen RGB-Werte der Patches linear in den gewünschten [0, 255] Bereich.
    double Max=0;
    for (unsigned long PatchMeshNr=0; PatchMeshNr<PatchMeshes.Size(); PatchMeshNr++)
        for (unsigned long PatchNr=0; PatchNr<PatchMeshes[PatchMeshNr].Patches.Size(); PatchNr++)
        {
            const cf::PatchT& Patch=PatchMeshes[PatchMeshNr].Patches[PatchNr];

            if (!Patch.InsideFace) continue;

            const VectorT& RGB=Patch.TotalEnergy;

            if (RGB.x>Max) Max=RGB.x;
            if (RGB.y>Max) Max=RGB.y;
            if (RGB.z>Max) Max=RGB.z;
        }

    if (Max==0) Max=1.0;
    Max=1.0/Max;
    for (unsigned long PatchMeshNr=0; PatchMeshNr<PatchMeshes.Size(); PatchMeshNr++)
        for (unsigned long PatchNr=0; PatchNr<PatchMeshes[PatchMeshNr].Patches.Size(); PatchNr++)
        {
            cf::PatchT& Patch=PatchMeshes[PatchMeshNr].Patches[PatchNr];

            Patch.TotalEnergy=scale(Patch.TotalEnergy, Max);

            // This is the forced application of a gamma correction by 2.0 (the sqrt(x) is equivalent to pow(x, 1.0/2.0)).
            //
            // Q: Why here and not in the Cafu engine, at load time?
            // A: 1. Applying gamma to all patches at load time is expensive at every map load, implying a sub-optimal experience for the user.
            //    2. Full numeric precision is only available here. Later the patch values are rounded to and kept as unsigned chars,
            //       limiting their precision to one of only 256 possible values.
            //
            // Q: Why choose a gamma value of 2.0, and not any other value?
            // A: During rendering, lightmaps and texture images are combined by multiplication.
            //    This has a tendency to darken the overall image, because e.g. a texel value of 0.5 multiplied with a lightmap value of 0.5
            //    yields a pixel value of only 0.25. If we wanted a texel value of 0.5 and a lightmap value of 0.5 to yield a pixel value of
            //    0.5, we had to apply a gamma correction of 2.0 to both the texture images and lightmaps. This is arbitrary though, and as it
            //    is out of the question to modify the texture images anyway, we just implement a gamma correction of 2.0 for the ilghtmaps here.
            Patch.TotalEnergy.x=sqrt(Patch.TotalEnergy.x);
            Patch.TotalEnergy.y=sqrt(Patch.TotalEnergy.y);
            Patch.TotalEnergy.z=sqrt(Patch.TotalEnergy.z);

            Patch.TotalEnergy=scale(Patch.TotalEnergy, 255.0);
        }
}
