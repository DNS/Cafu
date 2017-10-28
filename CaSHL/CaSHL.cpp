/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/************************************************/
/***                                          ***/
/*** Cafu Spherical Harmonic Lighting Utility ***/
/***                                          ***/
/*** Der Herr sprach                          ***/
/***   Es werde Licht!                        ***/
/*** Und es wurde Licht.                      ***/
/*** Genesis                                  ***/
/***                                          ***/
/************************************************/

// ALLGEMEINE BEMERKUNGEN ZU FACES, SHLMAPS UND PATCHES:
// Wir definieren eine SHLMap als ein Rechteck aus s*t quadratischen Patches, die jeweils eine Face "abdecken".
// Man stelle sich die unendlich große Ebene, in der die Face liegt, als von quadratischen Patches überzogen vor,
// wie ein kariertes Mathe-Schulheft. Wichtig ist nun, daß der Ursprung des Patch-Rasters *ganzzahlig* mit dem
// World-Origin zusammenfällt. D.h., wenn man die Ebene entlang ihres Normalenvektors solange verschiebt, bis der
// World-Origin in ihr liegt, muß ein Schnittpunkt des Patch-Rasters damit zusammenfallen.
// Im Gegensatz zu einem früheren Ansatz, bei dem die Verschiebung des Patch-Rasters sich an der kleinsten s- und t-Koordinate
// der Face orientiert hat, stellen wir mit diesem Vorgehen sicher, daß sich Patches von benachbarte Faces stets *vollständig*
// überlappen (oder gar nicht). Beliebige teilweise Überlappungen kommen nicht mehr vor.
// Das Rechteck sollte bei gegebener Seitenlänge der Patches und gegebener Orientierung (entlang des UV-Koordinatensystems,
// welches man mit PlaneT::GetSpanVectors() erhält) möglichst kleine s- und t-Abmessungen haben.
// Außerdem ziehen wir noch einen 1 Patch breiten Rahmen drumherum. Damit soll dem OpenGL-Renderer Rechnung getragen werden,
// der zu jeder (s,t)-Koordinate den Mittelwert des umliegenden 2x2-Quadrats bestimmt (bilinear Filtering).
// Betrachte dazu auch die Darstellung im Cafu Tech-Archive vom 28. Oktober 2003.

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <conio.h>
#include <direct.h>
#else
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#define _stricmp strcasecmp
#define _getch getchar
#endif

#include <time.h>
#include <stdio.h>

#include "Templates/Array.hpp"
#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/ConsoleInterpreter.hpp"
#include "ConsoleCommands/ConsoleStdout.hpp"
#include "FileSys/FileManImpl.hpp"
#include "GuiSys/GuiResources.hpp"
#include "Math3D/BoundingBox.hpp"
#include "Math3D/Vector3.hpp"
#include "Bitmap/Bitmap.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/MaterialManagerImpl.hpp"
#include "Models/ModelManager.hpp"
#include "SceneGraph/Node.hpp"
#include "SceneGraph/BspTreeNode.hpp"
#include "SceneGraph/FaceNode.hpp"
#include "ClipSys/CollisionModelMan_impl.hpp"

#include "CaSHLWorld.hpp"

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


const double REFLECTIVITY=0.3;  // Gleiche Reflektivität für alle Faces und für alle Wellenlängen
const double Pi          =3.14159265358979323846;

// Defined in CaSHLWorld.hpp now!
//
// struct PatchT
// {
//     VectorT UnradiatedEnergy;       // Noch nicht in die Umgebung abgestrahlte Energie     (per unit time per unit area)
//     VectorT TotalEnergy;            // Energie, die dieser Patch in die Umgebung abstrahlt (per unit time per unit area)
//
//     VectorT Coord;                  // Position (+safety) in world space of the center of the patch. Valid only if InsideFace==true.
//     bool    InsideFace;             // InsideFace==true <==> this patch is not completely outside its face
// };

enum FaceVis { NO_VISIBILITY, PARTIAL_VISIBILITY, FULL_VISIBILITY };
ArrayT< ArrayT<FaceVis> > FacePVS;  // The set of faces each face can see (see InitializeFacePVSMatrix() for a description!)
ArrayT< ArrayT<PatchT > > Patches;  // Each face gets a set of patches, PatchT is declared in CaSHLWorld.hpp


#include "Init1.cpp"    // void InitializeFacePVSMatrix(const cf::SceneGraph::BspTreeNodeT& Map                         ) { ... }
#include "Init2.cpp"    // void InitializePatches      (const cf::SceneGraph::BspTreeNodeT& Map, const SkyDomeT& SkyDome) { ... }


// Strahlt den Transfer der Patches im n*n Quadrat ab, dessen linke obere Ecke bei (s_i, t_i) liegt.
void RadiateTransfer(const CaSHLWorldT& CaSHLWorld, unsigned long Face_i, unsigned long s_i, unsigned long t_i, char n)
{
    const cf::SceneGraph::BspTreeNodeT& Map  =CaSHLWorld.GetBspTree();
    const unsigned long       NR_OF_SH_COEFFS=cf::SceneGraph::SHLMapManT::NrOfBands * cf::SceneGraph::SHLMapManT::NrOfBands;
    const double              PATCH_SIZE     =Map.GetSHLMapPatchSize();
    const cf::SceneGraph::FaceNodeT::SHLMapInfoT& SMI=Map.FaceChildren[Face_i]->SHLMapInfo;

    unsigned long  Big_P_i_Count=0;
    VectorT        Big_P_i_Coord;
#if USE_NORMALMAPS
    VectorT        Big_P_i_Normal;
#endif
    ArrayT<double> Big_P_i_SHCoeffs_UnradiatedTransfer;

    while (Big_P_i_SHCoeffs_UnradiatedTransfer.Size()<NR_OF_SH_COEFFS) Big_P_i_SHCoeffs_UnradiatedTransfer.PushBack(0.0);

    // Bilde den Positions-Durchschnitt bzw. die UnradiatedTransfer-Summe aller Patches im n*n Quadrat,
    // wobei (s_i, t_i) die linke obere Ecke ist und nur Patches innerhalb der Face berücksichtigt werden.
    for (char y=0; y<n; y++)
        for (char x=0; x<n; x++)
        {
            if (s_i+x+1>SMI.SizeS) continue;
            if (t_i+y+1>SMI.SizeT) continue;

            PatchT& P_i=Patches[Face_i][(t_i+y)*SMI.SizeS+(s_i+x)];
            if (!P_i.InsideFace) continue;

            Big_P_i_Count++;
            Big_P_i_Coord =Big_P_i_Coord +P_i.Coord;
#if USE_NORMALMAPS
            Big_P_i_Normal=Big_P_i_Normal+P_i.Normal;
#endif

            for (unsigned long CoeffNr=0; CoeffNr<P_i.SHCoeffs_UnradiatedTransfer.Size(); CoeffNr++)
            {
                Big_P_i_SHCoeffs_UnradiatedTransfer[CoeffNr]+=P_i.SHCoeffs_UnradiatedTransfer[CoeffNr];

                // By being added to the Big_P_i above, this patch has radiated its transfer. Job done. Mission accomplished.
                P_i.SHCoeffs_UnradiatedTransfer[CoeffNr]=0.0;
            }
        }

    if (!Big_P_i_Count) return;
    Big_P_i_Coord =scale(Big_P_i_Coord, 1.0/double(Big_P_i_Count));
#if USE_NORMALMAPS
    Big_P_i_Normal=normalize(Big_P_i_Normal, 0.0);
#endif


    // Betrachte alle Patches aller Faces im PVS der Face Face_i.
    for (unsigned long Face_j=0; Face_j<Map.FaceChildren.Size(); Face_j++)
    {
        // Vermeide alle unnötigen und evtl. rundungsfehlergefährdeten Berechnungen.
        // Die folgende Zeile fängt auch alle Fälle ab, in denen Face_j in der Ebene von Face_i liegt
        // und insb. für die Face_i==Face_j gilt. Vgl. die Erstellung und Optimierung der FacePVS-Matrix!
        if (FacePVS[Face_i][Face_j]==NO_VISIBILITY) continue;
        if (Map.FaceChildren[Face_j]->Polygon.Plane.GetDistance(Big_P_i_Coord)<0.1) continue;

        bool ExplicitTestRequired=(FacePVS[Face_i][Face_j]!=FULL_VISIBILITY);

        for (unsigned long Patch_j=0; Patch_j<Patches[Face_j].Size(); Patch_j++)
        {
            PatchT& P_j=Patches[Face_j][Patch_j];
            if (!P_j.InsideFace) continue;

            // Einsparen des Wurzelziehens: Rechne einfach mit dem Quadrat weiter!
            const VectorT Ray       =P_j.Coord-Big_P_i_Coord;
         // double        RayLength =length(Ray);
            double        RayLength2=dot(Ray, Ray);

         // if (RayLength <0.5    ) continue;   // Kommt das jemals vor?
            if (RayLength2<0.5*0.5) continue;

         // VectorT Dir_ij =scale(Ray, 1.0/RayLength );
            VectorT Dir_ij2=scale(Ray, 1.0/RayLength2);   // Dir_ij2==Dir_ij/RayLength

            if (ExplicitTestRequired)
                if (CaSHLWorld.TraceRay(Big_P_i_Coord, Ray)<1.0) continue;

            if (RayLength2<PATCH_SIZE*PATCH_SIZE)
            {
                RayLength2=PATCH_SIZE*PATCH_SIZE;
                Dir_ij2   =scale(Ray, 1.0/RayLength2);
            }

#if USE_NORMALMAPS
            const double cos1__= dot(Map.FaceChildren[Face_i]->Polygon.Plane.Normal, Dir_ij2); if (cos1__<=0) continue;
            const double cos2__=-dot(Map.FaceChildren[Face_j]->Polygon.Plane.Normal, Dir_ij2); if (cos2__<=0) { printf("cos2__<=0\n"); continue; }   // Sollte niemals vorkommen (wg. PlaneDist-Check oben)!
            const double cos1_ = dot(Big_P_i_Normal, Dir_ij2); if (cos1_ <=0) continue;
            const double cos2_ =-dot(P_j.Normal    , Dir_ij2); if (cos2_ <=0) continue;
#else
         // const double cos1 = dot(Map.FaceChildren[Face_i]->Polygon.Plane.Normal, Dir_ij ); if (cos1 <=0) continue;
         // const double cos2 =-dot(Map.FaceChildren[Face_j]->Polygon.Plane.Normal, Dir_ij ); if (cos2 <=0) { printf("cos2 <=0\n"); continue; }   // Sollte niemals vorkommen (wg. PlaneDist-Check oben)!
            const double cos1_= dot(Map.FaceChildren[Face_i]->Polygon.Plane.Normal, Dir_ij2); if (cos1_<=0) continue;
            const double cos2_=-dot(Map.FaceChildren[Face_j]->Polygon.Plane.Normal, Dir_ij2); if (cos2_<=0) { printf("cos2_<=0\n"); continue; }   // Sollte niemals vorkommen (wg. PlaneDist-Check oben)!
#endif

            // 'Alternative', einfache Herleitung des Form-Faktors:
            // Betrachte die Halbkugel über dem Patch i mit Radius RayLength. RayLength soll groß genug sein,
            // d.h. Patch j soll problemlos als ein Teil der Halbkugeloberfläche betrachtet werden können.
            // Die prozentuale Sichtbarkeit erhalten wir also sofort aus A_j/O, wobei A_j der Flächeninhalt des Patches j ist
            // und O der Oberflächeninhalt der Halbkugel, O=0.5*4*pi*RayLength^2.
            // cos1 und cos2 berücksichtigen dann noch die gegenseitige Verdrehung der Patches und wir sind fertig.
            // Einziges Problem: Obige Herleitung enthält noch einen Faktor 1/2, für den ich leider keine Erklärung habe.
            // Noch eine Alternative: Man muß RayLength ausdrücken in Patch-Längen, nicht in Millimetern!
         // double FormFactor_ij=PATCH_SIZE*PATCH_SIZE/3.14159265359*cos1 *cos2 /(RayLength*RayLength);
            double FormFactor_ij=PATCH_SIZE*PATCH_SIZE/3.14159265359*cos1_*cos2_;

            // Die Flächeninhalte scheinen sich herauszukürzen!?
            // (Im FormFactor ist P_j.Area/P_i.Area enthalten, und dieser wird hier multipliziert mit P_i.Area/P_j.Area.)
            // Wir müssen nichtmal Big_P_i_Count hineinmultiplizieren, da Big_P_i_UnradiatedEnergy schon die Summe der Einzelpatches ist!
            for (unsigned long CoeffNr=0; CoeffNr<Big_P_i_SHCoeffs_UnradiatedTransfer.Size(); CoeffNr++)
            {
                const double DeltaTransfer=Big_P_i_SHCoeffs_UnradiatedTransfer[CoeffNr]*REFLECTIVITY*FormFactor_ij;

                P_j.SHCoeffs_UnradiatedTransfer[CoeffNr]+=DeltaTransfer;
                P_j.SHCoeffs_TotalTransfer     [CoeffNr]+=DeltaTransfer;
            }
        }
    }
}


// Computes the Associated Legendre Polynomial at 'x'.
double ALP(int l, int m, double x)
{
    // Start with rule #2.
    double Pmm  =1.0;
    double S1mx2=sqrt(1.0-x*x);
    double fact =1.0;

    for (int i=0; i<m; i++)
    {
        Pmm  *= -1.0*fact*S1mx2;
        fact +=  2.0;
    }
    if (l==m) return Pmm;

    double Pmmp1=x*(2.0*m+1.0)*Pmm;
    if (l==m+1) return Pmmp1;

    double Pll=0.0;

    for (int ll=m+2; ll<=l; ll++)
    {
        Pll  =(x*(2.0*ll-1.0)*Pmmp1 - (ll+m-1.0)*Pmm) / (ll-m);
        Pmm  =Pmmp1;
        Pmmp1=Pll;
    }
    return Pll;
}


double factorial(int n)
{
    static bool   IsInitialized=false;
    static double Factorials[33];

    if (!IsInitialized)
    {
        Factorials[0]=1.0;

        for (char i=1; i<33; i++)
            Factorials[i]=double(i)*Factorials[i-1];

        IsInitialized=true;
    }

    return Factorials[n<33 ? n : 32];
}


// Computes a spherical harmonic function.
// 'l'     is the band, range [0...N].
// 'm'     is in range [-l...l].
// 'theta' is the first polar angle, in range [0...Pi].
// 'phi'   is the second polar angle, in range [0...2*Pi].
// This function is described in detail in the paper by Robin Green.
double SphericalHarmonic(int l, int m, double theta, double phi)
{
    const double K=sqrt((2.0*l+1.0)/(4.0*Pi) * factorial(l-abs(m))/factorial(l+abs(m)));

         if (m>0) return sqrt(2.0)*cos( m*phi)*K*ALP(l,  m, cos(theta));
    else if (m<0) return sqrt(2.0)*sin(-m*phi)*K*ALP(l, -m, cos(theta));
    else /*m==0*/ return                       K*ALP(l,  0, cos(theta));
}


struct SphericalSampleT
{
 // VectorT        Angles;      // The position on the sphere in polar coordinates.
    VectorT        UnitVector;  // The same position as a unit vector.
    ArrayT<double> Coeffs;      // For an n-band approximation, these are the n^2 coefficients (values of the y(l, m, theta, phi) function).
};


void DirectLighting(const CaSHLWorldT& CaSHLWorld, const unsigned long SqrtNrOfSamples)
{
    const cf::SceneGraph::BspTreeNodeT& Map=CaSHLWorld.GetBspTree();

    printf("\n%-50s %s\n", "*** PHASE I - performing direct lighting ***", GetTimeSinceProgramStart());


    // This is a first test with SH lighting.
    // In order to keep things initially simple, the following implements "Shadowed Diffuse Transfer",
    // as detailed in Robin Greens paper on pages 29 and subsequent.
    ArrayT<SphericalSampleT> SphericalSamples;
    ArrayT<unsigned long>    SkyFaces;
    unsigned long            FaceNr;
    const unsigned long      NR_OF_SH_COEFFS=cf::SceneGraph::SHLMapManT::NrOfBands * cf::SceneGraph::SHLMapManT::NrOfBands;


    // Bilde zuerst ein LookUp-Array, das die Nummern aller Faces mit Sky-Texture enthält.
    for (FaceNr=0; FaceNr<Map.FaceChildren.Size(); FaceNr++)
        if (length(Vector3T<double>(Map.FaceChildren[FaceNr]->Material->meta_SunLight_Irr))>0.1 &&
            length(Vector3T<double>(Map.FaceChildren[FaceNr]->Material->meta_SunLight_Dir))>0.1) SkyFaces.PushBack(FaceNr);


    for (unsigned long SampleX=0; SampleX<SqrtNrOfSamples; SampleX++)
        for (unsigned long SampleY=0; SampleY<SqrtNrOfSamples; SampleY++)
        {
            const double x    =double(SampleX)/double(SqrtNrOfSamples) + 0.5/double(SqrtNrOfSamples);
            const double y    =double(SampleY)/double(SqrtNrOfSamples) + 0.5/double(SqrtNrOfSamples);
            const double theta=2.0*acos(sqrt(1.0-x));
            const double phi  =2.0*Pi*y;

            SphericalSamples.PushBackEmpty(1);
         // SphericalSamples[SphericalSamples.Size()-1].Angles    =VectorT(theta, phi);
            SphericalSamples[SphericalSamples.Size()-1].UnitVector=VectorT(sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta));

            for (int l=0; l<cf::SceneGraph::SHLMapManT::NrOfBands; l++)
                for (int m=-l; m<=l; m++)   // const int Index=l*(l+1)+m;
                    SphericalSamples[SphericalSamples.Size()-1].Coeffs.PushBack(SphericalHarmonic(l, m, theta, phi));

            if (SphericalSamples[SphericalSamples.Size()-1].Coeffs.Size()!=NR_OF_SH_COEFFS) Error("Bad number of coeffs in %s, line %u.", __FILE__, __LINE__);
        }


    // I assume (think/guess/hope) that the range of the SphericalHarmonic function does not exceed [-1...1].
    // It is important to know this range because we later want to efficiently store our computed results,
    // e.g. as fixed-point values with limited precision (like compressed into a char).
    // However, to be safe, a rigorous mathematically founded answer would be required, which im still lacking.
    {
        double Min=(SphericalSamples.Size()>0 && NR_OF_SH_COEFFS>0) ? SphericalSamples[0].Coeffs[0] : 0.0;
        double Max=(SphericalSamples.Size()>0 && NR_OF_SH_COEFFS>0) ? SphericalSamples[0].Coeffs[0] : 0.0;

        for (unsigned long SampleNr=0; SampleNr<SphericalSamples.Size(); SampleNr++)
            for (unsigned long CoeffNr=0; CoeffNr<NR_OF_SH_COEFFS; CoeffNr++)
            {
                const double Value=SphericalSamples[SampleNr].Coeffs[CoeffNr];

                if (Value<Min) Min=Value;
                if (Value>Max) Max=Value;
            }

        printf("SHL INFO:  Min %15.10f    Max %15.10f\n", Min, Max);
        if (Min<-1.0 || Max>1.0) Error("Assumed range of SphericalHarmonic() is out of bounds.");   // Should never happen...
    }


    for (FaceNr=0; FaceNr<Map.FaceChildren.Size(); FaceNr++)
    {
        const cf::SceneGraph::FaceNodeT::SHLMapInfoT& SMI=Map.FaceChildren[FaceNr]->SHLMapInfo;

        printf("%5.1f%%\r", (double)FaceNr/Map.FaceChildren.Size()*100.0);
        fflush(stdout);

        for (unsigned long t=0; t<SMI.SizeT; t++)
            for (unsigned long s=0; s<SMI.SizeS; s++)
            {
                PatchT& Patch=Patches[FaceNr][t*SMI.SizeS+s];

                // A patch that is entirely outside of its face is not considered.
                if (!Patch.InsideFace) continue;

                for (unsigned long SampleNr=0; SampleNr<SphericalSamples.Size(); SampleNr++)
                {
#if USE_NORMALMAPS
                    const double CosTerm =dot(SphericalSamples[SampleNr].UnitVector, Patch.Normal);
                    const double CosTerm2=dot(SphericalSamples[SampleNr].UnitVector, Map.FaceChildren[FaceNr]->Polygon.Plane.Normal);

                    if (CosTerm>0.0 && CosTerm2>0.0)
#else
                    const double CosTerm=dot(SphericalSamples[SampleNr].UnitVector, Map.FaceChildren[FaceNr]->Polygon.Plane.Normal);

                    if (CosTerm>0.0)
#endif
                    {
                        // The ray is in the upper hemisphere.
                        // Now figure out if it hits a "sky" face.
                        const VectorT Ray=SphericalSamples[SampleNr].UnitVector*9999999.9;
                        const VectorT Hit=Patch.Coord+Ray*CaSHLWorld.TraceRay(Patch.Coord, Ray);

                        // Teste, ob 'Hit' in einer Face mit Sky-Texture liegt.
                        unsigned long FNr;

                        for (FNr=0; FNr<SkyFaces.Size(); FNr++)
                        {
                            const Polygon3T<double>& SkyFace=Map.FaceChildren[SkyFaces[FNr]]->Polygon;

                            if (fabs(SkyFace.Plane.GetDistance(Hit))>0.2) continue;  // Ist 'Hit' zu weit von der SkyFace weg?

                            unsigned long VNr;
                            for (VNr=0; VNr<SkyFace.Vertices.Size(); VNr++)
                                if (SkyFace.GetEdgePlane(VNr, MapT::RoundEpsilon).GetDistance(Hit)<-0.1) break;

                            if (VNr==SkyFace.Vertices.Size()) break;
                        }

                        if (FNr<SkyFaces.Size())
                        {
                            // The ray actually hit the sky!
                            for (unsigned long CoeffNr=0; CoeffNr<NR_OF_SH_COEFFS; CoeffNr++)
                                Patch.SHCoeffs_TotalTransfer[CoeffNr]+=CosTerm*SphericalSamples[SampleNr].Coeffs[CoeffNr];
                        }
                    }
                }

                // When done, all coefficients should be in range [-4*Pi...4*Pi].
                for (unsigned long CoeffNr=0; CoeffNr<NR_OF_SH_COEFFS; CoeffNr++)
                {
                    Patch.SHCoeffs_TotalTransfer[CoeffNr]*=4.0*Pi/SphericalSamples.Size();

                    // Initially, the Unradiated Transfer is the same as the Total Transfer,
                    // exactly analogous to the direct lighting phase in traditional radiosity.
                    Patch.SHCoeffs_UnradiatedTransfer[CoeffNr]=Patch.SHCoeffs_TotalTransfer[CoeffNr];
                }
            }
    }
}


unsigned long BounceLighting(const CaSHLWorldT& CaSHLWorld, const char BLOCK_SIZE, double& StopUT, const bool AskForMore)
{
    const cf::SceneGraph::BspTreeNodeT& Map=CaSHLWorld.GetBspTree();

    printf("\n%-50s %s\n", "*** PHASE II - performing bounce lighting ***", GetTimeSinceProgramStart());

    unsigned long IterationCount =0;
    unsigned long FullSearchCount=0;

    while (true)
    {
        unsigned long Face_i=0;
        unsigned long s_i   =0;
        unsigned long t_i   =0;
        double        BestUT=0;     // Best unradiated transfer amount found.

        // Finde eine Face mit einem Patch mit einem großen "Unradiated Transfer" (nicht notwendigerweise die größte, damit es schnell geht).
        unsigned long FaceNr;

        for (FaceNr=0; FaceNr<Map.FaceChildren.Size(); FaceNr++)
        {
            // Achtung: Patches[FaceNr].Size kann auch 0 sein!
            unsigned long                                 NrOfSamples=Patches[FaceNr].Size()<10 ? Patches[FaceNr].Size()/2 : 10;
            const cf::SceneGraph::FaceNodeT::SHLMapInfoT& SMI        =Map.FaceChildren[FaceNr]->SHLMapInfo;

            for (unsigned long SampleNr=0; SampleNr<NrOfSamples; SampleNr++)
            {
                unsigned long s=rand() % SMI.SizeS;     // Funktioniert nicht mehr gut wenn SMI.SizeS>RAND_MAX
                unsigned long t=rand() % SMI.SizeT;     // Funktioniert nicht mehr gut wenn SMI.SizeT>RAND_MAX

                s=(s/BLOCK_SIZE)*BLOCK_SIZE;
                t=(t/BLOCK_SIZE)*BLOCK_SIZE;

                unsigned long Count =0;
                double        ThisUT=0.0;

                for (char y=0; y<BLOCK_SIZE; y++)
                    for (char x=0; x<BLOCK_SIZE; x++)
                    {
                        if (s+x+1>SMI.SizeS) continue;
                        if (t+y+1>SMI.SizeT) continue;

                        const PatchT& P_i=Patches[FaceNr][(t+y)*SMI.SizeS+(s+x)];
                        if (!P_i.InsideFace) continue;

                        for (unsigned long CoeffNr=0; CoeffNr<P_i.SHCoeffs_UnradiatedTransfer.Size(); CoeffNr++)
                            ThisUT+=fabs(P_i.SHCoeffs_UnradiatedTransfer[CoeffNr]);

                        Count++;
                    }

                if (!Count) continue;
                ThisUT/=double(Count);

                if (ThisUT>BestUT)
                {
                    Face_i=FaceNr;
                    s_i   =s;
                    t_i   =t;
                    BestUT=ThisUT;
                }
            }
        }

        // Sollte der BestUT unter StopUT sein, wird hier nach einem besseren Wert gesucht.
        // Beim erstbesseren Wert wird dieser genommen und abgebrochen, ansonsten gesucht bis zum Schluß.
        // Wenn alle Faces nach dem besten Wert durchsucht werden sollen, muß "&& BestUT<StopUT" auskommentiert werden.
        if (BestUT<StopUT)
        {
            FullSearchCount++;      // Zähle, wie oft wir alles abgesucht haben!

            for (FaceNr=0; FaceNr<Map.FaceChildren.Size() && BestUT<StopUT; FaceNr++)
            {
                const cf::SceneGraph::FaceNodeT::SHLMapInfoT& SMI=Map.FaceChildren[FaceNr]->SHLMapInfo;

                for (unsigned long s=0; s<SMI.SizeS; s++)
                    for (unsigned long t=0; t<SMI.SizeT; t++)
                    {
                        const  PatchT& P=Patches[FaceNr][t*SMI.SizeS+s];

                        double ThisUT=0.0;
                        for (unsigned long CoeffNr=0; CoeffNr<P.SHCoeffs_UnradiatedTransfer.Size(); CoeffNr++)
                            ThisUT+=fabs(P.SHCoeffs_UnradiatedTransfer[CoeffNr]);

                        if (ThisUT>BestUT)
                        {
                            Face_i=FaceNr;
                            s_i   =s;
                            t_i   =t;
                            BestUT=ThisUT;
                        }
                    }
            }
        }

        printf("Iteration%6lu, BestUT %6.2f, F_i%5lu, FullSearch%4lu (%5.1f%%)\r", IterationCount, BestUT, Face_i, FullSearchCount, 100.0*float(FullSearchCount)/float(IterationCount+1));
        fflush(stdout);

        if (BestUT<StopUT)  // Es gab keinen besseren Wert mehr -- wir können also abbrechen!
        {
            printf("\n");
            if (!AskForMore) break;

            time_t StartTime=time(NULL);

            printf("\nStopUT value %10.7f has been reached.\n", StopUT);
            printf("Press 'y' to confirm exit and save current result,\n");
            printf("or any other key to divide StopUT by 10 and continue lighting!\n");

            char Key=_getch(); if (Key==0) (void)_getch();

            unsigned long TotalSec=(unsigned long)difftime(time(NULL), StartTime);
            unsigned long Sec     =TotalSec % 60;
            unsigned long Min     =(TotalSec/60) % 60;
            unsigned long Hour    =TotalSec/3600;
            printf("Length of break (waiting for your decision) was %2lu:%2lu:%2lu.\n", Hour, Min, Sec);

            if (Key=='y') break;
            StopUT/=10.0;
        }

        RadiateTransfer(CaSHLWorld, Face_i, s_i, t_i, BLOCK_SIZE);
        IterationCount++;

#ifdef _WIN32
        // TODO: Ein (sinnvolles!) 'kbhit()' Äquivalent für Linux muß erst noch gefunden werden...
        if (_kbhit())
        {
            char Key=_getch(); if (Key==0) (void)_getch();

            if (Key==' ')
            {
                printf("\nINTERRUPTED BY USER! Press 'y' to confirm exit and save current result,\n");
                printf("or any other key to continue lighting!\n");

                Key=_getch(); if (Key==0) (void)_getch();
                if (Key=='y') break;
            }
        }
#endif
    }

    return IterationCount;
}


#include "Ward97.cpp"   // void ToneReproduction(const cf::SceneGraph::BspTreeNodeT& Map) { ... }


void PostProcessBorders(const CaSHLWorldT& CaSHLWorld)
{
    const cf::SceneGraph::BspTreeNodeT& Map=CaSHLWorld.GetBspTree();

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

    // Für alle Patches einer Face, die keinen SamplePoint innerhalb ihrer Face haben, ermittele ihren Wert aus dem
    // Durchschnitt ihrer acht umliegenden Patches, sofern diese mit SamplePoints innerhalb der Face liegen.
    // Diese Methode ist sehr einfach und schnell, da sie immer nur eine Face gleichzeitig betrachtet,
    // die Nachbarumgebung hat keinen Einfluß.
    // Dennoch ist diese Schleife ein guter Anfang, und war vorher sogar der *einzige* Nachbearbeitungsschritt!
    // TODO: Ein "Weighted Average" wäre vielleicht besser, zumindest die "Ecken" des 3x3-Feldes könnten u.U.
    // schwächer eingebracht werden (z.B. als ob es ein Kreis mit Radius 1,5 wäre). Nochmal überdenken!
    // Damit könnte dann wohl auch die Spezialbehandlung der Ränder entfallen!?
    for (unsigned long FaceNr=0; FaceNr<Map.FaceChildren.Size(); FaceNr++)
    {
        const cf::SceneGraph::FaceNodeT::SHLMapInfoT& SMI=Map.FaceChildren[FaceNr]->SHLMapInfo;

        for (unsigned long t=0; t<SMI.SizeT; t++)
            for (unsigned long s=0; s<SMI.SizeS; s++)
            {
                if (Patches[FaceNr][t*SMI.SizeS+s].InsideFace) continue;

                const unsigned long SMI_SizeS=SMI.SizeS;
                const unsigned long SMI_SizeT=SMI.SizeT;

                ArrayT<double>& Cs=Patches[FaceNr][t*SMI.SizeS+s].SHCoeffs_TotalTransfer;

                     if (s==0           && t>0 && t<SMI_SizeT-1) Cs=Patches[FaceNr][           t *SMI.SizeS+          1].SHCoeffs_TotalTransfer;    // linker  Rand (ohne Ecken)
                else if (s==SMI_SizeS-1 && t>0 && t<SMI_SizeT-1) Cs=Patches[FaceNr][           t *SMI.SizeS+SMI.SizeS-2].SHCoeffs_TotalTransfer;    // rechter Rand (ohne Ecken)
                else if (t==0           && s>0 && s<SMI_SizeS-1) Cs=Patches[FaceNr][           1 *SMI.SizeS+          s].SHCoeffs_TotalTransfer;    // oberer  Rand (ohne Ecken)
                else if (t==SMI_SizeT-1 && s>0 && s<SMI_SizeS-1) Cs=Patches[FaceNr][(SMI.SizeT-2)*SMI.SizeS+          s].SHCoeffs_TotalTransfer;    // unterer Rand (ohne Ecken)
                else // Alle anderen Patches (inkl. Ecken)
                {
                    const unsigned long NR_OF_SH_COEFFS=cf::SceneGraph::SHLMapManT::NrOfBands * cf::SceneGraph::SHLMapManT::NrOfBands;
                    ArrayT<double>      Average;
                    char                AverageCount=0;

                    while (Average.Size()<NR_OF_SH_COEFFS) Average.PushBack(0.0);

                    // Der Patch liegt in der Mitte eines 3x3-Feldes bei Koordinate (1,1).
                    for (char y=0; y<=2; y++)
                        for (char x=0; x<=2; x++)
                        {
                            if (x==1 && y==1) continue;                 // Nur die acht umliegenden Patches betrachten

                            if (s==0           && x==0) continue;       // Linken  Rand beachten
                            if (s==SMI_SizeS-1 && x==2) continue;       // Rechten Rand beachten
                            if (t==0           && y==0) continue;       // Oberen  Rand beachten
                            if (t==SMI_SizeT-1 && y==2) continue;       // Unteren Rand beachten

                            if (!Patches[FaceNr][(t+y-1)*SMI.SizeS+(s+x-1)].InsideFace) continue;

                            for (unsigned long CoeffNr=0; CoeffNr<NR_OF_SH_COEFFS; CoeffNr++)
                                Average[CoeffNr]+=Patches[FaceNr][(t+y-1)*SMI.SizeS+(s+x-1)].SHCoeffs_TotalTransfer[CoeffNr];
                            AverageCount++;
                        }

                    if (AverageCount)
                        for (unsigned long CoeffNr=0; CoeffNr<NR_OF_SH_COEFFS; CoeffNr++)
                            Cs[CoeffNr]=Average[CoeffNr]/double(AverageCount);
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
    //
    // Vorgehensweise:
    // Zu jedem Patch P1 jeder Face suchen wir alle Patches Pi heraus, die irgendetwas zu P1 beitragen könnten.
    // Dazu müssen sich P1 und die Pi insbesondere in der selben Ebene überlappen.
    // Wegen der global einheitlichen Ausrichtung aller Patches in einer Ebene sind Überlappungen immer "ganz oder gar nicht".
    // Jeder Patch wird klassifiziert, ob er vollständig in seiner Face liegt (INNER), auf dem Rand (PARTIAL),
    // oder vollständig außerhalb (OUTER). Somit ergeben sich folgende Möglichkeiten:
    //
    // P1 \ Pi |  INNER  | PARTIAL |  OUTER
    // --------+---------+---------+---------
    // INNER   |    11   |    12   |    13
    // PARTIAL |    21   |    22   |    23
    // OUTER   |    31   |    32   |    33
    //
    // Die Fälle P1==INNER (11, 12, 13) interessieren uns nicht, da wir für solche P1 einwandfreie Berechnungsergebnisse haben,
    // und es keinen Grund gibt, mit anderen Werten (von Pi's) daran etwas herumzufummeln.
    // Die Fälle Pi==OUTER (13, 23, 33) können wir auch direkt überspringen, weil es keinen Sinn macht, P1 irgendwie mit einem
    // anderen OUTER-Patch zu modifizieren - dieser hat schließlich selbst keinen verwendbaren Wert.
    // Der Fall 21 kann niemals vorkommen, denn ein Patch kann nicht vollständig in einer Face liegen, und teilweise in einer anderen
    // (Faces überlappen sich nicht!). Es müssen also nur die Fälle 22, 31 und 32 betrachtet werden.
    //
    // In allen drei verbleibenden Fällen gilt, daß ein Pi nur dann beitragen darf, wenn er damit kein "Light Bleeding" verursacht.
    // "Light Bleeding" ist die Beeinflussung von P1 durch andere Patches Pi, deren Faces sich zwar nahe, aber in Wirklichkeit z.B.
    // durch eine dünne "Wand" getrennt sind, oder die Patches liegen "um die Ecke".
    // Um das "Light Bleeding" Problem zu minimieren, erlauben wir die Beeinflussung von P1 durch ein Pi nur dann, wenn ein Punkt,
    // der garantiert in der Face von P1 und möglichst nahe bei P1 liegt, die Pi.Coord "sehen" kann.
    // Analytisch mag das nicht 100%ig korrekt sein (???), ergibt in der Praxis aber einwandfreie Ergebnisse!
    //
    // Fall 31 läßt sich nun leicht abhaken: Max. ein Pi ist möglich, und damit erhalten wir P1=Pi.
    // Für Fall 32 erhalten wir P1="area-weighted average of the involved Pi".
    //
    // Fall 22 ist etwas schwieriger: Im Idealfall würden wir schreiben: P1="area-weighted average of the involved Pi and P1 itself".
    // Dummerweise verändern wir damit P1, welches aber wiederum später eines der jetzigen Pi beeinflussen wird, wenn dieses Pi an der
    // Reihe ist. Dann müßte P1 aber seinen Original-Wert haben, nicht den, den wir gerade dabei sind hineinzuschreiben.
    // (Das sieht man auch daran, daß Fall 22 in obiger Matrix auf der Hauptdiagonalen liegt. Die beiden Fälle 31 und 32 (und 21) liegen
    // im unteren linken Dreieck, während das rechte obere Dreieck (12, 13, 23) "inaktiv" ist. Deshalb sind 31 und 32 unproblematisch.)
    // Lösung: Schreibe den gefundenen Wert nicht nur nach P1, sondern auch in ALLE beteiligten Pi. Das würde das Problem vollständig(!)
    // lösen, denn spätere Mittelwertbildungen wären zwar redundant, kämen aber zu dem selben (korrekten) Ergebnis.
    // Leider macht das "Light Bleeding"-Problem einen Strich durch die Rechnung! Wenn z.B. Pi={ P2, P3 }, dann könnte P1 von P2 beeinflußt
    // werden, P3 aber wegen "Light Bleeding" vom Einfluß auf P1 ausgeschlossen werden. P2 könnte später aber sehr wohl von P1 UND P3 beeinflußt
    // werden! P2 drücken wir aber nach obiger "Lösung" aber schon mal einen Mittelwert auf, und insgesamt kommt es zum falschen Ergebnis.
    // INSGESAMT wäre die einzige WIRKLICHE Lösung, ein Backup aller Patches anzulegen, und alle Mittelwerte stets daraus zu bilden.
    // Das verdoppelt aber sofort den Speicherbedarf, der ohnehin schon bei mehreren hundert MB liegt. Außerdem ist der bei
    // der ersten "Lösung" entstehende Fehler wohl sehr selten und sehr klein. Konsequenz: Wir ignorieren ihn, und wenden die erste Lösung an!
    //
    // Zuletzt ist noch zu beachten, daß das Ausschließen von Patches wg. Light Bleeding dazu führen kann, daß die Summe der Gewichte beim
    // weighted average < 1.0 wird. In diesen Fällen müssen die Gewichte "renormalisiert" werden.
    // Zu diesen Ausführungen siehe auch die Skizze im Cafu Tech-Archive vom 10.12.2003!
    const double  PATCH_SIZE          =Map.GetSHLMapPatchSize();
    unsigned long PatchesWorkedOnCount=0;

    for (unsigned long Face1Nr=0; Face1Nr<Map.FaceChildren.Size(); Face1Nr++)
    {
        const Polygon3T<double>& Face1=Map.FaceChildren[Face1Nr]->Polygon;
        const cf::SceneGraph::FaceNodeT::LightMapInfoT& Face1_SMI=Map.FaceChildren[Face1Nr]->LightMapInfo;
        ArrayT<unsigned long> NearFaces;

        printf("%5.1f%%\r", (double)Face1Nr/Map.FaceChildren.Size()*100.0);
        fflush(stdout);

        // Bilde zuerst eine Liste (von Indizes) von Faces, die Face1 "nahe" sind.
        for (unsigned long Face2Nr=0; Face2Nr<Map.FaceChildren.Size(); Face2Nr++)
        {
            const Polygon3T<double>& Face2=Map.FaceChildren[Face2Nr]->Polygon;

            // Wir wollen nicht gegen uns selbst testen!
            if (Face1Nr==Face2Nr) continue;

            // Nur Faces in der gleichen Ebene mit gleicher Orientierung durchgehen lassen!
            if (Face1.WhatSide(Face2.Plane, MapT::RoundEpsilon)!=Polygon3T<double>::InIdentical) continue;

            // Faces, die zu weit voneinander entfernt liegen, brauchen nicht weiter betrachtet zu werden!
            const VectorT BorderPadding(PATCH_SIZE, PATCH_SIZE, PATCH_SIZE);

            BoundingBox3T<double> BB1(Face1.Vertices); BB1.Min-=BorderPadding; BB1.Max+=BorderPadding;
            BoundingBox3T<double> BB2(Face2.Vertices); BB2.Min-=BorderPadding; BB2.Max+=BorderPadding;

            if (!BB1.Intersects(BB2)) continue;

            // Ein Sanity-Check, wg. Rundungsfehlern.
            // Eigentlich müssen beide Planes nämlich exakt gleich sein!
            if (Face1.Plane.Normal.x!=Face2.Plane.Normal.x) printf("WARNING: Face1.Plane.Normal.x!=Face2.Plane.Normal.x (%.15f != %.15f)\n", Face1.Plane.Normal.x, Face2.Plane.Normal.x);
            if (Face1.Plane.Normal.y!=Face2.Plane.Normal.y) printf("WARNING: Face1.Plane.Normal.y!=Face2.Plane.Normal.y (%.15f != %.15f)\n", Face1.Plane.Normal.y, Face2.Plane.Normal.y);
            if (Face1.Plane.Normal.z!=Face2.Plane.Normal.z) printf("WARNING: Face1.Plane.Normal.z!=Face2.Plane.Normal.z (%.15f != %.15f)\n", Face1.Plane.Normal.z, Face2.Plane.Normal.z);
            if (Face1.Plane.Dist    !=Face2.Plane.Dist    ) printf("WARNING: Face1.Plane.Dist    !=Face2.Plane.Dist     (%.15f != %.15f)\n", Face1.Plane.Dist    , Face2.Plane.Dist    );

            // Diese Face kommt in Betracht, speichere ihre Nummer.
            NearFaces.PushBack(Face2Nr);
        }

        // Bereite nun das PatchPoly vor. Unten werden dann nur noch die 4 Vertices ausgefüllt.
        // WICHTIG: Dieses PatchPoly ist für zwei sich überlappende Patches EXAKT IDENTISCH, da alle Patches gleich ausgerichtet sind!
        // Der folgende Code ist SEHR ähnlich zu dem Code in InitializePatches() (Init2.cpp)!

        // Bestimme die Spannvektoren der gemeinsamen Ebene.
        // (Face1 und die Faces2 unten liegen in einer *gemeinsamen* Ebene!)
        VectorT U;
        VectorT V;

        Face1.Plane.GetSpanVectors(U, V);

        // Finde SmallestU und SmallestV.
        double Face1_SmallestU=dot(Face1.Vertices[0], U);
        double Face1_SmallestV=dot(Face1.Vertices[0], V);

        for (unsigned long VertexNr=1; VertexNr<Face1.Vertices.Size(); VertexNr++)
        {
            double u=dot(Face1.Vertices[VertexNr], U);
            double v=dot(Face1.Vertices[VertexNr], V);

            if (u<Face1_SmallestU) Face1_SmallestU=u;
            if (v<Face1_SmallestV) Face1_SmallestV=v;
        }

        Face1_SmallestU=floor(Face1_SmallestU/PATCH_SIZE);
        Face1_SmallestV=floor(Face1_SmallestV/PATCH_SIZE);

        const VectorT UV_Origin=scale(Face1.Plane.Normal, Face1.Plane.Dist);
        const VectorT Safety   =scale(Face1.Plane.Normal, 0.1);

        Polygon3T<double> PatchPoly;

        PatchPoly.Plane=dot(Face1.Plane.Normal, cross(U, V))<0 ? Face1.Plane : Face1.Plane.GetMirror();
        PatchPoly.Vertices.PushBackEmpty(4);

        // Betrachte nun alle Patches von Face1.
        for (unsigned long t1=0; t1<Face1_SMI.SizeT; t1++)
            for (unsigned long s1=0; s1<Face1_SMI.SizeS; s1++)
            {
                PatchT& Patch1=Patches[Face1Nr][t1*Face1_SMI.SizeS+s1];

                // Rekonstruiere das Polygon zu Patch1.
                PatchPoly.Vertices[0]=UV_Origin+scale(U, (Face1_SmallestU+s1-1.0)*PATCH_SIZE)+scale(V, (Face1_SmallestV+t1-1.0)*PATCH_SIZE);
                PatchPoly.Vertices[1]=UV_Origin+scale(U, (Face1_SmallestU+s1    )*PATCH_SIZE)+scale(V, (Face1_SmallestV+t1-1.0)*PATCH_SIZE);
                PatchPoly.Vertices[2]=UV_Origin+scale(U, (Face1_SmallestU+s1    )*PATCH_SIZE)+scale(V, (Face1_SmallestV+t1    )*PATCH_SIZE);
                PatchPoly.Vertices[3]=UV_Origin+scale(U, (Face1_SmallestU+s1-1.0)*PATCH_SIZE)+scale(V, (Face1_SmallestV+t1    )*PATCH_SIZE);

                // Klassifizierung: Falls Patch1 *vollständig in* seiner Face liegt, haben wir dafür einen
                // einwandfreien Berechnungswert, und wir wollen daran nicht rumfummeln (Fälle 11, 12, 13)!
                // Ob Patch1 PARTIAL oder OUTER ist, wird weiter unten genauer klassifiziert.
                if (Face1.Encloses(PatchPoly, true, MapT::RoundEpsilon)) continue;


                // Den Mittelpunkt des PatchPoly bestimmen, inkl. "Safety".
                const VectorT PatchPoly_Center=scale(PatchPoly.Vertices[0]+PatchPoly.Vertices[1]+PatchPoly.Vertices[2]+PatchPoly.Vertices[3], 0.25)+Safety;

                // Suche einen Punkt *IN* Face1 heraus, der "nahe" bei Patch1 liegt. Wird unten benötigt.
                double  MinDistance=3.0*PATCH_SIZE;
                VectorT InnerPointCloseToPatch1;

                for (unsigned long PatchNr=0; PatchNr<Patches[Face1Nr].Size(); PatchNr++)
                {
                    const PatchT& TempPatch=Patches[Face1Nr][PatchNr];

                    // 'TempPatch' darf auch ruhig 'Patch1' sein, da 'Patch1.Coord' durchaus ein Punkt in Face1 ist, der "nahe" bei Patch1 liegt!
                    if (!TempPatch.InsideFace) continue;

                    double Distance=length(TempPatch.Coord-PatchPoly_Center);

                    if (Distance<MinDistance)
                    {
                        MinDistance            =Distance;
                        InnerPointCloseToPatch1=TempPatch.Coord;
                    }
                }

                // Wurde auch etwas in der Nähe gefunden?
                if (MinDistance==3.0*PATCH_SIZE) continue;


                // Betrachte nun die umliegenden Faces, um herauszufinden, ob sie Patches enthalten,
                // die zu "unserem" Patch etwas beitragen könnten.
                ArrayT<PatchT*> ContributingPatches;
                ArrayT<double > ContributingPatchesInFace;  // Mit wieviel % seiner Fläche liegt der Patch in seiner Face?

                for (unsigned long NearNr=0; NearNr<NearFaces.Size(); NearNr++)
                {
                    const Polygon3T<double>& Face2=Map.FaceChildren[NearFaces[NearNr]]->Polygon;
                    const cf::SceneGraph::FaceNodeT::LightMapInfoT& Face2_SMI=Map.FaceChildren[NearFaces[NearNr]]->LightMapInfo;

                    double Face2_SmallestU=dot(Face2.Vertices[0], U);
                    double Face2_SmallestV=dot(Face2.Vertices[0], V);

                    for (unsigned long VertexNr=1; VertexNr<Face2.Vertices.Size(); VertexNr++)
                    {
                        double u=dot(Face2.Vertices[VertexNr], U);
                        double v=dot(Face2.Vertices[VertexNr], V);

                        if (u<Face2_SmallestU) Face2_SmallestU=u;
                        if (v<Face2_SmallestV) Face2_SmallestV=v;
                    }

                    Face2_SmallestU=floor(Face2_SmallestU/PATCH_SIZE);
                    Face2_SmallestV=floor(Face2_SmallestV/PATCH_SIZE);

                    // Gehe die Patches von Face2 durch
                    for (unsigned long t2=0; t2<Face2_SMI.SizeT; t2++)
                        for (unsigned long s2=0; s2<Face2_SMI.SizeS; s2++)
                            // Überlappen sich die Patches? Beachte: Wenn, dann ist es eine *exakte* Überlappung, wegen gleichem Patch-Alignment!
                            if ((unsigned long)(Face1_SmallestU)+s1==(unsigned long)(Face2_SmallestU)+s2 &&
                                (unsigned long)(Face1_SmallestV)+t1==(unsigned long)(Face2_SmallestV)+t2)
                            {
                                PatchT& Patch2=Patches[NearFaces[NearNr]][t2*Face2_SMI.SizeS+s2];

                                // Nur "äußere" Patches von Face1 mit "inneren" Patches von Face1 korrigieren!
                                if (!Patch2.InsideFace) continue;

                                // Avoid "Light Bleeding" by this simple visibility test.
                                if (CaSHLWorld.TraceRay(InnerPointCloseToPatch1, Patch2.Coord-InnerPointCloseToPatch1)<1.0) continue;


                                ArrayT< Polygon3T<double> > NewPolygons;

                                PatchPoly.GetChoppedUpAlong(Face2, MapT::RoundEpsilon, NewPolygons);
                                if (NewPolygons.Size()==0) Error("PolygonChopUp failed in PostProcessBorders().");

                                // Mit wieviel % seiner Fläche liegt PatchPoly in Face2?
                                const double AreaRatio=NewPolygons[NewPolygons.Size()-1].GetArea()/PatchPoly.GetArea();

                                ContributingPatches      .PushBack(&Patch2);
                                ContributingPatchesInFace.PushBack(AreaRatio);


                                // Directly continue with the next face.
                                t2=Face2_SMI.SizeT;
                                break;
                            }
                }


                if (ContributingPatches.Size()==0) continue;

                const unsigned long NR_OF_SH_COEFFS=cf::SceneGraph::SHLMapManT::NrOfBands * cf::SceneGraph::SHLMapManT::NrOfBands;


                // Klassifiziere Patch1 weiter:
                // Daß er INNER ist, haben wir oben schon ausgeschlossen.
                // Er ist PARTIAL, wenn Patch1.InsideFace==true, sonst OUTER.
                if (Patch1.InsideFace)
                {
                    // Patch1 is "PARTIAL" (partially inside of Face1) - dies ist also Fall 22.
                    // Note that in case 22, Patch1 is contributing to itself, thus add it to the ContributingPatches and ContributingPatchesInFace arrays.
                    ArrayT< Polygon3T<double> > NewPolygons;

                    PatchPoly.GetChoppedUpAlong(Face1, MapT::RoundEpsilon, NewPolygons);
                    if (NewPolygons.Size()==0) Error("PolygonChopUp failed in PostProcessBorders().");

                    // Mit wieviel % seiner Fläche liegt PatchPoly in Face1?
                    const double AreaRatio=NewPolygons[NewPolygons.Size()-1].GetArea()/PatchPoly.GetArea();

                    ContributingPatches      .PushBack(&Patch1);
                    ContributingPatchesInFace.PushBack(AreaRatio);
                }


                // Re-normalize the contribution percentages of the contributing patches.
                // This is necessary because light-bleeding might omit patches that otherwise had contributed.
                unsigned long CPNr;
                double        Sum=0.0;

                for (CPNr=0; CPNr<ContributingPatchesInFace.Size(); CPNr++) Sum+=ContributingPatchesInFace[CPNr];
                for (CPNr=0; CPNr<ContributingPatchesInFace.Size(); CPNr++) ContributingPatchesInFace[CPNr]/=Sum;


                // Compute the weighted average.
                ArrayT<double> ResultCoeffs;

                unsigned long CoeffNr;
                for (CoeffNr=0; CoeffNr<NR_OF_SH_COEFFS; CoeffNr++) ResultCoeffs.PushBack(0.0);

                for (CPNr=0; CPNr<ContributingPatches.Size(); CPNr++)
                    for (CoeffNr=0; CoeffNr<NR_OF_SH_COEFFS; CoeffNr++)
                        ResultCoeffs[CoeffNr]+=ContributingPatches[CPNr]->SHCoeffs_TotalTransfer[CoeffNr]*ContributingPatchesInFace[CPNr];

                if (Patch1.InsideFace)
                {
                    for (CPNr=0; CPNr<ContributingPatches.Size(); CPNr++)
                        ContributingPatches[CPNr]->SHCoeffs_TotalTransfer=ResultCoeffs;     // Case 22.
                }
                else Patch1.SHCoeffs_TotalTransfer=ResultCoeffs;    // Case 31 or 32.

                PatchesWorkedOnCount++;
            }
    }

    printf("Borders completed. %lu patches modified in 2nd part.\n", PatchesWorkedOnCount);
}


void Usage()
{
    printf("\n");
    printf("USAGE: CaSHL WorldName [OPTIONS]\n");
    printf("\n");
    printf("-Bands n       Number of SH bands (yielding n*n SH coeffs).\n");
    printf("               n must be in range 0..10, default is %u.\n", cf::SceneGraph::SHLMapManT::NrOfBands);
    printf("-NrOfSamples n Approx. number of SH samples (I'll determine the exact number).\n");
    printf("               n must be in range 20..100000, default is 10000.\n");
    printf("-SkipBL        Skip bounce lighting. Without BL, what I compute is analogous to\n");
    printf("               Robin Greens \"2 Shadowed Diffuse Transfer\". With BL, it is\n");
    printf("               analogous to \"3 Diffuse Interreflected Transfer\" (p. 31).\n");
    printf("-BlockSize n   Radiative block size for faster bounce lighting.\n");
    printf("               n must be in range 1..8, default is 3.\n");
    printf("-StopUT f      Stop value for unradiated transfer. 0 < f <= 10, default is 0.1.\n");
    printf("-AskForMore    Asks for a new StopUT value when the old one has been reached.\n");
    printf("-NoFullVis     Do not precompute 'full vis' acceleration information.\n");
    printf("               Makes initialization much faster, but lighting a bit slower.\n");
    printf("-fast          Same as \"-BlockSize 5 -NoFullVis\".\n");
    printf("-Reps n        Number of representative SH vectors used for compression.\n");
    printf("               Default is %u. 0 means no compression.\n", cf::SceneGraph::SHLMapManT::NrOfRepres);
    printf("\n");
    printf("\n");
    printf("EXAMPLES:\n");
    printf("\n");
    printf("CaSHL WorldName -AskForMore\n");
    printf("    I'll start with the default parameters, light the world WorldName and\n");
    printf("    finally ask you what to do when the StopUT value has been reached.\n");
    printf("\n");
    printf("CaSHL WorldName -StopUT 0.1\n");
    printf("    Most worlds of the Cafu demo release are lit with these switches.\n");
    printf("\n");
    printf("CaSHL WorldName -BlockSize 1 -StopUT 0.1\n");
    printf("    This is ideal for batch file processing: WorldName is lit without further\n");
    printf("    user questioning and I'll terminate as soon as StopUT has been reached.\n");
    printf("    Note that BlockSize and StopUT are set for high-quality lighting here.\n");
    printf("\n");
    printf("CaSHL WorldName -fast\n");
    printf("CaSHL WorldName -BlockSize 5 -NoFullVis\n");
    printf("    Fast and ugly lighting, intended for quick tests during world development.\n");
    exit(1);
}


static void WriteLogFileEntry(const char* WorldPathName, double StopUT, char BlockSize, unsigned long IterationCount)
{
    char          DateTime [256]="unknown";
    char          HostName [256]="unknown";
    char          WorldName[256]="unknown";
    time_t        Time          =time(NULL);
    unsigned long RunningSec    =(unsigned long)difftime(Time, ProgramStartTime);
    FILE*         LogFile       =fopen("CaSHL.log", "a");

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
    fprintf(LogFile, "%-16s %-16s%3lu:%02lu:%02lu [%-16s]%8.5f %ux%u%8lu\n", DateTime, WorldName, RunningSec/3600, (RunningSec/60) % 60, RunningSec % 60, HostName, StopUT, BlockSize, BlockSize, IterationCount);
    fclose(LogFile);
}


int main(int ArgC, const char* ArgV[])
{
    // cf::GameSys::GetComponentTIM().Init();      // The one-time init of the GameSys components type info manager.
    // cf::GameSys::GetGameSysEntityTIM().Init();  // The one-time init of the GameSys entity type info manager.
    // cf::GameSys::GetWorldTIM().Init();          // The one-time init of the GameSys world type info manager.

    struct CaSHLOptionsT
    {
        char          BlockSize;
        double        StopUT;
        bool          AskForMore;
        bool          UseFullVis;
        unsigned long SqrtNrOfSamples;
        bool          SkipBL;

        CaSHLOptionsT() : BlockSize(3), StopUT(0.1), AskForMore(false), UseFullVis(true), SqrtNrOfSamples(100), SkipBL(false) {}
    } CaSHLOptions;

    cf::SceneGraph::SHLMapManT::NrOfBands=4;


    // Init screen
    printf("\n*** Cafu SHL Utility, Version 01 (%s) ***\n\n\n", __DATE__);
#if USE_NORMALMAPS
#if !defined(_MSC_VER)
    printf("This version of CaSHL takes the normal-maps of surfaces into account!\n");

    // As we need access to the normal-maps, do a quick check if "./Textures" is a proper directory.
    DIR* TempDir=opendir("./Textures");

    if (TempDir==NULL)
    {
        printf("I'm sorry, but for taking normal-maps into account, I need to be able to find\n");
        printf("them. I thus looked for the \"./Textures\" directory, but wasn't able to find it.\n");
        printf("Please cd into the \"Games/DeathMatch\" directory and run me again from there!\n");

        Error("Texture directory not found.\n");
    }
    closedir(TempDir);
#endif
#endif

    // Initialize the FileMan by mounting the default file system.
    // Note that specifying "./" (instead of "") as the file system description effectively prevents the use of
    // absolute paths like "D:\abc\someDir\someFile.xy" or "/usr/bin/xy". This however should be fine for this application.
    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_LOCAL_PATH, "./", "");
    // cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/DeathMatch/Textures/TechDemo.zip", "Games/DeathMatch/Textures/TechDemo/", "Ca3DE");
    // cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/DeathMatch/Textures/SkyDomes.zip", "Games/DeathMatch/Textures/SkyDomes/", "Ca3DE");

    if (ArgC<2) Usage();

    // Process command line
    for (int CurrentArg=2; CurrentArg<ArgC; CurrentArg++)
    {
        if (!_stricmp(ArgV[CurrentArg], "-Bands"))
        {
            if (CurrentArg+1==ArgC) Error("I can't find a number after \"-Bands\"!");
            CurrentArg++;

            cf::SceneGraph::SHLMapManT::NrOfBands=atoi(ArgV[CurrentArg]);
            if (cf::SceneGraph::SHLMapManT::NrOfBands>10) Error("Bands must be in range 0..10.");
        }
        else if (!_stricmp(ArgV[CurrentArg], "-NrOfSamples"))
        {
            if (CurrentArg+1==ArgC) Error("I can't find a number after \"-NrOfSamples\"!");
            CurrentArg++;

            const int NrOfSamples=atoi(ArgV[CurrentArg]);
            if (NrOfSamples<20 || NrOfSamples>100000) Error("NrOfSamples must be in range 20..100000.");

            CaSHLOptions.SqrtNrOfSamples=(unsigned long)(sqrt(double(NrOfSamples))+0.5);
        }
        else if (!_stricmp(ArgV[CurrentArg], "-SkipBL"))
        {
            CaSHLOptions.SkipBL=true;
        }
        else if (!_stricmp(ArgV[CurrentArg], "-BlockSize"))
        {
            if (CurrentArg+1==ArgC) Error("I can't find a number after \"-BlockSize\"!");
            CurrentArg++;
            CaSHLOptions.BlockSize=atoi(ArgV[CurrentArg]);
            if (CaSHLOptions.BlockSize<1 || CaSHLOptions.BlockSize>8) Error("BlockSize must be in range 1..8.");
        }
        else if (!_stricmp(ArgV[CurrentArg], "-StopUT"))
        {
            if (CurrentArg+1==ArgC) Error("I can't find a number after \"-StopUT\"!");
            CurrentArg++;
            CaSHLOptions.StopUT=atof(ArgV[CurrentArg]);
            if (CaSHLOptions.StopUT<=0.0 || CaSHLOptions.StopUT>10.0) Error("StopUT must be in ]0, 10].");
        }
        else if (!_stricmp(ArgV[CurrentArg], "-AskForMore"))
        {
            CaSHLOptions.AskForMore=true;
        }
        else if (!_stricmp(ArgV[CurrentArg], "-NoFullVis"))
        {
            CaSHLOptions.UseFullVis=false;
        }
        else if (!_stricmp(ArgV[CurrentArg], "-fast"))
        {
            CaSHLOptions.BlockSize=5;
            CaSHLOptions.UseFullVis=false;
        }
        else if (!_stricmp(ArgV[CurrentArg], "-Reps"))
        {
            if (CurrentArg+1==ArgC) Error("I can't find a number after \"-Reps\"!");
            CurrentArg++;

            cf::SceneGraph::SHLMapManT::NrOfRepres=atoi(ArgV[CurrentArg]);
            if (cf::SceneGraph::SHLMapManT::NrOfRepres>65536) Error("Reps must be in range 0..65536.");
        }
        else if (ArgV[CurrentArg][0]==0)
        {
            // The argument is "", the empty string.
            // This can happen under Linux, when CaSHL is called via wxExecute() with white-space trailing the command string.
        }
        else
        {
            printf("\nSorry, I don't know what \"%s\" means.\n", ArgV[CurrentArg]);
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
        printf("Loading World '%s'.\n", ArgV[1]);

        const char          Save_NrOfBands=cf::SceneGraph::SHLMapManT::NrOfBands;
        const unsigned long Save_NrOfReps =cf::SceneGraph::SHLMapManT::NrOfRepres;

        ModelManagerT             ModelMan;
        cf::GuiSys::GuiResourcesT GuiRes(ModelMan);
        CaSHLWorldT               CaSHLWorld(ArgV[1], ModelMan, GuiRes);

        cf::SceneGraph::SHLMapManT::NrOfBands =Save_NrOfBands;
        cf::SceneGraph::SHLMapManT::NrOfRepres=Save_NrOfReps;

        // Print out options summary
        printf("\n");
        printf("- cf::SceneGraph::SHLMapManT::NrOfBands is %u (yielding %u^2==%lu SH coeffs).\n", cf::SceneGraph::SHLMapManT::NrOfBands, cf::SceneGraph::SHLMapManT::NrOfBands, (unsigned long)(cf::SceneGraph::SHLMapManT::NrOfBands)*(unsigned long)(cf::SceneGraph::SHLMapManT::NrOfBands));
        printf("- Number of SH samples is %lu.\n", CaSHLOptions.SqrtNrOfSamples*CaSHLOptions.SqrtNrOfSamples);
        printf("- I will %s the bounce lighting phase.\n", CaSHLOptions.SkipBL ? "SKIP" : "NOT skip");
        printf("- BlockSize is %ux%u.\n", CaSHLOptions.BlockSize, CaSHLOptions.BlockSize);
        printf("- StopUT    is %.3f.\n", CaSHLOptions.StopUT);
        printf("- I will %s you for more.\n", CaSHLOptions.AskForMore ? "ASK" : "NOT ask");
        printf("- I will %s the 'full vis' acceleration.\n", CaSHLOptions.UseFullVis ? "USE" : "NOT use");
        printf("- cf::SceneGraph::SHLMapManT::NrOfRepres is %u (compression is %s).\n", cf::SceneGraph::SHLMapManT::NrOfRepres, cf::SceneGraph::SHLMapManT::NrOfRepres>0 ? "ON" : "OFF");
        if (cf::SceneGraph::SHLMapManT::NrOfRepres>0)
        {
            const unsigned long NR_OF_SH_COEFFS    =cf::SceneGraph::SHLMapManT::NrOfBands * cf::SceneGraph::SHLMapManT::NrOfBands;
            const unsigned long NrOfColumns        =(cf::SceneGraph::SHLMapManT::NrOfRepres+255)/256;   // =ceil(double(cf::SceneGraph::SHLMapManT::NrOfRepres)/256);
            const unsigned long NrOfPixelsPerVector=(NR_OF_SH_COEFFS+3)/4;

            unsigned long Width=1; while (Width<NrOfColumns*NrOfPixelsPerVector) Width*=2;

            printf("  The look-up texture of representatives in the engine will have size %lu x 256 (%.1f%% unused).\n", Width, 100.0-100.0*double(cf::SceneGraph::SHLMapManT::NrOfRepres*NrOfPixelsPerVector)/double(256*Width));
        }

        // Initialize
        InitializeFacePVSMatrix(CaSHLWorld, CaSHLOptions.UseFullVis);   // Init1.cpp
        InitializePatches(CaSHLWorld.GetBspTree());                     // Init2.cpp

        // Perform lighting.
        DirectLighting(CaSHLWorld, CaSHLOptions.SqrtNrOfSamples);
        unsigned long IterationCount=CaSHLOptions.SkipBL ? 0 : BounceLighting(CaSHLWorld, CaSHLOptions.BlockSize, CaSHLOptions.StopUT, CaSHLOptions.AskForMore);

        if (!CaSHLOptions.SkipBL) ToneReproduction(CaSHLWorld.GetBspTree());    // Ward97.cpp
        PostProcessBorders(CaSHLWorld);

        printf("\n%-50s %s\n", "*** Write Patch coeffs back into SHLMaps ***", GetTimeSinceProgramStart());
        CaSHLWorld.PatchesToSHLMaps(Patches);

        printf("\n%-50s %s\n", "*** Saving World ***", GetTimeSinceProgramStart());
        printf("%s\n", ArgV[1]);
        CaSHLWorld.SaveToDisk(ArgV[1]);

        WriteLogFileEntry(ArgV[1], CaSHLOptions.StopUT, CaSHLOptions.BlockSize, IterationCount);
        printf("\n%-50s %s\n", "COMPLETED.", GetTimeSinceProgramStart());
    }
    catch (const WorldT::LoadErrorT& E)
    {
        printf("\nType \"CaSHL\" (without any parameters) for help.\n");
        Error(E.Msg);
    }
    catch (const WorldT::SaveErrorT& E)
    {
        printf("\nType \"CaSHL\" (without any parameters) for help.\n");
        Error(E.Msg);
    }

    return 0;
}
