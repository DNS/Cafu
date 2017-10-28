/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/*****************************/
/***                       ***/
/*** Cafu Lighting Utility ***/
/***                       ***/
/*** Der Herr sprach       ***/
/***   Es werde Licht!     ***/
/*** Und es wurde Licht.   ***/
/*** Genesis               ***/
/***                       ***/
/*****************************/

// ALLGEMEINE BEMERKUNGEN ZU FACES, LIGHTMAPS UND PATCHES:
// Wir definieren eine LightMap als ein Rechteck aus s*t quadratischen Patches, die jeweils eine Face "abdecken".
// Das Rechteck sollte bei gegebener Seitenlänge der Patches und gegebener Orientierung (entlang des UV-Koordinatensystems,
// welches man mit Plane3T<double>::GetSpanVectors() erhält) möglichst kleine s- und t-Abmessungen haben. D.h., daß der linke Rand
// der LightMap mit der kleinsten U-Koordinate der Vertices der Face zusammenfallen soll und der obere Rand mit der kleinsten
// V-Koordinate.
// Außerdem ziehen wir noch einen 1 Patch breiten Rahmen drumherum. Damit soll dem OpenGL-Renderer Rechnung getragen werden,
// der zu jeder (s,t)-Koordinate den Mittelwert des umliegenden 2x2-Quadrats bestimmt (bilinear Filtering).

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <conio.h>
#else
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#define _getch getchar
#endif

#include <time.h>
#include <stdio.h>
#include <cassert>

#include "Templates/Array.hpp"
#include "Math3D/Matrix3x3.hpp"
#include "Math3D/Plane3.hpp"
#include "Bitmap/Bitmap.hpp"
#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/ConsoleInterpreter.hpp"
#include "ConsoleCommands/ConsoleStdout.hpp"
#include "FileSys/FileManImpl.hpp"
#include "GameSys/AllComponents.hpp"
#include "GameSys/CompLightRadiosity.hpp"
#include "GameSys/Entity.hpp"
#include "GameSys/World.hpp"
#include "GuiSys/GuiResources.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/MaterialManagerImpl.hpp"
#include "Models/ModelManager.hpp"
#include "SceneGraph/Node.hpp"
#include "SceneGraph/BspTreeNode.hpp"
#include "SceneGraph/FaceNode.hpp"
#include "SoundSystem/SoundShaderManagerImpl.hpp"
#include "SoundSystem/SoundSys.hpp"
#include "ClipSys/CollisionModelMan_impl.hpp"
#include "String.hpp"

#include "CaLightWorld.hpp"

#if defined(_WIN32)
    #if defined(_MSC_VER)
        #define vsnprintf _vsnprintf
    #endif
#endif


static cf::ConsoleStdoutT ConsoleStdout;
cf::ConsoleI* Console=&ConsoleStdout;

static cf::FileSys::FileManImplT FileManImpl;
cf::FileSys::FileManI* cf::FileSys::FileMan=&FileManImpl;

static cf::ClipSys::CollModelManImplT CCM;
cf::ClipSys::CollModelManI* cf::ClipSys::CollModelMan=&CCM;

ConsoleInterpreterI* ConsoleInterpreter=NULL;
MaterialManagerI*    MaterialManager   =NULL;

static SoundShaderManagerImplT s_SSM;
SoundShaderManagerI* SoundShaderManager = &s_SSM;

SoundSysI* SoundSystem = NULL;


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


static void Error(const char* ErrorText, ...)
{
    va_list ArgList;
    char    ErrorString[256];

    if (ErrorText!=NULL)
    {
        va_start(ArgList, ErrorText);
            vsnprintf(ErrorString, 256, ErrorText, ArgList);
        va_end(ArgList);

        printf("\nFATAL ERROR: %s\n", ErrorString);
    }

    printf("Program aborted.\n\n");
    exit(1);
}


/// This class implements a diagonally symmetric matrix for two-bit numbers,
/// that is, the elements of the matrix can only be the numbers 0, 1, 2 or 3.
///
/// Implementation notes:
/// - Only the lower left half of the matrix is physically stored.
/// - Each matrix element is physically represented by two bits.
/// - The elements in turn are stored in tuples of 16 in "unsigned long"s.
/// - Assumes that the type "unsigned long" has (at least) 32 bits.
class DiagMatrixT
{
    public:

    /// Creates a DiagMatrixT of size 0.
    DiagMatrixT()
    {
    }

    /// (Re-)sets the size of this DiagMatrixT to n*n and initializes the elements with zeros.
    void SetSize(unsigned long n)
    {
        const unsigned long NrOfElements=(n*(n+1))/2;

        Data.Clear();
        Data.PushBackEmpty((NrOfElements+15)/16);

        for (unsigned long i=0; i<Data.Size(); i++) Data[i]=0;
    }

    /// Returns the value at (row, col).
    char GetValue(unsigned long row, unsigned long col) const
    {
        if (col>row) return GetValue(col, row);

        const unsigned long ElementIdx=(row*(row+1))/2 + col;
        const unsigned long DataIdx   =ElementIdx / 16;     // The requested value is somewhere in Data[DataIdx],
        const unsigned long DataOfs   =ElementIdx % 16;     // namely the DataOfs-th one.

        return char((Data[DataIdx] >> (DataOfs*2)) & 3);
    }

    /// Sets the value at (row, col). Note that value must be 0, 1, 2 or 3.
    void SetValue(unsigned long row, unsigned long col, char value)
    {
        if (col>row) { SetValue(col, row, value); return; }

        const unsigned long ElementIdx=(row*(row+1))/2 + col;
        const unsigned long DataIdx   =ElementIdx / 16;     // The requested value is somewhere in Data[DataIdx],
        const unsigned long DataOfs   =ElementIdx % 16;     // namely the DataOfs-th one.

        unsigned long ClearMask=~(3ul << (DataOfs*2));      // 1s everywhere, except for the two bits of our element data.
        unsigned long Val      =value & 3;

        Data[DataIdx]&=ClearMask;
        Data[DataIdx]|=(Val << (DataOfs*2));
    }

    unsigned long GetBytesAlloced() const
    {
        return Data.Size()*sizeof(unsigned long);
    }

    /// Implements a simple test case for objects of this class. Returns true if all tests pass, false otherwise.
    /// The contents of the matrix after the test is undefined.
    bool Test();


    private:

    ArrayT<unsigned long> Data;
};


bool DiagMatrixT::Test()
{
    SetSize(3);
    if (Data.Size()<1) return false;

    // Test 1
    {
        // Note that a 5*5 matrix requires physical storage for 15 elements,
        // and thus 30 bits, which in turn all fit into the first "unsigned long".
        Data[0]=0;
        unsigned long Val=0;
        for (unsigned long r=0; r<=4; r++)
            for (unsigned long c=0; c<=r; c++)
            {
                SetValue(r, c, char(Val % 4));
                if (GetValue(r, c)!=Val % 4) return false;
                if (GetValue(c, r)!=Val % 4) return false;
                Val++;
            }

        if (Data[0]!=0x24E4E4E4) return false;
    }

    // Test 2
    {
        // Same as above, but with r and c in SetValue() reversed.
        Data[0]=0;
        unsigned long Val=0;
        for (unsigned long r=0; r<=4; r++)
            for (unsigned long c=0; c<=r; c++)
            {
                SetValue(c, r, char(Val % 4));
                if (GetValue(r, c)!=Val % 4) return false;
                if (GetValue(c, r)!=Val % 4) return false;
                Val++;
            }

        if (Data[0]!=0x24E4E4E4) return false;
    }

    Data[0]=0;
    return true;
}


static double Max3(const VectorT& V)
{
    double m=V.x;

    if (V.y>m) m=V.y;
    if (V.z>m) m=V.z;

    return m;
}


const double REFLECTIVITY=0.3;  // Gleiche Reflektivität für alle Faces und für alle Wellenlängen


ArrayT<cf::PatchMeshT> PatchMeshes; // The patch meshes that we should consider for radiosity lighting.

#include "Init2.cpp"    // void InitializePatches      (const cf::SceneGraph::BspTreeNodeT& Map, const SkyDomeT& SkyDome) { ... }


enum PatchMeshesMutualVisE { NO_VISIBILITY=0, PARTIAL_VISIBILITY=1/*, FULL_VISIBILITY=2*/ };

std::map< const cf::SceneGraph::GenericNodeT*, ArrayT<unsigned long> > NodePtrToPMIndices;
DiagMatrixT PatchMeshesPVS;     // The matrix that determines which patch meshes a patch mesh can see (see InitializePatchMeshesPVS() for a description!).

#include "Init1.cpp"    // void InitializePatchMeshesPVS(const cf::SceneGraph::BspTreeNodeT& Map                         ) { ... }


static unsigned long Count_AllCalls=0;
static unsigned long Count_DivgWarnCalls=0;

// Strahlt die Energie der Patches im n*n Quadrat ab, dessen linke obere Ecke bei (s_i, t_i) liegt
void RadiateEnergy(const CaLightWorldT& CaLightWorld, unsigned long PM_i, unsigned long s_i, unsigned long t_i, char n)
{
    cf::PatchMeshT& PatchMesh_i=PatchMeshes[PM_i];

    cf::PatchT    Big_P_i;
    unsigned long Big_P_i_Count=0;

    // A cf::PatchT has no explicit constructor. While its Vector3T<T> components are implicitly 0'ed,
    // make sure that the Area members starts at 0, too.
    Big_P_i.Area=0;

    // Bilde den Positions-Durchschnitt bzw. die UnradiatedEnergy-Summe aller Patches im n*n Quadrat,
    // wobei (s_i, t_i) die linke obere Ecke ist und nur Patches innerhalb des Patch Meshes (InsideFace) berücksichtigt werden.
    for (char y=0; y<n; y++)
        for (char x=0; x<n; x++)
        {
            unsigned long s_=s_i+x;
            unsigned long t_=t_i+y;

            if (PatchMesh_i.WrapsHorz && s_>=PatchMesh_i.Width ) s_-=PatchMesh_i.Width;
            if (PatchMesh_i.WrapsVert && t_>=PatchMesh_i.Height) t_-=PatchMesh_i.Height;

            if (s_>=PatchMesh_i.Width ) continue;
            if (t_>=PatchMesh_i.Height) continue;

            cf::PatchT& P_i=PatchMesh_i.GetPatch(s_, t_);
            if (!P_i.InsideFace) continue;

            Big_P_i_Count++;
            Big_P_i.Coord           +=P_i.Coord;
            Big_P_i.Normal          +=P_i.Normal;
            Big_P_i.Area            +=P_i.Area;
            Big_P_i.UnradiatedEnergy+=P_i.UnradiatedEnergy;

            P_i.UnradiatedEnergy=VectorT(0, 0, 0);
        }

    if (!Big_P_i_Count) return;

    const double NormalLen=length(Big_P_i.Normal);

    // Before CaLight took the individual areas of the patches into account (it was assumed that they all had the same area PATCH_SIZE*PATCH_SIZE),
    // the Big_P_i patch worked (and still does) by simply summing up the UnradiatedEnergy fields of its component P_i patches.
    // This in turn naturally shows how to extend the code to also take patch areas into account, because thinking of Big_P_i as a patch of the
    // same size as any patch at that time (namely PATCH_SIZE*PATCH_SIZE, or rather the average of the area of its component patches) but with
    // a higher UnradiatedEnergy (the sum of all component patches) is equivalent to thinking of Big_P_i as having only the *average*
    // UnradiatedEnergy but the sum of the *areas* of its component patches!
    // This naturally leads to the conclusion that AreaRatio_ij=Big_P_i.Area/P_j.Area should enter the equation below.
    // Note that independent of that, with Big_P_i we have the choice to either average its UnradiatedEnergy or its Area, thus thinking about
    // it the one way or the other -- the net result in DeltaRadiosity below is the same. For historical reasons, I've chosen to keep the
    // UnradiatedEnergy and average the Area.
    Big_P_i.Coord*=1.0/Big_P_i_Count;   // TODO! Sollte   Big_P_i.Coord=P_i.Coord;   setzen, wobei P_i derjenige Patch ist der der Durchschnittsposition am nächsten kommt. Nur so kommen wir auch mit gekrümmten, twosided PatchMeshes wirklich klar!
    Big_P_i.Normal=(NormalLen>0.000001) ? Big_P_i.Normal/NormalLen : Vector3dT(0, 0, 1);
    Big_P_i.Area/=Big_P_i_Count;
    // printf("%f %lu blocksize=%i^2 %f\n", Big_P_i.Area, Big_P_i_Count, n, PATCH_SIZE*PATCH_SIZE);

    // It doesn't matter if we take the total or the average Big_P_i.Area here - the division cancels itself out below.
    const double MinRayLength=sqrt(Big_P_i.Area);

    // We keep here the maximum amount of energy that's left to be shot to other patches, or rather, as the value is premultiplied by REFLECTIVITY,
    // the maximum amount of energy that all the other patches may have accumulated as UnradiatedEnergy when Big_P_i has finished shooting.
    // This mechanism is used as a safety-guard against diverging behaviour below.
    // Divergence should theoretically never occur, but practically is can arise when the RayLength below is too short in relation to the patch size.
    // Note that the EnergyBudget code as well as the MinRayLength/SafeRayLength code both fight divergency problems, so it might be
    // worthwhile to temporariliy disable one while you wish to examine the other.
    // The factor DIVG_MARGIN is for rounding errors, and intentionally chosen big, because in the presence of translucent surfaces,
    // we actually (and wrongly) shoot more energy than we normally should, namely that onto the first opaque surface as well
    // as onto all translucent surfaces along the way. (That is, whenever faces that receive but DO NOT(!) block radiance are present.)
    // Also see   svn log -r 287   point f) for some additional information.
    // However, note that with   svn log -r 289   I largely disable this stuff again, because I feel that the "divergency case" triggers far
    // too often at DIVG_MARGIN=1.6 (which in turn aborts the radiation of the current patch hard in mid-progress).
    // As I don't seem to understand the nature of the frequent occurrences sufficiently, I rather collect only statistics about this approach
    // for now instead of having hard aborts with unknown results on lighting quality, and rely on the "MinRayLength" as the sole divergency
    // prevention technique.
    const double DIVG_MARGIN=2.1;
    VectorT EnergyBudget=Big_P_i.UnradiatedEnergy*(REFLECTIVITY*DIVG_MARGIN);

    Count_AllCalls++;


    // Betrachte alle Patches aller Patch Meshes im PVS des Patch Meshes PM_i.
    for (unsigned long PM_j=0; PM_j<PatchMeshes.Size(); PM_j++)
    {
        // Vermeide alle unnötigen und evtl. rundungsfehlergefährdeten Berechnungen.
        // Wenn PM_i und PM_j planar sind, fängt die folgende Zeile auch alle Fälle ab, in denen PM_j in der Ebene
        // von PM_i liegt und insb. für die PM_i==PM_j gilt. Für nichtplanare PM muß all das aber nicht unbedingt gelten
        // (z.B. Terrains und manche Bezier Patches beleuchten sich selbst)!
        // Vgl. die Erstellung und Optimierung der PatchMeshesPVS-Matrix!
        if (PatchMeshesPVS.GetValue(PM_i, PM_j)==NO_VISIBILITY) continue;

        cf::PatchMeshT& PatchMesh_j=PatchMeshes[PM_j];

#if 1
        // It would be interesting to know how fast dynamic_cast really is.
        // May it be faster to disable this "abbreviation" code after all??
        const cf::SceneGraph::FaceNodeT* FaceNode_j=dynamic_cast<const cf::SceneGraph::FaceNodeT*>(PatchMesh_j.Node);
        if (FaceNode_j && FaceNode_j->Polygon.Plane.GetDistance(Big_P_i.Coord)<0.1) continue;
#endif

        for (unsigned long Patch_j=0; Patch_j<PatchMesh_j.Patches.Size(); Patch_j++)
        {
            cf::PatchT& P_j=PatchMesh_j.Patches[Patch_j];
            if (!P_j.InsideFace) continue;


            const VectorT Ray      =P_j.Coord-Big_P_i.Coord;
            const double  RayLength=length(Ray);

            if (RayLength<0.5) continue;   // Kommt das jemals vor?

            const VectorT Dir_ij=scale(Ray, 1.0/RayLength);
            const double  cos1  = dot(Big_P_i.Normal, Dir_ij); if (cos1<=0) continue;
            const double  cos2  =-dot(P_j.Normal,     Dir_ij); if (cos2<=0) continue;

            if (CaLightWorld.TraceRay(Big_P_i.Coord, Ray)<1.0) continue;

            // 'Alternative', einfache Herleitung des Form-Faktors:
            // Betrachte die Halbkugel über dem Patch i mit Radius RayLength. RayLength soll groß genug sein,
            // d.h. Patch j soll problemlos als ein Teil der Halbkugeloberfläche betrachtet werden können.
            // Die prozentuale Sichtbarkeit erhalten wir also sofort aus P_j.Area/O, wobei O der Oberflächeninhalt der Halbkugel ist,
            // O=0.5*4*pi*RayLength^2.
            // cos1 und cos2 berücksichtigen dann noch die gegenseitige Verdrehung der Patches und wir sind fertig.
            // Einziges Problem: Obige Herleitung enthält noch einen Faktor 1/2, für den ich leider keine Erklärung habe.
            // Noch eine Alternative: Man muß RayLength ausdrücken in Patch-Längen, nicht in Millimetern!
            const double SafeRayLength=(RayLength>MinRayLength) ? RayLength : MinRayLength;

         // const double FormFactor_ij=P_j.Area/3.14159265359*cos1*cos2/(SafeRayLength*SafeRayLength);
         // const double AreaRatio_ij =Big_P_i.Area/P_j.Area;
         // const double Total_ij     =FormFactor_ij*AreaRatio_ij;
            const double Total_ij     =Big_P_i.Area/3.14159265359*cos1*cos2/SafeRayLength/SafeRayLength;

            // if (Total_ij>1.0) printf("WARNING: Total_ij==%f > 1.0\n", Total_ij);

            // Beachte: Big_P_i.UnradiatedEnergy ist schon die Summe der Einzelpatches, nicht deren Durchschnitt!
            // Clamp Total_ij to its natural maximum of 1.0 in order to avoid an increase of light energy in the system (even if REFLECTIVITY is at 100%).
            const VectorT DeltaRadiosity=scale(Big_P_i.UnradiatedEnergy, (Total_ij<1.0) ? REFLECTIVITY*Total_ij : REFLECTIVITY);

            EnergyBudget-=DeltaRadiosity;
            if (EnergyBudget.x<0.0 || EnergyBudget.y<0.0 || EnergyBudget.z<0.0)
            {
                static unsigned long Count_LastWarn=0;

                if (Count_LastWarn!=Count_AllCalls)
                {
                    Count_DivgWarnCalls++;
                    // printf("\nDivergency warning for patch mesh %lu. Total divergency warning count: %lu.\n", PM_i, Count_DivgWarnCalls);
                    Count_LastWarn=Count_AllCalls;      // The last divergency warning was generated in this call.
                }

                // return;      // Note that this *aborts* the radiation of the energy of this patch.
            }

            P_j.UnradiatedEnergy+=DeltaRadiosity;
            P_j.TotalEnergy     +=DeltaRadiosity;
            P_j.EnergyFromDir   -=Dir_ij*Max3(DeltaRadiosity);  // The -= instead of += is intentional, as we want the direction from j to i.
        }
    }
}


inline static double ComputePatchWeight(unsigned long i, unsigned long Width, double RangeWidth)
{
    const double PatchBegin=double(i  )/Width;
    const double PatchEnd  =double(i+1)/Width;

    double RangeBegin=0.5-RangeWidth*0.5;
    double RangeEnd  =0.5+RangeWidth*0.5;

    assert(PatchBegin<=PatchEnd);
    assert(RangeBegin<=RangeEnd);

    // Clip the range against the patch (build the union of the two intervals).
    if (PatchBegin>=RangeEnd  ) return 0;   // The union is empty.
    if (PatchEnd  <=RangeBegin) return 0;   // The union is empty.

    if (RangeBegin<PatchBegin) RangeBegin=PatchBegin;
    if (RangeEnd  >PatchEnd  ) RangeEnd  =PatchEnd;

    return (RangeEnd-RangeBegin)/(PatchEnd-PatchBegin);
}


void DirectLighting(const CaLightWorldT& CaLightWorld, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& AllEnts, const char BLOCK_SIZE, const double METERS_PER_WORLD_UNIT)
{
    const cf::SceneGraph::BspTreeNodeT& Map=CaLightWorld.GetBspTree();

    printf("\n%-50s %s\n", "*** PHASE I - performing direct lighting ***", GetTimeSinceProgramStart());


    // 1. Area light sources
    // *********************

    // A patch mesh is an area light source if it has a material for which a radiant exitance value has been defined.
    // Here we assign such patch meshes their initial radiating power and also make them radiate it off into the environment.
    unsigned long LightSourceCount=0;

    for (unsigned long PatchMeshNr=0; PatchMeshNr<PatchMeshes.Size(); PatchMeshNr++)
    {
        cf::PatchMeshT& PM     =PatchMeshes[PatchMeshNr];
        const VectorT   RadExit=VectorT(PM.Material->meta_RadiantExitance_Values);

        if (length(RadExit)<0.1) continue;

        LightSourceCount++;
        printf("%5.1f%%\r", (double)PatchMeshNr/PatchMeshes.Size()*100.0);
        fflush(stdout);

        double LongerSideRange =1.0;
        double ShorterSideRange=1.0;

        if (PM.Material->Name=="TechDemo/lights/reactor-light1")
        {
            LongerSideRange =0.77;
            ShorterSideRange=0.49;
        }
        else if (PM.Material->Name=="TechDemo/lights/reactor-trimlight1")
        {
            LongerSideRange =0.25;
            ShorterSideRange=0.55;
        }

        const double WidthRange =(PM.Width>=PM.Height) ? LongerSideRange : ShorterSideRange;
        const double HeightRange=(PM.Width>=PM.Height) ? ShorterSideRange : LongerSideRange;

        // Ordne den Patches den RadExit-Wert zu.
        for (unsigned long t=0; t<PM.Height; t++)
            for (unsigned long s=0; s<PM.Width; s++)
            {
                cf::PatchT& Patch=PM.Patches[t*PM.Width+s];

                // Ein Patch darf zum Leuchten nicht komplett außerhalb seiner Face liegen!
                if (!Patch.InsideFace) continue;

                const double    Weight=ComputePatchWeight(s, PM.Width, WidthRange)*ComputePatchWeight(t, PM.Height, HeightRange);
                const Vector3dT RadExitWeighted=RadExit*Weight;

                Patch.UnradiatedEnergy+=RadExitWeighted;
                Patch.TotalEnergy     +=RadExitWeighted;
                Patch.EnergyFromDir   +=Patch.Normal*Max3(RadExitWeighted);
            }

        // Die Patches dürfen auch gleich einmal strahlen.
        // Könnte man hier auch weglassen, aber so ist es eigentlich im Sinne von 'direct lighting'.
        for (unsigned long t=0; t<PM.Height; t+=BLOCK_SIZE)
            for (unsigned long s=0; s<PM.Width; s+=BLOCK_SIZE)
                RadiateEnergy(CaLightWorld, PatchMeshNr, s, t, BLOCK_SIZE);
    }
    printf("1. # area  light sources: %6lu\n", LightSourceCount);


    // 2. Point light sources
    // **********************

    ArrayT<bool> PatchMeshIsInPVS;
    PatchMeshIsInPVS.PushBackEmpty(PatchMeshes.Size());

    unsigned int RL_Count = 0;

    // Let point light sources radiate their energy into the environment.
    // Subtlety: In comparison with the sunlight calculations, point light sources cast 'harder' shadows
    // because we only consider one sample point for each patch (its Coord), instead of five as for the sunlight ray tests.
    for (unsigned int EntNr = 0; EntNr < AllEnts.Size(); EntNr++)
    {
        IntrusivePtrT<cf::GameSys::ComponentRadiosityLightT> RL = dynamic_pointer_cast<cf::GameSys::ComponentRadiosityLightT>(AllEnts[EntNr]->GetComponent("RadiosityLight"));
        if (RL == NULL) continue;

        RL_Count++;

        // "RL" is short for "Radiosity Light", "PL" is short for "Point Light".
        const Vector3dT     PL_Origin    = AllEnts[EntNr]->GetTransform()->GetOriginWS().AsVectorOfDouble();
        const Vector3dT     PL_Dir       = (cf::math::Matrix3x3fT(AllEnts[EntNr]->GetTransform()->GetQuatWS()) * Vector3fT(1, 0, 0)).AsVectorOfDouble();
        const Vector3dT     PL_Intensity = RL->GetColor().AsVectorOfDouble() * RL->GetIntensity();
        const double        cosPLAngle   = cos(RL->GetConeAngle() / 2.0f);
        const unsigned long PL_LeafNr    = Map.WhatLeaf(PL_Origin);

        if (PL_Intensity.x == 0.0 && PL_Intensity.y == 0.0 && PL_Intensity.z == 0.0) continue;

        printf("%5.1f%%\r", double(EntNr) / AllEnts.Size() * 100.0);
        fflush(stdout);

        for (unsigned long PatchMeshNr=0; PatchMeshNr<PatchMeshIsInPVS.Size(); PatchMeshNr++)
            PatchMeshIsInPVS[PatchMeshNr]=false;

        for (unsigned long LeafNr=0; LeafNr<Map.Leaves.Size(); LeafNr++)
        {
            const cf::SceneGraph::BspTreeNodeT::LeafT& L=Map.Leaves[LeafNr];

            if (!Map.IsInPVS(LeafNr, PL_LeafNr)) continue;

            for (unsigned long SetNr=0; SetNr<L.FaceChildrenSet.Size(); SetNr++)
            {
                const ArrayT<unsigned long>& PMIndices=NodePtrToPMIndices[Map.FaceChildren[L.FaceChildrenSet[SetNr]]];

                for (unsigned long i=0; i<PMIndices.Size(); i++)
                    PatchMeshIsInPVS[PMIndices[i]]=true;
            }

            for (unsigned long SetNr=0; SetNr<L.OtherChildrenSet.Size(); SetNr++)
            {
                const ArrayT<unsigned long>& PMIndices=NodePtrToPMIndices[Map.OtherChildren[L.OtherChildrenSet[SetNr]]];

                for (unsigned long i=0; i<PMIndices.Size(); i++)
                    PatchMeshIsInPVS[PMIndices[i]]=true;
            }
        }


        for (unsigned long PatchMeshNr=0; PatchMeshNr<PatchMeshes.Size(); PatchMeshNr++)
        {
            if (!PatchMeshIsInPVS[PatchMeshNr]) continue;

            cf::PatchMeshT& PM=PatchMeshes[PatchMeshNr];

#if 1
            // It would be interesting to know how fast dynamic_cast really is.
            // May it be faster to disable this "abbreviation" code after all??
            const cf::SceneGraph::FaceNodeT* FaceNode=dynamic_cast<const cf::SceneGraph::FaceNodeT*>(PM.Node);
            if (FaceNode!=NULL && FaceNode->Polygon.Plane.GetDistance(PL_Origin)<0.1) continue;
#endif

            for (unsigned long t=0; t<PM.Height; t++)
                for (unsigned long s=0; s<PM.Width; s++)
                {
                    cf::PatchT& Patch=PM.Patches[t*PM.Width+s];
                    if (!Patch.InsideFace) continue;

                    const VectorT  LightRay      =Patch.Coord-PL_Origin;    // Patch.Coord already includes a small safety distance to its plane.
                    const double   LightRayLength=length(LightRay);
                    const VectorT  LightRayDir   =scale(LightRay, 1.0/LightRayLength);
                    const double   LightRayDot   =dot(LightRayDir, Patch.Normal);   // The cosine of the angle between the two vectors.

                    // The LightRay must meet the patch from above (opposite to the direction of its normal).
                    if (LightRayDot>0.0) continue;

                    // Test if this ray is inside the defined cone.
                    if (dot(LightRayDir, PL_Dir) < cosPLAngle) continue;

                    if (CaLightWorld.TraceRay(PL_Origin, LightRay)<1.0) continue;

                    if (LightRayLength >= Map.GetLightMapPatchSize())
                    {
                        // Let I be an abbreviation for PL_Intensity, which is specified in [W/sr]. Then, any sphere centered at PL_Origin with radius
                        // r receives a total power of 4*pi*I [W] on its surface, which is equal to 4*pi*(r^2) [unit of r^2]. It follows immediately
                        // that if we choose r=1m, I/(r^2) yields the 'irradiance' in [W/m^2] for the surface of the sphere with radius r=1m.
                        // Note that 'irradiance' is a little misleading here, because we only deal with rays of light that meet at PL_Origin.
                        // The sphere is not considered and can not be treated as an area light source!
                        // Now choose r = LightRayLength * METERS_PER_WORLD_UNIT (the length of the light ray in meters, not in world units).
                        // Thus, I/((LightRayLength * METERS_PER_WORLD_UNIT)^2) gives us the 'irradiance' [W/m^2] for the surface of the sphere with radius LightRayLength.
                        // This is especially true for the point where LightRay ends. Therefore, because LightRay ends in the center of the patch we
                        // are interested in, we have calculated exactly what we need!
                        // Note that we assumed that the patch is formed like a part of the sphere. Actually, that is not true -- patches are planar.
                        // Fortunately, we can ignore that because the patch should be small enough compared to the radius of the sphere.
                        // Related to that is the fact that every point inside the patch has a different LightRay. We ignore that, too, because
                        // we consider the center of the patch and hope that by doing so, +/- errors cancel each other out.
                        // Finally, we need to take the orientation of the patch into account by multiplying with the cosine of the relative angle:
                        // -I*(1000/LightRayLength)^2*dot(VectorUnit(LightRay), F.Plane.Normal)
                        // This assumes that LightRay is not the null vector, that PL_Origin is on front of F.Plane and that F.Normal is a unit vector!
                        const double    c           = 1.0 / (LightRayLength * METERS_PER_WORLD_UNIT);
                        const Vector3dT DeltaEnergy = scale(PL_Intensity, -REFLECTIVITY*c*c*LightRayDot);

                        Patch.UnradiatedEnergy+=DeltaEnergy;
                        Patch.TotalEnergy     +=DeltaEnergy;
                        Patch.EnergyFromDir   -=LightRayDir*Max3(DeltaEnergy);  // The -= is intentional, as we want the direction from the patch to the PL.
                    }
                    else
                    {
                        // Physikalisch korrekt wäre eine bis zur Unendlichkeit zunehmende Intensität, je kleiner LightRayLength.
                        // Ist natürlich Blödsinn, da point light sources in der Realität nicht existieren.
                        // Daher erzwingen wir hier einfach eine LightRayLength von Map.GetLightMapPatchSize() und vernachlässigen
                        // die Orientierung des Patches.
                        const double  c           = 1.0 / (Map.GetLightMapPatchSize() * METERS_PER_WORLD_UNIT);
                        const VectorT DeltaEnergy = scale(PL_Intensity, REFLECTIVITY*c*c);

                        Patch.UnradiatedEnergy+=DeltaEnergy;
                        Patch.TotalEnergy     +=DeltaEnergy;
                        Patch.EnergyFromDir   -=LightRayDir*Max3(DeltaEnergy);  // The -= is intentional, as we want the direction from the patch to the PL.
                    }
                }
        }
    }
    printf("2. # point light sources: %6u\n", RL_Count);
}


#include "Ward97.cpp"   // void ToneReproduction(const cf::SceneGraph::BspTreeNodeT& Map) { ... }


void PostProcessBorders(const CaLightWorldT& CaLightWorld)
{
    // An dieser Stelle haben wir nun quasi drei Sorten von Patches für eine Face:
    // a) Patches, die 'InsideFace' liegen, und das vollständig.
    // b) Patches, die 'InsideFace' liegen, aber nur teilweise (ihre Patch.Coord ist entsprechend verschoben!).
    // c) Patches, die nicht 'InsideFace' liegen.
    // Für Patch-Sorten a) und b) hat unser Algorithmus Patch.TotalEnergy-Werte berechnet.
    // Unsere Aufgabe hier ist im wesentlichen das sinnvolle Ausfüllen der TotalEnergy-Werte von Patches der Sorte c).
    // Dies ist notwendig, da die Patches von OpenGL beim Rendern bilinear interpoliert werden (2x2-Array Durchschnitt),
    // und deswegen ohne weitere Maßnahmen schwarze Ränder bekämen.
    printf("\n%-50s %s\n", "*** Post-Process Borders ***", GetTimeSinceProgramStart());


    // ERSTER TEIL
    // ***********

    // Für alle Patches einer Face, die noch keine Energie abgekriegt haben (weil sie keinen SamplePoint innerhalb ihrer Face
    // haben oder durch z.B. BezierPatches unglücklich "abgeschattet" wurden), ermittele ihren Wert aus dem
    // Durchschnitt ihrer acht umliegenden Patches, sofern diese Energie abgekiegt haben (d.h. mit SamplePoints innerhalb
    // der Face liegen und nicht komplett abgeschattet wurden).
    // Diese Methode ist sehr einfach und schnell, da sie immer nur eine Face gleichzeitig betrachtet,
    // die Nachbarumgebung hat keinen Einfluß.
    // Dennoch ist diese Schleife ein guter Anfang, und war vorher sogar der *einzige* Nachbearbeitungsschritt!
    for (unsigned long PatchMeshNr=0; PatchMeshNr<PatchMeshes.Size(); PatchMeshNr++)
    {
        cf::PatchMeshT& PM=PatchMeshes[PatchMeshNr];
        ArrayT<bool>    InsideFaceEx;

        for (unsigned long PatchNr=0; PatchNr<PM.Patches.Size(); PatchNr++)
        {
            const Vector3dT& TE=PM.Patches[PatchNr].TotalEnergy;

            // In the extended meaning, a patch is *not* inside face if InsideFace==false or TE=(0, 0, 0).
            InsideFaceEx.PushBack(PM.Patches[PatchNr].InsideFace && (TE.x>0.5/255.0 || TE.y>0.5/255.0 || TE.z>0.5/255.0));
        }

        for (unsigned long t=0; t<PM.Height; t++)
            for (unsigned long s=0; s<PM.Width; s++)
            {
                cf::PatchT& Patch=PM.GetPatch(s, t);

                if (InsideFaceEx[t*PM.Width+s]) continue;

                Patch.TotalEnergy =VectorT(0, 0, 0);
                Vector3dT AvgDir  =VectorT(0, 0, 0);    // Do NOT(!) build the average directly in Patch.EnergyFromDir, or else you may end with zero-vector dirs for patches in the midst of faces that have received no light at all!
                double CoveredArea=0.0;

                // Der Patch liegt in der Mitte eines 3x3-Feldes bei Koordinate (1,1).
                for (char y=0; y<=2; y++)
                    for (char x=0; x<=2; x++)
                    {
                        if (x==1 && y==1) continue;     // Nur die acht umliegenden Patches betrachten.

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

                        if (!InsideFaceEx[Ny*PM.Width+Nx]) continue;

                        const double      RelevantArea=(x!=1 && y!=1) ? 0.25 : 0.5;
                        const cf::PatchT& Neighb      =PM.GetPatch(Nx, Ny);

                        Patch.TotalEnergy+=Neighb.TotalEnergy  *RelevantArea;
                        AvgDir           +=Neighb.EnergyFromDir*RelevantArea;
                        CoveredArea      +=RelevantArea;
                    }

                if (CoveredArea>0.0001)
                {
                    Patch.TotalEnergy  /=CoveredArea;
                    Patch.EnergyFromDir+=AvgDir/CoveredArea;
                }
            }
    }


    // ZWEITER TEIL
    // ************

    // Betrachte im nächsten Schritt Faces, die in einer gemeinsamen Ebene nahe beieinander liegen, und versuche,
    // den "Übergang" zu verbessern. Der vorangegangene erste Schritt eliminiert zwar Fehlfarben an den Rändern,
    // die OpenGL's bilinear Filtering ansonsten ins Spiel gebracht hätte, an Stellen mit hohen Kontrasten
    // ("scharfe" Schatten usw.) sieht man aber unbeabsichtigte, harte Übergänge an den Kanten solcher Faces.
    // Der folgende Code eliminiert solche Sprünge nun größtenteils, indem er auch die Patches der anderen Faces betrachtet.
    // Damit werden für die Ränder die "realen" Berechnungsergebnisse eingebracht, nicht einfach nur eigene Mittelwerte.
    // Der folgende Code könnte algorithmisch effizienter geschrieben sein (z.B. Vorausberechnen von wiederkehrenden
    // Werten, statt diese jedesmal in einer Schleife neu zu berechnen), aber die praktische Laufzeit ist akzeptabel.
    // Außerdem ist der Code z.T. "experimentell" oder zumindest mathematisch nicht komplett durchdacht und die zugrunde-
    // liegende Theorie ist evtl. sogar unvollständig oder falsch. Die Ergebnisse sind aber trotzdem ein voller Erfolg!
    // Es besser zu machen ist jedenfalls SEHR schwierig. Bsp: Gewichtung und Art und Weise bei den Mittelwertbildungen usw.
    // Bzgl. des "Light Bleeding" Problems scheint es sogar ÜBERHAUPT KEINE befriedigende, korrekte Lösung zu geben:
    // Selbst mit Ausführung des folgenden Codes lassen sich nicht alle "Sprünge" zwischen Faces eliminieren,
    // insbesondere an "Ecken" nicht. Der Grund liegt in der Natur der Patches, OpenGLs bilinear Filtering und dem
    // Konflikt mit dem "Light Bleeding" Problem. Eine korrekte Lösung ist somit *unmöglich*!
    // WICHTIG: Im Gegensatz z.B. zum Filtern des Sonnenlichts ist es in diesem Algorithmus *nicht* notwendig,
    // erstmal ein Backup aller TotalEnergy-Werte der Patches aller Faces anzulegen, um korrekt Mittelwerte bilden zu
    // können. Der Grund ist, daß wir nur *äußere* Patches einer Face mit *inneren* Patches der anderen Faces modifizieren!
    // Es kommt also niemals zu Überschneidungen: (*) Kein Patch wird gesetzt/modifiziert, und später nochmal für das Setzen
    // bzw. die Modifikation eines anderen Patches herangezogen! Dieses Ideal wird allerdings aufgeweicht durch die
    // Existenz von inneren Patches, die nur teilweise innerhalb ihrer Face liegen: Hier wäre eine Verletzung der Eigenschaft
    // (*) durchaus möglich. In einigen wenigen Spezialfällen kann dies durch die Eigenschaften der Plane3T<double>::GetSpanVectors()
    // Funktion geheilt werden (ohne weitere Begründung), im Allgemeinfall jedoch nicht!
    const double  PATCH_SIZE          =CaLightWorld.GetBspTree().GetLightMapPatchSize();
    unsigned long PatchesWorkedOnCount=0;

    for (unsigned long PatchMesh1Nr=0; PatchMesh1Nr<PatchMeshes.Size(); PatchMesh1Nr++)
    {
        cf::PatchMeshT&                  PM1      =PatchMeshes[PatchMesh1Nr];
        const cf::SceneGraph::FaceNodeT* FaceNode1=dynamic_cast<const cf::SceneGraph::FaceNodeT*>(PM1.Node);

        if (FaceNode1==NULL) continue;

        const Polygon3T<double>& Face1=FaceNode1->Polygon;
        ArrayT<unsigned long>    NearPatchMeshes;

        printf("%5.1f%%\r", (double)PatchMesh1Nr/PatchMeshes.Size()*100.0);
        fflush(stdout);

        // Bilde zuerst eine Liste (von Indizes) von Faces, die Face1 "nahe" sind.
        for (unsigned long PatchMesh2Nr=0; PatchMesh2Nr<PatchMeshes.Size(); PatchMesh2Nr++)
        {
            cf::PatchMeshT&                  PM2      =PatchMeshes[PatchMesh2Nr];
            const cf::SceneGraph::FaceNodeT* FaceNode2=dynamic_cast<const cf::SceneGraph::FaceNodeT*>(PM2.Node);

            if (FaceNode2==NULL) continue;

            const Polygon3T<double>& Face2=FaceNode2->Polygon;

            // Wir wollen nicht gegen uns selbst testen!
            if (PatchMesh1Nr==PatchMesh2Nr) continue;

            // Nur Faces in der gleichen Ebene mit gleicher Orientierung durchgehen lassen!
            if (Face1.WhatSide(Face2.Plane, MapT::RoundEpsilon)!=Polygon3T<double>::InIdentical) continue;

            // Faces, die zu weit voneinander entfernt liegen, brauchen nicht weiter betrachtet zu werden!
            const VectorT BorderPadding(PATCH_SIZE, PATCH_SIZE, PATCH_SIZE);

            BoundingBox3T<double> BB1(Face1.Vertices); BB1.Min=BB1.Min-BorderPadding; BB1.Max=BB1.Max+BorderPadding;
            BoundingBox3T<double> BB2(Face2.Vertices); BB2.Min=BB2.Min-BorderPadding; BB2.Max=BB2.Max+BorderPadding;

            if (!BB1.Intersects(BB2)) continue;

            // Diese Face kommt in Betracht, speichere ihre Nummer.
            NearPatchMeshes.PushBack(PatchMesh2Nr);
        }

        // Bereite nun das Patch1Poly vor. Unten werden dann nur noch die 4 Vertices ausgefüllt.
        // Der folgende Code ist SEHR ähnlich zu dem Code in cf::SceneGraph::FaceNodeT::CreatePatchMeshes()!

        // Bestimme die Spannvektoren.
        VectorT Face1_U;
        VectorT Face1_V;

        Face1.Plane.GetSpanVectors(Face1_U, Face1_V);

        // Finde SmallestU und SmallestV.
        double Face1_SmallestU=dot(Face1.Vertices[0], Face1_U);
        double Face1_SmallestV=dot(Face1.Vertices[0], Face1_V);

        for (unsigned long VertexNr=1; VertexNr<Face1.Vertices.Size(); VertexNr++)
        {
            double u=dot(Face1.Vertices[VertexNr], Face1_U);
            double v=dot(Face1.Vertices[VertexNr], Face1_V);

            if (u<Face1_SmallestU) Face1_SmallestU=u;
            if (v<Face1_SmallestV) Face1_SmallestV=v;
        }

        const VectorT Face1_UV_Origin=scale(Face1.Plane.Normal, Face1.Plane.Dist);
        const VectorT Face1_Safety   =scale(Face1.Plane.Normal, 0.1);

        Polygon3T<double> Patch1Poly;

        Patch1Poly.Plane=dot(Face1.Plane.Normal, cross(Face1_U, Face1_V))<0 ? Face1.Plane : Face1.Plane.GetMirror();
        Patch1Poly.Vertices.PushBackEmpty(4);

        // Betrachte nun alle Patches von Face1
        for (unsigned long t1=0; t1<PM1.Height; t1++)
            for (unsigned long s1=0; s1<PM1.Width; s1++)
            {
                cf::PatchT&     Patch1=PM1.Patches[t1*PM1.Width+s1];
                ArrayT<double>  Patch1_OverlapRatios;
                ArrayT<VectorT> Patch1_OverlapTotalEnergies;
                ArrayT<VectorT> Patch1_OverlapEnergyFromDirs;

                // Rekonstruiere das Polygon zu Patch1
                Patch1Poly.Vertices[0]=Face1_UV_Origin+scale(Face1_U, Face1_SmallestU+(s1-1.0)*PATCH_SIZE)+scale(Face1_V, Face1_SmallestV+(t1-1.0)*PATCH_SIZE);
                Patch1Poly.Vertices[1]=Face1_UV_Origin+scale(Face1_U, Face1_SmallestU+ s1     *PATCH_SIZE)+scale(Face1_V, Face1_SmallestV+(t1-1.0)*PATCH_SIZE);
                Patch1Poly.Vertices[2]=Face1_UV_Origin+scale(Face1_U, Face1_SmallestU+ s1     *PATCH_SIZE)+scale(Face1_V, Face1_SmallestV+ t1     *PATCH_SIZE);
                Patch1Poly.Vertices[3]=Face1_UV_Origin+scale(Face1_U, Face1_SmallestU+(s1-1.0)*PATCH_SIZE)+scale(Face1_V, Face1_SmallestV+ t1     *PATCH_SIZE);

                // WICHTIG: Falls Patch1 *vollständig in* seiner Face liegt, haben wir für dessen TotalEnergy einen
                // einwandfreien Berechnungswert, und wir wollen daran nicht rumfummeln!
                // Die anderen beiden Fälle für Patch1 sind:
                // 1) 'InsideFace', aber nicht vollständig, d.h. Patch1Poly ragt etwas aus seiner Face heraus.
                // 2) Nicht 'InsideFace', der Patch hat bestenfalls oben im ersten Teil einen Wert zugewiesen bekommen.
                // Diese beiden Fälle wollen wir also nachbearbeiten, und zwar durch Mittelwertbildung mit überlappenden,
                // INNEREN Patches von benachbarten Faces. Mehr dazu unten. Lasse also 1) und 2) passieren
                // (die alte Bed. "if (Patch1.InsideFace) continue;" hätte nur 2) durchgehen lassen).
                if (Face1.Encloses(Patch1Poly, true, MapT::RoundEpsilon)) continue;

                // Den Mittelpunkt des Patch1Poly bestimmen, inkl. "Safety", sowie den Flächeninhalt
                VectorT Patch1Poly_Center=scale(Patch1Poly.Vertices[0]+Patch1Poly.Vertices[1]+Patch1Poly.Vertices[2]+Patch1Poly.Vertices[3], 0.25)+Face1_Safety;
                double  Patch1Poly_Area  =Patch1Poly.GetArea();

                // Suche einen Punkt *IN* Face1 heraus, der "nahe" bei Patch1 liegt. Wird unten benötigt.
                double  MinDistance=3.0*PATCH_SIZE;
                VectorT InnerPointCloseToPatch1;

                for (unsigned long PatchNr=0; PatchNr<PM1.Patches.Size(); PatchNr++)
                {
                    const cf::PatchT& TempPatch=PM1.Patches[PatchNr];

                    // 'TempPatch' darf auch ruhig 'Patch1' sein, da 'Patch1.Coord' durchaus ein Punkt in Face1 ist,
                    // der "nahe" bei Patch1 liegt!
                    if (!TempPatch.InsideFace) continue;

                    double Distance=length(TempPatch.Coord-Patch1Poly_Center);

                    if (Distance<MinDistance)
                    {
                        MinDistance            =Distance;
                        InnerPointCloseToPatch1=TempPatch.Coord;
                    }
                }

                // Wurde auch etwas in der Nähe gefunden?
                if (MinDistance==3.0*PATCH_SIZE) continue;

                // Betrachte nun die umliegenden Faces
                for (unsigned long NearNr=0; NearNr<NearPatchMeshes.Size(); NearNr++)
                {
                    cf::PatchMeshT&                  PM2      =PatchMeshes[NearPatchMeshes[NearNr]];
                    const cf::SceneGraph::FaceNodeT* FaceNode2=dynamic_cast<const cf::SceneGraph::FaceNodeT*>(PM2.Node);

                    assert(FaceNode2!=NULL);

                    const Polygon3T<double>& Face2=FaceNode2->Polygon;

                    // Bereite nun das Patch2Poly vor. Unten werden dann nur noch die 4 Vertices ausgefüllt.
                    // Der folgende Code ist SEHR ähnlich zu dem Code in InitializePatches() (Init2.cpp)!
                    VectorT Face2_U;
                    VectorT Face2_V;

                    Face2.Plane.GetSpanVectors(Face2_U, Face2_V);

                    double Face2_SmallestU=dot(Face2.Vertices[0], Face2_U);   // Finde SmallestU und SmallestV
                    double Face2_SmallestV=dot(Face2.Vertices[0], Face2_V);

                    unsigned long VertexNr;

                    for (VertexNr=1; VertexNr<Face2.Vertices.Size(); VertexNr++)
                    {
                        double u=dot(Face2.Vertices[VertexNr], Face2_U);
                        double v=dot(Face2.Vertices[VertexNr], Face2_V);

                        if (u<Face2_SmallestU) Face2_SmallestU=u;
                        if (v<Face2_SmallestV) Face2_SmallestV=v;
                    }

                    const VectorT Face2_UV_Origin=scale(Face2.Plane.Normal, Face2.Plane.Dist);

                    Polygon3T<double> Patch2Poly;

                    Patch2Poly.Plane=dot(Face2.Plane.Normal, cross(Face2_U, Face2_V))<0 ? Face2.Plane : Face2.Plane.GetMirror();
                    Patch2Poly.Vertices.PushBackEmpty(4);

                    // Gehe die Patches von Face2 durch
                    for (unsigned long t2=0; t2<PM2.Height; t2++)
                        for (unsigned long s2=0; s2<PM2.Width; s2++)
                        {
                            const cf::PatchT& Patch2=PM2.Patches[t2*PM2.Width+s2];

                            // Nur "äußere" Patches von Face1 mit "inneren" Patches von Face1 korrigieren!
                            if (!Patch2.InsideFace) continue;

                            // Rekonstruiere das Polygon zu Patch2
                            Patch2Poly.Vertices[0]=Face2_UV_Origin+scale(Face2_U, Face2_SmallestU+(s2-1.0)*PATCH_SIZE)+scale(Face2_V, Face2_SmallestV+(t2-1.0)*PATCH_SIZE);
                            Patch2Poly.Vertices[1]=Face2_UV_Origin+scale(Face2_U, Face2_SmallestU+ s2     *PATCH_SIZE)+scale(Face2_V, Face2_SmallestV+(t2-1.0)*PATCH_SIZE);
                            Patch2Poly.Vertices[2]=Face2_UV_Origin+scale(Face2_U, Face2_SmallestU+ s2     *PATCH_SIZE)+scale(Face2_V, Face2_SmallestV+ t2     *PATCH_SIZE);
                            Patch2Poly.Vertices[3]=Face2_UV_Origin+scale(Face2_U, Face2_SmallestU+(s2-1.0)*PATCH_SIZE)+scale(Face2_V, Face2_SmallestV+ t2     *PATCH_SIZE);

                            // Überlappen sich PatchPoly1 und PatchPoly2?
                            if (!Patch1Poly.Overlaps(Patch2Poly, false, MapT::RoundEpsilon)) continue;

                            // Zerschneide Patch2Poly entlang Patch1Poly, und behalte nur das Stück, das "in" Patch1Poly liegt:
                            ArrayT< Polygon3T<double> > NewPolygons;

                            Patch2Poly.GetChoppedUpAlong(Patch1Poly, MapT::RoundEpsilon, NewPolygons);
                            if (NewPolygons.Size()==0) Error("PolygonChopUp failed in PostProcessBorders().");

                            // Bestimme den Mittelpunkt des überlappenden Stücks in Patch1Poly (inkl. "Safety") und prüfe,
                            // ob von dort aus der nahe Punkt in Face1 erreichbar ist.
                            const Polygon3T<double>& OverlapPoly      =NewPolygons[NewPolygons.Size()-1];
                            VectorT                  OverlapPolyCenter=OverlapPoly.Vertices[0];

                            for (VertexNr=1; VertexNr<OverlapPoly.Vertices.Size(); VertexNr++)
                                OverlapPolyCenter=OverlapPolyCenter+OverlapPoly.Vertices[VertexNr];

                            OverlapPolyCenter=scale(OverlapPolyCenter, 1.0/double(OverlapPoly.Vertices.Size()))+Face1_Safety;

                            // Begründung für den folgenden Test:
                            // Es besteht die Gefahr, daß wir an dieser Stelle unerwünschtes "Light Bleeding" erzeugen.
                            // "Light Bleeding" ist die Beeinflussung von Patches durch andere Patches, deren Faces sich
                            // zwar nahe sind, aber in Wirklichkeit z.B. durch eine dünne "Wand" getrennt,
                            // oder die Patches liegen "um die Ecke".
                            // Um das "Light Bleeding" Problem zu minimieren, erlauben wir die Beeinflussung von Patch1
                            // durch Patch2 nur dann, wenn das 'OverlapPolyCenter' vom 'InnerPointCloseToPatch1' aus
                            // sichtbar ist.
                            // All dies ist analytisch nicht wirklich befriedigend -- eine bessere Lösung scheint es
                            // aber auch nicht zu geben: Zu groß ist der Konflikt bzw. die gestellten Ansprüche.
                            // Das praktische Ergebnis ist allerdings sehr wohl brauchbar, denn das Ziel wird,
                            // abgesehen von kleineren "Ausreißern", erreicht.
                            if (CaLightWorld.TraceRay(InnerPointCloseToPatch1, OverlapPolyCenter-InnerPointCloseToPatch1)<1.0) continue;

                            // Zu wieviel Prozent überlappt das verbleibende Stück PatchPoly1?
                            double OverlapRatio=OverlapPoly.GetArea()/Patch1Poly_Area;

                            // 'OverlapRatio*=0.5;', falls 'Patch1.InsideFace==true' ist!?
                            // Merke das Ergebnis zur späteren Durchschnittsbildung.
                            Patch1_OverlapRatios        .PushBack(OverlapRatio        );
                            Patch1_OverlapTotalEnergies .PushBack(Patch2.TotalEnergy  );
                            Patch1_OverlapEnergyFromDirs.PushBack(Patch2.EnergyFromDir);
                        }
                }


                // Die folgende Zeile ist nicht wirklich nötig. Der 'PatchesWorkedOnCount' wird dadurch aber sinnvoller.
                if (Patch1_OverlapRatios.Size()==0) continue;

                double OverlapRatioSum=0.0;

                for (unsigned long OverlapNr=0; OverlapNr<Patch1_OverlapRatios.Size(); OverlapNr++)
                {
                    if (Patch1_OverlapRatios[OverlapNr]<0.000) Error("Negative overlap percentage!");
                    if (Patch1_OverlapRatios[OverlapNr]>1.001) Error("Overlap is greater than 100%%!");

                    OverlapRatioSum+=Patch1_OverlapRatios[OverlapNr];
                }

                if (OverlapRatioSum>1.0)
                {
                    // Da eine Face an einem Vertice mit beliebig vielen anderen Faces "zusammenstoßen" kann,
                    // kann ein Patch dieser Face dort von beliebig vielen anderen Patches überlagert werden,
                    // die sich dann z.T. wieder unter sich überlappen.
                    // So kann eine Abdeckung mit Patches über 100% entstehen.
                    // Wir skalieren dann einfach wieder auf 100% zurück.
                    for (unsigned long OverlapNr=0; OverlapNr<Patch1_OverlapRatios.Size(); OverlapNr++)
                        Patch1_OverlapRatios[OverlapNr]/=OverlapRatioSum;
                }
                else
                {
                    // Patch1 war nicht ganz von anderen Patches bedeckt.
                    // Fülle daher den Rest mit dem eigenen, alten Wert auf!
                    Patch1_OverlapRatios        .PushBack(1.0-OverlapRatioSum);
                    Patch1_OverlapTotalEnergies .PushBack(Patch1.TotalEnergy);
                    Patch1_OverlapEnergyFromDirs.PushBack(Patch1.EnergyFromDir);
                }

                Patch1.TotalEnergy  =VectorT(0,0,0);
                Patch1.EnergyFromDir=VectorT(0,0,0);

                for (unsigned long OverlapNr=0; OverlapNr<Patch1_OverlapRatios.Size(); OverlapNr++)
                {
                    Patch1.TotalEnergy  +=scale(Patch1_OverlapTotalEnergies [OverlapNr], Patch1_OverlapRatios[OverlapNr]);
                    Patch1.EnergyFromDir+=scale(Patch1_OverlapEnergyFromDirs[OverlapNr], Patch1_OverlapRatios[OverlapNr]);
                }

                PatchesWorkedOnCount++;
            }
    }

    printf("Borders completed. %lu patches modified in 2nd part.\n", PatchesWorkedOnCount);
}


unsigned long BounceLighting(const CaLightWorldT& CaLightWorld, const char BLOCK_SIZE, double& StopUE, const bool AskForMore, const char* WorldName)
{
    printf("\n%-50s %s\n", "*** PHASE II - performing bounce lighting ***", GetTimeSinceProgramStart());

    unsigned long IterationCount =0;
    unsigned long FullSearchCount=0;

    while (true)
    {
        unsigned long PM_i  =0;
        unsigned long s_i   =0;
        unsigned long t_i   =0;
        double        BestUE=0;     // Best unradiated energy amount found

        // Finde ein PatchMesh mit einem Patch mit einer großen UnradiatedEnergy (nicht notwendigerweise die größte, damit es schnell geht).
        for (unsigned long PatchMeshNr=0; PatchMeshNr<PatchMeshes.Size(); PatchMeshNr++)
        {
            // Achtung: PM.Patches.Size() kann auch 0 sein!
            const cf::PatchMeshT& PM         =PatchMeshes[PatchMeshNr];
            unsigned long         NrOfSamples=PM.Patches.Size()<10 ? PM.Patches.Size()/2 : 10;

            for (unsigned long SampleNr=0; SampleNr<NrOfSamples; SampleNr++)
            {
                unsigned long s=rand() % PM.Width;      // Funktioniert nicht mehr gut wenn PM.Width >RAND_MAX
                unsigned long t=rand() % PM.Height;     // Funktioniert nicht mehr gut wenn PM.Height>RAND_MAX

                s=(s/BLOCK_SIZE)*BLOCK_SIZE;
                t=(t/BLOCK_SIZE)*BLOCK_SIZE;

                unsigned long Count =0;
                double        ThisUE=0;

                for (char y=0; y<BLOCK_SIZE; y++)
                    for (char x=0; x<BLOCK_SIZE; x++)
                    {
                        unsigned long s_=s+x;
                        unsigned long t_=t+y;

                        if (PM.WrapsHorz && s_>=PM.Width ) s_-=PM.Width;
                        if (PM.WrapsVert && t_>=PM.Height) t_-=PM.Height;

                        if (s_>=PM.Width ) continue;
                        if (t_>=PM.Height) continue;

                        const cf::PatchT& P_i=PM.Patches[t_*PM.Width+s_];
                        if (!P_i.InsideFace) continue;

                        ThisUE+=P_i.UnradiatedEnergy.x+P_i.UnradiatedEnergy.y+P_i.UnradiatedEnergy.z;
                        Count++;
                    }

                if (!Count) continue;
                ThisUE/=double(Count);

                if (ThisUE>BestUE)
                {
                    PM_i  =PatchMeshNr;
                    s_i   =s;
                    t_i   =t;
                    BestUE=ThisUE;
                }
            }
        }

        // Sollte die BestUE unter StopUE sein, wird hier nach einem besseren Wert gesucht.
        // Beim erstbesseren Wert wird dieser genommen und abgebrochen, ansonsten gesucht bis zum Schluß.
        // Wenn alle Faces nach dem besten Wert durchsucht werden sollen, muß "&& BestUE<StopUE" auskommentiert werden.
        if (BestUE<StopUE)
        {
            FullSearchCount++;      // Zähle, wie oft wir alles abgesucht haben!

            for (unsigned long PatchMeshNr=0; PatchMeshNr<PatchMeshes.Size() && BestUE<StopUE; PatchMeshNr++)
            {
                const cf::PatchMeshT& PM=PatchMeshes[PatchMeshNr];

                for (unsigned long s=0; s<PM.Width; s++)
                    for (unsigned long t=0; t<PM.Height; t++)
                    {
                        const cf::PatchT& Patch = PM.Patches[t*PM.Width + s];

                        if (!Patch.InsideFace) continue;

                        const VectorT& E      = Patch.UnradiatedEnergy;
                        const double   ThisUE = E.x + E.y + E.z;

                        if (ThisUE>BestUE)
                        {
                            PM_i  =PatchMeshNr;
                            s_i   =s;
                            t_i   =t;
                            BestUE=ThisUE;
                        }
                    }
            }
        }

        printf("Iteration%6lu, BestUE %6.2f, PM_i%6lu, FullSearch%4lu (%5.1f%%)\r", IterationCount, BestUE, PM_i, FullSearchCount, 100.0*float(FullSearchCount)/float(IterationCount+1));
        fflush(stdout);

        if (BestUE<StopUE)  // Es gab keinen besseren Wert mehr -- wir können also abbrechen!
        {
            printf("\n");
            if (!AskForMore) break;

            time_t StartTime=time(NULL);

            printf("\nStopUE value %10.7f has been reached.\n", StopUE);
            printf("Press 'y' to confirm exit and save current result,\n");
            printf("or any other key to divide StopUE by 10 and continue lighting!\n");

            char Key=_getch(); if (Key==0) (void)_getch();

            unsigned long TotalSec=(unsigned long)difftime(time(NULL), StartTime);
            unsigned long Sec     =TotalSec % 60;
            unsigned long Min     =(TotalSec/60) % 60;
            unsigned long Hour    =TotalSec/3600;
            printf("Length of break (waiting for your decision) was %2lu:%2lu:%2lu.\n", Hour, Min, Sec);

            if (Key=='y') break;
            StopUE/=10.0;
        }

        RadiateEnergy(CaLightWorld, PM_i, s_i, t_i, BLOCK_SIZE);
        IterationCount++;

#ifdef _WIN32
        // TODO: Ein (sinnvolles!) 'kbhit()' Äquivalent für Linux muß erst noch gefunden werden...
        if (_kbhit())
        {
            char Key=_getch(); if (Key==0) (void)_getch();

            if (Key==' ')
            {
                printf("\nINTERRUPTED BY USER!\n");
                printf("Enter 'Y' to confirm exit and save current result,\n");
                printf("Enter 'S' to save the intermediate result, then continue,\n");
                printf("or press any other key to continue lighting!\n");

                Key=_getch(); if (Key==0) (void)_getch();

                if (Key=='Y') break;
                if (Key=='S')
                {
                    printf("\n%-50s %s\n", "*** START Saving of Intermediate Results ***", GetTimeSinceProgramStart());
                    ArrayT<cf::PatchMeshT> SafePMs=PatchMeshes;


                    ToneReproduction(CaLightWorld);
                    PostProcessBorders(CaLightWorld);

                    printf("\n%-50s %s\n", "*** Write Patch values back into LightMaps ***", GetTimeSinceProgramStart());
                    for (unsigned long PatchMeshNr=0; PatchMeshNr<PatchMeshes.Size(); PatchMeshNr++)
                    {
                        cf::PatchMeshT&               PM     =PatchMeshes[PatchMeshNr];
                        cf::SceneGraph::GenericNodeT* PM_Node=const_cast<cf::SceneGraph::GenericNodeT*>(PM.Node);

                        // Need a non-const pointer to the "source" NodeT of the patch mesh here.
                        PM_Node->BackToLightMap(PM, CaLightWorld.GetBspTree().GetLightMapPatchSize());
                    }

                    printf("\n%-50s %s\n", "*** Saving World ***", GetTimeSinceProgramStart());
                    std::string SaveName=std::string(WorldName)+"_";
                    printf("%s\n", SaveName.c_str());
                    CaLightWorld.SaveToDisk(SaveName.c_str());


                    // Restore the state as it was before the saving.
                    PatchMeshes=SafePMs;
                    printf("\n%-50s %s\n", "*** END Saving of Intermediate Results ***", GetTimeSinceProgramStart());
                }
            }
        }
#endif
    }

    return IterationCount;
}


void Usage()
{
    printf("\n");
    printf("USAGE: CaLight WorldName [OPTIONS]\n");
    printf("\n");
    printf("-gd=somePath   Specifies the game directory of the world. Try for example\n");
    printf("               \"Games/DeathMatch\" or whatever fits on your system.\n");
    printf("-BlockSize n   Radiative block size for faster bounce lighting.\n");
    printf("               n must be in range 1..8, default is 3.\n");
    printf("-StopUE f      Stop value for unradiated energy. 0 < f <= 10, default is 1.0.\n");
    printf("-AskForMore    Asks for a new StopUE value when the old one has been reached.\n");
    printf("-UseBS4DL      Normally, direct area lighting uses a 'BlockSize' value of 1.\n");
    printf("               Use this to use the same value as for bounce lighting.\n");
    printf("-onlyEnts      Process entities only (not the world).\n");
    printf("-fast          Same as \"-BlockSize 5 -UseBS4DL\".\n");
    printf("\n");
    printf("\n");
    printf("EXAMPLES:\n");
    printf("\n");
    printf("CaLight WorldName -AskForMore -gd=Games/DeathMatch\n");
    printf("    I'll start with the default parameters, show you the 'Options' dialog box,\n");
    printf("    light the world WorldName and finally ask you what to do when the\n");
    printf("    StopUE value has been reached.\n");
    printf("    \"Games/DeathMatch\" is searched for this worlds game related stuff.\n");
    printf("\n");
    printf("CaLight WorldName -StopUE 0.1\n");
    printf("    Most worlds of the Cafu demo release are lit with these switches.\n");
    printf("    \".\" (the default directory for -gd) is searched for game related stuff.\n");
    printf("\n");
    printf("CaLight WorldName -BlockSize 1 -StopUE 0.1\n");
    printf("    This is ideal for batch file processing: WorldName is lit without further\n");
    printf("    user questioning and I'll terminate as soon as StopUE has been reached.\n");
    printf("    Note that BlockSize and StopUE are set for high-quality lighting here.\n");
    printf("\n");
    printf("CaLight WorldName -fast\n");
    printf("CaLight WorldName -BlockSize 5 -UseBS4DL\n");
    printf("    Fast and ugly lighting, intended for quick tests during world development.\n");
    exit(1);
}


static void WriteLogFileEntry(const char* WorldPathName, double StopUE, char BlockSize, unsigned long IterationCount)
{
    char          DateTime [256]="unknown";
    char          HostName [256]="unknown";
    char          WorldName[256]="unknown";
    time_t        Time          =time(NULL);
    unsigned long RunningSec    =(unsigned long)difftime(Time, ProgramStartTime);
    FILE*         LogFile       =fopen("CaLight.log", "a");

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
    fprintf(LogFile, "%-16s %-16s%3lu:%02lu:%02lu [%-16s]%8.5f %ux%u%8lu\n", DateTime, WorldName, RunningSec/3600, (RunningSec/60) % 60, RunningSec % 60, HostName, StopUE, BlockSize, BlockSize, IterationCount);
    fclose(LogFile);
}


int main(int ArgC, const char* ArgV[])
{
    cf::GameSys::GetComponentTIM().Init();      // The one-time init of the GameSys components type info manager.
    cf::GameSys::GetGameSysEntityTIM().Init();  // The one-time init of the GameSys entity type info manager.
    cf::GameSys::GetWorldTIM().Init();          // The one-time init of the GameSys world type info manager.

    struct CaLightOptionsT
    {
        std::string GameDirName;
        char        BlockSize;
        double      StopUE;
        bool        AskForMore;
        bool        UseBlockSizeForDirectL;
        bool        EntitiesOnly;

        CaLightOptionsT() : GameDirName("."), BlockSize(3), StopUE(1.0), AskForMore(false), UseBlockSizeForDirectL(false), EntitiesOnly(false) {}
    } CaLightOptions;


    // Init screen
    printf("\n*** Cafu Lighting Utility, Version 3 (%s) ***\n\n\n", __DATE__);

#ifndef _WIN32
    printf("Reminder:\n");
    printf("The Linux version of CaLight is equivalent to the Win32 version, except that\n");
    printf("the 2nd phase (bounce lighting) cannot manually be terminated\n");
    printf("by the user ahead of time.\n\n");
#endif

    // Run the testcase of the new matrix code.
    { DiagMatrixT TestM; if (!TestM.Test()) Error("DiagMatrixT::Test() failed."); }

    if (ArgC<2) Usage();

    // Initialize the FileMan by mounting the default file system.
    // Note that specifying "./" (instead of "") as the file system description effectively prevents the use of
    // absolute paths like "D:\abc\someDir\someFile.xy" or "/usr/bin/xy". This however should be fine for this application.
    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_LOCAL_PATH, "./", "");
    // cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/DeathMatch/Textures/TechDemo.zip", "Games/DeathMatch/Textures/TechDemo/", "Ca3DE");
    // cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/DeathMatch/Textures/SkyDomes.zip", "Games/DeathMatch/Textures/SkyDomes/", "Ca3DE");

    // Process command line
    for (int CurrentArg=2; CurrentArg<ArgC; CurrentArg++)
    {
        if (_strnicmp(ArgV[CurrentArg], "-gd=", 4)==0)
        {
            CaLightOptions.GameDirName=ArgV[CurrentArg]+4;
        }
        else if (!_stricmp(ArgV[CurrentArg], "-BlockSize"))
        {
            if (CurrentArg+1==ArgC) Error("I can't find a number after \"-BlockSize\"!");
            CurrentArg++;
            CaLightOptions.BlockSize=atoi(ArgV[CurrentArg]);
            if (CaLightOptions.BlockSize<1 || CaLightOptions.BlockSize>8) Error("BlockSize must be in range 1..8.");
        }
        else if (!_stricmp(ArgV[CurrentArg], "-StopUE"))
        {
            if (CurrentArg+1==ArgC) Error("I can't find a number after \"-StopUE\"!");
            CurrentArg++;
            CaLightOptions.StopUE=atof(ArgV[CurrentArg]);
            if (CaLightOptions.StopUE<=0.0 || CaLightOptions.StopUE>10.0) Error("StopUE must be in ]0, 10].");
        }
        else if (!_stricmp(ArgV[CurrentArg], "-AskForMore"))
        {
            CaLightOptions.AskForMore=true;
        }
        else if (!_stricmp(ArgV[CurrentArg], "-UseBS4DL"))
        {
            CaLightOptions.UseBlockSizeForDirectL=true;
        }
        else if (!_stricmp(ArgV[CurrentArg], "-onlyEnts"))
        {
            CaLightOptions.EntitiesOnly=true;
        }
        else if (!_stricmp(ArgV[CurrentArg], "-fast"))
        {
            CaLightOptions.BlockSize=5;
            CaLightOptions.UseBlockSizeForDirectL=true;
        }
        else if (ArgV[CurrentArg][0]==0)
        {
            // The argument is "", the empty string.
            // This can happen under Linux, when CaLight is called via wxExecute() with white-space trailing the command string.
        }
        else
        {
            printf("\nSorry, I don't know what \"%s\" means.\n", ArgV[CurrentArg]);
            Usage();
        }
    }


    // Setup the global MaterialManager pointer.
    static MaterialManagerImplT MatManImpl;

    MaterialManager=&MatManImpl;

    if (MaterialManager->RegisterMaterialScriptsInDir(CaLightOptions.GameDirName+"/Materials", CaLightOptions.GameDirName+"/").Size()==0)
    {
        printf("\nNo materials found in scripts in \"%s/Materials\".\n", CaLightOptions.GameDirName.c_str());
        printf("Please use the -gd=... option in order to specify the game directory name,\n");
        printf("or run CaLight without any parameters for more help.\n");
        Error("No materials found.");
    }


    try
    {
        printf("Loading World '%s'.\n", ArgV[1]);
        ModelManagerT             ModelMan;
        cf::GuiSys::GuiResourcesT GuiRes(ModelMan);
        CaLightWorldT             CaLightWorld(ArgV[1], ModelMan, GuiRes);

        std::string ScriptName = cf::String::StripExt(ArgV[1]) + ".cent";
        ScriptName = cf::String::Replace(ScriptName, "/Worlds/", "/Maps/");
        ScriptName = cf::String::Replace(ScriptName, "\\Worlds\\", "\\Maps\\");

        cf::UniScriptStateT                ScriptState;
        IntrusivePtrT<cf::GameSys::WorldT> ScriptWorld;

        cf::GameSys::WorldT::InitScriptState(ScriptState);

        ScriptWorld = new cf::GameSys::WorldT(
            cf::GameSys::WorldT::RealmOther,
            ScriptState,
            ModelMan,
            GuiRes,
            *cf::ClipSys::CollModelMan,   // TODO: The CollModelMan should not be a global, but rather be instantiated along with the ModelMan and GuiRes.
            NULL,       // No clip world for this instance.
            NULL);      // No physics world for this instance.

        ScriptWorld->LoadScript(ScriptName, cf::GameSys::WorldT::InitFlag_OnlyStatic);

        ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > AllEnts;
        ScriptWorld->GetRootEntity()->GetAll(AllEnts);

        unsigned long IterationCount=0;

        if (!CaLightOptions.EntitiesOnly)
        {
            // Print out options summary
            const char BlockSize4DirectLighting=CaLightOptions.UseBlockSizeForDirectL ? CaLightOptions.BlockSize : 1;

            printf("\n");
            printf("- BlockSize is %ux%u.\n", CaLightOptions.BlockSize, CaLightOptions.BlockSize);
            printf("- StopUE    is %.3f.\n", CaLightOptions.StopUE);
            printf("- I will %s you for more.\n", CaLightOptions.AskForMore ? "ASK" : "NOT ask");
            printf("- BlockSize for direct lighting is %ux%u.\n", BlockSize4DirectLighting, BlockSize4DirectLighting);

            // Initialize
            InitializePatches(CaLightWorld);                // Init2.cpp

            // Assert that "outer" patches don't have any energy.
            for (unsigned long PatchMeshNr = 0; PatchMeshNr < PatchMeshes.Size(); PatchMeshNr++)
            {
                const cf::PatchMeshT& PM = PatchMeshes[PatchMeshNr];

                for (unsigned long PatchNr = 0; PatchNr < PM.Patches.Size(); PatchNr++)
                {
                    const cf::PatchT& Patch = PM.Patches[PatchNr];

                    if (!Patch.InsideFace && (Patch.UnradiatedEnergy != Vector3dT() || Patch.TotalEnergy != Vector3dT()))
                    {
                        Error("There is a patch that is not inside its face, but has energy!");
                    }
                }
            }

            // Create a mapping from NodeTs to their patch meshes, with the patch meshes being given as a list of indices into the PatchMeshes array.
            for (unsigned long PatchMeshNr=0; PatchMeshNr<PatchMeshes.Size(); PatchMeshNr++)
                NodePtrToPMIndices[PatchMeshes[PatchMeshNr].Node].PushBack(PatchMeshNr);

            InitializePatchMeshesPVSMatrix(CaLightWorld);   // Init1.cpp

            // Perform lighting
            DirectLighting(CaLightWorld, AllEnts, BlockSize4DirectLighting, ScriptWorld->GetMillimetersPerWorldUnit() / 1000.0);
            IterationCount=BounceLighting(CaLightWorld, CaLightOptions.BlockSize, CaLightOptions.StopUE, CaLightOptions.AskForMore, ArgV[1]);

            printf("Info: %lu calls to RadiateEnergy() caused %lu potential divergency events.\n", Count_AllCalls, Count_DivgWarnCalls);


            ToneReproduction(CaLightWorld);                                     // Ward97.cpp
            PostProcessBorders(CaLightWorld);

            printf("\n%-50s %s\n", "*** Write Patch values back into LightMaps ***", GetTimeSinceProgramStart());
            for (unsigned long PatchMeshNr=0; PatchMeshNr<PatchMeshes.Size(); PatchMeshNr++)
            {
                cf::PatchMeshT&               PM     =PatchMeshes[PatchMeshNr];
                cf::SceneGraph::GenericNodeT* PM_Node=const_cast<cf::SceneGraph::GenericNodeT*>(PM.Node);

                // Need a non-const pointer to the "source" NodeT of the patch mesh here.
                PM_Node->BackToLightMap(PM, CaLightWorld.GetBspTree().GetLightMapPatchSize());
            }
        }

        // Create (fake) lightmaps for (brush or bezier patch based) entities.
        printf("\n%-50s %s\n", "*** Creating entity lightmaps ***", GetTimeSinceProgramStart());
        CaLightWorld.CreateLightMapsForEnts(AllEnts);

        printf("\n%-50s %s\n", "*** Saving World ***", GetTimeSinceProgramStart());
        printf("%s\n", ArgV[1]);
        CaLightWorld.SaveToDisk(ArgV[1]);


        WriteLogFileEntry(ArgV[1], CaLightOptions.StopUE, CaLightOptions.BlockSize, IterationCount);
        printf("\n%-50s %s\n", "COMPLETED.", GetTimeSinceProgramStart());
    }
    catch (const WorldT::LoadErrorT& E)
    {
        printf("\nType \"CaLight\" (without any parameters) for help.\n");
        Error(E.Msg);
    }
    catch (const WorldT::SaveErrorT& E)
    {
        printf("\nType \"CaLight\" (without any parameters) for help.\n");
        Error(E.Msg);
    }
    catch (const cf::GameSys::WorldT::InitErrorT& IE)
    {
        Error(IE.what());
    }

    return 0;
}
