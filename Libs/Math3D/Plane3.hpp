/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/**************/
/*** Plane3 ***/
/**************/

#ifndef CAFU_MATH_PLANE_HPP_INCLUDED
#define CAFU_MATH_PLANE_HPP_INCLUDED

#include "Vector3.hpp"
#include "Templates/Array.hpp"


/// This class represents a plane in three-dimensional space.
/// Eigenschaften der Ebene (Vereinbarungen):
/// 1. Die Ebenengleichung lautet VectorDot(UnitNormal,x)=Dist
/// 2. Der Normalenvektor muß stets der EINHEITSnormalenvektor sein (Länge 1)
/// 3. ==> Einen Stützvektor erhalten wir aus VectorScale(UnitNormal,Dist)
/// 4. Der vordere Halbraum / die Oberseite der Ebene liegt in Richtung des Normalenvektors
///    (Siehe auch: Mathematikheft Nr. 5, 24.01.96, "Die Bedeutung des Vorzeichens", Fälle 2+4)
template<class T>
class Plane3T
{
    public:

    Vector3T<T> Normal; ///< Normal vector of this plane.
    T           Dist;   ///< Distance to origin (0, 0, 0).



    /// The default constructor.
    Plane3T() : Normal(0, 0, 0), Dist(0) { }

    /// An explicit constructor.
    Plane3T(const Vector3T<T>& Normal_, T Dist_) : Normal(Normal_), Dist(Dist_) { }

    /// This constructor creates the plane that contains the points ABC in clockwise order.
    /// The normal vector then points towards the observer (orientation!).
    /// @param A First point.
    /// @param B Second point.
    /// @param C Third point.
    /// @param Epsilon Tolerance value.
    /// @throws DivisionByZeroE if cross(C-A, B-A) yields a vector shorter than Epsilon.
    Plane3T(const Vector3T<T>& A, const Vector3T<T>& B, const Vector3T<T>& C, const T Epsilon)
    {
        // Gegenüber einer Herleitung einer solchen Ebene per Hand wurde das Vorzeichen von Dist umgedreht,
        // da die Ebenengleichung und der Stützvektor sonst weniger schön aussähen (das entspräche der
        // Ebenengleichung UnitNormal*x+Dist=0 anstatt UnitNormal*x-Dist=0).
        Normal=normalize(cross(C-A, B-A), Epsilon);
        Dist  =dot(A, Normal);
    }



    /// Returns true if the plane is valid, that is, if the normal vector is valid and (roughly) has length 1.
    bool IsValid() const
    {
        if (!Normal.IsValid()) return false;

        const T len2=Normal.GetLengthSqr();

        return len2>0.9*0.9 && len2<1.1*1.1;
    }

    /// Compute two orthogonal span vectors for this plane.
    void GetSpanVectors(Vector3T<T>& U, Vector3T<T>& V) const
    {
        // Rundungsfehler sind ein echtes Problem:
        // Zwei Planes mit SEHR ähnlichem Normalenvektor können trotzdem völlig verschiedene Spannvektoren bekommen.
        // Betrachte z.B. Planes mit Normalenvektor (sqrt(2)/2, sqrt(2)/2, 0).
        // Minimale Abweichungen können bereits entstehen, wenn der Compiler zur Optimierung floating-point
        // Zwischenergebnisse in den Registern der FPU stehen läßt (OpenWatcom mit -otexan switches).
        // Demgegenüber werden Zwischenergebnisse ohne Optimierung (unter Linux/g++ sogar immer) in eine 8-Byte
        // Speicherzelle geschrieben. Weil FPU Register mehr Bits (d.h. Präzision) haben, kommt es im letzteren
        // Fall zu einer Rundung, was wiederum dazu führen kann, daß in einem Fall die x-Komponente des obigen
        // Beispielvektors größer ist als die y-Komponente, und im anderen Fall kleiner.
        // Das Problem tritt also verschärft mit "45° Planes" auf. Es läßt sich nicht aus der Welt schaffen,
        // aber wir können es auf andere, ganz selten vorkommende Winkel "verschieben", indem wir nach den
        // ersten drei Nachkommastellen wie unten gezeigt selbst runden.
        int max=-99999;

        int x=int(fabs(Normal.x*1000.0)); if (x>max) { max=x; U=Vector3T<T>(0, 1, 0); }
        int y=int(fabs(Normal.y*1000.0)); if (y>max) { max=y; U=Vector3T<T>(1, 0, 0); }
        int z=int(fabs(Normal.z*1000.0)); if (z>max) { max=z; U=Vector3T<T>(1, 0, 0); }

        // Projeziere U auf die durch den Ursprung verschobene Plane, um sicherzustellen, daß U in ihr liegt.
        // VectorDot(Normal, U) entspricht PlaneDist(Plane3T(Normal, 0), U).
        U=normalize(U-scale(Normal, dot(Normal, U)), T(0.0));

        // Finde V als Orthogonale zu Normal und U. (VectorCross zweier Einheitsvektoren ist wieder ein Einheitsvektor.  (sicher?!?) )
        V=cross(Normal, U);
    }

    /// Compute two orthogonal span vectors for this plane, using a different method than GetSpanVectors().
    /// @param U First span vector.
    /// @param V Second span vector.
    /// TODO: Is this method more robust than GetSpanVectors()? See the implementation of GetSpanVectors() to understand the problem.
    ///   That is, does this method *not* produce very different span vectors for very similar planes in some rare cases?
    ///   If I understand atan2() correctly, it's two parameters refer to the Gegenkathete and Ankathete of an orthogonal triangle.
    ///   Thus, atan2() should be able to operate smoothly even if one of its arguments is zero or near-zero.
    ///   This seems to imply the smoothness of its results, and thus the robustness of this method, which solves the problem.
    ///   But what if they are both zero? Does atan2() then yield an error? Or 0? Is its behaviour then consistent across platforms?
    void GetSpanVectorsByRotation(Vector3T<T>& U, Vector3T<T>& V) const
    {
        // The key idea here is as follows:
        // Given the X-vector (1 0 0), figure out by which angle it must be rotated around the Y axis and
        // then by which angle the result must be rotated around the Z axis in order to obtain the Normal vector.
        // It's actually a simple idea, see my Tech-Archive sheet from 2005-10-20, on which I sketched a figure and derived the stuff below.

        // Determine the angles of rotation around the Y and Z axes that rotate (1 0 0) to the normal,
        // that is, Vector3T<T>(1, 0, 0).GetRotY(RadToDeg(RotY)).GetRotZ(RadToDeg(RotZ)) == Normal.
        // See http://en.wikipedia.org/wiki/Atan2 for an excellent article about the atan2() function.
        const T RotY=-atan2(Normal.z, sqrt(Normal.y*Normal.y+Normal.x*Normal.x));
        const T RotZ= atan2(Normal.y, Normal.x);

        // U=Vector3T<T>(0, 1, 0).GetRotY(RadToDeg(RotY)).GetRotZ(RadToDeg(RotZ));
        U.x=-sin(RotZ);
        U.y= cos(RotZ);
        U.z=0;

        // V=Vector3T<T>(0, 0, -1).GetRotY(RadToDeg(RotY)).GetRotZ(RadToDeg(RotZ));
        // We use the negative z-axis here, i.e. the V texture-coord axis is along -Z.
        // WARNING: Don't change this! It must *always* be the *same* in all programs (CaWE, CaBSP, Cafu, ...).
        // Removing the "-" signs below would also break the D3 map importer in CaWE.
        V.x=-sin(RotY)*cos(RotZ);
        V.y=-sin(RotY)*sin(RotZ);
        V.z=-cos(RotY);
    }

    /// Returns the same, but mirrored plane, i.e. the plane with the reversed orientation.
    Plane3T GetMirror() const
    {
        return Plane3T(-Normal, -Dist);
    }

    /// Determines the distance from A to this plane.
    T GetDistance(const Vector3T<T>& A) const
    {
        // Put A into the Hessesche Normalenform of this plane.
        return dot(Normal, A)-Dist;
    }

    /// Intersect the line through A and B with this plane.
    /// @param  A First point of the line.
    /// @param  B Second point of the line.
    /// @param  cosEpsilon    If the angle between the planes normal vector and AB is too close to 90 degrees,
    ///         AB roughly parallels the plane. The cosine of this angle is then close to 0. This value defines
    ///         the minimally allowed cosine for which AB is considered not paralleling the plane.
    /// @throws DivisionByZeroE if AB parallels the plane too much.
    Vector3T<T> GetIntersection(const Vector3T<T>& A, const Vector3T<T>& B, const T cosEpsilon) const
    {
        // Die Geradengleichung x=a+r*(b-a) einfach in die Hessesche Normalenform der Ebene einsetzen und nach r auflösen.
        // Das erhaltene r wird nun einfach wiederum in die Geradengleichung eingesetzt --> {S}.
        const Vector3T<T> AB=B-A;
        const T     cosAngle=dot(Normal, AB);

        if (fabs(cosAngle)<=cosEpsilon) throw DivisionByZeroE();

        return A + AB*(Dist-dot(Normal, A))/cosAngle;
    }

    /// Returns whether this plane and plane P are truly (bit-wise) identical.
    /// Use this operator with care, as it comes *without* any epsilon threshold for taking rounding errors into account.
    /// @param P   Plane to compare to.
    bool operator == (const Plane3T& P) const
    {
        return Normal==P.Normal && Dist==P.Dist;
    }

    /// Returns whether this plane and plane P are not equal (bit-wise).
    /// Use this operator with care, as it comes *without* any epsilon threshold for taking rounding errors into account.
    /// @param P   Plane to compare to.
    bool operator != (const Plane3T& P) const
    {
        return Normal!=P.Normal || Dist!=P.Dist;
    }


    /// Builds the intersection of three planes.
    /// @param P1 First plane.
    /// @param P2 Second plane.
    /// @param P3 Third plane.
    /// @throws DivisionByZeroE if the intersection of the three planes has not exactly one solution.
    Vector3T<T> static GetIntersection(const Plane3T& P1, const Plane3T& P2, const Plane3T& P3)
    {
        // Ein Gaußsches Gleichungssystem erhält man aus den Koordinatenformen der Ebenen. Hier wird der Nenner genau dann Null,
        // wenn es keine, unendlich viele bzw. NICHT genau EINE Lösung gibt. Keine Lösung gibt es, wenn mindestens zwei Ebenen
        // parallel sind und sie nicht ineinanderliegen. Unendlich viele Lösungen gibt es, wenn mindestens zwei Ebenen parallel
        // sind und sie ineinanderliegen. Lösung nach dem Determinantenverfahren bzw. der Cramerschen Regel.
        // Siehe Mathematikbuch Analytische Geometrie, Seiten 157 bis 163.
        const Vector3T<T> A(P1.Normal.x, P2.Normal.x, P3.Normal.x);
        const Vector3T<T> B(P1.Normal.y, P2.Normal.y, P3.Normal.y);
        const Vector3T<T> C(P1.Normal.z, P2.Normal.z, P3.Normal.z);
        const Vector3T<T> D(P1.Dist,     P2.Dist,     P3.Dist    );

        const T Nenner=dot(cross(A, B), C);

        if (Nenner==0) throw DivisionByZeroE();

        return Vector3T<T>(dot(cross(D, B), C)/Nenner,
                           dot(cross(A, D), C)/Nenner,
                           dot(cross(A, B), D)/Nenner);
    }


    /// Computes the convex hull of a set of points.
    /// @param Points          The set of points for which the convex hull is computed.
    /// @param HullPlanes      The array in which the convex hull planes are returned.
    /// @param HullPlanesPIs   If non-NULL, in this array are triples of indices into Points returned, where the i-th triple indicates from which points the i-th hull plane was built.
    /// @param Epsilon         In order to fight rounding errors, the "thickness" of a hull plane is assumed to be 2*Epsilon.
    static void ConvexHull(const ArrayT< Vector3T<T> >& Points, ArrayT<Plane3T>& HullPlanes, ArrayT<unsigned long>* HullPlanesPIs, const T Epsilon);
};


template<class T> inline std::string convertToString(const Plane3T<T>& P)
{
    const int sigdigits=std::numeric_limits<T>::digits10;

    std::ostringstream out;

    out << std::setprecision(sigdigits) << convertToString(P.Normal) << ", " << P.Dist;

    return out.str();
}


template<class T> inline std::ostream& operator << (std::ostream& os, const Plane3T<T>& P)
{
    return os << P.Normal << ", " << P.Dist;
}


/// Typedef for a Vector3T of floats.
typedef Plane3T<float> Plane3fT;

/// Typedef for a Vector3T of doubles.
typedef Plane3T<double> Plane3dT;

#endif
