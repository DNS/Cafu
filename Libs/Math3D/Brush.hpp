/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MATH_BRUSH_HPP_INCLUDED
#define CAFU_MATH_BRUSH_HPP_INCLUDED

#include "BoundingBox.hpp"
#include "Plane3.hpp"
#include "Templates/Array.hpp"


/// This classes describes the trace (Verfolgungsergebnis) of a vector or a bounding box with regards to a brush.
/// The name VB_TraceT means "vector-brush trace".
template<class T>
class VB_Trace3T
{
    public:

    T           Fraction;       ///< This is how far we got, 0 <= Fraction <= 1.
    bool        StartSolid;     ///< Did the movement start inside the brush, "in solid"?
    Vector3T<T> ImpactNormal;   ///< On impact, this is the normal vector of the hit plane.

    /// Constructor.
    VB_Trace3T(T Fraction_)
        : Fraction(Fraction_),
          StartSolid(false)
    {
    }
};


/// Diese Klasse implementiert Brushes.
/// Ein Brush ist nichts weiter als eine Ansammlung (Array) von Planes,
/// deren Schnitt ein konvexes, dreidimensionales Polyhedron darstellt.
///
/// Eigenschaften des Brushs (Vereinbarungen):
/// 1. Die Normalenvektoren der Ebenen müssen nach AUßEN zeigen.
template<class T>
class Brush3T
{
    public:

    ArrayT< Plane3T<T> > Planes; ///< Array of planes this brush consists of.


    /// Default constructor.
    Brush3T() {}

    /// Generiert einen Brush aus einer BoundingBox 'BB', die vorher nach 'Pos' verschoben wird.
    Brush3T(const BoundingBox3T<T>& BB, const Vector3T<T>& Pos);

    /// Creates a triangular, zero-volume brush from the vertices A, B and C.
    /// @param A First vertice.
    /// @param B Second vertice.
    /// @param C Third vertice.
    /// @param Epsilon Tolerance value.
    /// @param IncludeBevelPlanes If false, only five planes (two for the sides and three for the edges) will be created.
    ///                           If true, also bevel planes will be included.
    Brush3T(const Vector3T<T>& A, const Vector3T<T>& B, const Vector3T<T>& C, const T Epsilon, bool IncludeBevelPlanes = true);

    /// Traces the (relative) bounding box 'BB' from the (absolute) 'Origin' along 'Dir' towards the end position 'Origin+VectorScale(Dir, Trace.Fraction)'.
    /// The result is returned in 'Trace'.
    /// This method handles the bloating and unbloating of the brush according to a description by Kekoa Proudfoot:
    /// Tracing the 'BB' against this brush is reduced to a simple ray collision test against the "bloated" brush.
    /// @param BB Bounding box to trace.
    /// @param Origin Origin of the trace.
    /// @param Dir Direction of the trace.
    /// @param Trace Trace result.
    void TraceBoundingBox(const BoundingBox3T<T>& BB, const Vector3T<T>& Origin, const Vector3T<T>& Dir, VB_Trace3T<T>& Trace) const;
};

#endif
