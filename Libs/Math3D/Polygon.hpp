/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/***************/
/*** Polygon ***/
/***************/

#ifndef CAFU_MATH_POLYGON_HPP_INCLUDED
#define CAFU_MATH_POLYGON_HPP_INCLUDED

#include "Plane3.hpp"
#include "Templates/Array.hpp"


/// This class implements convex polygons.
///
/// Properties / invariants of polygons:
/// 1. Polygons are always convex.
/// 2. Polygons are always on the "front" side of their plane.
/// 3. The vertices of a polygon are always ordered in clockwise (CW) order,
///    as seen from the front half-space (against the normal vectors direction) of the polygons plane.
/// 4. A polygon has at least three vertices and its plane is not degenerate, or it is the "null" polygon.
/// 5. There are no coincident vertices, that is, V[n]!=V[n+1] for all n.
template<class T>
class Polygon3T
{
    public:

    ArrayT< Vector3T<T> > Vertices; ///< Vertices of this polygon.
    Plane3T<T>            Plane;    ///< Plane of this polygon.


    /// Describes on which side of a plane the polygon is.
    enum SideT
    {
        Empty      =0,  ///< The polygon is empty (it has no vertices).
        On         =1,  ///< The polygon has vertices in the plane. Note that this value alone is never returned by WhatSide().
        Front      =2,  ///< The polygon has vertices on the front-side of the plane.
        FrontAndOn =3,  ///< The polygon has vertices on the front-side of and in the plane.
        Back       =4,  ///< The polygon has vertices on the back-side of the plane.
        BackAndOn  =5,  ///< The polygon has vertices on the back-side of and in the plane.
        Both       =6,  ///< The polygon has vertices on both sides of the plane.
        BothAndOn  =7,  ///< The polygon has vertices on both sides of and in the plane.
        InIdentical=8,  ///< The polygon is in the plane.
        InMirrored =9   ///< The polygon is in the mirrored version of the plane (opposite normal vectors).
    };


    /// This methods returns whether a polygon is valid wrt. the above stipulated properties/terms.
    /// @param RoundEpsilon     The epsilon tolerance that is acceptable for rounding errors.
    /// @param MinVertexDist    The minimum distance that vertices must be apart.
    ///     The MinVertexDist value should be a good deal larger than the RoundEpsilon value, because *very* subtle problems
    ///     can easily arise whenever the "structural object size" is as small as the maximally permitted rounding error!
    /// @returns true if the polygon is valid, false otherwise.
    bool IsValid(const double RoundEpsilon, const double MinVertexDist) const;

    /// Returns the plane that contains the polygon edge defined by (VertexNr, VertexNr+1), that is orthogonal to the polygons
    /// plane and whose normal vector points towards the polygon center.
    ///
    /// @param  VertexNr   The edge defined by (VertexNr, VertexNr+1) for which a plane should be created.
    /// @param  Epsilon    Maximum error value.
    /// @throws DivisionByZeroE   if the edge is shorter than accounted for by Epsilon.
    Plane3T<T> GetEdgePlane(unsigned long VertexNr, const T Epsilon) const
    {
        return Plane3T<T>(Vertices[VertexNr],
                          Vertices[VertexNr+1<Vertices.Size() ? VertexNr+1 : 0],
                          Vertices[VertexNr]-Plane.Normal, Epsilon);
    }

    /// Returns a mirrored copy of this polygon, with reversed plane and reversed order.
    /// (If this polygon was valid, the mirrored polygon is valid, too.)
    Polygon3T<T> GetMirror() const;

    /// Determines the spatial area (Flächeninhalt) of this polygon.
    T GetArea() const;

    /// Determines whether this polygon has a vertex A, within Epsilon tolerance of the existing vertices.
    bool HasVertex(const Vector3T<T>& A, const T Epsilon) const;

    /// Determines on what side of plane P this polygon is.
    /// @param P The plane to check with.
    /// @param HalfPlaneThickness DOCTODO
    /// @returns one of the values of SideT. Note however that SideT::On is never returned in favour of either SideT::InIdentical or SideT::InMirrored.
    ///     If this polygon has edges that are shorter than the plane thickness (==2*HalfPlaneThickness) and SideT::InIdentical or SideT::InMirrored
    ///     is returned, the result is inreliable and should not be trusted. (The polygon should then be considered as invalid and be discarded.)
    SideT WhatSide(const Plane3T<T>& P, const T HalfPlaneThickness) const;

    /// Like WhatSide(), but it integrates the "...AndOn" answers with the non-"AndOn" answers.
    /// That is, instead of FrontAndOn only Front is returned, instead of BackAndOn Back, and instead of BothAndOn Both is returned.
    /// @param P The plane to check with.
    /// @param HalfPlaneThickness DOCTODO
    /// @see WhatSide()
    SideT WhatSideSimple(const Plane3T<T>& P, const T HalfPlaneThickness) const;

    /// Determines whether this polygon overlaps OtherPoly (in the same plane).
    ///
    /// @param OtherPoly   The polygon for which is determined whether it is overlapped by this polygon.
    /// @param ReportTouchesAsOverlaps   If true, an overlap is reported even if it has zero area, that is, even if the two polygons merely "touch"
    ///        within the thickness of the edges. If false, an overlap is only reported if it has a positive, non-zero area.
    /// @param EdgeThickness describes both the "thickness" of the edges within which small gaps (i.e. non-overlaps)
    ///        are tolerated, *and* the minimum length of the edges of this polygon and OtherPoly.
    /// @throws InvalidOperationE if an edge of this polygon or OtherPoly is shorter than EdgeThickness.
    /// @returns true if there is an overlap, false otherwise.
    bool Overlaps(const Polygon3T<T>& OtherPoly, bool ReportTouchesAsOverlaps, const T EdgeThickness) const;

    /// Determines whether this polygon entirely encloses OtherPoly (in the same or mirrored plane).
    ///
    /// @param OtherPoly   The polygon for which is determined whether it is enclosed by this polygon.
    /// @param MayTouchEdges   If true, OtherPoly may touch the edges of this polygon within EdgeThickness tolerance and is still being
    ///        considered enclosed. If false, OtherPoly must truly be inside this polygon, and it must not touch its edges.
    /// @param EdgeThickness describes both the "thickness" of the edges of this polygon *and* the minimum length of the edges of this polygon and OtherPoly.
    /// @returns true if this polygon encloses OtherPoly, false otherwise.
    ///          All edge lengths of this and OtherPoly should be longer than 2*EdgeThinkness, or else the result will be undefined.
    /// @throws InvalidOperationE if an edge of this polygon is shorter than EdgeThickness.
    bool Encloses(const Polygon3T<T>& OtherPoly, bool MayTouchEdges, const T EdgeThickness) const;

    /// Splits the polygon along the passed plane.
    /// WARNING: This function can return invalid polygons from a valid polygon!
    /// This happens for example if the polygon is a sharp wedge whose top is cut of. If the wedge was sharp enough,
    /// vertices may be created at the cut surface that concur with each other!
    /// Therefore the resulting polygons should ALWAYS be checked for validity by IsValid()!
    /// @param SplitPlane The plane to split the polygon with.
    /// @param HalfPlaneThickness DOCTODO
    /// @return The resulting polygons.
    ArrayT< Polygon3T<T> > GetSplits(const Plane3T<T>& SplitPlane, const T HalfPlaneThickness) const;

    /// Cuts this polygon along the edges of Poly. Prerequisite: this->Overlaps(Poly)!
    /// The last polygon in NewPolys is the overlapping one (*), the front entries lie outside the polygon.
    /// (If this polygon is contained in Poly (even then this->Overlaps(Poly) is true), NewPolys contains only a copy of this polygon.)
    /// This also works without this prerequisite, but (*) is no longer true!
    /// WARNING: This function uses GetSplits() and therfore has the same troubles as this method!
    /// @param Poly Polygon to cut this polygon with.
    /// @param EdgeThickness DOCTODO
    /// @param NewPolys The resulting polygons.
    void GetChoppedUpAlong(const Polygon3T<T>& Poly, const T EdgeThickness, ArrayT< Polygon3T<T> >& NewPolys) const;


    /// If poly builds t-junctions at this polygon, the according vertives are added to this polygon.
    /// WARNING: The resulting polygon will, in general, be *INVALID* (several vertices per edge).
    /// @param Poly Polygon that is checked for t-junctions with this polygon.
    /// @param EdgeThickness DOCTODO
    void FillTJunctions(const Polygon3T<T>& Poly, const T EdgeThickness);


    /// Merge Poly1 and Poly2 under the following prerequisites:
    /// - Poly1 and Poly2 have at least 3 vertices (non-degenerate).
    /// - Both polygons lie in the same plane with the same orientation.
    /// - Poly1 and Poly2 have one common, but opposed edge.
    /// - The new polygon is (at the welded joint) also konvex.
    /// @param Poly1 First polygon to merge.
    /// @param Poly2 Second polygon to merge.
    /// @param EdgeThickness DOCTODO
    /// @throws InvalidOperationE if Poly1 and Poly2 could not be merged.
    static Polygon3T<T> Merge(const Polygon3T<T>& Poly1, const Polygon3T<T>& Poly2, const T EdgeThickness);

    /// Given a brush (here: array of polygons), from which only the planes are known.
    /// WARNING: All normal vectors of the polygons must point to the outside!
    /// Find the vertices of all polygons and sort them in clockwise direction.
    /// WARNING: Degenerated inputs are also possible, e.g. the planes from a cube with one side missing.
    /// Those cases can't be caught here as errors because this functionality is necessary for portal creation.
    /// In this case polygons with fewer than 3 vertices are returned and an explicit validity check is necessary!
    /// It is also possible that the input planes of a polygon for example specify a triangle where one edge is shorter than GeometryEpsilon.
    /// In this case the vertices would be combined and a polygon with only 2 vertices would be the result!
    /// Further example for similar degenerated cases are also possible.
    /// Consequence: Check the returned polygon sets ALWAYS for validity, e.g. Polys[...].Vertices.Size()>=3 !
    /// @param Polys Polygons to complete
    /// @param HalfPlaneThickness DOCTODO
    /// @param BoundingSphereCenter DOCTODO
    /// @param BoundingSphereRadius DOCTODO
    static void Complete(ArrayT< Polygon3T<T> >& Polys, const T HalfPlaneThickness, const Vector3T<T>& BoundingSphereCenter=Vector3T<T>(0, 0, 0), const T BoundingSphereRadius=10.0*1000.0*1000.0);
};

#endif
