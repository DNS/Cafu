/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MATH_QUATERNION_HPP_INCLUDED
#define CAFU_MATH_QUATERNION_HPP_INCLUDED

#include "Vector3.hpp"
#include <algorithm>


namespace cf
{
    namespace math
    {
        template<class T> class AnglesT;
        template<class T> class Matrix3x3T;


        /// This class represents a quaternion.
        template<class T>
        class QuaternionT
        {
            public:

            T x;
            T y;
            T z;
            T w;


            /// The default constructor. It initializes the quaternion from the given x_, y_, z_ and w_ respectively.
            QuaternionT(T x_=0, T y_=0, T z_=0, T w_=1)
                : x(x_), y(y_), z(z_), w(w_) { }

            /// Constructs a quaternion from an array of (at least) four values.
            template<class C> explicit QuaternionT(const C Values[])
                : x(T(Values[0])),
                  y(T(Values[1])),
                  z(T(Values[2])),
                  w(T(Values[3])) { }

            /// Constructs a quaternion from the given angles.
            /// See the documentation of the AnglesT class for details.
            QuaternionT(const AnglesT<T>& Angles);

            /// Constructs a quaternion from the given rotation matrix.
            /// If the matrix is not orthonormal, the result is undefined.
            QuaternionT(const Matrix3x3T<T>& Mat);

            /// Constructs a quaternion from a rotation axis and angle.
            /// This is useful, for example, to rotate one vector onto another, as is done in the
            /// implementation of cf::GameSys::ComponentTransformT::LookAt().
            /// @param Axis    The axis to rotate about. This given axis must be of unit length, or else the result is undefined.
            /// @param Angle   The angle to rotate about, in radians.
            QuaternionT(const Vector3T<T>& Axis, const T Angle);

            /// Constructs a quaternion from the first three components (x, y, z) of a unit quaternion.
            static QuaternionT FromXYZ(const Vector3T<T>& Vec)
            {
                const T ww = T(1.0) - Vec.x*Vec.x - Vec.y*Vec.y - Vec.z*Vec.z;

                // Note that convention is to use -sqrt(ww), not +sqrt(ww).
                // This must be taken into account in GetXYZ().
                return QuaternionT(Vec.x, Vec.y, Vec.z, (ww<0) ? 0 : -sqrt(ww));
            }

            /// Constructs a quaternion from three Euler angles.
            /// @param Pitch   the Euler rotation about the x-axis, in radians.
            /// @param Yaw     the Euler rotation about the y-axis, in radians.
            /// @param Roll    the Euler rotation about the z-axis, in radians.
            /// Note that the assignment of angles to axes assumes a right-handed coordinate system where the z-axis points towards the viewer.
            /// This is especially different from the coordinate system that class cf::math::AnglesT<T> uses, which is also right-handed, but
            /// rotated by 90 degrees so that the z-axis points up and the y-axis away from the viewer!
            static QuaternionT Euler(const T Pitch, const T Yaw, const T Roll);


            /// Returns the x, y and z components as a Vector3T<T>.
            Vector3T<T> GetXYZ() const
            {
                // Properly take the sign of w into account for consistence with the FromXYZ constructor.
                // Note that (-x, -y, -z, -w) is a different quaternion than (x, y, z, w), but it describes the *same* rotation.
                return w<0 ? Vector3T<T>(x, y, z) : Vector3T<T>(-x, -y, -z);
            }

            /// Returns the conjugate of this quaternion.
            /// If the quaternion is of unit length, then the conjugate is also its inverse.
            QuaternionT<T> GetConjugate() const { return QuaternionT<T>(-x, -y, -z, w); }

            /// Returns the dot product of this quaternion and B.
            T dot(const QuaternionT<T>& B) const { return x*B.x + y*B.y + z*B.z + w*B.w; }

            /// Returns the length of this quaternion.
            T length() const { return sqrt(lengthSqr()); }

            /// Returns the square of the length of this quaternion.
            T lengthSqr() const { return x*x + y*y + z*z + w*w; }

            /// Returns if this quaternion is normal, i.e. has length 1 within the given tolerance.
            /// @param Epsilon The tolerance value.
            bool IsNormal(const T Epsilon=0) const
            {
                return fabs(length()-T(1.0)) <= Epsilon;
            }


            /// Component access by index number (0 to 3) rather than by name.
            /// @param Index Index of the component to access. Can only be 0, 1, 2 or 3 (for x, y, z, w).
            /// @throws InvalidOperationE if Index is not 0, 1, 2 or 3.
            T& operator [] (unsigned int Index)
            {
                switch (Index)
                {
                    case 0: return x;
                    case 1: return y;
                    case 2: return z;
                    case 3: return w;
                    default: throw InvalidOperationE();
                }
            }

            /// Component access by index number (0 to 3) rather than by name.
            /// @param Index Index of the component to access. Can only be 0, 1, 2 or 3 (for x, y, z, w).
            /// @throws InvalidOperationE if Index is not 0, 1, 2 or 3.
            const T& operator [] (unsigned int Index) const
            {
                switch (Index)
                {
                    case 0: return x;
                    case 1: return y;
                    case 2: return z;
                    case 3: return w;
                    default: throw InvalidOperationE();
                }
            }

            /// Returns whether this quaternion and B are (bit-wise) identical.
            /// Use this operator with care, as it comes *without* any epsilon threshold to take rounding errors into account.
            /// @param B Quaternion to compare to.
            bool operator == (const QuaternionT<T>& B) const
            {
                return x==B.x && y==B.y && z==B.z && w==B.w;
            }

            /// Returns whether this quaternion and B are not equal (bit-wise).
            /// Use this operator with care, as it comes *without* any epsilon threshold to take rounding errors into account.
            /// @param B Quaternion to compare to.
            bool operator != (const QuaternionT<T>& B) const
            {
                return x!=B.x || y!=B.y || z!=B.z || w!=B.w;
            }

            /// The unary minus operator.
            QuaternionT<T> operator - () const
            {
                return QuaternionT<T>(-x, -y, -z, -w);
            }

            /// Returns the sum of this quaternion and B.
            QuaternionT<T> operator + (const QuaternionT<T>& B) const
            {
                return QuaternionT<T>(x+B.x, y+B.y, z+B.z, w+B.w);
            }

            /// Adds B to this quaternion.
            QuaternionT<T>& operator += (const QuaternionT<T>& B)
            {
                x+=B.x; y+=B.y; z+=B.z; w+=B.w;
                return *this;
            }

            /// Returns the difference between this quaternion and B.
            QuaternionT<T> operator - (const QuaternionT<T>& B) const
            {
                return QuaternionT<T>(x-B.x, y-B.y, z-B.z, w-B.w);
            }

            /// Subtracts B from this quaternion.
            QuaternionT<T>& operator -= (const QuaternionT<T>& B)
            {
                x-=B.x; y-=B.y; z-=B.z; w-=B.w;
                return *this;
            }

            /// Returns the quaternion Q that expresses the combined rotation of this quaternion and \c B, <tt>Q = this*B</tt>.
            QuaternionT<T> operator * (const QuaternionT<T>& B) const
            {
                return QuaternionT<T>(
                    w*B.x + x*B.w + y*B.z - z*B.y,
                    w*B.y + y*B.w + z*B.x - x*B.z,
                    w*B.z + z*B.w + x*B.y - y*B.x,
                    w*B.w - x*B.x - y*B.y - z*B.z);
            }

            /// Returns a copy of this quaternion scaled by s.
            QuaternionT<T> operator * (const T s) const
            {
                return QuaternionT<T>(x*s, y*s, z*s, w*s);
            }

            /// Scales this quaternion by s.
            QuaternionT<T>& operator *= (const T s)
            {
                x*=s; y*=s; z*=s; w*=s;
                return *this;
            }

            /// Returns a copy of this quaternion divided by s, assuming that s is not 0.
            QuaternionT<T> operator / (const T s) const
            {
                return QuaternionT<T>(x/s, y/s, z/s, w/s);  // Intentionally don't multiply with the precomputed reciprocal.
            }

            /// Divides this quaternion by s, assuming that s is not 0.
            QuaternionT<T>& operator /= (const T s)
            {
                x/=s; y/=s; z/=s; w/=s;   // Intentionally don't multiply with the precomputed reciprocal.
                return *this;
            }
        };


        typedef QuaternionT<float>  QuaternionfT;   ///< Typedef for a QuaternionT of floats.
        typedef QuaternionT<double> QuaterniondT;   ///< Typedef for a QuaternionT of doubles.
    }
}


/// Returns the dot product of A and B.
template<class T> inline T dot(const cf::math::QuaternionT<T>& A, const cf::math::QuaternionT<T>& B)
{
    return A.dot(B);
}


/// Returns the length of A.
template<class T> inline T length(const cf::math::QuaternionT<T>& A)
{
    return A.length();
}


/// Returns the normalized (unit length) version of A.
/// @throws DivisionByZeroE if length(A)<=Epsilon.
template<class T> inline cf::math::QuaternionT<T> normalize(const cf::math::QuaternionT<T>& A, const T Epsilon)
{
    const T len=length(A);

    // Use <= (instead of <) for consistence with Vector3T<>::normalize() and normalizeOr0().
    if (len<=Epsilon) throw DivisionByZeroE();

    return A/len;
}


/// Returns the normalized (unit length) version of A if length(A)>Epsilon, or the identity quaternion otherwise.
template<class T> inline cf::math::QuaternionT<T> normalizeOr0(const cf::math::QuaternionT<T>& A, const T Epsilon=0)
{
    const T len=length(A);

    return (len>Epsilon) ? A/len : cf::math::QuaternionT<T>();
}


/// This methods implements spherical linear interpolation:
/// It returns the interpolated quaternion between input quaternions P and Q, according to scalar t (at uniform angular velocity).
template<class T> inline cf::math::QuaternionT<T> slerp(
    const cf::math::QuaternionT<T>& P, cf::math::QuaternionT<T> Q, const T t, const bool ShortPath=true)
{
    T CosOmega=dot(P, Q);

    if (CosOmega<0 && ShortPath)
    {
        CosOmega=-CosOmega;
        Q=-Q;
    }

    CosOmega=std::min(CosOmega, T( 1.0));
    CosOmega=std::max(CosOmega, T(-1.0));

    // Note that the angle of the rotation that is described is actually 2*Omega.
    const T Omega   =acos(CosOmega);
    const T SinOmega=sin (Omega);

    if (SinOmega > T(0.01))
    {
        // Omega is at least 0,57° larger than 0° or less than 180°,
        // thus the division below is numerically stable: implement normal slerp.
        const T tp=sin((T(1.0)-t)*Omega)/SinOmega;
        const T tq=sin(        t *Omega)/SinOmega;

        return P*tp + Q*tq;
    }

    if (CosOmega>0)
    {
        // Omega is close to 0°.
        // For numerical stability, implement normalized linear interpolation.
        return normalizeOr0(P*(T(1.0)-t) + Q*t);
    }

    // Omega is close to 180°, describing a rotation of about 360°.
    // P and Q are thus on opposite (180°) points on the unit sphere, e.g. the north and south pole.
    // The problem is solved by finding an arbitrary point E on the "equator". The interpolation is
    // then done "through" that point, so that t==0 maps to P, t==0.5 to E, and t==1 to "2PE", which is Q.
    assert(!ShortPath);
    const T PI=T(3.14159265358979323846);

    return P*sin((T(0.5)-t)*PI) + cf::math::QuaternionT<T>(-P.y, P.x, -P.w, P.z)*sin(t*PI);
}


template<class T> inline std::string convertToString(const cf::math::QuaternionT<T>& A)
{
    // From MSDN documentation: "digits10 returns the number of decimal digits that the type can represent without loss of precision."
    // For floats, that's usually 6, for doubles, that's usually 15. However, we want to use the number of *significant* decimal digits here,
    // see http://www.open-std.org/JTC1/sc22/wg21/docs/papers/2006/n2005.pdf for details.
    const int sigdigits=std::numeric_limits<T>::digits10 + 3;

    std::ostringstream out;

    out << std::setprecision(sigdigits) << "(" << A.x << ", " << A.y << ", " << A.z << ", " << A.w << ")";

    return out.str();
}


template<class T> inline std::ostream& operator << (std::ostream& os, const cf::math::QuaternionT<T>& A)
{
    return os << "(" << A.x << ", " << A.y << ", " << A.z << ", " << A.w << ")";
}

#endif
