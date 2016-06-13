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


double MaxAbsCoeff(const ArrayT<double>& Coeffs)
{
    if (Coeffs.Size()==0) return 0.0;

    double m=fabs(Coeffs[0]);

    for (unsigned long CoeffNr=1; CoeffNr<Coeffs.Size(); CoeffNr++)
        if (fabs(Coeffs[CoeffNr])>m) m=fabs(Coeffs[CoeffNr]);

    return m;
}


// Diese Funktion findet einen Tone-Reproduction Operator (Funktion) nach Ward97,
// anhand dessen die Energiewerte der Patches in RGB-Tripel umgewandelt werden.
void ToneReproduction(const cf::SceneGraph::BspTreeNodeT& Map)
{
    printf("\n%-50s %s\n", "*** Tone Reproduction (Ward97) ***", GetTimeSinceProgramStart());

    const unsigned long   NR_OF_SH_COEFFS=cf::SceneGraph::SHLMapManT::NrOfBands * cf::SceneGraph::SHLMapManT::NrOfBands;
    const unsigned long   NrOfBins=300;
    ArrayT<unsigned long> Bins;
    unsigned long         BinNr;

    for (BinNr=0; BinNr<NrOfBins; BinNr++) Bins.PushBack(0);

    double MinBrightness=log(0.0001);
    double MaxBrightness=MinBrightness;

    // Suche die MaxBrightness
    for (unsigned long FaceNr=0; FaceNr<Map.FaceChildren.Size(); FaceNr++)
        for (unsigned long PatchNr=0; PatchNr<Patches[FaceNr].Size(); PatchNr++)
        {
            if (!Patches[FaceNr][PatchNr].InsideFace) continue;

            double Luminance=MaxAbsCoeff(Patches[FaceNr][PatchNr].SHCoeffs_TotalTransfer);

            if (Luminance<0.0001) continue;
            double Brightness=log(Luminance);

            if (Brightness>MaxBrightness) MaxBrightness=Brightness;
        }

    printf("MinBrightness: %9.5f\n", MinBrightness);
    printf("MaxBrightness: %9.5f\n", MaxBrightness);

    // Bilde das Histogram
    for (unsigned long FaceNr=0; FaceNr<Map.FaceChildren.Size(); FaceNr++)
        for (unsigned long PatchNr=0; PatchNr<Patches[FaceNr].Size(); PatchNr++)
        {
            if (!Patches[FaceNr][PatchNr].InsideFace) continue;

            double Luminance=MaxAbsCoeff(Patches[FaceNr][PatchNr].SHCoeffs_TotalTransfer);

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
        for (unsigned long FaceNr=0; FaceNr<Map.FaceChildren.Size(); FaceNr++)
            for (unsigned long PatchNr=0; PatchNr<Patches[FaceNr].Size(); PatchNr++)
            {
                if (!Patches[FaceNr][PatchNr].InsideFace) continue;

                ArrayT<double>& TotalTransfer=Patches[FaceNr][PatchNr].SHCoeffs_TotalTransfer;
                double          Luminance    =MaxAbsCoeff(TotalTransfer);

                if (Luminance<0.0001) Luminance=0.0001;
                double Brightness=log(Luminance);

                BinNr=(unsigned long)((Brightness-MinBrightness)/(MaxBrightness-MinBrightness)*NrOfBins);
                if (BinNr>NrOfBins-1) BinNr=NrOfBins-1;

                double DisplayLuminance=pow(DisplayLuminanceMax, BinsNormSum[BinNr]);

                for (unsigned long CoeffNr=0; CoeffNr<NR_OF_SH_COEFFS; CoeffNr++)
                    TotalTransfer[CoeffNr]*=DisplayLuminance/Luminance;
            }
    }

    // Skaliere die nun vorhandenen RGB-Werte der Patches linear in den gewünschten [0, 255] Bereich.
    double Max=0;
    for (unsigned long FaceNr=0; FaceNr<Map.FaceChildren.Size(); FaceNr++)
        for (unsigned long PatchNr=0; PatchNr<Patches[FaceNr].Size(); PatchNr++)
        {
            if (!Patches[FaceNr][PatchNr].InsideFace) continue;

            for (unsigned long CoeffNr=0; CoeffNr<NR_OF_SH_COEFFS; CoeffNr++)
                if (fabs(Patches[FaceNr][PatchNr].SHCoeffs_TotalTransfer[CoeffNr])>Max) Max=fabs(Patches[FaceNr][PatchNr].SHCoeffs_TotalTransfer[CoeffNr]);
        }

    if (Max==0.0) Max=1.0;

    // The 1.0 was 255.0 for CaLight, and should probably be 0.7... here (the fabs() value of the min/max values reported by DirectLighting()).
    Max=1.0/Max;

    for (unsigned long FaceNr=0; FaceNr<Map.FaceChildren.Size(); FaceNr++)
        for (unsigned long PatchNr=0; PatchNr<Patches[FaceNr].Size(); PatchNr++)
            for (unsigned long CoeffNr=0; CoeffNr<NR_OF_SH_COEFFS; CoeffNr++)
                Patches[FaceNr][PatchNr].SHCoeffs_TotalTransfer[CoeffNr]*=Max;
}
