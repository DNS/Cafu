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

/****************************************/
/***                                  ***/
/*** Ca3DE Potentially Visibility Set ***/
/***                                  ***/
/****************************************/

#include <time.h>
#include <stdio.h>

#include "CaPVSWorld.hpp"
#include "Win32/Win32Console.hpp"


const time_t          ProgramStartTime=time(NULL);
Win32ConsoleT         Win32Console;
ArrayT<SuperLeafT>    SuperLeaves;
ArrayT<unsigned long> SuperLeavesPVS;
unsigned long         MasterSuperLeafNr;


// This function determines the adjacency cell graph of the 'SuperLeaves'.
// It is filled-in and stored in the 'Neighbours' members of the 'SuperLeaves'.
void DetermineAdjacencyGraph()
{
    Win32Console.FunctionID("SuperLeaves Adjacency Graph");

    for (unsigned long SL1Nr=0; SL1Nr<SuperLeaves.Size(); SL1Nr++)
        for (unsigned long SL2Nr=0; SL2Nr<SuperLeaves.Size(); SL2Nr++)
        {
            if (SL1Nr==SL2Nr || !BoundingBoxTest(SuperLeaves[SL1Nr].BB, SuperLeaves[SL2Nr].BB)) continue;

            // Jedes Portal des ersten Leafs gegen jedes Portal des zweiten Leafs
            // pr�fen und testen, ob sie sich schneiden (dann sind sie durchl�ssig).
            // BEACHTE: SuperLeaves haben *alle* Portale ihrer Leaves!
            // D.h. es k�nnen sich nicht nur Portale auf ihrer konvexen H�lle befinden, sondern auch "innen drin".
            // Wegen der Konvexit�t der SuperLeaves d�rfte das aber keine Rolle spielen und folgendes m��te trotzdem korrekt funktionieren.
            for (unsigned long Portal1Nr=0; Portal1Nr<SuperLeaves[SL1Nr].Portals.Size(); Portal1Nr++)
                for (unsigned long Portal2Nr=0; Portal2Nr<SuperLeaves[SL2Nr].Portals.Size(); Portal2Nr++)
                {
                    if (!PolygonOverlap(SuperLeaves[SL1Nr].Portals[Portal1Nr], SuperLeaves[SL2Nr].Portals[Portal2Nr])) continue;

                    // Folgender Check ist zwar redundant, aber zumindest sinnvoll.
                    // Da diese Funktion sowieso schnell genug ist, lasse ihn nicht weg!
                    if (PolygonWhatSide(SuperLeaves[SL1Nr].Portals[Portal1Nr], SuperLeaves[SL2Nr].Portals[Portal2Nr].Plane)!=-3) continue;

                    ArrayT<PolygonT> NewPortals;
                    PolygonChopUp(SuperLeaves[SL2Nr].Portals[Portal2Nr], SuperLeaves[SL1Nr].Portals[Portal1Nr], NewPortals);

                    SuperLeaves[SL1Nr].Neighbours.PushBackEmpty();
                    SuperLeaves[SL1Nr].Neighbours[SuperLeaves[SL1Nr].Neighbours.Size()-1].SuperLeafNr=SL2Nr;
                    SuperLeaves[SL1Nr].Neighbours[SuperLeaves[SL1Nr].Neighbours.Size()-1].SubPortal  =NewPortals[NewPortals.Size()-1];
                }
        }

    printf("done.\n");
}


// Markiere das SuperLeaf 'SLNr' als von 'MasterSuperLeafNr' aus sichtbar.
inline void FlagVisible(unsigned long SLNr)
{
    const unsigned long PVSTotalBitNr=MasterSuperLeafNr*SuperLeaves.Size()+SLNr;
    const unsigned long PVS_W32_Nr   =PVSTotalBitNr >> 5;
    const unsigned long PVSBitMask   =1 << (PVSTotalBitNr & 31);

    SuperLeavesPVS[PVS_W32_Nr]|=PVSBitMask;
}


// Bestimme die Sichtpyramide (Frustum), die eine Lichtquelle (LightSource) durch ein Loch (Hole) wirft.
// Dabei leuchtet die Lichtquelle in die entgegengesetzte(!!) Richtung ihres Normalenvektors, und das Loch
// l��t auch nur Licht in der Gegenrichtung seines Normalenvektors durch. W�rde man das Loch zur Lichtquelle
// und umgekehrt machen (PolygonMirror), w�re das Frustum das gleiche, die Ebenen w�ren aber gespiegelt!
// Voraussetzung: Die Ebene der Lichtquelle schneidet nicht das Loch und umgekehrt. Dieser Fall wird nicht
// abgedeckt bzw. ist gar nicht durchdacht. Da die Leaves konvex sind usw. wird dies aber immer eingehalten!
inline void FindFrustum(const PolygonT& LightSource, const PolygonT& Hole, ArrayT<PlaneT>& Frustum)
{
    Frustum.Clear();   // Frustum.ClearForOverwrite();

    unsigned long V2=Hole.Vertices.Size()-1;
    for(unsigned long V3=0; V3<Hole.Vertices.Size(); V3++)
    {
        for (unsigned long V1=0; V1<LightSource.Vertices.Size(); V1++)
        {
            // Eigentlich w�rde ich hier gerne folgenden Wunsch-Code schreiben:
            //     try
            //     {
            //         PlaneT FrustumPlane(Hole.Vertices[V2], LightSource.Vertices[V1], Hole.Vertices[V3]);
            //
            //         // ...
            //     }
            //     catch (DivisionByZero) { }  // Nicht m�gliche FrustumPlanes einfach ignorieren.
            // Aus irgendeinem Grund ist die Verwendung oder das Fangen der DivisionByZero-Exception aber sehr langsam.
            // Deshalb rolle ich lieber den PlaneT-Konstruktor aus, um ohne dieses Exception-Handling auszukommen.
            // Das Programm wird *deutlich* schneller, ca. Faktor 1,5. Ob das eine Schw�che des Watcom-Compilers ist??
            VectorT Normal(VectorCross(Hole.Vertices[V3]-Hole.Vertices[V2], LightSource.Vertices[V1]-Hole.Vertices[V2]));
            double  NLength=VectorLength(Normal);

            if (NLength<0.1) continue;
            Normal=VectorScale(Normal, 1.0/NLength);

            PlaneT FrustumPlane(Normal, VectorDot(Hole.Vertices[V2], Normal));

            // Diese neue FrustumPlane nur dann akzeptieren, wenn das Hole auf ihrer Vorderseite liegt
            // (konstruktionsbedingt sowieso der Fall!) und die LightSource auf ihrer R�ckseite liegt.
            // Wenn eine Edge des Hole in der Ebene der LightSource liegt, darf die LightSource
            // auch in der FrustumPlane liegen.
            if (PolygonWhatSide(LightSource, FrustumPlane)<0)
            {
                Frustum.PushBack(FrustumPlane);
                break;
            }
        }

        V2=V3;
    }

    // Rollen vertauschen: Das Loch sei nun die Lichtquelle, und die Lichtquelle das Loch! Siehe Skizze!
    V2=LightSource.Vertices.Size()-1;
    for(V3=0; V3<LightSource.Vertices.Size(); V3++)
    {
        for (unsigned long V1=0; V1<Hole.Vertices.Size(); V1++) // Optimize: Check if edges are in already existing frustum planes!
        {
            // Eigentlich w�rde ich hier gerne folgenden Wunsch-Code schreiben:
            //     try
            //     {
            //         PlaneT FrustumPlane(LightSource.Vertices[V2], Hole.Vertices[V1], LightSource.Vertices[V3]);
            //
            //         // ...
            //     }
            //     catch (DivisionByZero) { }  // Nicht m�gliche Ebenen einfach ignorieren.
            // Aus irgendeinem Grund ist die Verwendung oder das Fangen der DivisionByZero-Exception aber sehr langsam.
            // Deshalb rolle ich lieber den PlaneT-Konstruktor aus, um ohne dieses Exception-Handling auszukommen.
            // Das Programm wird *deutlich* schneller, ca. Faktor 1,5. Ob das eine Schw�che des Watcom-Compilers ist??
            VectorT Normal(VectorCross(LightSource.Vertices[V3]-LightSource.Vertices[V2], Hole.Vertices[V1]-LightSource.Vertices[V2]));
            double  NLength=VectorLength(Normal);

            if (NLength<0.1) continue;
            Normal=VectorScale(Normal, 1.0/NLength);

            PlaneT FrustumPlane(Normal, VectorDot(LightSource.Vertices[V2], Normal));

            // Diese neue FrustumPlane nur dann akzeptieren, wenn die LightSource auf ihrer R�ckseite
            // liegt (konstruktionsbedingt sowieso der Fall!) und das Hole auf ihrer Vorderseite liegt.
            // Wenn eine Edge der LightSource in der Ebene des Holes liegt, darf das Hole
            // auch in der FrustumPlane liegen.
            const signed char Side=PolygonWhatSide(Hole, FrustumPlane);    // Wegen dem Rollentausch ist die Orientierung f�r diesen Test falsch, ...

            if (Side==1 || Side==-3)
            {
                Frustum.PushBack(FrustumPlane);                           // ...im Gesamten aber wieder richtig!
                break;
            }
        }

        V2=V3;
    }
}


// Betrete das SuperLeaf 'SLNr' durch das 'EnteringPortal' des vorhergehenden SuperLeafs.
void FindVisibleLeaves(unsigned long SLNr, const PolygonT& MasterPortal, const PolygonT& EnteringPortal)
{
    FlagVisible(SLNr);

    // Das Frustum MasterPortal --> EnteringPortal bestimmen.
    ArrayT<PlaneT> Frustum;
    FindFrustum(MasterPortal, EnteringPortal, Frustum);

    for (unsigned long NeighbourNr=0; NeighbourNr<SuperLeaves[SLNr].Neighbours.Size(); NeighbourNr++)
    {
        PolygonT NextPortal=SuperLeaves[SLNr].Neighbours[NeighbourNr].SubPortal;

        // Abk�rzung: Gleiche Ebene? Dann weiter mit dem n�chsten Portal!
        if (abs(PolygonWhatSide(NextPortal, EnteringPortal.Plane))==3) continue;

        // Clippe 'NextPortal' gegen das Frustum MasterPortal --> EnteringPortal.
        static PolygonT ClipPortal;

        for (unsigned long FrustumNr=0; FrustumNr<Frustum.Size(); FrustumNr++)
        {
            const signed char Side=PolygonWhatSide(NextPortal, Frustum[FrustumNr]);

                 if (Side==2) PolygonSplit(NextPortal, Frustum[FrustumNr], NextPortal, ClipPortal);
            else if (Side!=1) break;
        }
        if (FrustumNr<Frustum.Size()) continue;

        // Clippe 'MasterPortal' gegen das Frustum NextPortal --> EnteringPortal.
        // Um die Portale nicht spiegeln zu m�ssen, das gespiegelte Frustum benutzen!
        ArrayT<PlaneT> Frustum2;
        FindFrustum(EnteringPortal, NextPortal, Frustum2);

        PolygonT MP=MasterPortal;

        for (FrustumNr=0; FrustumNr<Frustum2.Size(); FrustumNr++)
        {
            const signed char Side=PolygonWhatSide(MP, Frustum2[FrustumNr]);

                 if (Side== 2) PolygonSplit(MP, Frustum2[FrustumNr], ClipPortal, MP);
            else if (Side!=-1) break;
        }
        if (FrustumNr<Frustum2.Size()) continue;

        FindVisibleLeaves(SuperLeaves[SLNr].Neighbours[NeighbourNr].SuperLeafNr, MP, NextPortal);
    }
}


void BuildPVS()
{
    Win32Console.FunctionID("Potentially Visibility Set");

    // F�r jedes SuperLeaf das PVS bestimmen.
    for (MasterSuperLeafNr=0; MasterSuperLeafNr<SuperLeaves.Size(); MasterSuperLeafNr++)
    {
        Win32Console.RefreshStatusLine((double)MasterSuperLeafNr/SuperLeaves.Size());

        FlagVisible(MasterSuperLeafNr);

        // F�r den alten Algorithmus war hier vermerkt, da� "outer leaves" keine Neighbours/Portals haben,
        // wegen der Eigenschaften von Portalize() und FillInside() des CaBSP Programms.
        // Der neue Algorithmus verwendet SuperLeaves, bei denen �berhaupt nicht zwischen "inner" und "outer" unterschieden wird.
        // Alles was z�hlt, sind die Portale. Deshalb setzen sich SuperLeaves korrekt aus beliebigen Leaves eines Sub-Trees zusammen.
        for (unsigned long NeighbourNr=0; NeighbourNr<SuperLeaves[MasterSuperLeafNr].Neighbours.Size(); NeighbourNr++)
        {
            const unsigned long NeighbourSLNr=SuperLeaves[MasterSuperLeafNr].Neighbours[NeighbourNr].SuperLeafNr;
            const PolygonT&     MasterPortal =SuperLeaves[MasterSuperLeafNr].Neighbours[NeighbourNr].SubPortal;

            // Der unmittelbare Nachbar ist auf jeden Fall sichtbar.
            FlagVisible(NeighbourSLNr);

            for (unsigned long NNr=0; NNr<SuperLeaves[NeighbourSLNr].Neighbours.Size(); NNr++)
            {
                const unsigned long NeighboursNeighbourSLNr=SuperLeaves[NeighbourSLNr].Neighbours[NNr].SuperLeafNr;
                const PolygonT&     EnteringPortal         =SuperLeaves[NeighbourSLNr].Neighbours[NNr].SubPortal;

                if (abs(PolygonWhatSide(MasterPortal, EnteringPortal.Plane))==3) continue;

                FindVisibleLeaves(NeighboursNeighbourSLNr, MasterPortal, EnteringPortal);
            }
        }
    }

    printf("SuperLeavesPVS      :       done\n");
}


void WriteLogFileEntry(const char* WorldPathName, unsigned long CheckSum)
{
    FILE* LogFile=fopen("CaPVS.log", "a");

    if (LogFile)
    {
        SYSTEMTIME    SystemTime;
        char          WorldName[_MAX_FNAME];
        char          ComputerName[MAX_COMPUTERNAME_LENGTH+1]="unknown";
        unsigned long LengthCN  =MAX_COMPUTERNAME_LENGTH+1;
        unsigned long RunningSec=(unsigned long)difftime(time(NULL), ProgramStartTime);

        GetLocalTime(&SystemTime);
        GetComputerName(ComputerName, &LengthCN);
        _splitpath(WorldPathName, NULL, NULL, WorldName, NULL);

        // Date, Time, WorldName, TimeForCompletion on [ComputerName]
        fprintf(LogFile, "%02u.%02u.%02u %2u:%02u %-16s%3u:%02u:%02u [%-16s] %10u\n",
            SystemTime.wDay, SystemTime.wMonth, SystemTime.wYear % 100, SystemTime.wHour, SystemTime.wMinute,
            WorldName, RunningSec/3600, (RunningSec/60) % 60, RunningSec % 60, ComputerName, CheckSum);
        fclose(LogFile);
    }
}


void Usage()
{
    printf("\n");
    printf("USAGE: CaPVS Path/WorldName.cw [OPTIONS]\n");
    printf("\n");
    printf("OPTIONS:\n");
    printf("-maxRecDepthSL n : Leaves that are stored in the BSP tree and have a greater\n");
    printf("                   depth than this value get combined in a common SuperLeaf\n");
    printf("                   that is created at this depth value.\n");
    printf("-minAreaSL a     : BSP sub-trees whose non-detail faces have a common area of\n");
    printf("                   less than this value get combined in a SuperLeaf.\n");
    printf("-onlySLs         : Do not compute the PVS, just print out how many SuperLeaves\n");
    printf("                   would be created. Used for estimating the above parameter\n");
    printf("                   values (speed vs. quality).\n");
    printf("\n");

    exit(1);
}


int main(int ArgC, const char* ArgV[])
{
    unsigned long MaxRecDepthSL  =0xFFFFFFFF;
    double        MinAreaSL      =0.0;
    bool          OnlySuperLeaves=false;

    Win32Console.InitScreen("Ca3DE Potentially Visibility Set Utility, Version 04", __DATE__);

    if (ArgC<2) Usage();

    for (int ArgNr=2; ArgNr<ArgC; ArgNr++)
    {
        if (!stricmp(ArgV[ArgNr], "-maxRecDepthSL"))
        {
            ArgNr++;
            if (ArgNr>=ArgC) Usage();

            MaxRecDepthSL=atoi(ArgV[ArgNr]);
            printf("maxRecDepthSL == %u\n", MaxRecDepthSL);
        }
        else if (!stricmp(ArgV[ArgNr], "-minAreaSL"))
        {
            ArgNr++;
            if (ArgNr>=ArgC) Usage();

            MinAreaSL=atof(ArgV[ArgNr]);
            if (MinAreaSL<0.0) MinAreaSL=0.0;
            printf("minAreaSL     == %.3f\n", MinAreaSL);
        }
        else if (!stricmp(ArgV[ArgNr], "-onlySLs"))
        {
            OnlySuperLeaves=true;
        }
        else
        {
            printf("Unknown option '%s'.\n", ArgV[ArgNr]);
            Usage();
        }
    }

    try
    {
        printf("Load World %s\n", ArgV[1]);

        CaPVSWorldT* CaPVSWorld=new CaPVSWorldT(ArgV[1], MaxRecDepthSL, MinAreaSL);

        // Robustness against the 'sharp wedge' problem is granted:
        // DetermineAdjacencyGraph() is the only place where PolygonOverlap() is called, but only once on *input* portals.
        // Everywhere else (especially in BuildPVS() and its sub-functions), only PolygonSplit() and PolygonWhatSide() are called.
        CaPVSWorld->CreateSuperLeaves(SuperLeaves);
        if (OnlySuperLeaves) return 0;
        DetermineAdjacencyGraph();

        // SuperLeavesPVS erzeugen und zur�cksetzen (v�llige Blindheit).
        SuperLeavesPVS.PushBackEmpty((SuperLeaves.Size()*SuperLeaves.Size()+31)/32);
        for (unsigned long Vis=0; Vis<SuperLeavesPVS.Size(); Vis++) SuperLeavesPVS[Vis]=0;

        // Berechne das PVS der SuperLeaves.
        BuildPVS();

        // 'SuperLeavesPVS' ins PVS der 'CaPVSWorld' �bertragen.
        CaPVSWorld->StorePVS(SuperLeaves, SuperLeavesPVS);

        // Drucke Statistiken aus und erhalte eine Pr�fsumme zur�ck.
        unsigned long CheckSum=CaPVSWorld->GetChecksumAndPrintStats();

        // Speichere die World zur�ck auf Disk.
        Win32Console.FunctionID("Save World %s", ArgV[1]);
        CaPVSWorld->SaveToDisk(ArgV[1]);

        delete CaPVSWorld;
        WriteLogFileEntry(ArgV[1], CheckSum);
        printf("COMPLETED.\n");
    }
    catch (const WorldT::LoadErrorT& E) { Win32Console.Error(E.Msg); }
    catch (const WorldT::SaveErrorT& E) { Win32Console.Error(E.Msg); }

    return 0;
}
