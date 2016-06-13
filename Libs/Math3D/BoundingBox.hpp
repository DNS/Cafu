/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/********************/
/*** Bounding Box ***/
/********************/

#ifndef CAFU_MATH_BOUNDING_BOX_HPP_INCLUDED
#define CAFU_MATH_BOUNDING_BOX_HPP_INCLUDED

#include "Vector3.hpp"
#include "Plane3.hpp"
#include "Templates/Array.hpp"

#include <cassert>


/// This class represents an axis-aligned bounding-box ("AABB") in 3-dimensional space.
template<class T>
class BoundingBox3T
{
    public:

    /// Information about on which side of a plane the bounding box is.
    enum SideT
    {
        Back =-1,   ///< The bounding box is on the back-side of the plane.
        Both = 0,   ///< The bounding box is on both sides of the plane (i.e. it is intersected by the plane).
        Front= 1,   ///< The bounding box is on the front-side of the plane.
    };


    /// The default constructor creates a bounding-box that is "uninitialized", i.e. whose dimensions are not specified.
    /// Inserting the first vertex into the bounding-box will automatically turn it into a regular, initialized instance.
    BoundingBox3T();

    /// Constructs a zero-volume bounding box at A.
    /// @param A   The spatial location to initialize the bounding-box at.
    explicit BoundingBox3T(const Vector3T<T>& A);

    /// Constructs a bounding box as defined by A and B.
    /// @param A   The first  corner point that defines the dimensions of the bounding-box.
    /// @param B   The second corner point that defines the dimensions of the bounding-box.
    BoundingBox3T(const Vector3T<T>& A, const Vector3T<T>& B);

    /// Constructs a bounding box from the vertices of the array A.
    /// The bounding-box will be "uninitialized" if A is empty.
    /// @param A   An array of vertices whose elements define the dimensions of the bounding-box.
    explicit BoundingBox3T(const ArrayT< Vector3T<T> >& A);


    /// Casts this BoundingBox3T<T> to a BoundingBox3T<float>, so that the cast is explicitly and easy to see and find in user code.
    BoundingBox3T<float> AsBoxOfFloat() const
    {
        BoundingBox3T<float> BB;

        if (IsInited())
        {
            BB.Min = Min.AsVectorOfFloat();
            BB.Max = Max.AsVectorOfFloat();
        }

        return BB;
    }

    /// Casts this BoundingBox3T<T> to a BoundingBox3T<double>, so that the cast is explicitly and easy to see and find in user code.
    BoundingBox3T<double> AsBoxOfDouble() const
    {
        BoundingBox3T<double> BB;

        if (IsInited())
        {
            BB.Min = Min.AsVectorOfDouble();
            BB.Max = Max.AsVectorOfDouble();
        }

        return BB;
    }


    /// Determines whether a bounding-box is valid.
    /// A bounding-box is valid if it is in "uninitialized" or properly initialized state.
    /// (A bounding-box is invalid if it has negative volume.)
    /// This method is needed mostly for debugging, as a bounding-box should *always* be valid.
    bool IsValid() const;

    /// Returns whether this bounding-box has been initialized with at least one point in space.
    /// A bounding-box that has not yet been initialized can be initalized by inserting the first vertex.
    bool IsInited() const;


    /// Inserts A into this boundig-box, growing it appropriately.
    /// @param A   The point to be inserted into the bounding-box.
    void Insert(const Vector3T<T>& A);

    /// An equivalent to Insert(A), but more readable.
    void operator += (const Vector3T<T>& A) { Insert(A); }

    /// Inserts all vertices of A into this boundig-box, growing it appropriately.
    /// @param A   The list of points to be inserted into the bounding-box.
    void Insert(const ArrayT< Vector3T<T> >& A);

    /// An equivalent to Insert(A), but more readable.
    void operator += (const ArrayT< Vector3T<T> >& A) { Insert(A); }

    /// Inserts the given bounding-box into this one, growing it appropriately.
    /// This is also called "merging" or "combining" BB with this bounding box.
    /// @param BB   The bounding-box to be combined/merged with this bounding-box. BB.IsInited() must be true.
    void Insert(const BoundingBox3T<T>& BB);

    /// An equivalent to Insert(BB), but more readable.
    void operator += (const BoundingBox3T<T>& BB) { Insert(BB); }

    /// Like Insert(BB), but with this version it suffices when BB is only valid, i.e. it can be initialized or uninitialized.
    /// If BB is uninitialized, this bounding-box is not changed. Otherwise, BB is inserted normally: this->Insert(BB).
    /// @param BB   The bounding-box to be combined/merged with this bounding-box. BB.IsValid() must be true.
    void InsertValid(const BoundingBox3T<T>& BB);


    /// This method returns a copy of this bounding box that is slightly enlarged by Epsilon (or shrunk if Epsilon is negative).
    /// The returned box is very useful with the containment / intersection / test methods when rounding errors are an issue!
    /// Note that it is easy to control the desired effect by passing either a positive number to make the box slightly larger,
    /// or by passing a negative number to make the box slightly smaller.
    /// For example, if BB is a bounding box, BB.GetEpsilonBox(0.1).Contains(A) returns true even if A is actually a bit outside of BB,
    /// or BB.GetEpsilonBox(-0.3).Intersects(OtherBB) yields false even if BB and OtherBB are neighboured and share a plane.
    /// @param Epsilon   The amount by which the bounding-box is expanded.
    BoundingBox3T<T> GetEpsilonBox(const T Epsilon) const
    {
        assert(IsInited());

        const Vector3T<T> Eps=Vector3T<T>(Epsilon, Epsilon, Epsilon);
        BoundingBox3T<T>  BB(*this);

        // Don't use the (Min-Eps, Max+Eps) constructor here, as it involved an additional call to Insert().
        BB.Min-=Eps;
        BB.Max+=Eps;

        // Maybe the box got smaller, now make sure it didn't get negative.
        if (BB.Min.x>BB.Max.x) BB.Min.x=BB.Max.x=(BB.Min.x+BB.Max.x)*0.5f;
        if (BB.Min.y>BB.Max.y) BB.Min.y=BB.Max.y=(BB.Min.y+BB.Max.y)*0.5f;
        if (BB.Min.z>BB.Max.z) BB.Min.z=BB.Max.z=(BB.Min.z+BB.Max.z)*0.5f;

        return BB;
    }

    /// Returns the overall bounding box that is defined by translating (moving) this bounding box in a linear fashion from point Start to End.
    /// @param Start   The start point for the thought linear movement of this bounding box.
    /// @param End     The end   point for the thought linear movement of this bounding box.
    /// @returns the overall bounding box that contains the entire movement.
    BoundingBox3T<T> GetOverallTranslationBox(const Vector3T<T>& Start, const Vector3T<T>& End) const;

    /// Determines whether this bounding box contains A.
    /// @param A   The point for which containment should be determined.
    bool Contains(const Vector3T<T>& A) const;

    /// Determines whether this bounding box and BB intersect.
    /// @param BB   The other bounding box to test with.
    bool Intersects(const BoundingBox3T<T>& BB) const;

    /// Determines whether this bounding box and BB intersect or touch each other.
    /// This method is only used in the ClipSys, and the Intersects() method should always be preferred.
    /// @param BB   The other bounding box to test with.
    bool IntersectsOrTouches(const BoundingBox3T<T>& BB) const;

    /// Determines on what side of plane P the this BB is.
    /// @param P       The plane to test with.
    /// @param Epsilon Maximum error value.
    /// @returns SideT::Front if this box is completely on the front of P,
    ///          SideT::Back  if this box is completely on the back of P,
    ///          SideT::Both  if this box is on both sides of P.
    SideT WhatSide(const Plane3T<T>& P, const T Epsilon=0) const;
    SideT WhatSide_OLD(const Plane3T<T>& P, const T Epsilon=0) const;

    /// Returns the center point of the BB.
    Vector3T<T> GetCenter() const { assert(IsInited()); return (Min+Max)/2; }

    /// Determines the distance from the plane P to the nearest point of the BB.
    /// @param P   The plane to test with.
    /// @returns the distance from the plane P to the nearest point of the BB, 0 whenever the BB intersects P.
    T GetDistance(const Plane3T<T>& P) const;

    /// Traces a ray against this bounding-box, and returns whether it was hit.
    /// The ray for the trace is defined by RayOrigin + RayDir*Fraction, where Fraction is a scalar >= 0.
    /// If a hit was detected, the Fraction is returned. The method only accounts for "incoming" hits,
    /// that is, where the ray comes from "outside" of the boundig-box and enters its inside.
    /// Hits where the ray leaves the inner of the bounding-box to the outside are not reported.
    ///
    /// @param RayOrigin   The point where the ray starts.
    /// @param RayDir      A vector of non-zero length that describes the direction the ray extends to (not required to be a unit vector).
    /// @param Fraction    On hit, the scalar along RayDir at which the hit occurred is returned here.
    ///
    /// @returns true if the ray hit this bounding-box, false otherwise. On hit, the Fraction is returned via reference paramaters.
    bool TraceRay(const Vector3T<T>& RayOrigin, const Vector3T<T>& RayDir, T& Fraction) const;

    /// Explicitly returns the eight corner vertices of this bounding box.
    /// @param Vertices   An array of eight vertices in which the eight corner vertices are returned.
    void GetCornerVertices(Vector3T<T>* Vertices) const;

    /// Splits the quad that is defined by this bounding box and constructs new bounding boxes from the results.
    /// @param SplitPlane       The plane along which this bounding box is split.
    /// @param PlaneThickness   The "thickness" that is attributed to the SplitPlane to account for rounding-error.
    /// @returns an array of two bounding-boxes that correspond to the front and back split results.
    ArrayT< BoundingBox3T<T> > GetSplits(const Plane3T<T>& SplitPlane, const T PlaneThickness) const;


    Vector3T<T> Min;    ///< The minimum-coordinate corner of the bounding-box.
    Vector3T<T> Max;    ///< The maximum-coordinate corner of the bounding-box.
};


/// Typedef for a BoundingBox3T of floats.
typedef BoundingBox3T<float> BoundingBox3fT;

/// Typedef for a BoundingBox3T of doubles.
typedef BoundingBox3T<double> BoundingBox3dT;

#endif
