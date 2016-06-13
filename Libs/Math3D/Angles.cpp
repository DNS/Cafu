/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Angles.hpp"
#include "Matrix3x3.hpp"
#include "Quaternion.hpp"


using namespace cf::math;


// The static PI constant.
template<> /*static*/ const double AnglesT<float> ::PI=3.14159265358979323846;
template<> /*static*/ const double AnglesT<double>::PI=3.14159265358979323846;


template<class T> AnglesT<T>::AnglesT(const Matrix3x3T<T>& M)
{
    Init(M);
}


template<class T> AnglesT<T>::AnglesT(const QuaternionT<T>& Quat)
{
    // The simple code below does a few unnecessary computations when constructing
    // the matrix from Quat, because not all elements of the matrix are used in Init().
    // Apart from that, this approach seems to be the best we can do. Also see:
    //   - http://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
    //   - http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/
    Init(Matrix3x3T<T>(Quat));
}


// See FromAngles_Aircraft() and http://de.wikipedia.org/wiki/Roll-Nick-Gier-Winkel,
// section "Z, Y', X'' Konvention" for details.
template<class T> void AnglesT<T>::Init(const Matrix3x3T<T>& M)
{
    // Extract the basis vectors from the matrix.
    const Vector3T<T> forward(M[0][0], M[1][0], M[2][0]);   // First  column, x-axis.
    const Vector3T<T> left   (M[0][1], M[1][1], M[2][1]);   // Second column.
    const Vector3T<T> up     (M[0][2], M[1][2], M[2][2]);   // Third  column.

    const T xyDist = sqrt(forward[0]*forward[0] + forward[1]*forward[1]);

    if (xyDist > 0.001f)
    {
        yaw()   = RadToDeg(atan2( forward[1], forward[0]));
        pitch() = RadToDeg(atan2(-forward[2], xyDist));
        roll()  = RadToDeg(atan2( left[2], up[2]));
    }
    else
    {
        // The forward vector points mostly along the z-axis,
        // which means that we have a Gimbal Lock, http://de.wikipedia.org/wiki/Gimbal_Lock
        yaw()   = RadToDeg(atan2(-left[0], left[1]));
        pitch() = RadToDeg(atan2(-forward[2], xyDist));
        roll()  = 0;    // No roll, because one degree of freedom has been lost (that is, yaw==roll).
    }

    assert(M.IsEqual(Matrix3x3T<T>(*this), 0.001f));
}


template class AnglesT<float>;
template class AnglesT<double>;
