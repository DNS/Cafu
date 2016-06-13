/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "CollisionModel_static.hpp"
#include "CollisionModel_static_BulletAdapter.hpp"
#include "TraceResult.hpp"
#include "TraceSolid.hpp"

#include "ConsoleCommands/Console.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "Math3D/BezierPatch.hpp"
#include "Math3D/Brush.hpp"
#include "Math3D/Pluecker.hpp"
#include "Math3D/Polygon.hpp"
#include "SceneGraph/_aux.hpp"
#include "Terrain/Terrain.hpp"
#include "MapFile.hpp"

#include <algorithm>

#if defined(_WIN32) && defined(_MSC_VER)
    // Turn off warning C4355: 'this' : used in base member initializer list.
    #pragma warning(disable:4355)
#endif


using namespace cf::ClipSys;


unsigned long CollisionModelStaticT::s_CheckCount = 0;


/// This representation of a TraceSolidT is used in the implementation of CollisionModelStaticT
/// as a performance optimization, allowing it to shortcut the frequent and expensive access to
/// the TraceSolidT's virtual methods.
class CollisionModelStaticT::InternalTraceSolidT
{
    public:

    InternalTraceSolidT(const TraceSolidT& TraceSolid)
        : NumVerts(TraceSolid.GetNumVertices()),
          Verts(TraceSolid.GetVertices())
    {
        // Call the virtual functions only if absolutely necessary.
        // In CaLight, this yields a small but noticeable speedup.
        if (NumVerts > 1)
        {
            NumPlanes = TraceSolid.GetNumPlanes();
            Planes    = TraceSolid.GetPlanes();
            NumEdges  = TraceSolid.GetNumEdges();
            Edges     = TraceSolid.GetEdges();
        }
        else
        {
            NumPlanes = 0;
            Planes    = NULL;
            NumEdges  = 0;
            Edges     = NULL;
        }
    }


    unsigned int              NumVerts;
    const Vector3dT*          Verts;
    unsigned int              NumPlanes;
    const Plane3dT*           Planes;
    unsigned int              NumEdges;
    const TraceSolidT::EdgeT* Edges;
};

typedef CollisionModelStaticT::InternalTraceSolidT IntTSolidT;


class CollisionModelStaticT::TraceParamsT
{
    public:

    TraceParamsT(const bool GenericBrushes_, const TraceSolidT& TraceSolid_, const Vector3dT& Start_, const Vector3dT& Ray_, unsigned long ClipMask_, TraceResultT& Result_)
        : GenericBrushes(GenericBrushes_),
          TraceSolid(TraceSolid_),
          TraceBB(TraceSolid_.GetBB()),
          Start(Start_),
          Ray(Ray_),
          ClipMask(ClipMask_),
          Result(Result_)
    {
    }


    const bool           GenericBrushes;
    const IntTSolidT     TraceSolid;
    const BoundingBox3dT TraceBB;
    const Vector3dT&     Start;
    const Vector3dT&     Ray;
    const unsigned long  ClipMask;
    TraceResultT&        Result;
};


CollisionModelStaticT::PolygonT::PolygonT()
    : Parent(NULL),
      Material(NULL),
      CheckCount(0)
{
    Vertices[0] = 0;
    Vertices[1] = 0;
    Vertices[2] = 0;
    Vertices[3] = 0;
}


CollisionModelStaticT::PolygonT::PolygonT(CollisionModelStaticT* Parent_, MaterialT* Material_,
        unsigned long A, unsigned long B, unsigned long C, unsigned long D)
    : Parent(Parent_),
      Material(Material_),
      CheckCount(0)
{
    Vertices[0] = A;
    Vertices[1] = B;
    Vertices[2] = C;
    Vertices[3] = D;
}


BoundingBox3dT CollisionModelStaticT::PolygonT::GetBB() const
{
    BoundingBox3dT BB(Parent->m_Vertices[Vertices[0]]);

    BB.Insert(Parent->m_Vertices[Vertices[1]]);
    BB.Insert(Parent->m_Vertices[Vertices[2]]);

    if (Vertices[3] != 0xFFFFFFFF)
        BB.Insert(Parent->m_Vertices[Vertices[3]]);

    return BB;
}


void CollisionModelStaticT::PolygonT::TraceConvexSolid(
    const InternalTraceSolidT& TraceSolid, const Vector3dT& Start, const Vector3dT& Ray, unsigned long ClipMask, TraceResultT& Result) const
{
    // If the ClipFlags of this polygon don't match the ClipMask, it doesn't interfere with the trace.
    if (!Material) return;
    if ((Material->ClipFlags & ClipMask) == 0) return;

    // If the trace solid is empty, it cannot collide with the polygon.
    if (TraceSolid.NumVerts == 0) return;

    // Use the optimized point trace whenever possible.
    if (TraceSolid.NumVerts == 1)
    {
        // TraceSolid.Verts[0] is normally supposed to be (0, 0, 0), but let's support the generic case.
        TraceRay(Start + TraceSolid.Verts[0], Ray, ClipMask, Result);
        return;
    }

    // If the overall bounding box of the movement of TraceSolid from Start to Start+Ray*Result.Fraction
    // doesn't intersect with our BB, this brush doesn't interfere with the trace.
    // However, it's probably more expensive to compute the OverallBB here than running the full tests below.
    // Instead, the caller should do this test, as it is in a much better position
    // (it has to compute the OverallBB only once for possible reuse with many other traces).
 // if (!OverallBB.Intersects(BB)) return;


    try
    {
        const Plane3dT Plane(Parent->m_Vertices[Vertices[0]], Parent->m_Vertices[Vertices[1]], Parent->m_Vertices[Vertices[2]], 0.1);

        // Polygons with "single-sided" materials only block movement from front to back.
        if (!Material->TwoSided && dot(Ray, Plane.Normal) >= 0) return;


        // Assemble a brush from the polygon, then trace the solid against that brush.
        // Brushes are - by nature - solid and "unsided"... PolyBrushSides[1] can NOT be omitted,
        // even if the polygon is one-sided, or otherwise we had some polygon with infinite extent.
        static BrushT        PolyBrush;
        static BrushT::SideT PolyBrushSides[2];
        static unsigned long ReverseVIs[4];
        static BrushT::EdgeT PolyBrushEdges[4];
        const unsigned long  NUM_VERTS = IsTriangle() ? 3 : 4;

        // Setup the PolyBrush.
        PolyBrush.Parent        = Parent;
        PolyBrush.Sides         = PolyBrushSides;
        PolyBrush.NrOfSides     = 2;
     // PolyBrush.BB            = GetBB();    // Not needed here.
        PolyBrush.Contents      = Material->ClipFlags;
     // PolyBrush.CheckCount    = 0;          // Not needed here.

        PolyBrush.HullVerts     = const_cast<unsigned long*>(Vertices);   // Note that both BrushT as well as PolygonT vertex indices are assumed to point into Parent->m_Vertices!
        PolyBrush.NrOfHullVerts = NUM_VERTS;
        PolyBrush.HullEdges     = PolyBrushEdges;
        PolyBrush.NrOfHullEdges = NUM_VERTS;

        // Setup the PolyBrushSides.
        PolyBrushSides[0].Plane        = Plane;
        PolyBrushSides[0].Vertices     = const_cast<unsigned long*>(Vertices);    // Note that both BrushT::SideT as well as PolygonT vertex indices point into Parent->m_Vertices!
        PolyBrushSides[0].NrOfVertices = NUM_VERTS;
        PolyBrushSides[0].Material     = Material;

        PolyBrushSides[1].Plane        = Plane.GetMirror();
        PolyBrushSides[1].Vertices     = ReverseVIs;
        PolyBrushSides[1].NrOfVertices = NUM_VERTS;
        PolyBrushSides[1].Material     = Material;

        // Setup the ReverseVIs and PolyBrushEdges.
        for (unsigned long viNr = 0; viNr < NUM_VERTS; viNr++)
        {
            ReverseVIs[viNr] = Vertices[NUM_VERTS - 1 - viNr];

            PolyBrushEdges[viNr].A = Vertices[viNr];
            PolyBrushEdges[viNr].B = Vertices[(viNr + 1) % NUM_VERTS];
        }

        // Actually trace the solid.
        PolyBrush.TraceConvexSolid(TraceSolid, Start, Ray, ClipMask, Result);
    }
    catch (const DivisionByZeroE&) { }
}


/*
 * There are several resources in the internet about ray-triangle intersection.
 * Apparently the most widely referenced and acknowledged with supposedly fast code is the paper by Möller and Trumbore:
 *     Paper: http://www.cs.virginia.edu/~gfx/Courses/2003/ImageSynthesis/papers/Acceleration/Fast%20MinimumStorage%20RayTriangle%20Intersection.pdf
 *     Code:  http://www.cs.lth.se/home/Tomas_Akenine_Moller/raytri/
 *
 * An alternative that claims to provide better performance when the normal is known is at
 *     http://www.softsurfer.com/Archive/algorithm_0105/algorithm_0105.htm
 *
 * Another more recent alternative that conceptually is the same as my own approach is at
 *     http://www.graphicon.ru/2007/proceedings/Papers/Paper_46.pdf
 *
 * Most important seems to be the insight of recent years that the majority of the ray-triangle intersection approaches are all
 * equivalent as shown in http://www.cs.utah.edu/~aek/research/triangle.pdf and http://realtimecollisiondetection.net/blog/?p=13,
 * because they eventually all reduce to the computation of signed volumes.
 * (Non-)Surprisingly, this is also described in my old linear algebra book from school very well ("Das Spatprodukt").
 *
 * I tried none of the above listed algorithms, because the fact of algorithm equivalency, considerations about the likelyhood of
 * cache misses when too much triangle data is precomputed (such as the normals), and the russian paper (Paper_46.pdf) convinced
 * me to go with the approach that I was about to try before having read all the papers anyway.
 * Although the resulting algorithm is very fast, it would certainly still be worthwhile to profile some of the other variants, too.
 */
void CollisionModelStaticT::PolygonT::TraceRay(
    const Vector3dT& Start, const Vector3dT& Ray, unsigned long ClipMask, TraceResultT& Result) const
{
    using namespace cf::math;

    // If the ClipFlags don't match the ClipMask, this polygon doesn't interfere with the trace.
    if (!Material) return;
    if ((Material->ClipFlags & ClipMask) == 0) return;

    const Vector3dT& A = Parent->m_Vertices[Vertices[0]];
    const Vector3dT& B = Parent->m_Vertices[Vertices[1]];
    const Vector3dT& C = Parent->m_Vertices[Vertices[2]];
    const Vector3dT& D = IsTriangle() ? A : Parent->m_Vertices[Vertices[3]];

    const PlueckerdT R = PlueckerdT::CreateFromRay(Start, Ray);

    // We use Pluecker coordinates for the orientation tests.
    // Note that Christer Ericson has shown in his blog at http://realtimecollisiondetection.net/blog/?p=13
    // that scalar triple products (Spatprodukte) are equivalent and could be used as well.  ;-)
    if (!Material->TwoSided)
    {
        if (R * PlueckerdT::CreateFromLine(A, B) >= 0) return;
        if (R * PlueckerdT::CreateFromLine(B, C) >= 0) return;
        if (R * PlueckerdT::CreateFromLine(C, D) >= 0) return;

        if (!IsTriangle())
        {
            if (R * PlueckerdT::CreateFromLine(D, A) >= 0) return;
        }
    }
    else
    {
        int Count = 0;

        // Should not change Count if result == 0...
        Count += (R * PlueckerdT::CreateFromLine(A, B) >= 0) ? -1 : 1;
        Count += (R * PlueckerdT::CreateFromLine(B, C) >= 0) ? -1 : 1;
        Count += (R * PlueckerdT::CreateFromLine(C, D) >= 0) ? -1 : 1;

        if (IsTriangle())
        {
            if (Count != -3 && Count != 3) return;
        }
        else
        {
            Count += (R * PlueckerdT::CreateFromLine(D, A) >= 0) ? -1 : 1;
            if (Count != -4 && Count != 4) return;
        }
    }

    // The "aperture" test passed, now compute the fraction.
    const Plane3dT Plane(A, B, C, 0.1);

    // Bestimmen, bei welchem Bruchteil (Fraction F) von Dir wir die Plane schneiden.
    const double Nenner = dot(Plane.Normal, Ray);

    // Dir muß dem Normalenvektor der Ebene wirklich entgegenzeigen! Ist der Nenner==0, so ist Dir parallel zur Plane,
    // und mit dieser Plane ex. kein Schnittpunkt. Ist der Nenner>0, nutzen wir die Konvexität des Brushs aus:
    // Es gibt damit nur genau einen Schnittpunkt mit dem Brush (Eintrittsstelle) und die Plane behindert nicht
    // eine Bewegung von ihr weg (Feststecken wenn Dist==0 (s.u.)).
    if (Nenner == 0) return;
    assert(Material->TwoSided || Nenner < 0);   // If material is single sided, then Nenner<0, a consequence of the Pluecker tests above.

    const double Dist = Plane.GetDistance(Start);
    const double F    = -(Dist - 0.03125) / Nenner;

    // The intersection is only valid if F is in the proper interval.
    if (F < 0 || F >= Result.Fraction) return;

    // Hit the polygon!
    Result.Fraction     = F;
    Result.ImpactNormal = (Nenner < 0) ? Plane.Normal : -Plane.Normal;  // Handle two-sided polygons properly.
    Result.Material     = Material;
}


CollisionModelStaticT::BrushT::BrushT()
    : Parent(NULL),
      Sides(NULL),
      NrOfSides(0),
      BB(),
      Contents(0),
      CheckCount(0),
      HullVerts(NULL),
      NrOfHullVerts(0),
      HullEdges(NULL),
      NrOfHullEdges(0)
{
}


// /// The destructor.
// CollisionModelStaticT::BrushT::~BrushT()
// {
//     if (Parent!=NULL)
//     {
//         Parent->m_VerticesPool.Free(HullVerts);
//         Parent->m_EdgesPool.Free(HullEdges);
//     }
// }


void CollisionModelStaticT::BrushT::CacheHullVerts() const
{
    assert(HullVerts == NULL);  // Don't call this method more than once.

    static ArrayT<unsigned long> Verts;
    Verts.Overwrite();

    for (unsigned long SideNr = 0; SideNr < NrOfSides; SideNr++)
    {
        for (unsigned long VertexNr = 0; VertexNr < Sides[SideNr].NrOfVertices; VertexNr++)
        {
            const unsigned long VI = Sides[SideNr].Vertices[VertexNr];

            if (Verts.Find(VI) < 0) Verts.PushBack(VI);
        }
    }

    HullVerts = Parent->m_VerticesPool.Alloc(Verts.Size());
    NrOfHullVerts = Verts.Size();

    for (unsigned long VertexNr = 0; VertexNr < NrOfHullVerts; VertexNr++)
        HullVerts[VertexNr] = Verts[VertexNr];
}


void CollisionModelStaticT::BrushT::CacheHullEdges() const
{
    assert(HullEdges == NULL);  // Don't call this method more than once.

    static ArrayT<BrushT::EdgeT> Edges;
    Edges.Overwrite();

    for (unsigned long SideNr = 0; SideNr < NrOfSides; SideNr++)
    {
        if (Sides[SideNr].NrOfVertices < 3) continue;

        unsigned long V1Nr    = Sides[SideNr].NrOfVertices - 1;
        unsigned long V1Index = Sides[SideNr].Vertices[V1Nr];

        for (unsigned long V2Nr = 0; V2Nr < Sides[SideNr].NrOfVertices; V2Nr++)
        {
            const unsigned long V2Index = Sides[SideNr].Vertices[V2Nr];
            unsigned long EdgeNr;

            for (EdgeNr = 0; EdgeNr < Edges.Size(); EdgeNr++)
            {
                const EdgeT& E = Edges[EdgeNr];

                if ((E.A == V1Index && E.B == V2Index) || (E.A == V2Index && E.B == V1Index)) break;
            }

            if (EdgeNr == Edges.Size())
            {
                Edges.PushBackEmpty();
                Edges[EdgeNr].A = V1Index;
                Edges[EdgeNr].B = V2Index;
            }

            V1Nr    = V2Nr;
            V1Index = V2Index;
        }
    }

    HullEdges = Parent->m_EdgesPool.Alloc(Edges.Size());
    NrOfHullEdges = Edges.Size();

    for (unsigned long EdgeNr = 0; EdgeNr < NrOfHullEdges; EdgeNr++)
        HullEdges[EdgeNr] = Edges[EdgeNr];
}


// The code of this method has been developed from our Brush3T<T>::TraceBoundingBox() implementation.
// It has grown much more complicated though, because it supports tracing arbitrary convex polyhedrons rather than only AABBs,
// and the bevel planes are not precomputed externally to the brush, but dynamically on the fly.
void CollisionModelStaticT::BrushT::TraceConvexSolid(
    const InternalTraceSolidT& TraceSolid, const Vector3dT& Start, const Vector3dT& Ray, unsigned long ClipMask, TraceResultT& Result) const
{
    // If the contents of this brush doesn't match the ContMask, it doesn't interfere with the trace.
    if ((Contents & ClipMask) == 0) return;

    // If the trace solid is empty, it cannot collide with the brush.
    if (TraceSolid.NumVerts == 0) return;

    // Use the optimized point trace whenever possible.
    if (TraceSolid.NumVerts == 1)
    {
        // TraceSolid.Verts[0] is normally supposed to be (0, 0, 0), but let's support the generic case.
        TraceRay(Start + TraceSolid.Verts[0], Ray, ClipMask, Result);
        return;
    }

    // If the overall bounding box of the movement of TraceSolid from Start to Start+Ray*Result.Fraction
    // doesn't intersect with our BB, this brush doesn't interfere with the trace.
    // However, it's probably more expensive to compute the OverallBB here than running the full tests below.
    // Instead, the caller should do this test, as it is in a much better position
    // (it has to compute the OverallBB only once for possible reuse with many other traces).
 // if (!OverallBB.Intersects(BB)) return;


    // Create an array of all relevant planes for the brush.
    // As described in the paper "An Object-Space Method for Calculating the Minkowski Sums of Simple 3D Objects"
    // at http://www.cmpe.boun.edu.tr/medialab/object_space_min_sum.pdf by Diktas and Sahiner, note that the
    // relevant planes come from three sources:
    //   a) the planes of this brush,
    //   b) the (mirrored) planes of the trace solid,
    //   c) the "bevel" planes generated by sweeping the edges of the trace solid along the edges of the brush.
    static ArrayT<const Plane3dT*> AllPlanes;
    static ArrayT<Plane3dT>        BevelPlanes;

    AllPlanes.Overwrite();
    BevelPlanes.Overwrite();

    // Case a), add the planes of this brush.
    for (unsigned long PlaneNr = 0; PlaneNr < NrOfSides; PlaneNr++)
        AllPlanes.PushBack(&Sides[PlaneNr].Plane);

    // Case b), add the (mirrored) planes of the trace solid.
    if (HullVerts == NULL) CacheHullVerts();

    BevelPlanes.PushBackEmpty(TraceSolid.NumPlanes);

    for (unsigned long PlaneNr = 0; PlaneNr < TraceSolid.NumPlanes; PlaneNr++)
    {
        Plane3dT& BP = BevelPlanes[PlaneNr];

        BP.Normal = -TraceSolid.Planes[PlaneNr].Normal;
        BP.Dist   = dot(BP.Normal, Parent->m_Vertices[HullVerts[0]]);

        for (unsigned long VertexNr = 1; VertexNr < NrOfHullVerts; VertexNr++)
        {
            const double Dist = dot(BP.Normal, Parent->m_Vertices[HullVerts[VertexNr]]);

            if (Dist > BP.Dist) BP.Dist = Dist;
        }
    }

    // Case c), add the planes obtained by sweeping the edges of the trace solid along the edges of the brush.
    if (HullEdges == NULL) CacheHullEdges();

    for (unsigned long BrushEdgeNr = 0; BrushEdgeNr < NrOfHullEdges; BrushEdgeNr++)
    {
        const EdgeT& EdgeBrush = HullEdges[BrushEdgeNr];

        for (unsigned int TSEdgeNr = 0; TSEdgeNr < TraceSolid.NumEdges; TSEdgeNr++)
        {
            const TraceSolidT::EdgeT& EdgeTS = TraceSolid.Edges[TSEdgeNr];

            Plane3dT SepPlane;

            SepPlane.Normal = cross(Parent->m_Vertices[EdgeBrush.B] - Parent->m_Vertices[EdgeBrush.A],
                                    TraceSolid.Verts[EdgeTS.B] - TraceSolid.Verts[EdgeTS.A]);

            const double NormalLen = length(SepPlane.Normal);
            if (NormalLen < 0.00001) continue;
            SepPlane.Normal /= NormalLen;

            // If this brush is completely on one side of the SepPlane and the trace solid is completely on the other,
            // we have found a valid separation plane.
            // As SepPlane also contains an edge of each object, it follows that the objects can collide at the considered edges,
            // which in turn implies that SepPlane is a proper bevel plane.
            unsigned long TsCountAbove = 0;
            unsigned long TsCountIn    = 0;
            unsigned long TsCountBelow = 0;

            SepPlane.Dist = dot(SepPlane.Normal, TraceSolid.Verts[EdgeTS.A]);

            for (unsigned int VertexNr = 0; VertexNr < TraceSolid.NumVerts; VertexNr++)
            {
                const double Dist = SepPlane.GetDistance(TraceSolid.Verts[VertexNr]);

                // TODO: 0.01 ...?
                     if (Dist >  0.01) TsCountAbove++;
                else if (Dist < -0.01) TsCountBelow++;
                else                   TsCountIn++;
            }

            if (TsCountIn >= 3) continue;                       // Irrelevant plane - duplicate of a regular TraceSolid plane.
            if (TsCountAbove > 0 && TsCountBelow > 0) continue; // Plane intersects TraceSolid, so it cannot be a separating plane.
            assert((TsCountAbove > 0) != (TsCountBelow > 0));   // != is the logical xor operator...


            unsigned long BrCountAbove = 0;
            unsigned long BrCountIn    = 0;
            unsigned long BrCountBelow = 0;

            SepPlane.Dist = dot(SepPlane.Normal, Parent->m_Vertices[EdgeBrush.A]);

            for (unsigned long VertexNr = 0; VertexNr < NrOfHullVerts; VertexNr++)
            {
                const double Dist = SepPlane.GetDistance(Parent->m_Vertices[HullVerts[VertexNr]]);

                // TODO: 0.01 ...?
                     if (Dist >  0.01) BrCountAbove++;
                else if (Dist < -0.01) BrCountBelow++;
                else                   BrCountIn++;
            }

            if (BrCountIn >= 3) continue;                       // Irrelevant plane - duplicate of a regular brush plane.
            if (BrCountAbove > 0 && BrCountBelow > 0) continue; // Plane intersects brush, so it cannot be a separating plane.
            assert((BrCountAbove > 0) != (BrCountBelow > 0));   // != is the logical xor operator...


            if (BrCountAbove > 0 && TsCountAbove > 0) continue;
            if (BrCountBelow > 0 && TsCountBelow > 0) continue;

            BevelPlanes.PushBack(BrCountBelow > 0 ? SepPlane : SepPlane.GetMirror());
        }
    }

    // Append all the BevelPlanes to the AllPlanes.
    for (unsigned long PlaneNr = 0; PlaneNr < BevelPlanes.Size(); PlaneNr++)
        AllPlanes.PushBack(&BevelPlanes[PlaneNr]);


    // Determine the "Bloat-Distance-Offsets" for each plane of the brush.
    // For a given brush plane P that offset is the (negative) distance between planes Po and Pv,
    // where Po is a plane parallel to P through the origin of the convex polyhedron defined by HullVerts,
    // and   Pv is a plane parallel to P through the "outmost" vertex of the convex polyhedron in direction of the normal vector.
    static ArrayT<double> BloatDistanceOffsets;

    while (BloatDistanceOffsets.Size() < AllPlanes.Size()) BloatDistanceOffsets.PushBack(0.0);

    for (unsigned long PlaneNr = 0; PlaneNr < AllPlanes.Size(); PlaneNr++)
    {
        const Plane3dT* P = AllPlanes[PlaneNr];

        BloatDistanceOffsets[PlaneNr] = dot(P->Normal, TraceSolid.Verts[0]);

        for (unsigned int VertexNr = 1; VertexNr < TraceSolid.NumVerts; VertexNr++)
        {
            const double Ofs = dot(P->Normal, TraceSolid.Verts[VertexNr]);

            // Yes, it's "<" rather than ">": Due to the direction of the P.Normal, the offsets are actually negative.
            if (Ofs < BloatDistanceOffsets[PlaneNr]) BloatDistanceOffsets[PlaneNr] = Ofs;
        }
    }

    // Wenn 'Start' im Inneren des (soliden) Brushs liegt, sitzen wir fest.
    {
        unsigned long PlaneNr;

        for (PlaneNr = 0; PlaneNr < AllPlanes.Size(); PlaneNr++)
            if (AllPlanes[PlaneNr]->GetDistance(Start) + BloatDistanceOffsets[PlaneNr] >= 0) break;

        if (PlaneNr == AllPlanes.Size())
        {
            Result.StartSolid = true;
            Result.Fraction   = 0;
            return;
        }
    }

    for (unsigned long PlaneNr = 0; PlaneNr < AllPlanes.Size(); PlaneNr++)
    {
        // Bestimmen, bei welchem Bruchteil (Fraction F) von Ray wir die Plane schneiden.
        const double Nenner = dot(AllPlanes[PlaneNr]->Normal, Ray);

        // Dir muß dem Normalenvektor der Ebene wirklich entgegenzeigen! Ist der Nenner==0, so ist Dir parallel zur Plane,
        // und mit dieser Plane ex. kein Schnittpunkt. Ist der Nenner>0, nutzen wir die Konvexität des Brushs aus:
        // Es gibt damit nur genau einen Schnittpunkt mit dem Brush (Eintrittsstelle) und die Plane behindert nicht
        // eine Bewegung von ihr weg (Feststecken wenn Dist==0 (s.u.)).
        if (Nenner >= 0) continue;

        const double Dist = AllPlanes[PlaneNr]->GetDistance(Start) + BloatDistanceOffsets[PlaneNr];
        double       F    = -Dist / Nenner;

        // Der Schnitt macht nur Sinn, wenn F im gewünschten Intervall liegt
        if (F < 0 || F >= Result.Fraction) continue;

        // Prüfen, ob Schnittpunkt wirklich auf dem Brush liegt
        {
            const Vector3dT HitPos = Start + Ray * F;
            unsigned long PNr;

            for (PNr = 0; PNr < AllPlanes.Size(); PNr++)
                if (PNr != PlaneNr /*Rundungsfehlern zuvorkommen!*/ && AllPlanes[PNr]->GetDistance(HitPos) + BloatDistanceOffsets[PNr] > 0.01) break;

            if (PNr < AllPlanes.Size()) continue;
        }

        // Wir haben die einzige Eintrittsstelle gefunden!
        // Eigentlich ist das errechete F einwandfrei. Wir wollen es jedoch nochmal etwas verringern, sodaß der sich
        // ergebende Schnittpunkt (HitPos) in einem Abstand von 0.03125 ÜBER der Ebene liegt! Bildhaft wird dazu die
        // Schnittebene um 0.03125 in Richtung ihres Normalenvektors geschoben und F wie oben neu errechnet.
        // Dies erspart uns ansonsten ernste Probleme:
        // - Wird diese Funktion nochmals mit dem Trace-Ergebnis (HitPos) als Start-Vektor aufgerufen,
        //   kann dieser neue Start-Vektor nicht wegen Rundungsfehlern in den Brush geraten (Solid!).
        // - Wird unser Trace-Ergebnis (HitPos) als Start-Vektor dieser Funktion, aber mit einem anderen
        //   Brush benutzt, der gegenüber diesem Brush nur in der Schnittebene verschoben ist (z.B. eine lange
        //   Wand, die aus mehreren "Backsteinen" besteht), kommt es auch hier nicht zum (falschen)
        //   "Hängenbleiben" an einer gemeinsamen Brush-Kante.
        // Aber: Die HitPos kann natürlich trotzdem näher als 0.03125 an der Ebene liegen, nämlich genau dann, wenn
        // es nicht zu einem Schnitt kam und Dir zufällig dort endet. Wir ignorieren diese Möglichkeit: Kommt es doch
        // noch zu einem Schnitt, wird eben F==0. Deshalb können wir uns auch in diesem Fall nicht durch Rundungsfehler
        // ins Innere des Brushs schaukeln.
        F = -(Dist - 0.03125) / Nenner;                 // Vgl. Berechnung von F oben!

        if (F < 0              ) F = 0;
        if (F > Result.Fraction) F = Result.Fraction;   // Pro forma.

        Result.Fraction     = F;
        Result.ImpactNormal = AllPlanes[PlaneNr]->Normal;
        Result.Material     = PlaneNr < NrOfSides ? Sides[PlaneNr].Material : NULL;
        break;
    }
}


void CollisionModelStaticT::BrushT::TraceRay(
    const Vector3dT& Start, const Vector3dT& Ray, unsigned long ClipMask, TraceResultT& Result) const
{
    // If the contents of this brush doesn't match the ContMask, it doesn't interfere with the trace.
    if ((Contents & ClipMask) == 0) return;


    // Wenn 'Start' im Inneren des (soliden) Brushs liegt, sitzen wir fest.
    unsigned long SideNr;

    for (SideNr = 0; SideNr < NrOfSides; SideNr++)
        if (Sides[SideNr].Plane.GetDistance(Start) >= 0) break;

    if (SideNr == NrOfSides)
    {
        Result.StartSolid = true;
        Result.Fraction   = 0;
        return;
    }

    for (SideNr = 0; SideNr < NrOfSides; SideNr++)
    {
        assert(Sides[SideNr].Material != NULL || !Parent->m_GenericBrushes);  // Assert that Material!=NULL or the brush is with precomputed bevels.
        if (Sides[SideNr].Material == NULL) continue;

        // Bestimmen, bei welchem Bruchteil (Fraction F) von Ray wir die Plane schneiden.
        const double Nenner = dot(Sides[SideNr].Plane.Normal, Ray);

        // Dir muß dem Normalenvektor der Ebene wirklich entgegenzeigen! Ist der Nenner==0, so ist Dir parallel zur Plane,
        // und mit dieser Plane ex. kein Schnittpunkt. Ist der Nenner>0, nutzen wir die Konvexität des Brushs aus:
        // Es gibt damit nur genau einen Schnittpunkt mit dem Brush (Eintrittsstelle) und die Plane behindert nicht
        // eine Bewegung von ihr weg (Feststecken wenn Dist==0 (s.u.)).
        if (Nenner >= 0) continue;

        const double Dist = Sides[SideNr].Plane.GetDistance(Start);
        double       F    = -Dist / Nenner;

        // Der Schnitt macht nur Sinn, wenn F im gewünschten Intervall liegt
        if (F < 0 || F >= Result.Fraction) continue;

        // Prüfen, ob Schnittpunkt wirklich auf dem Brush liegt
        const Vector3dT HitPos = Start + Ray * F;
        unsigned long SNr;

        for (SNr = 0; SNr < NrOfSides; SNr++)
            if (SNr != SideNr /*Rundungsfehlern zuvorkommen!*/ && Sides[SNr].Plane.GetDistance(HitPos) > 0.01) break;

        if (SNr < NrOfSides) continue;

        // Wir haben die einzige Eintrittsstelle gefunden!
        // Eigentlich ist das errechete F einwandfrei. Wir wollen es jedoch nochmal etwas verringern, sodaß der sich
        // ergebende Schnittpunkt (HitPos) in einem Abstand von 0.03125 ÜBER der Ebene liegt! Bildhaft wird dazu die
        // Schnittebene um 0.03125 in Richtung ihres Normalenvektors geschoben und F wie oben neu errechnet.
        // Dies erspart uns ansonsten ernste Probleme:
        // - Wird diese Funktion nochmals mit dem Trace-Ergebnis (HitPos) als Start-Vektor aufgerufen,
        //   kann dieser neue Start-Vektor nicht wegen Rundungsfehlern in den Brush geraten (Solid!).
        // - Wird unser Trace-Ergebnis (HitPos) als Start-Vektor dieser Funktion, aber mit einem anderen
        //   Brush benutzt, der gegenüber diesem Brush nur in der Schnittebene verschoben ist (z.B. eine lange
        //   Wand, die aus mehreren "Backsteinen" besteht), kommt es auch hier nicht zum (falschen)
        //   "Hängenbleiben" an einer gemeinsamen Brush-Kante.
        // Aber: Die HitPos kann natürlich trotzdem näher als 0.03125 an der Ebene liegen, nämlich genau dann, wenn
        // es nicht zu einem Schnitt kam und Dir zufällig dort endet. Wir ignorieren diese Möglichkeit: Kommt es doch
        // noch zu einem Schnitt, wird eben F==0. Deshalb können wir uns auch in diesem Fall nicht durch Rundungsfehler
        // ins Innere des Brushs schaukeln.
        F = -(Dist - 0.03125) / Nenner;                 // Vgl. Berechnung von F oben!

        if (F < 0              ) F = 0;
        if (F > Result.Fraction) F = Result.Fraction;   // Pro forma.

        Result.Fraction     = F;
        Result.ImpactNormal = Sides[SideNr].Plane.Normal;
        Result.Material     = Sides[SideNr].Material;
        break;
    }
}


void CollisionModelStaticT::BrushT::TraceBevelBB(
    const BoundingBox3dT& TraceBB, const Vector3dT& Start, const Vector3dT& Ray, unsigned long ClipMask, TraceResultT& Result) const
{
    assert(!Parent->m_GenericBrushes);

    // If the contents of this brush doesn't match the ContMask, it doesn't interfere with the trace.
    if ((Contents & ClipMask) == 0) return;

    // Assume that the caller already did these checks.
    // if (TraceBB.Min==TraceBB.Max) { TraceRay(Start+TraceBB.Min, Ray, ClipMask, Result); return; }
    // if (!OverallBB.Intersects(BB)) return;


    static ArrayT<double> BloatDistanceOffsets;

    // Bloat-Distance-Offsets für alle Planes dieses Brushs bestimmen.
    while (BloatDistanceOffsets.Size() < NrOfSides) BloatDistanceOffsets.PushBack(0.0);

    for (unsigned long SideNr = 0; SideNr < NrOfSides; SideNr++)
    {
        const Plane3dT& P = Sides[SideNr].Plane;

        BloatDistanceOffsets[SideNr] = dot(P.Normal, Vector3dT(P.Normal.x < 0 ? TraceBB.Max.x : TraceBB.Min.x,
                                           P.Normal.y < 0 ? TraceBB.Max.y : TraceBB.Min.y,
                                           P.Normal.z < 0 ? TraceBB.Max.z : TraceBB.Min.z));
    }

    // Wenn 'Start' im Inneren des (soliden) Brushs liegt, sitzen wir fest.
    unsigned long SideNr;

    for (SideNr = 0; SideNr < NrOfSides; SideNr++)
        if (Sides[SideNr].Plane.GetDistance(Start) + BloatDistanceOffsets[SideNr] >= 0) break;

    if (SideNr == NrOfSides)
    {
        Result.StartSolid = true;
        Result.Fraction   = 0;
        return;
    }

    for (SideNr = 0; SideNr < NrOfSides; SideNr++)
    {
        // Bestimmen, bei welchem Bruchteil (Fraction F) von Ray wir die Plane schneiden.
        const double Nenner = dot(Sides[SideNr].Plane.Normal, Ray);

        // Dir muß dem Normalenvektor der Ebene wirklich entgegenzeigen! Ist der Nenner==0, so ist Dir parallel zur Plane,
        // und mit dieser Plane ex. kein Schnittpunkt. Ist der Nenner>0, nutzen wir die Konvexität des Brushs aus:
        // Es gibt damit nur genau einen Schnittpunkt mit dem Brush (Eintrittsstelle) und die Plane behindert nicht
        // eine Bewegung von ihr weg (Feststecken wenn Dist==0 (s.u.)).
        if (Nenner >= 0) continue;

        const double Dist = Sides[SideNr].Plane.GetDistance(Start) + BloatDistanceOffsets[SideNr];
        double       F    = -Dist / Nenner;

        // Der Schnitt macht nur Sinn, wenn F im gewünschten Intervall liegt
        if (F < 0 || F >= Result.Fraction) continue;

        // Prüfen, ob Schnittpunkt wirklich auf dem Brush liegt
        const Vector3dT HitPos = Start + Ray * F;
        unsigned long SNr;

        for (SNr = 0; SNr < NrOfSides; SNr++)
            if (SNr != SideNr /*Rundungsfehlern zuvorkommen!*/ && Sides[SNr].Plane.GetDistance(HitPos) + BloatDistanceOffsets[SNr] > 0.01) break;

        if (SNr < NrOfSides) continue;

        // Wir haben die einzige Eintrittsstelle gefunden!
        // Eigentlich ist das errechete F einwandfrei. Wir wollen es jedoch nochmal etwas verringern, sodaß der sich
        // ergebende Schnittpunkt (HitPos) in einem Abstand von 0.03125 ÜBER der Ebene liegt! Bildhaft wird dazu die
        // Schnittebene um 0.03125 in Richtung ihres Normalenvektors geschoben und F wie oben neu errechnet.
        // Dies erspart uns ansonsten ernste Probleme:
        // - Wird diese Funktion nochmals mit dem Trace-Ergebnis (HitPos) als Start-Vektor aufgerufen,
        //   kann dieser neue Start-Vektor nicht wegen Rundungsfehlern in den Brush geraten (Solid!).
        // - Wird unser Trace-Ergebnis (HitPos) als Start-Vektor dieser Funktion, aber mit einem anderen
        //   Brush benutzt, der gegenüber diesem Brush nur in der Schnittebene verschoben ist (z.B. eine lange
        //   Wand, die aus mehreren "Backsteinen" besteht), kommt es auch hier nicht zum (falschen)
        //   "Hängenbleiben" an einer gemeinsamen Brush-Kante.
        // Aber: Die HitPos kann natürlich trotzdem näher als 0.03125 an der Ebene liegen, nämlich genau dann, wenn
        // es nicht zu einem Schnitt kam und Dir zufällig dort endet. Wir ignorieren diese Möglichkeit: Kommt es doch
        // noch zu einem Schnitt, wird eben F==0. Deshalb können wir uns auch in diesem Fall nicht durch Rundungsfehler
        // ins Innere des Brushs schaukeln.
        F = -(Dist - 0.03125) / Nenner;                 // Vgl. Berechnung von F oben!

        if (F < 0              ) F = 0;
        if (F > Result.Fraction) F = Result.Fraction;   // Pro forma.

        Result.Fraction     = F;
        Result.ImpactNormal = Sides[SideNr].Plane.Normal;
        Result.Material     = Sides[SideNr].Material;
        break;
    }
}


CollisionModelStaticT::TerrainRefT::TerrainRefT()
    : Terrain(NULL),
      Material(NULL),
      BB(),
      CheckCount(0)
{
}


CollisionModelStaticT::TerrainRefT::TerrainRefT(const TerrainT* Terrain_, MaterialT* Material_, const BoundingBox3dT& BB_)
    : Terrain(Terrain_),
      Material(Material_),
      BB(BB_),
      CheckCount(0)
{
}


// FIXME: This implementation does only work when called with the root node.
// FIXME:   When called with a "not root"-node, we additionally have to loop through all parents and take their polygons,
// FIXME:   brushes and terrains into account, too, due to the fact that the elements of the parents are - per definition -
// FIXME:   inside all children!!  (Fortunately, this method is never called with anything but the root node, thus there
// FIXME:   is no immediate danger, but the problem should be fixed asap nevertheless!)
BoundingBox3dT CollisionModelStaticT::NodeT::GetBB() const
{
    const CollisionModelStaticT::NodeT* Node = this;
    BoundingBox3dT BB;

    while (true)
    {
        for (unsigned long PolyNr = 0;    PolyNr    < Node->Polygons.Size(); PolyNr++   ) BB += Node->Polygons[PolyNr]->GetBB();
        for (unsigned long BrushNr = 0;   BrushNr   < Node->Brushes.Size();  BrushNr++  ) BB += Node->Brushes[BrushNr]->BB;
        for (unsigned long TerrainNr = 0; TerrainNr < Node->Terrains.Size(); TerrainNr++) BB += Node->Terrains[TerrainNr]->BB;

        if (Node->PlaneType == NodeT::NONE) break;

        // Recurse into Node->Children[1].
        const BoundingBox3dT Child1BB = Node->Children[1]->GetBB();
        if (Child1BB.IsInited()) BB += Child1BB;    // Child1BB.IsInited() can be legally "false" here!

        // Recurse into Node->Children[0] (tail recursion).
        Node = Node->Children[0];
    }

    // Note that it is well possible that BB.IsInited()==false here,
    // e.g. when the parent of this leaf node has objects only on its "other" side.
    return BB;
}


// FIXME: This implementation does only work when called with the root node.
// FIXME:   When called with a "not root"-node, we additionally have to loop through all parents and take their polygons,
// FIXME:   brushes and terrains into account, too, due to the fact that the elements of the parents are - per definition -
// FIXME:   inside all children!!  (Fortunately, this method is never called with anything but the root node, thus there
// FIXME:   is no immediate danger, but the problem should be fixed asap nevertheless!)
unsigned long CollisionModelStaticT::NodeT::GetContents() const
{
    const CollisionModelStaticT::NodeT* Node     = this;
    unsigned long                       Contents = 0;

    while (true)
    {
        for (unsigned long PolyNr = 0;    PolyNr    < Node->Polygons.Size(); PolyNr++   ) if (Node->Polygons[PolyNr]->Material) Contents |= Node->Polygons[PolyNr]->Material->ClipFlags;
        for (unsigned long BrushNr = 0;   BrushNr   < Node->Brushes.Size();  BrushNr++  ) Contents |= Node->Brushes[BrushNr]->Contents;
        for (unsigned long TerrainNr = 0; TerrainNr < Node->Terrains.Size(); TerrainNr++) if (Node->Terrains[TerrainNr]->Material) Contents |= Node->Terrains[TerrainNr]->Material->ClipFlags;

        if (Node->PlaneType == NodeT::NONE) break;

        Contents |= Node->Children[1]->GetContents();
        Node = Node->Children[0];
    }

    return Contents;
}


bool CollisionModelStaticT::NodeT::IntersectsAllChildren(const BoundingBox3dT& BB) const
{
    if (PlaneType != NONE)
    {
        if (BB.Min[PlaneType] >= PlaneDist) return false;
        if (BB.Max[PlaneType] <= PlaneDist) return false;

        if (!Children[0]->IntersectsAllChildren(BB)) return false;
        if (!Children[1]->IntersectsAllChildren(BB)) return false;
    }

    return true;
}


bool CollisionModelStaticT::NodeT::DetermineSplitPlane(const BoundingBox3dT& NodeBB, const double MIN_NODE_SIZE)
{
    PlaneTypeE PlaneTypes_SBLE[3] = { ALONG_X, ALONG_Y, ALONG_Z };
    Vector3dT  NodeBBSize_SBLE    = NodeBB.Max - NodeBB.Min;

    // SBLE: "Sorted by largest extent"  :-)
    for (int i = 0; i < 2; i++)
    {
        if (NodeBBSize_SBLE[i] < NodeBBSize_SBLE[i + 1])
        {
            std::swap(PlaneTypes_SBLE[i], PlaneTypes_SBLE[i + 1]);
            std::swap(NodeBBSize_SBLE[i], NodeBBSize_SBLE[i + 1]);

            i = -1;
        }
    }

    // A split of a node that is normally too small for further splits is forced anyway if there are more than 100 polygons inside it.
    const bool ForceSplit = (NodeBBSize_SBLE[0] < MIN_NODE_SIZE && Polygons.Size() > 100);


    // Try to find an axis-aligned split plane.
    // This is achieved by trying all the bounding-box planes of the contents of this node AND ALL ANCESTOR nodes.
    // For best results, planes that are orthogonal to the axis with the largest extent are tried first.
    for (int i = 0; i < 3; i++)
    {
        const PlaneTypeE CurrentPlaneType = PlaneTypes_SBLE[i];   // Currently considered plane type. Ideally we never consider PlaneTypes_SBLE[1] and PlaneTypes_SBLE[2].
        double           BestOffset       = NodeBBSize_SBLE[i];   // Two times the distance from the center of NodeBB.

        // If the node has reached the minimum size (and is not forcibly split), then stop: We've reached a leaf.
        if (!ForceSplit && BestOffset < MIN_NODE_SIZE) break;

        // Consider the planes of the sides of the bounding-boxes of all brushes.
        for (const CollisionModelStaticT::NodeT* Ancestor = this; Ancestor != NULL; Ancestor = Ancestor->Parent)
        {
            for (unsigned long BrushNr = 0; BrushNr < Ancestor->Brushes.Size(); BrushNr++)
            {
                const CollisionModelStaticT::BrushT* Brush = Ancestor->Brushes[BrushNr];

                for (unsigned int Side = 0; Side < 2; Side++)
                {
                    const double CurrentPlaneDist = (Side == 0) ? Brush->BB.Min[CurrentPlaneType] : Brush->BB.Max[CurrentPlaneType];

                    // The split plane must intersect NodeBB per definition.
                    if (CurrentPlaneDist >= NodeBB.Max[CurrentPlaneType] || CurrentPlaneDist <= NodeBB.Min[CurrentPlaneType]) continue;

                    // Offset is (two times) the distance of the plane from the center of NodeBB.
                    const double CurrentOffset = fabs((NodeBB.Max[CurrentPlaneType] - CurrentPlaneDist) - (CurrentPlaneDist - NodeBB.Min[CurrentPlaneType]));

                    if (CurrentOffset < BestOffset)
                    {
                        BestOffset = CurrentOffset;
                        PlaneType  = CurrentPlaneType;
                        PlaneDist  = CurrentPlaneDist;
                    }
                }
            }
        }

        // Consider the planes of the sides of the bounding-boxes of all polygons.
        for (const CollisionModelStaticT::NodeT* Ancestor = this; Ancestor != NULL; Ancestor = Ancestor->Parent)
        {
            for (unsigned long PolyNr = 0; PolyNr < Ancestor->Polygons.Size(); PolyNr++)
            {
                const CollisionModelStaticT::PolygonT* Poly   = Ancestor->Polygons[PolyNr];
                const BoundingBox3dT                   PolyBB = Poly->GetBB();

                for (unsigned int Side = 0; Side < 2; Side++)
                {
                    const double CurrentPlaneDist = (Side == 0) ? PolyBB.Min[CurrentPlaneType] : PolyBB.Max[CurrentPlaneType];

                    // The split plane must intersect NodeBB per definition.
                    if (CurrentPlaneDist >= NodeBB.Max[CurrentPlaneType] || CurrentPlaneDist <= NodeBB.Min[CurrentPlaneType]) continue;

                    // Offset is (two times) the distance of the plane from the center of NodeBB.
                    const double CurrentOffset = fabs((NodeBB.Max[CurrentPlaneType] - CurrentPlaneDist) - (CurrentPlaneDist - NodeBB.Min[CurrentPlaneType]));

                    if (CurrentOffset < BestOffset)
                    {
                        BestOffset = CurrentOffset;
                        PlaneType  = CurrentPlaneType;
                        PlaneDist  = CurrentPlaneDist;
                    }
                }
            }
        }

        // Consider the planes of the sides of the bounding-boxes of all terrains.
        for (const CollisionModelStaticT::NodeT* Ancestor = this; Ancestor != NULL; Ancestor = Ancestor->Parent)
        {
            for (unsigned long TerrainNr = 0; TerrainNr < Ancestor->Terrains.Size(); TerrainNr++)
            {
                const CollisionModelStaticT::TerrainRefT* Terrain = Ancestor->Terrains[TerrainNr];

                for (unsigned int Side = 0; Side < 2; Side++)
                {
                    const double CurrentPlaneDist = (Side == 0) ? Terrain->BB.Min[CurrentPlaneType] : Terrain->BB.Max[CurrentPlaneType];

                    // The split plane must intersect NodeBB per definition.
                    if (CurrentPlaneDist >= NodeBB.Max[CurrentPlaneType] || CurrentPlaneDist <= NodeBB.Min[CurrentPlaneType]) continue;

                    // Offset is (two times) the distance of the plane from the center of NodeBB.
                    const double CurrentOffset = fabs((NodeBB.Max[CurrentPlaneType] - CurrentPlaneDist) - (CurrentPlaneDist - NodeBB.Min[CurrentPlaneType]));

                    if (CurrentOffset < BestOffset)
                    {
                        BestOffset = CurrentOffset;
                        PlaneType  = CurrentPlaneType;
                        PlaneDist  = CurrentPlaneDist;
                    }
                }
            }
        }

        // Check if we were able to choose a split plane orthogonal to the current (largest extent) axis.
        if (BestOffset < NodeBBSize_SBLE[i])
        {
            // If the split was forced due to too many polygons in the node, we accept the found plane unconditionally.
            if (ForceSplit) return true;

            // In the normal case, the plane is OK if it isn't too close to the sides of NodeBB.
            // (If too close, skip it and rather try plane types along the lesser axes instead.)
            if ((NodeBB.Max[CurrentPlaneType] - PlaneDist > MIN_NODE_SIZE * 0.5) &&
                (PlaneDist - NodeBB.Min[CurrentPlaneType] > MIN_NODE_SIZE * 0.5)) return true;
        }
    }

    // No split plane was found.
    PlaneType = NONE;
    PlaneDist = 0.0;
    return false;
}


static double clamp(double min, double v, double max)
{
    if (v < min) return min;
    if (v > max) return max;

    return v;
}


void CollisionModelStaticT::NodeT::Trace(const Vector3dT& A, const Vector3dT& B, double FracA, double FracB, const TraceParamsT& Params) const
{
    // If we already hit something nearer, there is no need to check this node.
    // Note that we really have to compare with `<` here, not `<=`, because if both sides
    // are 0.0, we have to run the code below in order to not miss and thus fail to report
    // actual starts in solid! See http://trac.cafu.de/ticket/148 for details.
    if (Params.Result.Fraction < FracA) return;

    // Trace against all the brushes of this node.
    for (unsigned long BrushNr = 0; BrushNr < Brushes.Size(); BrushNr++)
    {
        const BrushT* Brush = Brushes[BrushNr];

        if (Brush->CheckCount == s_CheckCount) continue;
        Brush->CheckCount = s_CheckCount;

        if (Params.TraceSolid.NumVerts == 1)    // Also checked by BrushT::TraceConvexSolid(), but not by BrushT::TraceBevelBB(), thus anticipate it here.
        {
            // TraceSolid.Verts[0] is normally supposed to be (0, 0, 0), but let's support the generic case.
            Brush->TraceRay(Params.Start + Params.TraceSolid.Verts[0], Params.Ray, Params.ClipMask, Params.Result);
        }
        else
        {
            if (Params.GenericBrushes) Brush->TraceConvexSolid(Params.TraceSolid, Params.Start, Params.Ray, Params.ClipMask, Params.Result);
                                  else Brush->TraceBevelBB(Params.TraceBB, Params.Start, Params.Ray, Params.ClipMask, Params.Result);
        }

        // If the contents of Brush got us stuck in solid, we can't go farther, so stop here.
        if (Params.Result.StartSolid) return;
    }

    // Trace against all the polygons of this node.
    for (unsigned long PolyNr = 0; PolyNr < Polygons.Size(); PolyNr++)
    {
        const PolygonT* Poly = Polygons[PolyNr];

        if (Poly->CheckCount == s_CheckCount) continue;
        Poly->CheckCount = s_CheckCount;

        if (Params.TraceSolid.NumVerts == 1)    // Also checked by PolygonT::TraceConvexSolid(), but checking it here saves another function call there.
        {
            Poly->TraceRay(Params.Start, Params.Ray, Params.ClipMask, Params.Result);
        }
        else
        {
            Poly->TraceConvexSolid(Params.TraceSolid, Params.Start, Params.Ray, Params.ClipMask, Params.Result);
        }

        // If the contents of Poly got us stuck in solid, we can't go farther, so stop here.
        if (Params.Result.StartSolid) return;
    }

    // Trace against all the terrains of this node.
    for (unsigned long TerrainNr = 0; TerrainNr < Terrains.Size(); TerrainNr++)
    {
        const TerrainRefT* Terrain = Terrains[TerrainNr];

        if (Terrain->CheckCount == s_CheckCount) continue;
        Terrain->CheckCount = s_CheckCount;

        // If the ClipFlags of this terrain don't match the ClipMask, it doesn't interfere with the trace.
        if (!Terrain->Material) continue;
        if ((Terrain->Material->ClipFlags & Params.ClipMask) == 0) continue;

        const double       OldFrac = Params.Result.Fraction;
        VB_Trace3T<double> LocalResult(Params.Result.Fraction);

        LocalResult.StartSolid   = Params.Result.StartSolid;
        LocalResult.ImpactNormal = Params.Result.ImpactNormal;

        Terrain->Terrain->TraceBoundingBox(Params.TraceBB, Params.Start, Params.Ray, LocalResult);

        Params.Result.Fraction     = LocalResult.Fraction;
        Params.Result.StartSolid   = LocalResult.StartSolid;
        Params.Result.ImpactNormal = LocalResult.ImpactNormal;

        if (Params.Result.Fraction < OldFrac)
        {
            Params.Result.Material = Terrain->Material;
        }

        // If the contents of Terrain got us stuck in solid, we can't go farther, so stop here.
        if (Params.Result.StartSolid) return;
    }

    // If this is a leaf node, we're done.
    if (PlaneType == NodeT::NONE) return;


    // Recurse into the children.
    const double DistA = A[PlaneType] - PlaneDist;   // The (signed) distance of A from the node plane.
    const double DistB = B[PlaneType] - PlaneDist;   // The (signed) distance of B from the node plane.

    // Adjust the plane according to the TraceBB - this is a case of "bloating" a BSP tree!
    const double EPSILON = 1.0;
    const double BloatOffset = std::max(fabs(Params.TraceBB.Min[PlaneType]), fabs(Params.TraceBB.Max[PlaneType])) + EPSILON;

    // Trace the TraceBB along the node plane.
    if (DistA >= BloatOffset && DistB >= BloatOffset)
    {
        // Both A and B are in front of the bloated node plane, so just recurse into the front sub-tree.
        Children[0]->Trace(A, B, FracA, FracB, Params);
        return;
    }

    if (DistA < -BloatOffset && DistB < -BloatOffset)
    {
        // Both A and B are behind the bloated node plane, so just recurse into the back sub-tree.
        Children[1]->Trace(A, B, FracA, FracB, Params);
        return;
    }

    if (fabs(DistA - DistB) < 0.001)
    {
        // The line through A and B is (almost) parallel to the node plane,
        // so just perform the full trace in both sub-trees.
        Children[0]->Trace(A, B, FracA, FracB, Params);
        Children[1]->Trace(A, B, FracA, FracB, Params);
        return;
    }

    // Compute the (sub-)fractions for node plane intersection in distance +/- BloatOffset.
    // A + (B-A)*SubFracBack  yields the intersection point with the node plane bloated towards the back.
    // A + (B-A)*SubFracFront yields the intersection point with the node plane bloated towards the front.
    const double SubFracBack  = clamp(0.0, (DistA + BloatOffset) / (DistA - DistB), 1.0);
    const double SubFracFront = clamp(0.0, (DistA - BloatOffset) / (DistA - DistB), 1.0);

    // Recursively trace into the "near" sub-tree first, then the "far", always with proper overlaps into the other.
    if (DistA < DistB)
    {
        // The line through A and B pierces the node plane from back to front / A to B (roughly along the normal vector).
        // Thus we first check the back sub-tree, then the front sub-tree, each with the appropriately clipped line segments.
        Children[1]->Trace(A, A + (B - A)*SubFracFront, FracA, FracA + (FracB - FracA)*SubFracFront, Params);
        Children[0]->Trace(A + (B - A)*SubFracBack, B,  FracA + (FracB - FracA)*SubFracBack, FracB,  Params);
    }
    else
    {
        // The line through A and B pierces the node plane from front to back / B to A (roughly opposite the normal vector).
        // Thus we first check the front sub-tree, then the back sub-tree, each with the appropriately clipped line segments.
        Children[0]->Trace(A, A + (B - A)*SubFracBack,  FracA, FracA + (FracB - FracA)*SubFracBack,  Params);
        Children[1]->Trace(A + (B - A)*SubFracFront, B, FracA + (FracB - FracA)*SubFracFront, FracB, Params);
    }
}


void CollisionModelStaticT::NodeT::Insert(const PolygonT* Poly)
{
    NodeT*               Node   = this;
    const BoundingBox3dT PolyBB = Poly->GetBB();

    while (Node->PlaneType != NodeT::NONE)
    {
        if (Node->IntersectsAllChildren(PolyBB)) break;

        if (PolyBB.Min[Node->PlaneType] >= Node->PlaneDist)
        {
            Node = Node->Children[0];
        }
        else if (PolyBB.Max[Node->PlaneType] <= Node->PlaneDist)
        {
            Node = Node->Children[1];
        }
        else
        {
            Node->Children[1]->Insert(Poly);
            Node = Node->Children[0];
        }
    }

    Node->Polygons.PushBack(Poly);
}


void CollisionModelStaticT::NodeT::Insert(const BrushT* Brush)
{
    NodeT* Node = this;

    while (Node->PlaneType != NodeT::NONE)
    {
        if (Node->IntersectsAllChildren(Brush->BB)) break;

        if (Brush->BB.Min[Node->PlaneType] >= Node->PlaneDist)
        {
            Node = Node->Children[0];
        }
        else if (Brush->BB.Max[Node->PlaneType] <= Node->PlaneDist)
        {
            Node = Node->Children[1];
        }
        else
        {
            Node->Children[1]->Insert(Brush);
            Node = Node->Children[0];
        }
    }

    Node->Brushes.PushBack(Brush);
}


void CollisionModelStaticT::NodeT::Insert(const TerrainRefT* Terrain)
{
    NodeT* Node = this;

    while (Node->PlaneType != NodeT::NONE)
    {
        if (Node->IntersectsAllChildren(Terrain->BB)) break;

        if (Terrain->BB.Min[Node->PlaneType] >= Node->PlaneDist)
        {
            Node = Node->Children[0];
        }
        else if (Terrain->BB.Max[Node->PlaneType] <= Node->PlaneDist)
        {
            Node = Node->Children[1];
        }
        else
        {
            Node->Children[1]->Insert(Terrain);
            Node = Node->Children[0];
        }
    }

    Node->Terrains.PushBack(Terrain);
}


struct LessVector3d
{
    // See "Die C++ Programmiersprache" by Bjarne Stroustrup pages 498 and 510 and
    // Scott Meyers "Effective STL" Item 21 for more information about this struct.
    bool operator () (const Vector3dT& v1, const Vector3dT& v2) const
    {
        // TODO: How big a difference does it make?
        // First check if v1 and v2 are the "same", then return false in accordance with Item 21.
        // const double VERTEX_EPSILON=0.1;     // Note:  MapT::RoundEpsilon is 2.0, use that instead?
        // if (fabs(v1.x-v2.x)<VERTEX_EPSILON &&
        //     fabs(v1.y-v2.y)<VERTEX_EPSILON &&
        //     fabs(v1.z-v2.z)<VERTEX_EPSILON) return false;

        // Now the usual comparison, also in accordance with Item 21.
        if (v1.x < v2.x) return true;
        if (v1.x > v2.x) return false;

        if (v1.y < v2.y) return true;
        if (v1.y > v2.y) return false;

        return v1.z < v2.z;
    }
};


// Findet zu einem Vertex v heraus, welcher Index vertexNum (ins model->vertices array) dazugehört.
// Falls es den Vertex in model->vertices noch nicht gibt, wird er eingefügt.
// Gibt true zurück, wenn es den Vertex schon gab, false wenn er eingefügt wurde.
static unsigned long GetVertex(std::map<Vector3dT, unsigned long, LessVector3d>& VertexMap, ArrayT<Vector3dT>& Vertices, const Vector3dT& Vertex)
{
    // const float INTEGRAL_EPSILON=0.01f;
    // Vector3fT vert;
    // for (int i=0; i<3; i++) vert[i]=(fabs(v[i] - cf::math::round(v[i])) < INTEGRAL_EPSILON) ? cf::math::round(v[i]) : v[i];

    std::map<Vector3dT, unsigned long, LessVector3d>::const_iterator It = VertexMap.find(Vertex);

    if (It != VertexMap.end())
    {
        // Vertex is already an element of VertexMap.
        return It->second;
    }

    // Insert the new vertex vert into the Vertices array and its index into the VertexMap.
    const unsigned long VertexNr = Vertices.Size();

    Vertices.PushBack(Vertex);
    VertexMap[Vertex] = VertexNr;
    return VertexNr;
}


CollisionModelStaticT::CollisionModelStaticT(std::istream& InFile, cf::SceneGraph::aux::PoolT& Pool, const ArrayT<TerrainRefT>& Terrains)
    : m_Name(),
      m_BB(),
      m_Contents(0),
      m_RootNode(NULL),
      m_BulletAdapter(new BulletAdapterT(*this)),
      m_GenericBrushes(true),
      m_Terrains(Terrains)
{
    using namespace cf::SceneGraph;

    // Read the file version and first data.
    const uint32_t FileVersion = aux::ReadUInt32(InFile);
    m_Name = aux::ReadString(InFile);

    // The FileVersion is currently not for error checking (we assume that all versions are supported),
    // but for managing backward-compatible changes to the file format.
    // May add error checking support by throwing an exception later.
    assert(FileVersion >= 5);
    (void)FileVersion;      // Have g++ not warn about FileVersion being an unused variable.


    // Read the vertices.
    m_Vertices.PushBackEmptyExact(aux::ReadUInt32(InFile));

    for (unsigned long VertexNr = 0; VertexNr < m_Vertices.Size(); VertexNr++)
    {
        m_Vertices[VertexNr] = aux::ReadVector3d(InFile);
    }


    // Read the brush side vertex indices.
    m_BrushSideVIs.PushBackEmptyExact(aux::ReadUInt32(InFile));

    for (unsigned long viNr = 0; viNr < m_BrushSideVIs.Size(); viNr++)
    {
        m_BrushSideVIs[viNr] = aux::ReadUInt32(InFile);
    }


    // Read the brush sides.
    m_BrushSides.PushBackEmptyExact(aux::ReadUInt32(InFile));

    for (unsigned long SideNr = 0; SideNr < m_BrushSides.Size(); SideNr++)
    {
        BrushT::SideT& Side = m_BrushSides[SideNr];

        Side.Plane.Normal = Pool.ReadVector3d(InFile);
        Side.Plane.Dist   = aux::ReadDouble(InFile);

        Side.Vertices     = &m_BrushSideVIs[0] + aux::ReadUInt32(InFile);
        Side.NrOfVertices = aux::ReadUInt32(InFile);

        // MatName is "" if this is a precomputed "bevel" side.
        const std::string MatName = Pool.ReadString(InFile);
        Side.Material             = MatName.empty() ? NULL : MaterialManager->GetMaterial(MatName);
    }


    // Read the brushes.
    m_GenericBrushes = (aux::ReadInt32(InFile) != 0);
    m_Brushes.PushBackEmptyExact(aux::ReadUInt32(InFile));

    for (unsigned long BrushNr = 0; BrushNr < m_Brushes.Size(); BrushNr++)
    {
        BrushT& Brush = m_Brushes[BrushNr];

        Brush.Parent    = this;
        Brush.Sides     = &m_BrushSides[0] + aux::ReadUInt32(InFile);
        Brush.NrOfSides = aux::ReadUInt32(InFile);

#if 0
        // Dependent information that is not saved to disk, but recomputed on demand.
        Brush.NrOfHullVerts = aux::ReadUInt32(InFile);
        Brush.HullVerts     = m_VerticesPool.Alloc(Brush.NrOfHullVerts);
        for (unsigned long VertexNr = 0; VertexNr < Brush.NrOfHullVerts; VertexNr++)
            Brush.HullVerts[VertexNr] = aux::ReadUInt32(InFile);

        Brush.NrOfHullEdges = aux::ReadUInt32(InFile);
        Brush.HullEdges     = m_EdgesPool.Alloc(Brush.NrOfHullEdges);
        for (unsigned long EdgeNr = 0; EdgeNr < Brush.NrOfHullEdges; EdgeNr++)
        {
            Brush.HullEdges[EdgeNr].A = aux::ReadUInt32(InFile);
            Brush.HullEdges[EdgeNr].B = aux::ReadUInt32(InFile);
        }
#endif

        Brush.BB.Min = aux::ReadVector3d(InFile);
        Brush.BB.Max = aux::ReadVector3d(InFile);

        // Recompute the combined contents of the brush.
        Brush.Contents = 0;
        for (unsigned long SideNr = 0; SideNr < Brush.NrOfSides; SideNr++)
            if (Brush.Sides[SideNr].Material)   // Material is NULL if this is a precomputed "bevel" side.
                Brush.Contents |= Brush.Sides[SideNr].Material->ClipFlags;

        Brush.CheckCount = 0;
    }


    // Read the polygons.
    m_Polygons.PushBackEmptyExact(aux::ReadUInt32(InFile));

    for (unsigned long PolyNr = 0; PolyNr < m_Polygons.Size(); PolyNr++)
    {
        PolygonT& Polygon = m_Polygons[PolyNr];

        Polygon.Parent = this;

        for (unsigned long VertexNr = 0; VertexNr < 4; VertexNr++)
            Polygon.Vertices[VertexNr] = aux::ReadUInt32(InFile);

        Polygon.Material   = MaterialManager->GetMaterial(Pool.ReadString(InFile));
        Polygon.CheckCount = 0;
    }


    // Read the nodes.
    ArrayT<NodeT*> NodeStack;

    m_RootNode = m_NodesPool.Alloc();
    m_RootNode->Parent = NULL;
    NodeStack.PushBack(m_RootNode);

    while (NodeStack.Size() > 0)
    {
        NodeT* CurrentNode = NodeStack[NodeStack.Size() - 1];
        NodeStack.DeleteBack();

        assert(sizeof(CurrentNode->PlaneType) == 4);
        InFile.read((char*)&CurrentNode->PlaneType, sizeof(CurrentNode->PlaneType));
        InFile.read((char*)&CurrentNode->PlaneDist, sizeof(CurrentNode->PlaneDist));

        CurrentNode->Polygons.PushBackEmptyExact(aux::ReadUInt32(InFile));
        for (unsigned long PolyNr = 0; PolyNr < CurrentNode->Polygons.Size(); PolyNr++)
            CurrentNode->Polygons[PolyNr] = &m_Polygons[0] + aux::ReadUInt32(InFile);

        CurrentNode->Brushes.PushBackEmptyExact(aux::ReadUInt32(InFile));
        for (unsigned long BrushNr = 0; BrushNr < CurrentNode->Brushes.Size(); BrushNr++)
            CurrentNode->Brushes[BrushNr] = &m_Brushes[0] + aux::ReadUInt32(InFile);

        CurrentNode->Terrains.PushBackEmptyExact(aux::ReadUInt32(InFile));
        for (unsigned long TerrainNr = 0; TerrainNr < CurrentNode->Terrains.Size(); TerrainNr++)
            CurrentNode->Terrains[TerrainNr] = &m_Terrains[0] + aux::ReadUInt32(InFile);

        if (CurrentNode->PlaneType != NodeT::NONE)
        {
            CurrentNode->Children[0] = m_NodesPool.Alloc();
            CurrentNode->Children[0]->Parent = CurrentNode;
            NodeStack.PushBack(CurrentNode->Children[0]);

            CurrentNode->Children[1] = m_NodesPool.Alloc();
            CurrentNode->Children[1]->Parent = CurrentNode;
            NodeStack.PushBack(CurrentNode->Children[1]);
        }
    }


    // Finish loading.
    m_BB       = m_RootNode->GetBB();         // Buffer bounding box for entire shape.
    m_Contents = m_RootNode->GetContents();   // Buffer contents     for entire shape.
}


static inline int Sign(const double a)
{
    if (a >  0.000001) return  1;
    if (a < -0.000001) return -1;

    return 0;
}


// Computes the bevel planes for clip brush CB, using the newer "sign flip at edge" method,
// as outlined by Karl Patrick at http://www.xp-cagey.com/?article=1000
// This method is also very simple and robust, but several hundred times faster than the convex hull method.
static ArrayT<Plane3dT> ComputeBevelPlanes(const ArrayT< Polygon3T<double> >& BrushPolys, const BoundingBox3dT& BrushBB, const double MAP_ROUND_EPSILON)
{
    ArrayT<Plane3dT> BevelPlanes;

    BevelPlanes.PushBack(Plane3dT(Vector3dT( 1.0,  0.0,  0.0),  BrushBB.Max.x));
    BevelPlanes.PushBack(Plane3dT(Vector3dT( 0.0,  1.0,  0.0),  BrushBB.Max.y));
    BevelPlanes.PushBack(Plane3dT(Vector3dT( 0.0,  0.0,  1.0),  BrushBB.Max.z));
    BevelPlanes.PushBack(Plane3dT(Vector3dT(-1.0,  0.0,  0.0), -BrushBB.Min.x));
    BevelPlanes.PushBack(Plane3dT(Vector3dT( 0.0, -1.0,  0.0), -BrushBB.Min.y));
    BevelPlanes.PushBack(Plane3dT(Vector3dT( 0.0,  0.0, -1.0), -BrushBB.Min.z));


    // Now consider all pairs of faces that have a common edge.
    for (unsigned long Poly1Nr = 0; Poly1Nr + 1 < BrushPolys.Size(); Poly1Nr++)
    {
        const Polygon3T<double>& P1 = BrushPolys[Poly1Nr];

        for (unsigned long Poly2Nr = Poly1Nr + 1; Poly2Nr < BrushPolys.Size(); Poly2Nr++)
        {
            const Polygon3T<double>& P2 = BrushPolys[Poly2Nr];

            for (unsigned long E1Nr = 0; E1Nr < P1.Vertices.Size(); E1Nr++)
                for (unsigned long E2Nr = 0; E2Nr < P2.Vertices.Size(); E2Nr++)
                    if (P1.Vertices[E1Nr].IsEqual(P2.Vertices[(E2Nr + 1) % P2.Vertices.Size()], MAP_ROUND_EPSILON) &&
                        P1.Vertices[(E1Nr + 1) % P1.Vertices.Size()].IsEqual(P2.Vertices[E2Nr], MAP_ROUND_EPSILON))
                    {
                        // Common edge found!
                        const VectorT A = P1.Vertices[E1Nr];
                        const VectorT B = P1.Vertices[(E1Nr + 1) % P1.Vertices.Size()];

                        if (Sign(P1.Plane.Normal.x)*Sign(P2.Plane.Normal.x) == -1) try { BevelPlanes.PushBack(Plane3dT(A, B, A + Vector3dT(Sign(P1.Plane.Normal.x) * 100.0, 0.0, 0.0), MAP_ROUND_EPSILON)); } catch (const DivisionByZeroE&) { }
                        if (Sign(P1.Plane.Normal.y)*Sign(P2.Plane.Normal.y) == -1) try { BevelPlanes.PushBack(Plane3dT(A, B, A + Vector3dT(0.0, Sign(P1.Plane.Normal.y) * 100.0, 0.0), MAP_ROUND_EPSILON)); } catch (const DivisionByZeroE&) { }
                        if (Sign(P1.Plane.Normal.z)*Sign(P2.Plane.Normal.z) == -1) try { BevelPlanes.PushBack(Plane3dT(A, B, A + Vector3dT(0.0, 0.0, Sign(P1.Plane.Normal.z) * 100.0), MAP_ROUND_EPSILON)); } catch (const DivisionByZeroE&) { }
                    }
        }
    }


    // Remove duplicates from the BevelPlanes.
    for (unsigned long Plane1Nr = 0; Plane1Nr + 1 < BevelPlanes.Size(); Plane1Nr++)
        for (unsigned long Plane2Nr = Plane1Nr + 1; Plane2Nr < BevelPlanes.Size(); Plane2Nr++)
        {
            if (fabs(BevelPlanes[Plane1Nr].Dist - BevelPlanes[Plane2Nr].Dist) > 0.1) continue;

            const double  S  = 100000.0;
            const VectorT S1 = scale(BevelPlanes[Plane1Nr].Normal, S);
            const VectorT S2 = scale(BevelPlanes[Plane2Nr].Normal, S);

            if (!S1.IsEqual(S2, MAP_ROUND_EPSILON)) continue;

            // These planes are "equal"!
            BevelPlanes[Plane2Nr] = BevelPlanes[BevelPlanes.Size() - 1];
            BevelPlanes.DeleteBack();
            Plane2Nr--;
        }

    for (unsigned long PolyNr = 0; PolyNr < BrushPolys.Size(); PolyNr++)
        for (unsigned long PlaneNr = 0; PlaneNr < BevelPlanes.Size(); PlaneNr++)
        {
            if (fabs(BrushPolys[PolyNr].Plane.Dist - BevelPlanes[PlaneNr].Dist) > 0.1) continue;

            const double  S  = 100000.0;
            const VectorT S1 = scale(BrushPolys[PolyNr].Plane.Normal, S);
            const VectorT S2 = scale(BevelPlanes[PlaneNr].Normal, S);

            if (!S1.IsEqual(S2, MAP_ROUND_EPSILON)) continue;

            // These planes are "equal"!
            BevelPlanes[PlaneNr] = BevelPlanes[BevelPlanes.Size() - 1];
            BevelPlanes.DeleteBack();
            PlaneNr--;
        }

    return BevelPlanes;
}


CollisionModelStaticT::CollisionModelStaticT(const cf::MapFileEntityT& Entity, const ArrayT<TerrainRefT>& Terrains, bool UseGenericBrushes,
        const double MAP_ROUND_EPSILON, const double MAP_MIN_VERTEX_DIST, const double BP_MAX_CURVE_ERROR, const double BP_MAX_CURVE_LENGTH, const double MIN_NODE_SIZE)
    : m_Name("map file entity model"),    // This should be distinct from *any* model file name on disk (in order to not confuse the model manager).
      m_BB(),
      m_Contents(0),
      m_RootNode(m_NodesPool.Alloc()),
      m_BulletAdapter(new BulletAdapterT(*this)),
      m_GenericBrushes(UseGenericBrushes),
      m_Terrains(Terrains)
{
    // This maps vertices (of type Vector3dT) to indices into the m_Vertices array,
    // so that shared vertices can be identified quickly.
    std::map<Vector3dT, unsigned long, LessVector3d> VertexMap;


    // For each primitive of the entity (that is, all brushes and all patches),
    // create appropriate brushes and polygons in the root node of this collision model.
    // The actual child nodes of the octree are created later.

    // Brushes first.
    for (unsigned long BrushNr = 0; BrushNr < Entity.MFBrushes.Size(); BrushNr++)
    {
        const cf::MapFileBrushT& MapBrush = Entity.MFBrushes[BrushNr];  // The "source" brush from the map file.
        BrushT                   Brush;                                 // The new brush to be created here.

        Brush.Parent = this;


        // 1. Compute the Brush.Contents.
        assert(Brush.Contents == 0);

        for (unsigned long PlaneNr = 0; PlaneNr < MapBrush.MFPlanes.Size(); PlaneNr++)
        {
            assert(MapBrush.MFPlanes[PlaneNr].Material != NULL);
            Brush.Contents |= MapBrush.MFPlanes[PlaneNr].Material->ClipFlags;
        }

        // If this brush clips nothing, there is no need to keep it.
        if (!Brush.Contents) continue;

        // Omit brushes that are solid only to the the CaBSP map compiler (radiance blockers for CaLight, triggers for Cafu, etc. are kept).
        if ((Brush.Contents & ~MaterialT::Clip_BspPortals) == 0) continue;


        // 2. Compute the BrushPolys.
        ArrayT< Polygon3T<double> > BrushPolys;

        BrushPolys.PushBackEmptyExact(MapBrush.MFPlanes.Size());

        for (unsigned long PlaneNr = 0; PlaneNr < MapBrush.MFPlanes.Size(); PlaneNr++)
            BrushPolys[PlaneNr].Plane = MapBrush.MFPlanes[PlaneNr].Plane;

        Polygon3T<double>::Complete(BrushPolys, MAP_ROUND_EPSILON);


        // 3. Compute the BrushBB.
     // Brush.Sides     = ...;    // Set later!
        Brush.NrOfSides = BrushPolys.Size();
        assert(!Brush.BB.IsInited());

        for (unsigned long PolyNr = 0; PolyNr < BrushPolys.Size(); PolyNr++)
        {
            m_BrushSides.PushBackEmpty();   // Now this brush cannot be "cancelled" any more!

            BrushT::SideT&     BrushSide = m_BrushSides[m_BrushSides.Size() - 1];
            Polygon3T<double>& BrushPoly = BrushPolys[PolyNr];

            // Should never trigger, because the polygons were all valid when CaBSP initially loaded them
            // (and did the same computations and validity check)!
            assert(BrushPoly.IsValid(MAP_ROUND_EPSILON, MAP_MIN_VERTEX_DIST));

            for (unsigned long VertexNr = 0; VertexNr < BrushPoly.Vertices.Size(); VertexNr++)
                m_BrushSideVIs.PushBack(::GetVertex(VertexMap, m_Vertices, BrushPoly.Vertices[VertexNr]));

            BrushSide.Plane        = BrushPoly.Plane;
         // BrushSide.Vertices     = ...;     // Set later!
            BrushSide.NrOfVertices = BrushPoly.Vertices.Size();
            BrushSide.Material     = MapBrush.MFPlanes[PolyNr].Material;

            Brush.BB += BrushPoly.Vertices;
        }


        // 4. Optionally compute the bevel planes.
        if (!UseGenericBrushes)
        {
            const ArrayT<Plane3dT> BevelPlanes = ComputeBevelPlanes(BrushPolys, Brush.BB, MAP_ROUND_EPSILON);

            // Add a brush side for each bevel plane.
            // Note that *all* added bevel sides are created with the "NULL" material,
            // even though it would be possible to intelligently "guess" a proper material.
            // This is done for two reasons:
            //   1. It is consistent with our "generic" brushes, where collisions with bevel planes
            //      return NULL materials, too (see BrushT::TraceConvexSolid() for details).
            //   2. The BrushT::TraceRay() method is fully consistent as well: For "generic" brushes as
            //      well as those that get bevel planes precomputed here, the behaviour is exactly the same,
            //      because it skips sides with NULL materials.
            for (unsigned long PlaneNr = 0; PlaneNr < BevelPlanes.Size(); PlaneNr++)
            {
                m_BrushSides.PushBackEmpty();
                BrushT::SideT& BrushSide = m_BrushSides[m_BrushSides.Size() - 1];

                BrushSide.Plane        = BevelPlanes[PlaneNr];
             // BrushSide.Vertices     = ...;     // Set later!
                BrushSide.NrOfVertices = 0;
                BrushSide.Material     = NULL;
            }

            Brush.NrOfSides += BevelPlanes.Size();
        }


        // 5. Add the brush to the collision shape.
        m_Brushes.PushBack(Brush);
    }

    // Now that m_BrushSides and m_BrushSideVIs are not reallocated any more, fix the pointers into them.
    unsigned long FirstSideNr = 0;
    for (unsigned long BrushNr = 0; BrushNr < m_Brushes.Size(); BrushNr++)
    {
        m_Brushes[BrushNr].Sides = &m_BrushSides[FirstSideNr];
        FirstSideNr += m_Brushes[BrushNr].NrOfSides;
    }
    assert(FirstSideNr == m_BrushSides.Size());

    unsigned long FirstVINr = 0;
    for (unsigned long SideNr = 0; SideNr < m_BrushSides.Size(); SideNr++)
    {
        m_BrushSides[SideNr].Vertices = NULL;

        if (m_BrushSides[SideNr].NrOfVertices > 0)
        {
            m_BrushSides[SideNr].Vertices = &m_BrushSideVIs[FirstVINr];
            FirstVINr += m_BrushSides[SideNr].NrOfVertices;
        }
    }
    assert(FirstVINr == m_BrushSideVIs.Size());


    // Bezier patches second. Note that for patches, only polygons are created, no volume brushes.
    for (unsigned long PatchNr = 0; PatchNr < Entity.MFPatches.Size(); PatchNr++)
    {
        const cf::MapFileBezierPatchT& Patch = Entity.MFPatches[PatchNr];

        // Omit polygons that are solid only to the the CaBSP and CaLight map compilers.
        assert(Patch.Material != NULL);
        assert(MaterialT::Clip_BlkButUtils == 0x3F);  // This should hold at the time it was written, but may fail when the MaterialT::ClipFlagsT enum is updated.
        if ((Patch.Material->ClipFlags & MaterialT::Clip_BlkButUtils) == 0) continue;


        // Turn the Patch into an object of type cf::math::BezierPatchT<double>.
        // TODO: The Patch object should directly come with a cf::math::BezierPatchT<double> member!
        ArrayT<Vector3dT> ControlPointsXYZ;

        for (unsigned long ComponentNr = 0; ComponentNr < Patch.ControlPoints.Size(); ComponentNr += 5)
            ControlPointsXYZ.PushBack(Vector3dT(Patch.ControlPoints[ComponentNr + 0], Patch.ControlPoints[ComponentNr + 1], Patch.ControlPoints[ComponentNr + 2]));

        cf::math::BezierPatchT<double> BezierPatch(Patch.SizeX, Patch.SizeY, ControlPointsXYZ);

        if (Patch.SubdivsHorz > 0 && Patch.SubdivsVert > 0)
        {
            // The mapper may have provided an explicit number of subdivisions in order to avoid gaps between adjacent bezier patches.
            // The casts to unsigned long are needed in order to resolve ambiguity of the overloaded Subdivide() method.
            BezierPatch.Subdivide((unsigned long)Patch.SubdivsHorz, (unsigned long)Patch.SubdivsVert);
        }
        else
        {
            BezierPatch.Subdivide(BP_MAX_CURVE_ERROR, BP_MAX_CURVE_LENGTH);
        }


        // Add the polygons of the BezierPatch to the root node.
        for (unsigned long j = 0; j + 1 < BezierPatch.Height; j++)
            for (unsigned long i = 0; i + 1 < BezierPatch.Width; i++)
            {
                const Vector3dT& v1 = BezierPatch.GetVertex(i    , j    ).Coord;
                const Vector3dT& v2 = BezierPatch.GetVertex(i + 1, j    ).Coord;
                const Vector3dT& v3 = BezierPatch.GetVertex(i + 1, j + 1).Coord;
                const Vector3dT& v4 = BezierPatch.GetVertex(i    , j + 1).Coord;

                const unsigned long A = ::GetVertex(VertexMap, m_Vertices, v1);
                const unsigned long B = ::GetVertex(VertexMap, m_Vertices, v2);
                const unsigned long C = ::GetVertex(VertexMap, m_Vertices, v3);
                const unsigned long D = ::GetVertex(VertexMap, m_Vertices, v4);

                try
                {
                    Plane3dT Plane(v1, v2, v3, 0.1);

                    // If v1 to v4 form a rectangle, create a single quad instead of two triangles.
                    // Note that this test is somewhat limited, but it covers the most important cases and is a lot
                    // less expensive than generally checking if v1 to v4 form a planar, convex, non-degenerate polygon...
                    if ((v1 + v3 - v2).IsEqual(v4, 0.01))
                    {
                        m_Polygons.PushBack(PolygonT(this, Patch.Material, A, B, C, D));
                        continue;
                    }

                    // This is actually the first of two triangles.
                    m_Polygons.PushBack(PolygonT(this, Patch.Material, A, B, C));
                }
                catch (const DivisionByZeroE&) { }

                try
                {
                    Plane3dT Plane(v1, v3, v4, 0.1);

                    // This is the second of two triangles.
                    m_Polygons.PushBack(PolygonT(this, Patch.Material, A, C, D));
                }
                catch (const DivisionByZeroE&) { }
            }
    }


#if 0
    // Terrains third: Create polygons for all terrains in the entity.
    for (unsigned long TerrainNr = 0; TerrainNr < Entity.MFTerrains.Size(); TerrainNr++)
    {
        const cf::MapFileTerrainT& Terrain = Entity.MFTerrains[TerrainNr];

        // Omit polygons that are solid only to the the CaBSP and CaLight map compilers.
        assert(Terrain.Material != NULL);
        assert(MaterialT::Clip_BlkButUtils == 0x3F);  // This should hold at the time it was written, but may fail when the MaterialT::ClipFlagsT enum is updated.
        if ((Terrain.Material->ClipFlags & MaterialT::Clip_BlkButUtils) == 0) continue;


        ArrayT<Vector3dT>     TerrainVecs;
        ArrayT<unsigned long> TerrainIDs;
        const double          s = 1.0 / (double(Terrain.SideLength - 1));
        const Vector3dT       TerrainScale = (Terrain.Bounds.Max - Terrain.Bounds.Min).GetScaled(Vector3dT(s, s, 1.0 / 65535.0));

        TerrainVecs.PushBackEmptyExact(Terrain.SideLength * Terrain.SideLength);
        TerrainIDs.PushBackEmptyExact(Terrain.SideLength * Terrain.SideLength);

        for (unsigned long y = 0; y < Terrain.SideLength; y++)
            for (unsigned long x = 0; x < Terrain.SideLength; x++)
            {
                const unsigned long i = Terrain.SideLength * y + x;

                TerrainVecs[i].x = Terrain.Bounds.Min.x + TerrainScale.x * double(x);
                TerrainVecs[i].y = Terrain.Bounds.Min.y + TerrainScale.y * double(y);
                TerrainVecs[i].z = Terrain.Bounds.Min.z + TerrainScale.z * double(Terrain.HeightData[i]);

                TerrainIDs[i] =::GetVertex(VertexMap, m_Vertices, TerrainVecs[i]);
            }


        for (unsigned long y = 0; y + 1 < Terrain.SideLength; y++)
            for (unsigned long x = 0; x + 1 < Terrain.SideLength; x++)
            {
                const Vector3dT& v1 = TerrainVecs[Terrain.SideLength * (y    ) + x    ];
                const Vector3dT& v2 = TerrainVecs[Terrain.SideLength * (y    ) + x + 1];
                const Vector3dT& v3 = TerrainVecs[Terrain.SideLength * (y + 1) + x + 1];
                const Vector3dT& v4 = TerrainVecs[Terrain.SideLength * (y + 1) + x    ];

                const unsigned long A = TerrainIDs[Terrain.SideLength * (y    ) + x    ];
                const unsigned long B = TerrainIDs[Terrain.SideLength * (y    ) + x + 1];
                const unsigned long C = TerrainIDs[Terrain.SideLength * (y + 1) + x + 1];
                const unsigned long D = TerrainIDs[Terrain.SideLength * (y + 1) + x    ];

                try
                {
                    Plane3dT Plane(v1, v2, v3, 0.1);

                    // If v1 to v4 form a rectangle, create a single quad instead of two triangles.
                    // Note that this test is somewhat limited, but it covers the most important cases and is a lot
                    // less expensive than generally checking if v1 to v4 form a planar, convex, non-degenerate polygon...
                    if ((v1 + v3 - v2).IsEqual(v4, 0.01))
                    {
                        m_Polygons.PushBack(PolygonT(this, Terrain.Material, A, B, C, D));
                        continue;
                    }

                    // This is actually the first of two triangles.
                    m_Polygons.PushBack(PolygonT(this, Terrain.Material, A, B, C));
                }
                catch (const DivisionByZeroE&) { }

                try
                {
                    Plane3dT Plane(v1, v3, v4, 0.1);

                    // This is the second of two triangles.
                    m_Polygons.PushBack(PolygonT(this, Terrain.Material, A, C, D));
                }
                catch (const DivisionByZeroE&) { }
            }
    }
#endif


    // Add all brushes to the root node.
    for (unsigned long BrushNr = 0; BrushNr < m_Brushes.Size(); BrushNr++)
        m_RootNode->Brushes.PushBack(&m_Brushes[BrushNr]);

    // Add all polygons to the root node.
    for (unsigned long PolyNr = 0; PolyNr < m_Polygons.Size(); PolyNr++)
        m_RootNode->Polygons.PushBack(&m_Polygons[PolyNr]);

    // Add all terrains to the root node.
    for (unsigned long TerrainNr = 0; TerrainNr < m_Terrains.Size(); TerrainNr++)
        m_RootNode->Terrains.PushBack(&m_Terrains[TerrainNr]);


    // Finish the collision shape.
    m_BB       = m_RootNode->GetBB();         // Buffer bounding box for entire shape.
    m_Contents = m_RootNode->GetContents();   // Buffer contents     for entire shape.

    BuildAxialBSPTree(m_RootNode, m_BB, MIN_NODE_SIZE);   // Finally create the axis-aligned BSP tree for this collision shape.

#ifdef DEBUG
    const BoundingBox3dT TestBB = m_RootNode->GetBB();

    assert(m_BB.Min == TestBB.Min);
    assert(m_BB.Max == TestBB.Max);
#endif

#if 0
    // Print some (memory usage) statistics:
    std::cout << "\n";
    std::cout << "Model name: " << m_Name << "\n";
    std::cout << "Model BB: " << m_BB.Min << " - " << m_BB.Max << "\n";
    std::cout << "Model old vertices: " << m_Vertices.Size() << " @ " << sizeof(VertexT) << ", total: " << m_Vertices.Size()*sizeof(VertexT) << "\n";
    std::cout << "Model old edges:    " << m_Edges.Size()    << " @ " << sizeof(EdgeT)   << ", total: " << m_Edges.Size()*sizeof(EdgeT)      << "\n";
    std::cout << "Model nodes: ...\n";
    std::cout << "\n";
    std::cout << "Model nodes pool:          " << m_NodesPool       .GetSize() << " @ " << sizeof(NodeT               ) << ", total: " << m_NodesPool       .GetSize()*sizeof(NodeT               ) << "\n";
    std::cout << "Model OLD polynodes pool:  " << m_PolygonNodesPool.GetSize() << " @ " << sizeof(ListNodeT<PolygonT*>) << ", total: " << m_PolygonNodesPool.GetSize()*sizeof(ListNodeT<PolygonT*>) << "\n";
    std::cout << "Model OLD polygons pool:   " << m_PolygonsPool    .GetSize() << " @ " << sizeof(PolygonT            ) << ", total: " << m_PolygonsPool    .GetSize()*sizeof(PolygonT            ) << "\n";
    std::cout << "Model OLD poly edges pool: " << m_PolygonEdgesPool.GetSize() << " @ " << sizeof(int                 ) << ", total: " << m_PolygonEdgesPool.GetSize()*sizeof(int                 ) << "\n";
    std::cout << "Model brush vertices pool: " << m_VerticesPool    .GetSize() << " @ " << sizeof(unsigned long       ) << ", total: " << m_VerticesPool    .GetSize()*sizeof(unsigned long       ) << "\n";
    std::cout << "Model brush edges pool:    " << m_EdgesPool       .GetSize() << " @ " << sizeof(BrushT::EdgeT       ) << ", total: " << m_EdgesPool       .GetSize()*sizeof(BrushT::EdgeT       ) << "\n";
    std::cout << "\n";
    std::cout << "Model new polygons:     " << m_Polygons.Size()   << " @ " << sizeof(PolygonT)      << ", total: " << m_Polygons.Size()*sizeof(PolygonT) << "\n";
    std::cout << "Model new brushes:      " << m_Brushes.Size()    << " @ " << sizeof(BrushT)        << ", total: " << m_Brushes.Size()*sizeof(BrushT) << "\n";
    std::cout << "Model brush sides:      " << m_BrushSides.Size() << " @ " << sizeof(BrushT::SideT) << ", total: " << m_BrushSides.Size()*sizeof(BrushT::SideT) << "\n";
    std::cout << "Model new vertices:     " << m_Vertices.Size()   << " @ " << sizeof(Vector3dT)     << ", total: " << m_Vertices.Size()*sizeof(Vector3dT) << "\n";
    std::cout << "Model new terrain refs: " << m_Terrains.Size()   << " @ " << sizeof(TerrainRefT)   << ", total: " << m_Terrains.Size()*sizeof(TerrainRefT) << "\n";
    std::cout << "\n";
#endif
}


CollisionModelStaticT::CollisionModelStaticT(unsigned long Width, unsigned long Height, const ArrayT<Vector3dT>& Mesh, MaterialT* Material, const double MIN_NODE_SIZE)
    : m_Name("custom mesh model"),    // This should be distinct from *any* model file name on disk (in order to not confuse the model manager).
      m_BB(),
      m_Contents(0),
      m_RootNode(m_NodesPool.Alloc()),
      m_BulletAdapter(new BulletAdapterT(*this)),
      m_GenericBrushes(true),
      m_Terrains()
{
    // This maps vertices (of type Vector3dT) to indices into the m_Vertices array,
    // so that shared vertices can be identified quickly.
    std::map<Vector3dT, unsigned long, LessVector3d> VertexMap;

    for (unsigned long RowNr = 0; RowNr + 1 < Height; RowNr++)
        for (unsigned long ColumnNr = 0; ColumnNr + 1 < Width; ColumnNr++)
        {
            const Vector3dT& v1 = Mesh[(RowNr    ) * Width + (ColumnNr    )];
            const Vector3dT& v2 = Mesh[(RowNr    ) * Width + (ColumnNr + 1)];
            const Vector3dT& v3 = Mesh[(RowNr + 1) * Width + (ColumnNr + 1)];
            const Vector3dT& v4 = Mesh[(RowNr + 1) * Width + (ColumnNr    )];

            const unsigned long A = ::GetVertex(VertexMap, m_Vertices, v1);
            const unsigned long B = ::GetVertex(VertexMap, m_Vertices, v2);
            const unsigned long C = ::GetVertex(VertexMap, m_Vertices, v3);
            const unsigned long D = ::GetVertex(VertexMap, m_Vertices, v4);

            try
            {
                Plane3dT Plane(v1, v2, v3, 0.1);

                // If v1 to v4 form a rectangle, create a single quad instead of two triangles.
                // Note that this test is somewhat limited, but it covers the most important cases and is a lot
                // less expensive than generally checking if v1 to v4 form a planar, convex, non-degenerate polygon...
                if ((v1 + v3 - v2).IsEqual(v4, 0.01))
                {
                    m_Polygons.PushBack(PolygonT(this, Material, A, B, C, D));
                    continue;
                }

                // This is actually the first of two triangles.
                m_Polygons.PushBack(PolygonT(this, Material, A, B, C));
            }
            catch (const DivisionByZeroE&) { }

            try
            {
                Plane3dT Plane(v1, v3, v4, 0.1);

                // This is the second of two triangles.
                m_Polygons.PushBack(PolygonT(this, Material, A, C, D));
            }
            catch (const DivisionByZeroE&) { }
        }


    // Add all polygons to the root node.
    for (unsigned long PolyNr = 0; PolyNr < m_Polygons.Size(); PolyNr++)
        m_RootNode->Polygons.PushBack(&m_Polygons[PolyNr]);


    // Finish the collision shape.
    m_BB       = m_RootNode->GetBB();         // Buffer bounding box for entire shape.
    m_Contents = m_RootNode->GetContents();   // Buffer contents     for entire shape.

    BuildAxialBSPTree(m_RootNode, m_BB, MIN_NODE_SIZE);   // Finally create the axis-aligned BSP tree for this collision shape.
}


/*
void CollisionModelStaticT::FreeTree(NodeT* Node)
{
    if (Node==NULL) return;

    // Delete the entire tree recursively.
    if (Node->PlaneType!=NodeT::NONE)
    {
        FreeTree(Node->Children[0]); Node->Children[0] = NULL;
        FreeTree(Node->Children[1]); Node->Children[1] = NULL;
    }

    m_NodesPool.Free(Node);
}
*/


CollisionModelStaticT::~CollisionModelStaticT()
{
    // It's not necessary to call FreeTree(), because m_NodesPool.Free() does actually nothing.
    // FreeTree(m_RootNode);

    delete m_BulletAdapter;
}


BoundingBox3dT CollisionModelStaticT::GetBoundingBox() const
{
    return m_BB;
}


unsigned long CollisionModelStaticT::GetContents() const
{
    return m_Contents;
}


void CollisionModelStaticT::SaveToFile(std::ostream& OutFile, cf::SceneGraph::aux::PoolT& Pool) const
{
    using namespace cf::SceneGraph;

    // Write the file version and first data.
    const uint32_t FileVersion = 5;

    aux::Write(OutFile, FileVersion);
    aux::Write(OutFile, m_Name);


    // Write the vertices.
    aux::Write(OutFile, aux::cnc_ui32(m_Vertices.Size()));

    for (unsigned long VertexNr = 0; VertexNr < m_Vertices.Size(); VertexNr++)
    {
        // The vertices are intentionally not written via Pool.Write(),
        // because they *are* "pooled" (free from duplicates) already.
        aux::Write(OutFile, m_Vertices[VertexNr]);
    }


    // Write the brush side vertex indices.
    aux::Write(OutFile, aux::cnc_ui32(m_BrushSideVIs.Size()));

    for (unsigned long viNr = 0; viNr < m_BrushSideVIs.Size(); viNr++)
    {
        aux::Write(OutFile, aux::cnc_ui32(m_BrushSideVIs[viNr]));
    }


    // Write the brush sides.
    aux::Write(OutFile, aux::cnc_ui32(m_BrushSides.Size()));

    for (unsigned long SideNr = 0; SideNr < m_BrushSides.Size(); SideNr++)
    {
        const BrushT::SideT& Side = m_BrushSides[SideNr];

        Pool.Write(OutFile, Side.Plane.Normal);
        aux::Write(OutFile, Side.Plane.Dist);

        aux::Write(OutFile, aux::cnc_i32(Side.Vertices ? Side.Vertices - &m_BrushSideVIs[0] : 0));  // Index of first vertex index in m_BrushSideVIs.
        aux::Write(OutFile, aux::cnc_ui32(Side.NrOfVertices));

        Pool.Write(OutFile, Side.Material ? Side.Material->Name : "");  // Side.Material==NULL can occur when m_GenericBrushes==false.
    }


    // Write the brushes.
    aux::Write(OutFile, int32_t(m_GenericBrushes));
    aux::Write(OutFile, aux::cnc_ui32(m_Brushes.Size()));

    for (unsigned long BrushNr = 0; BrushNr < m_Brushes.Size(); BrushNr++)
    {
        const BrushT& Brush = m_Brushes[BrushNr];

        aux::Write(OutFile, aux::cnc_i32(Brush.Sides - &m_BrushSides[0]));  // Index of first side in m_BrushSides.
        aux::Write(OutFile, aux::cnc_ui32(Brush.NrOfSides));

#if 0
        // Dependent information that is not saved to disk, but recomputed on demand.
        aux::Write(OutFile, Brush.NrOfHullVerts);
        for (unsigned long VertexNr = 0; VertexNr < Brush.NrOfHullVerts; VertexNr++)
            aux::Write(OutFile, Brush.HullVerts[VertexNr]);

        aux::Write(OutFile, Brush.NrOfHullEdges);
        for (unsigned long EdgeNr = 0; EdgeNr < Brush.NrOfHullEdges; EdgeNr++)
        {
            aux::Write(OutFile, Brush.HullEdges[EdgeNr].A);
            aux::Write(OutFile, Brush.HullEdges[EdgeNr].B);
        }
#endif

        // For non-generic brushes, the bounding-box cannot be recovered from the vertices when loading.
        aux::Write(OutFile, Brush.BB.Min);
        aux::Write(OutFile, Brush.BB.Max);

     // aux::Write(OutFile, Brush.Contents);    // Contents is recovered from the sides when loading.
    }


    // Write the polygons.
    aux::Write(OutFile, aux::cnc_ui32(m_Polygons.Size()));

    for (unsigned long PolyNr = 0; PolyNr < m_Polygons.Size(); PolyNr++)
    {
        const PolygonT& Polygon = m_Polygons[PolyNr];

        for (unsigned long VertexNr = 0; VertexNr < 4; VertexNr++)
            aux::Write(OutFile, aux::cnc_ui32(Polygon.Vertices[VertexNr]));

        Pool.Write(OutFile, Polygon.Material ? Polygon.Material->Name : "");
    }


    // Write the nodes.
    ArrayT<NodeT*> NodeStack;
    NodeStack.PushBack(m_RootNode);

    while (NodeStack.Size() > 0)
    {
        const NodeT* CurrentNode = NodeStack[NodeStack.Size() - 1];
        NodeStack.DeleteBack();

        OutFile.write((char*)&CurrentNode->PlaneType, sizeof(CurrentNode->PlaneType));
        OutFile.write((char*)&CurrentNode->PlaneDist, sizeof(CurrentNode->PlaneDist));

        aux::Write(OutFile, aux::cnc_ui32(CurrentNode->Polygons.Size()));
        for (unsigned long PolyNr = 0; PolyNr < CurrentNode->Polygons.Size(); PolyNr++)
            aux::Write(OutFile, aux::cnc_i32(CurrentNode->Polygons[PolyNr] - &m_Polygons[0]));      // Index of polygon in m_Polygons.

        aux::Write(OutFile, aux::cnc_ui32(CurrentNode->Brushes.Size()));
        for (unsigned long BrushNr = 0; BrushNr < CurrentNode->Brushes.Size(); BrushNr++)
            aux::Write(OutFile, aux::cnc_i32(CurrentNode->Brushes[BrushNr] - &m_Brushes[0]));       // Index of brush in m_Brushes.

        aux::Write(OutFile, aux::cnc_ui32(CurrentNode->Terrains.Size()));
        for (unsigned long TerrainNr = 0; TerrainNr < CurrentNode->Terrains.Size(); TerrainNr++)
            aux::Write(OutFile, aux::cnc_i32(CurrentNode->Terrains[TerrainNr] - &m_Terrains[0]));   // Index of terrain in m_Terrains.

        if (CurrentNode->PlaneType != NodeT::NONE)
        {
            NodeStack.PushBack(CurrentNode->Children[0]);
            NodeStack.PushBack(CurrentNode->Children[1]);
        }
    }
}


void CollisionModelStaticT::TraceConvexSolid(
    const TraceSolidT& TraceSolid, const Vector3dT& Start, const Vector3dT& Ray, unsigned long ClipMask, TraceResultT& Result) const
{
    CollisionModelStaticT::s_CheckCount++;

    TraceParamsT Params(m_GenericBrushes, TraceSolid, Start, Ray, ClipMask, Result);

    m_RootNode->Trace(Start, Start + Ray * Result.Fraction, 0, Result.Fraction, Params);
}


unsigned long CollisionModelStaticT::GetContents(const Vector3dT& Point, double BoxRadius, unsigned long ContMask) const
{
    static ArrayT<NodeT*> NodeStack;    // It's static for better performance, giving up thread safety.
    unsigned long         Contents = 0;

    CollisionModelStaticT::s_CheckCount++;

    assert(NodeStack.Size() == 0);
    NodeStack.Overwrite();
    NodeStack.PushBack(m_RootNode);

    while (NodeStack.Size() > 0)
    {
        const NodeT* Node = NodeStack[NodeStack.Size() - 1];
        NodeStack.DeleteBack();

        for (unsigned long BrushNr = 0; BrushNr < Node->Brushes.Size(); BrushNr++)
        {
            const BrushT* Brush = Node->Brushes[BrushNr];

            // Tested this brush already?
            if (Brush->CheckCount == s_CheckCount) continue;
            Brush->CheckCount = s_CheckCount;

            // Does the ContMask match?
            if ((Brush->Contents & ContMask) == 0) continue;

            // Is Point inside the brush?
            unsigned long SideNr;

            for (SideNr = 0; SideNr < Brush->NrOfSides; SideNr++)
            {
                const double Dist = Brush->Sides[SideNr].Plane.GetDistance(Point);

                if (Dist > BoxRadius) break;
            }

            if (SideNr < Brush->NrOfSides) continue;

            // Ok, the volume of the brush is actually relevant.
            Contents |= Brush->Contents;
        }

        if (Node->PlaneType != NodeT::NONE)
        {
            const double Dist = Point[Node->PlaneType] - Node->PlaneDist;

                 if (Dist >  BoxRadius) NodeStack.PushBack(Node->Children[0]);
            else if (Dist < -BoxRadius) NodeStack.PushBack(Node->Children[1]);
            else
            {
                NodeStack.PushBack(Node->Children[0]);
                NodeStack.PushBack(Node->Children[1]);
            }
        }
    }

    return Contents;
}


btCollisionShape* CollisionModelStaticT::GetBulletAdapter() const
{
    return m_BulletAdapter;
}


// See the comments in CaWE/OrthoBspTree.hpp for some additional information.
void CollisionModelStaticT::BuildAxialBSPTree(NodeT* Node, const BoundingBox3dT& NodeBB, const double MIN_NODE_SIZE)
{
    if (!Node->DetermineSplitPlane(NodeBB, MIN_NODE_SIZE)) return;

    // Create a front child for Node.
    BoundingBox3dT FrontBB = NodeBB;
    FrontBB.Min[Node->PlaneType] = Node->PlaneDist;

    NodeT* FrontNode = m_NodesPool.Alloc();
    FrontNode->Parent    = Node;
    FrontNode->PlaneType = NodeT::NONE;
    Node->Children[0] = FrontNode;

    // Create a back child for Node.
    BoundingBox3dT BackBB = NodeBB;
    BackBB.Max[Node->PlaneType] = Node->PlaneDist;

    NodeT* BackNode = m_NodesPool.Alloc();
    BackNode->Parent    = Node;
    BackNode->PlaneType = NodeT::NONE;
    Node->Children[1] = BackNode;

    // Insert the elements in Node at their proper place into the tree.
    for (NodeT* Ancestor = Node; Ancestor != NULL; Ancestor = Ancestor->Parent)
    {
        for (unsigned long PolyNr = 0; PolyNr < Ancestor->Polygons.Size(); PolyNr++)
        {
            const PolygonT* Poly = Ancestor->Polygons[PolyNr];

            if (!Ancestor->IntersectsAllChildren(Poly->GetBB()))
            {
                Ancestor->Insert(Poly);
                Ancestor->Polygons.RemoveAt(PolyNr);
                PolyNr--;
            }
        }

        for (unsigned long BrushNr = 0; BrushNr < Ancestor->Brushes.Size(); BrushNr++)
        {
            const BrushT* Brush = Ancestor->Brushes[BrushNr];

            if (!Ancestor->IntersectsAllChildren(Brush->BB))
            {
                Ancestor->Insert(Brush);
                Ancestor->Brushes.RemoveAt(BrushNr);
                BrushNr--;
            }
        }

        for (unsigned long TerrainNr = 0; TerrainNr < Ancestor->Terrains.Size(); TerrainNr++)
        {
            const TerrainRefT* Terrain = Ancestor->Terrains[TerrainNr];

            if (!Ancestor->IntersectsAllChildren(Terrain->BB))
            {
                Ancestor->Insert(Terrain);
                Ancestor->Terrains.RemoveAt(TerrainNr);
                TerrainNr--;
            }
        }
    }

    // Recurse.
    BuildAxialBSPTree(FrontNode, FrontBB, MIN_NODE_SIZE);
    BuildAxialBSPTree(BackNode,  BackBB, MIN_NODE_SIZE);
}
