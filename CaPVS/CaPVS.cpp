/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/***************************************/
/***                                 ***/
/*** Cafu Potentially Visibility Set ***/
/***                                 ***/
/*** To iterate is human.            ***/
/*** To recurse, divine.             ***/
/***   -- L. Peter Deutsch           ***/
/***                                 ***/
/*** Denn die einen sind im Dunkeln  ***/
/*** und die andern sind im Licht    ***/
/*** und man siehet die im Lichte    ***/
/*** die im Dunkeln sieht man nicht. ***/
/***   -- Bertolt Brecht             ***/
/***                                 ***/
/***************************************/

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#define _stricmp strcasecmp
#endif

#include <time.h>
#include <stdio.h>

#include "CaPVSWorld.hpp"
#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/ConsoleInterpreter.hpp"
#include "ConsoleCommands/ConsoleStdout.hpp"
#include "FileSys/FileManImpl.hpp"
#include "GuiSys/GuiResources.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/MaterialManagerImpl.hpp"
#include "Models/ModelManager.hpp"
#include "ClipSys/CollisionModelMan_impl.hpp"


static cf::ConsoleStdoutT ConsoleStdout;
cf::ConsoleI* Console=&ConsoleStdout;

static cf::FileSys::FileManImplT FileManImpl;
cf::FileSys::FileManI* cf::FileSys::FileMan=&FileManImpl;

static cf::ClipSys::CollModelManImplT CCM;
cf::ClipSys::CollModelManI* cf::ClipSys::CollModelMan=&CCM;

ConsoleInterpreterI* ConsoleInterpreter=NULL;
MaterialManagerI*    MaterialManager   =NULL;


const time_t ProgramStartTime=time(NULL);

// Returns a string with the elapsed time since program start.
// The string is in the format "hh:mm:ss".
static const char* GetTimeSinceProgramStart()
{
    const unsigned long TotalSec=(unsigned long)difftime(time(NULL), ProgramStartTime);
    const unsigned long Sec     =TotalSec % 60;
    const unsigned long Min     =(TotalSec/60) % 60;
    const unsigned long Hour    =TotalSec/3600;

    static char TimeString[24];
    sprintf(TimeString, "%2lu:%2lu:%2lu", Hour, Min, Sec);

    return TimeString;
}


ArrayT<SuperLeafT>              SuperLeaves;
ArrayT< BoundingBox3T<double> > SuperLeavesBBs;
ArrayT<unsigned long>           SuperLeavesPVS;


// Records in 'SuperLeavesPVS' that we can see from 'FromSL' to 'ToSL'.
inline void FlagVisible(unsigned long FromSL, unsigned long ToSL)
{
    const unsigned long PVSTotalBitNr=FromSL*SuperLeaves.Size()+ToSL;
    const unsigned long PVS_W32_Nr   =PVSTotalBitNr >> 5;
    const unsigned long PVSBitMask   =1 << (PVSTotalBitNr & 31);

    SuperLeavesPVS[PVS_W32_Nr]|=PVSBitMask;
}


// Returns 'true' if the 'SuperLeavesPVS' has a record that we can see from 'FromSL' to 'ToSL', 'false' otherwise.
inline bool IsVisible(unsigned long FromSL, unsigned long ToSL)
{
    const unsigned long PVSTotalBitNr=FromSL*SuperLeaves.Size()+ToSL;

    return bool((SuperLeavesPVS[PVSTotalBitNr >> 5] >> (PVSTotalBitNr & 31)) & 1);
}


// Returns how many other SuperLeaves each SuperLeaf can see in average.
// The lower bound is 1.0 (each SuperLeaf can only see itself).
// The upper bound is the number of SuperLeaves (each SuperLeaf can see all other SuperLeaves, including itself).
double GetAverageVisibility()
{
    unsigned long VisCount=0;

    for (unsigned long SL1=0; SL1<SuperLeaves.Size(); SL1++)
        for (unsigned long SL2=0; SL2<SuperLeaves.Size(); SL2++)
            if (IsVisible(SL1, SL2))
                VisCount++;

    return double(VisCount)/SuperLeaves.Size();
}


// This function determines the adjacency cell graph of the 'SuperLeaves'.
// It is filled-in and stored in the 'Neighbours' members/components of the 'SuperLeaves'.
// After this function was called, all components of all 'SuperLeaves' are completely filled-in.
void DetermineAdjacencyGraph()
{
    for (unsigned long SL1Nr=0; SL1Nr<SuperLeaves.Size(); SL1Nr++)
        for (unsigned long SL2Nr=0; SL2Nr<SuperLeaves.Size(); SL2Nr++)
        {
            if (SL1Nr==SL2Nr || !SuperLeaves[SL1Nr].BB.GetEpsilonBox(MapT::RoundEpsilon).Intersects(SuperLeaves[SL2Nr].BB)) continue;

            // Jedes Portal des ersten Leafs gegen jedes Portal des zweiten Leafs
            // prüfen und testen, ob sie sich schneiden (dann sind sie durchlässig).
            // BEACHTE: SuperLeaves haben *alle* Portale von *allen* ihrer Leaves!
            // D.h. es können sich nicht nur Portale auf ihrer konvexen Hülle befinden, sondern auch "innen drin".
            // Wegen der Konvexität der SuperLeaves dürfte das aber keine Rolle spielen und folgendes müßte trotzdem korrekt funktionieren.
            for (unsigned long Portal1Nr=0; Portal1Nr<SuperLeaves[SL1Nr].Portals.Size(); Portal1Nr++)
                for (unsigned long Portal2Nr=0; Portal2Nr<SuperLeaves[SL2Nr].Portals.Size(); Portal2Nr++)
                {
                    if (!SuperLeaves[SL1Nr].Portals[Portal1Nr].Overlaps(SuperLeaves[SL2Nr].Portals[Portal2Nr], false, MapT::RoundEpsilon)) continue;

                    // Folgender Check ist zwar redundant, aber zumindest sinnvoll.
                    // Da diese Funktion sowieso schnell genug ist, lasse ihn nicht weg!
                    if (SuperLeaves[SL1Nr].Portals[Portal1Nr].WhatSide(SuperLeaves[SL2Nr].Portals[Portal2Nr].Plane, MapT::RoundEpsilon)!=Polygon3T<double>::InMirrored) continue;

                    ArrayT< Polygon3T<double> > NewPortals;
                    SuperLeaves[SL1Nr].Portals[Portal1Nr].GetChoppedUpAlong(SuperLeaves[SL2Nr].Portals[Portal2Nr], MapT::RoundEpsilon, NewPortals);

                    SuperLeaves[SL1Nr].Neighbours.PushBackEmpty();
                    SuperLeaves[SL1Nr].Neighbours[SuperLeaves[SL1Nr].Neighbours.Size()-1].SuperLeafNr=SL2Nr;
                    SuperLeaves[SL1Nr].Neighbours[SuperLeaves[SL1Nr].Neighbours.Size()-1].SubPortal  =NewPortals[NewPortals.Size()-1];
                }
        }

    printf("SLs Adjacency Graph :       done\n");
}


// This function computes for each SuperLeaf the bounding box over its 'SubPortals'.
// Such bounding boxes are useful when a SuperLeaf is considered in its role as a "target" SuperLeaf.
// The results are stored in the 'SuperLeavesBBs', which is assumed to be empty before this function is called.
// SuperLeaves that have no (sub-)portals (and thus, no neighbours), get the default bounding box assigned.
void ComputeSuperLeavesBBs()
{
    for (unsigned long SL=0; SL<SuperLeaves.Size(); SL++)
    {
        SuperLeavesBBs.PushBackEmpty();

        if (SuperLeaves[SL].Neighbours.Size()==0) continue;

        SuperLeavesBBs[SL]=BoundingBox3T<double>(SuperLeaves[SL].Neighbours[0].SubPortal.Vertices);

        for (unsigned long NeighbourNr=1; NeighbourNr<SuperLeaves[SL].Neighbours.Size(); NeighbourNr++)
            SuperLeavesBBs[SL].Insert(SuperLeaves[SL].Neighbours[NeighbourNr].SubPortal.Vertices);
    }

    printf("SLs Bounding Boxes  :       done\n");
}


// Determines the trivial visibility (recorded in the 'SuperLeavesPVS').
// That is, each SuperLeaf can see itself as well as its immediate neighbours.
void DetermineTrivialVisibility()
{
    for (unsigned long SL=0; SL<SuperLeaves.Size(); SL++)
    {
        // Sich selbst als sichtbar markieren.
        FlagVisible(SL, SL);

        // Und die unmittelbaren Nachbarn als sichtbar markieren.
        for (unsigned long NeighbourNr=0; NeighbourNr<SuperLeaves[SL].Neighbours.Size(); NeighbourNr++)
        {
            // This is the only place where we ever omit flagging the mutual "vice-versa" visibility:
            // FlagVisible(SuperLeaves[SL].Neighbours[NeighbourNr].SuperLeafNr, SL);
            // It's just not needed: The 'SuperLeavesPVS' matrix *must* be symmetric anyway.
            FlagVisible(SL, SuperLeaves[SL].Neighbours[NeighbourNr].SuperLeafNr);
        }
    }

    printf("Trivial Visibility  : %10.5f\n", GetAverageVisibility());
}


// Determines a simple pre-PVS by sampling "random" rays (and stores it in 'SuperLeavesPVS').
// The result is a subset of the analytically determined, conservative, "exact" PVS.
// This function does not handle the fact that SuperLeaves can see themselves.
// Thus, calling 'DetermineTrivialVisibility()' before (or after) calling this function is required.
void DetermineRayPresampledVisibility(const CaPVSWorldT* CaPVSWorld)
{
    ArrayT< ArrayT<VectorT> > SuperLeavesCenters;
    ArrayT< ArrayT<bool   > > SuperLeavesCenterIsValid;

    for (unsigned long SL=0; SL<SuperLeaves.Size(); SL++)
    {
        SuperLeavesCenters      .PushBackEmpty();
        SuperLeavesCenterIsValid.PushBackEmpty();

        for (unsigned long NbNr=0; NbNr<SuperLeaves[SL].Neighbours.Size(); NbNr++)
        {
            const Polygon3T<double>& SubPortal=SuperLeaves[SL].Neighbours[NbNr].SubPortal;
            VectorT                  Center   =SubPortal.Vertices[0];

            for (unsigned long VertexNr=1; VertexNr<SubPortal.Vertices.Size(); VertexNr++) Center=Center+SubPortal.Vertices[VertexNr];
            Center=scale(Center, 1.0/double(SubPortal.Vertices.Size()))+scale(SubPortal.Plane.Normal, 0.2);

            // Not really necessary, but add a little check to see if everything is in order.
            // (Or, more precisely, assert that the 'Center' is really "inside" the SuperLeaf.)
            const unsigned long LeafNr=CaPVSWorld->WhatLeaf(Center);
            unsigned long LeafSetNr;

            for (LeafSetNr=0; LeafSetNr<SuperLeaves[SL].LeafSet.Size(); LeafSetNr++)
                if (LeafNr==SuperLeaves[SL].LeafSet[LeafSetNr]) break;

            SuperLeavesCenters      [SL].PushBack(Center);
            SuperLeavesCenterIsValid[SL].PushBack(LeafSetNr<SuperLeaves[SL].LeafSet.Size());

            // if (LeafSetNr>=SuperLeaves[SL1].LeafSet.Size()) printf("WARNING: Center not in its SuperLeaf. TODO: Print more extensive diagnostics.\n");
        }
    }

    for (unsigned long SL1=0; SL1+1<SuperLeaves.Size(); SL1++)
    {
        printf("%5.1f%%\r", (double)SL1/SuperLeaves.Size()*100.0);
        fflush(stdout);

        for (unsigned long SL2=SL1+1; SL2<SuperLeaves.Size(); SL2++)
        {
            if (IsVisible(SL1, SL2)) continue;

            for (unsigned long Nb1Nr=0; Nb1Nr<SuperLeaves[SL1].Neighbours.Size(); Nb1Nr++)
            {
                if (!SuperLeavesCenterIsValid[SL1][Nb1Nr]) continue;

                for (unsigned long Nb2Nr=0; Nb2Nr<SuperLeaves[SL2].Neighbours.Size(); Nb2Nr++)
                {
                    if (!SuperLeavesCenterIsValid[SL2][Nb2Nr]) continue;

                    const VectorT& Center1=SuperLeavesCenters[SL1][Nb1Nr];
                    const VectorT& Center2=SuperLeavesCenters[SL2][Nb2Nr];

                    // This ClipLine() only considers faces, but not BPs, terrains, etc.
                    if (CaPVSWorld->ClipLine(Center1, Center2-Center1)==1.0)
                    {
                        FlagVisible(SL1, SL2);
                        FlagVisible(SL2, SL1);
                        goto DoneWithSL2;
                    }
                }
            }

            DoneWithSL2:;
        }
    }

    printf("Estimated PVS       : %10.5f\n", GetAverageVisibility());
}


// Diese Funktion bestimmt die Sichtpyramide ('Frustum'), die eine Lichtquelle 'LightSource' durch ein Loch 'Hole' wirft.
//
// Dabei leuchtet die 'LightSource' in die *entgegengesetzte* Richtung ihres Normalenvektors,
// und das 'Hole' läßt auch nur Licht in der Gegenrichtung seines Normalenvektors durch.
// Beides ist sinnvoll, denn sowohl 'LightSource' als auch 'Hole' sind idR Portale von Leaves.
// Beachte, daß die Normalenvektoren von Portalen stets zur *Mitte* ihrer Leaves hin zeigen (nicht nach außen wie bei Brushes).
// Die 'LightSource' ist dann ein Portal des vorangegangenen Leafs, durch das das aktuelle Leaf betreten wird.
// Das 'Hole' ist ein Portal des aktuellen Leafs zum nächsten Leaf.
// Bemerkung: Würde man das Loch zur Lichtquelle und umgekehrt machen (PolygonMirror),
// wäre das Frustum das gleiche, dessen Ebenen wären aber gespiegelt!
//
// Es wird vorausgesetzt, daß 'LightSource' und 'Hole' gültige Polygone sind.
// Wenn daraus erfolgreich ein Frustum konstruiert werden kann, wird dieses zurückgegeben und es gilt 'Frustum.Size()>0'.
// Andernfalls scheitert die Funktion und es wird ein Frustum der Größe 0 zurückgegeben ('Frustum.Size()==0').
// Die Voraussetzung für den Erfolg dieser Funktion ist eine - in unserem Sinne - "vernünftige" Anordnung der beiden Polygone:
// a) Das 'Hole' muß komplett auf der (Licht emittierenden) Rückseite der 'LightSource'-Ebene liegen.
// b) Die 'LightSource' muß komplett auf der (lichtdurchlässigen) Vorderseite der 'Hole'-Ebene liegen.
// Beachte, daß im allgemeinen bzw. erweiterten Sinne andere Frustren durchaus auch sinnvoll sein können,
// z.B. wenn die 'LightSource' und das 'Hole' sich schneiden.
// Solche Fälle betrachten wir jedoch als ungültig und sie führen zum Scheitern der Funktion.
inline void FindFrustum(const Polygon3T<double>& LightSource, const Polygon3T<double>& Hole, ArrayT< Plane3T<double> >& Frustum)
{
    Frustum.Overwrite();

    if (Hole.WhatSideSimple(LightSource.Plane, MapT::RoundEpsilon)!=Polygon3T<double>::Back) return;
    if (LightSource.WhatSideSimple(Hole.Plane, MapT::RoundEpsilon)!=Polygon3T<double>::Front) return;

    unsigned long V2=Hole.Vertices.Size()-1;
    unsigned long V3;

    for (V3=0; V3<Hole.Vertices.Size(); V3++)
    {
        for (unsigned long V1=0; V1<LightSource.Vertices.Size(); V1++)
        {
            // Eigentlich würde ich hier gerne folgenden Wunsch-Code schreiben:
            //     try
            //     {
            //         Plane3T<double> FrustumPlane(Hole.Vertices[V2], LightSource.Vertices[V1], Hole.Vertices[V3]);
            //
            //         // ...
            //     }
            //     catch (DivisionByZero) { }  // Nicht mögliche FrustumPlanes einfach ignorieren.
            // Aus irgendeinem Grund ist die Verwendung oder das Fangen der DivisionByZero-Exception aber sehr langsam.
            // Deshalb rolle ich lieber den Plane3T<double>-Konstruktor aus, um ohne dieses Exception-Handling auszukommen.
            // Das Programm wird *deutlich* schneller, ca. Faktor 1,5. Ob das eine Schwäche des Watcom-Compilers ist??
            VectorT Normal(cross(Hole.Vertices[V3]-Hole.Vertices[V2], LightSource.Vertices[V1]-Hole.Vertices[V2]));
            double  NLength=length(Normal);

            if (NLength<MapT::RoundEpsilon) continue;
            Normal=scale(Normal, 1.0/NLength);

            Plane3T<double> FrustumPlane(Normal, dot(Hole.Vertices[V2], Normal));

            // Diese neue FrustumPlane nur dann akzeptieren, wenn das Hole auf ihrer Vorderseite liegt
            // (konstruktionsbedingt sowieso der Fall!) und die LightSource auf ihrer Rückseite liegt.
            // Wenn eine Edge des Hole in der Ebene der LightSource liegt, darf die LightSource
            // auch in der FrustumPlane liegen.
            Polygon3T<double>::SideT Side=LightSource.WhatSideSimple(FrustumPlane, MapT::RoundEpsilon);

            if (Side==Polygon3T<double>::Back || Side==Polygon3T<double>::InMirrored)
            {
                Frustum.PushBack(FrustumPlane);
                break;
            }
        }

        V2=V3;
    }

    // Rollen vertauschen: Das Loch sei nun die Lichtquelle, und die Lichtquelle das Loch! Siehe Skizze!
    V2=LightSource.Vertices.Size()-1;

    for (V3=0; V3<LightSource.Vertices.Size(); V3++)
    {
        for (unsigned long V1=0; V1<Hole.Vertices.Size(); V1++) // Optimize: Check if edges are in already existing frustum planes!
        {
            // Es bringt übrigens nichts, doppelt auftretende Planes hier vermeiden zu wollen!
            // Messungen waren z.B. 1:09:05 ohne Prüfung, 1:08:42 mit Prüfung auf Doppelvorkommen.
            // Könnte man aber später nochmal überprüfen...
         /* // Prüfe, ob wir diese Plane schon im Frustum haben.
            // Teste dazu, ob die drei Punkte in der Plane liegen.
            // Die Orientierung braucht dabei nicht beachtet zu werden.
            for (unsigned long FrustumNr=0; FrustumNr<FrustumSize1stPart; FrustumNr++)
            {
                const double Dist1=PlaneDistance(Frustum[FrustumNr],        Hole.Vertices[V1]);
                const double Dist2=PlaneDistance(Frustum[FrustumNr], LightSource.Vertices[V2]);
                const double Dist3=PlaneDistance(Frustum[FrustumNr], LightSource.Vertices[V3]);

                if (fabs(Dist1)<0.1 && fabs(Dist2)<0.1 && fabs(Dist3)<0.1) break;
            }
            if (FrustumNr<FrustumSize1stPart) continue; */

            // Eigentlich würde ich hier gerne folgenden Wunsch-Code schreiben:
            //     try
            //     {
            //         Plane3T<double> FrustumPlane(LightSource.Vertices[V2], Hole.Vertices[V1], LightSource.Vertices[V3]);
            //
            //         // ...
            //     }
            //     catch (DivisionByZero) { }  // Nicht mögliche Ebenen einfach ignorieren.
            // Aus irgendeinem Grund ist die Verwendung oder das Fangen der DivisionByZero-Exception aber sehr langsam.
            // Deshalb rolle ich lieber den Plane3T<double>-Konstruktor aus, um ohne dieses Exception-Handling auszukommen.
            // Das Programm wird *deutlich* schneller, ca. Faktor 1,5. Ob das eine Schwäche des Watcom-Compilers ist??
            VectorT Normal(cross(LightSource.Vertices[V3]-LightSource.Vertices[V2], Hole.Vertices[V1]-LightSource.Vertices[V2]));
            double  NLength=length(Normal);

            if (NLength<MapT::RoundEpsilon) continue;
            Normal=scale(Normal, 1.0/NLength);

            Plane3T<double> FrustumPlane(Normal, dot(LightSource.Vertices[V2], Normal));

            // Diese neue FrustumPlane nur dann akzeptieren, wenn die LightSource auf ihrer Rückseite
            // liegt (konstruktionsbedingt sowieso der Fall!) und das Hole auf ihrer Vorderseite liegt.
            // Wenn eine Edge der LightSource in der Ebene des Holes liegt, darf das Hole
            // auch in der FrustumPlane liegen.
            Polygon3T<double>::SideT Side=Hole.WhatSideSimple(FrustumPlane, MapT::RoundEpsilon);

            // Wegen dem Rollentausch ist die Orientierung für den WhatSideSimple() Test falsch, ...
            if (Side==Polygon3T<double>::Front || Side==Polygon3T<double>::InMirrored)
            {
                Frustum.PushBack(FrustumPlane);   // ...im Gesamten aber wieder richtig!
                break;
            }
        }

        V2=V3;
    }
}


// Enter the SuperLeaf 'CurrentSL' through the 'EnteringPortal' of the *preceding* SuperLeaf.
// The ancestors of 'CurrentSL' are recorded in 'AncestorSLs'. For the remaining parameters, see the implementation.
// Returns 'true' if visibility from the 'MasterSL' to the 'TargetSL' could be established, false otherwise.
bool DetermineVisibility(unsigned long CurrentSL, const Polygon3T<double>& EnteringPortal, ArrayT<unsigned long> AncestorSLs, const Polygon3T<double>& MasterPortal, const unsigned long TargetSL, BoundingBox3T<double> TargetBB)
{
    assert(TargetBB.IsInited());

    // Record that we can see from all ancestors into 'CurrentSL', and vice-versa.
    for (unsigned long AncestorNr=0; AncestorNr<AncestorSLs.Size(); AncestorNr++)
    {
        FlagVisible(AncestorSLs[AncestorNr], CurrentSL);
        FlagVisible(CurrentSL, AncestorSLs[AncestorNr]);
    }

    // If we reached the desired target, return success.
    if (CurrentSL==TargetSL) return true;

    // Clip 'TargetBB' against the plane of the 'EnteringPortal'. This is done to prevent the following kind of problem:
    // Consider a 'TargetSL' that is "thin" and diagonal. As a consequence, it will have a much too large 'TargetBB'.
    // If we then clip the 'TargetBB' against the frustum "MasterPortal --> EnteringPortal" below,
    // we might find that it is still (partially) inside this frustum.
    // However, these parts of the 'TargetBB' inside the frustum might be the much-too-large parts that arose from the special form of the 'TargetSL'.
    // In other words, had we done a better test instead (e.g. the "exact" one, by clipping the actual sub-portals of 'TargetSL' against the frustum),
    // we had found that the frustum actually missed the 'TargetSL' (that is, 'TargetSL' is entirely somewhere outside of the frustum).
    // In the former case, we'll find ourselves trying to find a visibility for something that's long "behind" us, and doing so at *exponential* costs!
    // In the latter case, we can skip all this from here.
    // Besides the "exact" test (which is also quite slow), we can also clip the 'TargetBB' against the plane of the 'EnteringPortal'.
    // The latter approach is conceptually a little different, but achieves the same positive net effect at lesser costs.
    switch (TargetBB.GetEpsilonBox(-MapT::RoundEpsilon).WhatSide(EnteringPortal.Plane))
    {
        case BoundingBox3T<double>::Front:
            return false;

        case BoundingBox3T<double>::Back:
            // Do nothing.
            break;

        case BoundingBox3T<double>::Both:
            // Note the /2 in MapT::RoundEpsilon/2, this should make sure that GetSplits() doesn't reach another conclusion than WhatSide(),
            // i.e. it prevents that GetSplits() cannot find a proper sub-box on *both* sides of the plane.
            TargetBB = TargetBB.GetSplits(EnteringPortal.Plane, MapT::RoundEpsilon/2)[1];

            if (!TargetBB.IsInited())
            {
                // If TargetSL only has a single portal that happens to be in one of the principal planes,
                // then TargetBB has the shape of a rectangle (zero volume). If its plane happens to be
                // identical with EnteringPortal.Plane, the methods WhatSide() and GetSplits() bring us here.
                return false;
            }

            break;
    }

    // Determine the frustum "MasterPortal --> EnteringPortal".
    ArrayT< Plane3T<double> > Frustum;    // Note that 'Frustum' cannot be declared as 'static', as this function recurses!
    FindFrustum(MasterPortal, EnteringPortal, Frustum);

    if (Frustum.Size()==0)
    {
        // printf("\nWARNING: Invalid frustum 1.\n");   // Should never happen.
        return false;
    }

    // Clip 'TargetBB' against the frustum "MasterPortal --> EnteringPortal".
    unsigned long FrustumNr;

    for (FrustumNr=0; FrustumNr<Frustum.Size(); FrustumNr++)
    {
        switch (TargetBB.GetEpsilonBox(-MapT::RoundEpsilon).WhatSide(Frustum[FrustumNr]))
        {
            case BoundingBox3T<double>::Front:
                // Do nothing.
                break;

            case BoundingBox3T<double>::Back:
                return false;

            case BoundingBox3T<double>::Both:
                // Note the /2 in MapT::RoundEpsilon/2, this should make sure that GetSplits() doesn't reach another conclusion than WhatSide(),
                // i.e. it prevents that GetSplits() cannot find a proper sub-box on *both* sides of the plane.
                TargetBB = TargetBB.GetSplits(Frustum[FrustumNr], MapT::RoundEpsilon/2)[0];

                if (!TargetBB.IsInited())
                {
                    // If TargetSL only has a single portal that happens to be in one of the principal planes,
                    // then TargetBB has the shape of a rectangle (zero volume). If its plane happens to be
                    // identical with Frustum[FrustumNr], the methods WhatSide() and GetSplits() bring us here.
                    return false;
                }

                break;
        }
    }

    // Bevor wir in das nächste SuperLeaf gehen, nimm das gegenwärtige 'CurrentSL' in die Liste der Vorgänger auf.
    AncestorSLs.PushBack(CurrentSL);

    for (unsigned long NeighbourNr=0; NeighbourNr<SuperLeaves[CurrentSL].Neighbours.Size(); NeighbourNr++)
    {
        const unsigned long NextSL    =SuperLeaves[CurrentSL].Neighbours[NeighbourNr].SuperLeafNr;
        Polygon3T<double>   NextPortal=SuperLeaves[CurrentSL].Neighbours[NeighbourNr].SubPortal;

        // Wenn für das 'NextSL' schon festgestellt wurde, daß das 'TargetSL' von dort aus nicht zu sehen ist, können wir direkt weitermachen.
        if (NextSL<AncestorSLs[0]/*MasterSL*/ && !IsVisible(NextSL, TargetSL)) continue;

        // Abkürzung: Gleiche Ebene? Dann weiter mit dem nächsten Portal!
        {
            Polygon3T<double>::SideT Side=NextPortal.WhatSide(EnteringPortal.Plane, MapT::RoundEpsilon);

            if (Side==Polygon3T<double>::InIdentical || Side==Polygon3T<double>::InMirrored) continue;
        }

        // Clippe 'NextPortal' gegen das Frustum "MasterPortal --> EnteringPortal".
        for (FrustumNr=0; FrustumNr<Frustum.Size(); FrustumNr++)
        {
            Polygon3T<double>::SideT Side=NextPortal.WhatSideSimple(Frustum[FrustumNr], MapT::RoundEpsilon);

                 if (Side==Polygon3T<double>::Both ) NextPortal=NextPortal.GetSplits(Frustum[FrustumNr], MapT::RoundEpsilon)[0];
            else if (Side!=Polygon3T<double>::Front) break;
        }
        if (FrustumNr<Frustum.Size()) continue;

        // Bestimme das Frustum "EnteringPortal --> NextPortal".
        static ArrayT< Plane3T<double> > Frustum2;
        FindFrustum(EnteringPortal, NextPortal, Frustum2);

        if (Frustum2.Size()==0)
        {
            // printf("\nWARNING: Invalid frustum 2.\n");   // Should never happen.
            continue;
        }

        // Clippe 'MasterPortal' gegen das Frustum "NextPortal --> EnteringPortal".
        // Um die Portale nicht spiegeln zu müssen, das gespiegelte Frustum benutzen!
        Polygon3T<double> MP=MasterPortal;

        for (FrustumNr=0; FrustumNr<Frustum2.Size(); FrustumNr++)
        {
            Polygon3T<double>::SideT Side=MP.WhatSideSimple(Frustum2[FrustumNr], MapT::RoundEpsilon);

                 if (Side==Polygon3T<double>::Both) MP=MP.GetSplits(Frustum2[FrustumNr], MapT::RoundEpsilon)[1];
            else if (Side!=Polygon3T<double>::Back) break;
        }
        if (FrustumNr<Frustum2.Size()) continue;

        if (DetermineVisibility(NextSL, NextPortal, AncestorSLs, MP, TargetSL, TargetBB)) return true;
    }

    return false;
}


// This functions determines if we can see from 'MasterSL' to 'TargetSL', that is, if 'TargetSL' is visible from 'MasterSL'.
// ATTENTION 1: IT IS ASSUMED THAT THE PVS FOR ALL SUPERLEAVES IN RANGE '0..MasterSL-1' HAS ALREADY BEEN ESTABLISHED!
// ATTENTION 2: THIS FUNCTION DOES NOT WORK IF 'MasterSL==TargetSL', OR 'TargetSL' IS AN IMMEDIATE NEIGHBOUR OF 'MasterSL'!
// If mutual visibility could be established, the result is recorded in 'SuperLeavesPVS', for both "from A to B" and "from B to A".
bool CanSeeFromAToB(unsigned long MasterSL, unsigned long TargetSL)
{
    // If the TargetSL's bounding-box is not initialized, it has no portals and thus cannot be visible from MasterSL.
    if (!SuperLeavesBBs[TargetSL].IsInited()) return false;

    ArrayT<unsigned long> AncestorSLs;
    AncestorSLs.PushBackEmpty(2);

    // Für den alten Algorithmus war hier vermerkt, daß "outer leaves" keine Neighbours/Portals haben,
    // wegen der Eigenschaften von Portalize() und FillInside() des CaBSP Programms.
    // Der neue Algorithmus verwendet SuperLeaves, bei denen überhaupt nicht zwischen "inner" und "outer" unterschieden wird.
    // Alles was zählt, sind die Portale. Deshalb setzen sich SuperLeaves korrekt aus beliebigen Leaves eines Sub-Trees zusammen.
    for (unsigned long NeighbourNr=0; NeighbourNr<SuperLeaves[MasterSL].Neighbours.Size(); NeighbourNr++)
    {
        const unsigned long      NeighbourSL =SuperLeaves[MasterSL].Neighbours[NeighbourNr].SuperLeafNr;
        const Polygon3T<double>& MasterPortal=SuperLeaves[MasterSL].Neighbours[NeighbourNr].SubPortal;

        // Wenn für das 'NeighbourSL' schon festgestellt wurde, daß das 'TargetSL' von dort aus nicht zu sehen ist,
        // sparen wir uns die Mühe, es trotzdem zu versuchen, und machen direkt weiter.
        if (NeighbourSL<MasterSL && !IsVisible(NeighbourSL, TargetSL)) continue;

        AncestorSLs[0]=MasterSL;
        AncestorSLs[1]=NeighbourSL;

        for (unsigned long NNNr=0; NNNr<SuperLeaves[NeighbourSL].Neighbours.Size(); NNNr++)
        {
            const unsigned long      NeighboursNeighbourSL=SuperLeaves[NeighbourSL].Neighbours[NNNr].SuperLeafNr;
            const Polygon3T<double>& EnteringPortal       =SuperLeaves[NeighbourSL].Neighbours[NNNr].SubPortal;

            if (NeighboursNeighbourSL==MasterSL) continue;

            const Polygon3T<double>::SideT Side=MasterPortal.WhatSide(EnteringPortal.Plane, MapT::RoundEpsilon);

            if (Side==Polygon3T<double>::InIdentical || Side==Polygon3T<double>::InMirrored) continue;

            // Wenn für das 'NeighboursNeighbourSL' schon festgestellt wurde, daß das 'TargetSL' von dort aus nicht zu sehen ist,
            // sparen wir uns die Mühe, es trotzdem zu versuchen, und machen direkt weiter.
            if (NeighboursNeighbourSL<MasterSL && !IsVisible(NeighboursNeighbourSL, TargetSL)) continue;

            // Das TOLLE: Sobald wir festgestellt haben, daß "irgendeine" Sichtbarkeit von 'MasterSL' nach 'TargetSL' existiert,
            // können wir sofort AUFHÖREN(!!!) und mit dem nächsten SuperLeaves-Paar weitermachen!
            if (DetermineVisibility(NeighboursNeighbourSL, EnteringPortal, AncestorSLs, MasterPortal, TargetSL, SuperLeavesBBs[TargetSL])) return true;
        }
    }

    return false;
}


// Analytically computes the 'SuperLeavesPVS'.
// A prior call to 'DetermineTrivialVisibility()' is assumed.
void BuildPVS()
{
    /* // Der komplette alte, aber einfache Code. Evtl. nützlich für Debugging-Zwecke.
    for (unsigned long MasterSL=0; MasterSL+1<SuperLeaves.Size(); MasterSL++)
    {
        printf("%5.1f%%\r", (double)MasterSL/SuperLeaves.Size()*100.0);
        fflush(stdout);

        // Bestimme für jedes SuperLeaves-Paar, ob eine gegenseitige Sichtbarkeit besteht.
        // Beachte, daß die folgende Schleife bei 'MasterSL+1' starten kann (statt bei '0'), da Sichtbarkeit immer wechselseitig ist.
        for (unsigned long TargetSL=MasterSL+1; TargetSL<SuperLeaves.Size(); TargetSL++)
        {
            // Wenn die Sichtbarkeit zwischen 'MasterSL' und 'TargetSL' bereits festgestellt wurde,
            // können wir direkt mit dem nächsten 'TargetSL' weitermachen. Sinnvoll für unmittelbare Nachbarn.
            // Auch vorangegangene Iterationen für andere, aber weiter weg liegende 'TargetSL's haben u.U.
            // "auf dem Weg" liegende SuperLeaves als sichtbar markiert, sodaß diese hier nicht nochmal getestet werden müssen.
            if (IsVisible(MasterSL, TargetSL)) continue;

            // Wenn das 'TargetSL' keine Nachbarn hat, können wir direkt mit dem nächsten 'TargetSL' weitermachen.
            // Für ein solches SL ohne Nachbarn haben wir sowieso keine sinnvolle "Target Bounding Box" in den 'SuperLeavesBBs' konstruiert!
            if (SuperLeaves[TargetSL].Neighbours.Size()==0) continue;

            // Intentionally ignore the returned result.
            CanSeeFromAToB(MasterSL, TargetSL);
        }
    } */


    // Für jedes SuperLeaf das PVS bestimmen.
    for (unsigned long MasterSL=0; MasterSL<SuperLeaves.Size(); MasterSL++)
    {
        printf("%5.1f%%\r", (double)MasterSL/SuperLeaves.Size()*100.0);
        fflush(stdout);

        // Initialisiere die Hilfsarrays für das 'MasterSL'.
        ArrayT<bool> AV;    // List of "Already     Visible" SuperLeaves from 'MasterSL'.
        ArrayT<bool> PV;    // List of "Potentially Visible" SuperLeaves from 'MasterSL'.
        ArrayT<bool> NV;    // List of "Not         Visible" SuperLeaves from 'MasterSL'.

        unsigned long SL;

        for (SL=0; SL<SuperLeaves.Size(); SL++)
        {
            AV.PushBack(false);
            PV.PushBack(false);
            NV.PushBack(false);
        }

        // Bestimme, welche SuperLeaves wir von 'MasterSL' aus schon sehen können.
        for (SL=0; SL<SuperLeaves.Size(); SL++)
            if (IsVisible(MasterSL, SL)) AV[SL]=true;

        // Bilde eine Liste 'PV' aller SuperLeaves, die von 'MasterSL' aus "potentially visible" sind.
        // Dies sind zunächst *alle* Nachbarn aller SuperLeaves in der 'AV' Liste, außer denjenigen,
        // die bereits in der 'AV' oder 'NV' Liste sind, oder deren Index-Nummern 'NeighbourSL' kleiner/gleich 'MasterSL' sind.
        // Der Grund für letzteres: Wenn früher schon festgestellt wurde, daß wir nicht von 'NeighbourSL' nach 'MasterSL' sehen können,
        // können wir uns jetzt den Test, ob wir von 'MasterSL' nach 'NeighbourSL' sehen können, sparen.
        // Wäre der frühere Test dagegen positiv gewesen, wäre 'AV[NeighbourSL]==true', was auch keinen 'PV'-Eintrag verursacht.
        for (SL=0; SL<SuperLeaves.Size(); SL++)
            if (AV[SL])
                for (unsigned long NeighbourNr=0; NeighbourNr<SuperLeaves[SL].Neighbours.Size(); NeighbourNr++)
                {
                    const unsigned long NeighbourSL=SuperLeaves[SL].Neighbours[NeighbourNr].SuperLeafNr;

                    if (!AV[NeighbourSL] && !NV[NeighbourSL] && NeighbourSL>MasterSL) PV[NeighbourSL]=true;
                }

        // For each 'TargetSL' in 'PV': Determine if 'TargetSL' is visible from 'MasterSL'.
        while (true)
        {
            unsigned long TargetSL;

            for (TargetSL=0; TargetSL<PV.Size(); TargetSL++) if (PV[TargetSL]) break;
            if (TargetSL>=PV.Size()) break;

            if (CanSeeFromAToB(MasterSL, TargetSL))
            {
                // Redo the init (but 'NV' is possibly not empty anymore).
                for (SL=0; SL<SuperLeaves.Size(); SL++)
                {
                    AV[SL]=false;
                    PV[SL]=false;
                }

                // Bestimme, welche SuperLeaves wir von 'MasterSL' aus schon sehen können.
                for (SL=0; SL<SuperLeaves.Size(); SL++)
                    if (IsVisible(MasterSL, SL)) AV[SL]=true;

                // Bilde eine Liste 'PV' aller SuperLeaves, die von 'MasterSL' aus "potentially visible" sind.
                // Dies sind zunächst *alle* Nachbarn aller SuperLeaves in der 'AV' Liste, außer denjenigen,
                // die bereits in der 'AV' oder 'NV' Liste sind, oder deren Index-Nummern 'NeighbourSL' kleiner/gleich 'MasterSL' sind.
                // Der Grund für letzteres: Wenn früher schon festgestellt wurde, daß wir nicht von 'NeighbourSL' nach 'MasterSL' sehen können,
                // können wir uns jetzt den Test, ob wir von 'MasterSL' nach 'NeighbourSL' sehen können, sparen.
                // Wäre der frühere Test dagegen positiv gewesen, wäre 'AV[NeighbourSL]==true', was auch keinen 'PV'-Eintrag verursacht.
                for (SL=0; SL<SuperLeaves.Size(); SL++)
                    if (AV[SL])
                        for (unsigned long NeighbourNr=0; NeighbourNr<SuperLeaves[SL].Neighbours.Size(); NeighbourNr++)
                        {
                            const unsigned long NeighbourSL=SuperLeaves[SL].Neighbours[NeighbourNr].SuperLeafNr;

                            if (!AV[NeighbourSL] && !NV[NeighbourSL] && NeighbourSL>MasterSL) PV[NeighbourSL]=true;
                        }
            }
            else
            {
                PV[TargetSL]=false;     // Delete 'TargetSL' from PV list.
                NV[TargetSL]=true;      // Add    'TargetSL' to   NV list.
            }
        }
    }

    printf("Final Avg Visibility: %10.5f\n", GetAverageVisibility());
}


static void WriteLogFileEntry(const char* WorldPathName, unsigned long CheckSum)
{
    char          DateTime [256]="unknown";
    char          HostName [256]="unknown";
    char          WorldName[256]="unknown";
    time_t        Time          =time(NULL);
    unsigned long RunningSec    =(unsigned long)difftime(Time, ProgramStartTime);
    FILE*         LogFile       =fopen("CaPVS.log", "a");

    if (!LogFile) return;

    strftime(DateTime, 256, "%d.%m.%Y %H:%M", localtime(&Time));
    DateTime[255]=0;

#ifdef _WIN32
    unsigned long Dummy=256;
    if (!GetComputerName(HostName, &Dummy)) sprintf(HostName, "unknown (look-up failed).");
#else
    // This function also works on Windows, but sadly requires calls to 'WSAStartup()' and 'WSACleanup()'.
    if (gethostname(HostName, 256)) sprintf(HostName, "unknown (look-up failed).");
#endif
    HostName[255]=0;

    if (WorldPathName)
    {
        // Dateinamen abtrennen (mit Extension).
        size_t i=strlen(WorldPathName);

        while (i>0 && WorldPathName[i-1]!='/' && WorldPathName[i-1]!='\\') i--;
        strncpy(WorldName, WorldPathName+i, 256);
        WorldName[255]=0;

        // Extension abtrennen.
        i=strlen(WorldName);

        while (i>0 && WorldName[i-1]!='.') i--;
        if (i>0) WorldName[i-1]=0;
    }

    // Date, Time, WorldName, TimeForCompletion on [HostName]
    fprintf(LogFile, "%-16s %-16s%3lu:%02lu:%02lu [%-16s] %10lu\n", DateTime, WorldName, RunningSec/3600, (RunningSec/60) % 60, RunningSec % 60, HostName, CheckSum);
    fclose(LogFile);
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
    // cf::GameSys::GetComponentTIM().Init();      // The one-time init of the GameSys components type info manager.
    // cf::GameSys::GetGameSysEntityTIM().Init();  // The one-time init of the GameSys entity type info manager.
    // cf::GameSys::GetWorldTIM().Init();          // The one-time init of the GameSys world type info manager.

    unsigned long MaxRecDepthSL  =0xFFFFFFFF;
    double        MinAreaSL      =0.0;
    bool          OnlySuperLeaves=false;

    printf("\n*** Cafu Potentially Visibility Set Utility, Version 05 (%s) ***\n\n\n", __DATE__);

    if (ArgC<2) Usage();

    // Initialize the FileMan by mounting the default file system.
    // Note that specifying "./" (instead of "") as the file system description effectively prevents the use of
    // absolute paths like "D:\abc\someDir\someFile.xy" or "/usr/bin/xy". This however should be fine for this application.
    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_LOCAL_PATH, "./", "");
    // cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/DeathMatch/Textures/TechDemo.zip", "Games/DeathMatch/Textures/TechDemo/", "Ca3DE");
    // cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/DeathMatch/Textures/SkyDomes.zip", "Games/DeathMatch/Textures/SkyDomes/", "Ca3DE");

    // Parse the command line.
    for (int ArgNr=2; ArgNr<ArgC; ArgNr++)
    {
        if (!_stricmp(ArgV[ArgNr], "-maxRecDepthSL"))
        {
            ArgNr++;
            if (ArgNr>=ArgC) Usage();

            MaxRecDepthSL=atoi(ArgV[ArgNr]);
            printf("maxRecDepthSL == %lu\n", MaxRecDepthSL);
        }
        else if (!_stricmp(ArgV[ArgNr], "-minAreaSL"))
        {
            ArgNr++;
            if (ArgNr>=ArgC) Usage();

            MinAreaSL=atof(ArgV[ArgNr]);
            if (MinAreaSL<0.0) MinAreaSL=0.0;
            printf("minAreaSL     == %.3f\n", MinAreaSL);
        }
        else if (!_stricmp(ArgV[ArgNr], "-onlySLs"))
        {
            OnlySuperLeaves=true;
        }
        else if (ArgV[ArgNr][0]==0)
        {
            // The argument is "", the empty string.
            // This can happen under Linux, when CaPVS is called via wxExecute() with white-space trailing the command string.
        }
        else
        {
            printf("Unknown option '%s'.\n", ArgV[ArgNr]);
            Usage();
        }
    }


    std::string GameDirectory=ArgV[1];

    // Determine the game directory, cleverly assuming that the destination file is in "Worlds".
    {
        // Strip the file name and extention off.
        size_t i=GameDirectory.find_last_of("/\\");

        GameDirectory=GameDirectory.substr(0, i==std::string::npos ? 0 : i)+"/..";
    }


    // Setup the global MaterialManager pointer.
    static MaterialManagerImplT MatManImpl;

    MaterialManager=&MatManImpl;

    if (MaterialManager->RegisterMaterialScriptsInDir(GameDirectory+"/Materials", GameDirectory+"/").Size()==0)
    {
        printf("\nNo materials found in scripts in \"%s/Materials\".\n", GameDirectory.c_str());
        printf("No materials found.\n\n");
        Usage();
    }


    try
    {
        // General note: Robustness against the 'sharp wedge' problem is granted:
        // DetermineAdjacencyGraph() is the only place where PolygonOverlap() is called, but only once on *input* portals.
        // Everywhere else (especially in BuildPVS() and its sub-functions), only PolygonSplit() and PolygonWhatSide() are called.
        printf("*** Load World %s ***\n", ArgV[1]);

        // 1. Load the 'CaPVSWorld'.
        ModelManagerT             ModelMan;
        cf::GuiSys::GuiResourcesT GuiRes(ModelMan);
        CaPVSWorldT*              CaPVSWorld = new CaPVSWorldT(ArgV[1], ModelMan, GuiRes, MaxRecDepthSL, MinAreaSL);

        // 2. Create the 'SuperLeaves'.
        CaPVSWorld->CreateSuperLeaves(SuperLeaves);
        if (OnlySuperLeaves) return 0;

        // 3. Determine the adjacency graph.
        //    This completely fills-in the remaining components of the 'SuperLeaves'.
        printf("\n%-50s %s\n", "*** Initialize ***", GetTimeSinceProgramStart());
        DetermineAdjacencyGraph();

        // 4. Compute for each SuperLeaf the bounding box over its (sub-)portals.
        ComputeSuperLeavesBBs();

        // 5. Create and initialize the 'SuperLeavesPVS' (reset to complete blindness (all 0s)).
        SuperLeavesPVS.PushBackEmpty((SuperLeaves.Size()*SuperLeaves.Size()+31)/32);
        for (unsigned long Vis=0; Vis<SuperLeavesPVS.Size(); Vis++) SuperLeavesPVS[Vis]=0;

        // 6. For each SuperLeaf, flag itself and the immediate neighbours as visible.
        DetermineTrivialVisibility();

        // 7. Do some simple ray tests in order to quickly obtain a good estimation of the actual PVS.
        //    Note that calling this function is ENTIRELY OPTIONAL, and the call can be omitted without danger!
        //    (The latter might e.g. be useful for debugging.)
        DetermineRayPresampledVisibility(CaPVSWorld);

        // 8. Finally calculate the 'SuperLeavesPVS' using analytical methods.
        printf("\n%-50s %s\n", "*** Potentially Visibility Set ***", GetTimeSinceProgramStart());
        BuildPVS();

        // 9. Carry the information in 'SuperLeavesPVS' into the PVS of the 'CaPVSWorld'.
        CaPVSWorld->StorePVS(SuperLeaves, SuperLeavesPVS);

        // 10. Print some statistics and obtain a checksum.
        unsigned long CheckSum=CaPVSWorld->GetChecksumAndPrintStats();

        // 11. Save the 'CaPVSWorld' back to disk.
        printf("\n%-50s %s\n", "*** Save World ***", GetTimeSinceProgramStart());
        printf("%s\n", ArgV[1]);
        CaPVSWorld->SaveToDisk(ArgV[1]);

        // 12. Clean-up and write log file entry.
        delete CaPVSWorld;
        WriteLogFileEntry(ArgV[1], CheckSum);
        printf("\n%-50s %s\n", "COMPLETED.", GetTimeSinceProgramStart());
    }
    catch (const WorldT::LoadErrorT& E)
    {
        printf("\nFATAL ERROR: %s\n", E.Msg);
        printf("Program aborted.\n\n");
        exit(1);
    }
    catch (const WorldT::SaveErrorT& E)
    {
        printf("\nFATAL ERROR: %s\n", E.Msg);
        printf("Program aborted.\n\n");
        exit(1);
    }

    return 0;
}
