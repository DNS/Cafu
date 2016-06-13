/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MATH_VECTOR3_HPP_INCLUDED
#define CAFU_MATH_VECTOR3_HPP_INCLUDED

#include "Errors.hpp"

#include <cassert>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>


/// This class represents a polymorphic 3-dimensional vector.
///
/// In order to clearly distinguish between methods that modify their "this" Vector3T<T> and those that don't,
/// the general idea is that all const, non-modifying methods start with "Get", e.g. GetLength().
/// However the dot() and cross() methods are exceptions for increased readability of user code.
/// Semantically and intuitively, we prefer to treat them as special binary infix operators as we do in
/// hand-written math -- its just the C++ language that doesn't provide us with appropriate symbols.
///
/// For operators, it is intuitively clear whether they modify the this object or not.
/// E.g. the += operator modifies the this object, the + operator does not.
/// Note that there is no need to define "constructive", binary infix operators (like the + operator)
/// outside (that is, not as members) of this class, because promotion of the left-hand argument as described
/// in the C++ FAQs does not occur in and thus is not relevant for this class (we have no constructor to
/// promote a built-in type to a Vector3T<T>).
/// See http://www.parashift.com/c++-faq-lite/operator-overloading.html#faq-13.9 items 5 to 7 (especially 7)
/// and http://www.parashift.com/c++-faq-lite/friends.html#faq-14.5 for details about this.
template<class T>
class Vector3T
{
    public:

    T x;    ///< The x-component of this vector.
    T y;    ///< The y-component of this vector.
    T z;    ///< The z-component of this vector.



    /// The default constructor. It initializes all components to zero.
    Vector3T() : x(0), y(0), z(0) { }

    /// This constructor initializes the components from x_, y_ and z_ respectively.
    Vector3T(T x_, T y_, T z_) : x(x_), y(y_), z(z_) { }

    /// This constructor initializes the components from an array of (at least) three Ts.
    template<class C> explicit Vector3T(const C Values[]) : x(T(Values[0])), y(T(Values[1])), z(T(Values[2])) { }



    /// Returns true if the vector is valid, that is, all components are non-NANs.
    bool IsValid() const
    {
        return true;
    }

    /// Component access by index number (0 to 2) rather than by name.
    /// @param Index Index of the component to access. Can only be 0, 1 or 2 (for x, y, z).
    /// @throws InvalidOperationE if Index is not 0, 1 or 2.
    T& operator [] (unsigned int Index)
    {
        switch (Index)
        {
            case 0: return x;
            case 1: return y;
            case 2: return z;
            default: throw InvalidOperationE();
        }
    }

    /// Component access by index number (0 to 2) rather than by name.
    /// @param Index Index of the component to access. Can only be 0, 1 or 2 (for x, y, z).
    /// @throws InvalidOperationE if Index is not 0, 1 or 2.
    const T& operator [] (unsigned int Index) const
    {
        switch (Index)
        {
            case 0: return x;
            case 1: return y;
            case 2: return z;
            default: throw InvalidOperationE();
        }
    }



    /// Gets this Vector3T<T> as a Vector3T<float>, so that the cast is explicitly visible in user code.
    Vector3T<float> AsVectorOfFloat() const
    {
#ifdef _WIN32
        assert(typeid(T)!=typeid(float));
#endif

        return Vector3T<float>(float(x), float(y), float(z));
    }

    /// Gets this Vector3T<T> as a Vector3T<double>, so that the cast is explicitly visible in user code.
    Vector3T<double> AsVectorOfDouble() const
    {
#ifdef _WIN32
        assert(typeid(T)!=typeid(double));
#endif

        return Vector3T<double>(double(x), double(y), double(z));
    }

    /// Gets this Vector3T<T> as a Vector3T<int>, so that the cast is explicitly visible in user code.
    Vector3T<int> AsVectorOfInt() const
    {
        return Vector3T<int>(int(x), int(y), int(z));
    }



    /// @name Group of const inspector methods. They are all const and thus do not modify this object.
    //@{

    // I've commented out this method because class methods of a template cannot be indivially specialized.
    // That it, the GetLength() method cannot be individually be specialized for floats.
    // Use the global length() function (defined below) instead!
    // /// Returns the length of this vector.
    // T GetLength() const { return sqrt(GetLengthSqr()); }

    /// Returns the square of the length of this vector.
    T GetLengthSqr() const { return x*x + y*y + z*z; }

    /// Returns whether this vector is equal to B within tolerance Epsilon, that is, whether it is geometrically closer to B than Epsilon.
    /// @param B Vector to compare to.
    /// @param Epsilon Tolerance value.
    /// @see operator ==
    bool IsEqual(const Vector3T<T>& B, const T Epsilon) const
    {
        return (*this-B).GetLengthSqr() <= Epsilon*Epsilon;
    }

    /// Returns a copy of this vector scaled by s, that is, the scalar product (Skalarmultiplikation) of this vector and s.
    /// @param s Scale factor to scale this vector by.
    /// @see  Also see the operator *, which does exactly the same.
    Vector3T<T> GetScaled(const T s) const
    {
        return Vector3T<T>(x*s, y*s, z*s);
    }

    /// Returns a copy of this vector non-uniformely scaled by S.
    Vector3T<T> GetScaled(const Vector3T<T>& S) const
    {
        return Vector3T<T>(x*S.x, y*S.y, z*S.z);
    }

    /// Returns a copy of this vector rotated around the x-axis by Angle degrees.
    Vector3T<T> GetRotX(const T Angle) const
    {
        const T RadAngle=Angle*T(3.14159265358979323846/180.0);
        const T SinAngle=sin(RadAngle);
        const T CosAngle=cos(RadAngle);

        return Vector3T<T>(x,
                           CosAngle*y-SinAngle*z,
                           SinAngle*y+CosAngle*z);
    }

    /// Returns a copy of this vector rotated around the y-axis by Angle degrees.
    Vector3T<T> GetRotY(const T Angle) const
    {
        const T RadAngle=Angle*T(3.14159265358979323846/180.0);
        const T SinAngle=sin(RadAngle);
        const T CosAngle=cos(RadAngle);

        return Vector3T<T>(CosAngle*x+SinAngle*z,
                           y,
                          -SinAngle*x+CosAngle*z);
    }

    /// Returns a copy of this vector rotated around the z-axis by Angle degrees.
    Vector3T<T> GetRotZ(const T Angle) const
    {
        const T RadAngle=Angle*T(3.14159265358979323846/180.0);
        const T SinAngle=sin(RadAngle);
        const T CosAngle=cos(RadAngle);

        return Vector3T<T>(CosAngle*x-SinAngle*y,
                           SinAngle*x+CosAngle*y,
                           z);
    }

    /// Returns two vectors that are orthogonal to this vector and to each other.
    void CreateOrthoVectors(Vector3T<T>& Left, Vector3T<T>& Down) const
    {
        const T DistSqr=x*x+y*y;

        if (DistSqr==0)
        {
            Left.x=1;
            Left.y=0;
            Left.z=0;
        }
        else
        {
            const T Dist=sqrt(DistSqr);

            Left.x=-y/Dist;
            Left.y=x/Dist;
            Left.z=0;
        }

        Down=Left.cross(*this);
    }

    //@}



    /// @name Group of (constructive) binary operators that do not modify their operands.
    //@{

    /// Returns whether this vector and B are truly (bit-wise) identical.
    /// Use this operator with care, as it comes *without* any epsilon threshold for taking rounding errors into account.
    /// @param B Vector to compare to.
    /// @see IsEqual()
    bool operator == (const Vector3T<T>& B) const
    {
        return x==B.x &&
               y==B.y &&
               z==B.z;
    }

    /// Returns whether this vector and B are not equal (bit-wise).
    /// Use this operator with care, as it comes *without* any epsilon threshold for taking rounding errors into account.
    /// @param B Vector to compare to.
    /// @see IsEqual()
    bool operator != (const Vector3T<T>& B) const
    {
        return x!=B.x ||
               y!=B.y ||
               z!=B.z;
    }

    /// Returns the sum of this Vector3T<T> and B.
    Vector3T<T> operator + (const Vector3T<T>& B) const
    {
        return Vector3T<T>(x+B.x, y+B.y, z+B.z);
    }

    /// Returns the difference between this Vector3T<T> and B.
    Vector3T<T> operator - (const Vector3T<T>& B) const
    {
        return Vector3T<T>(x-B.x, y-B.y, z-B.z);
    }

    /// The unary minus operator. B=-A is quasi identical with B=A.GetScaled(-1).
    Vector3T<T> operator - () const
    {
        return Vector3T<T>(-x, -y, -z);
    }

    /// Returns a copy of this vector scaled by s, that is, the scalar product (Skalarmultiplikation) of this vector and s.
    /// @param s Factor to multiply this vector with.
    /// @see GetScaled(), which does exactly the same.
    Vector3T<T> operator * (const T s) const
    {
        return GetScaled(s);
    }

    /// Returns a copy of this vector divided by s, that is, the scalar product (Skalarmultiplikation) of this vector and 1/s.
    Vector3T<T> operator / (const T s) const
    {
        // Cannot multiply by the reciprocal, because that won't work with integers.
        return Vector3T<T>(x/s, y/s, z/s);
    }

    /// Returns the dot product (Skalarprodukt) of this vector and B.
    T dot(const Vector3T<T>& B) const
    {
        return x*B.x + y*B.y + z*B.z;
    }

    /// Returns the cross product (Vektorprodukt) of this vector and B.
    Vector3T<T> cross(const Vector3T<T>& B) const
    {
        return Vector3T<T>(y*B.z - z*B.y,
                           z*B.x - x*B.z,
                           x*B.y - y*B.x);
    }

    //@}



    /// @name Group of operators that modify this vector.
    //@{

    /// Adds B to this vector.
    Vector3T<T>& operator += (const Vector3T<T>& B)
    {
        x+=B.x;
        y+=B.y;
        z+=B.z;

        return *this;
    }

    /// Subtracts B from this vector.
    Vector3T<T>& operator -= (const Vector3T<T>& B)
    {
        x-=B.x;
        y-=B.y;
        z-=B.z;

        return *this;
    }

    /// Scales this vector by s.
    Vector3T<T>& operator *= (const T s)
    {
        x*=s;
        y*=s;
        z*=s;

        return *this;
    }

    /// Divides this vector by s. Assumes that s is not 0.
    Vector3T<T>& operator /= (const T s)
    {
        // Cannot multiply by the reciprocal, because that won't work with integers.
        x/=s;
        y/=s;
        z/=s;

        return *this;
    }

    //@}
};


/// Returns A scaled by r, that is, the scalar product (Skalarmultiplikation) of A and r.
template<class T> inline Vector3T<T> scale(const Vector3T<T>& A, const T r)
{
    return Vector3T<T>(A.x*r, A.y*r, A.z*r);
}

/// Returns A, non-uniformely scaled by R.
template<class T> inline Vector3T<T> scale(const Vector3T<T>& A, const Vector3T<T>& R)
{
    return Vector3T<T>(A.x*R.x, A.y*R.y, A.z*R.z);
}

/// Returns the dot product (Skalarprodukt) of A and B.
template<class T> inline T dot(const Vector3T<T>& A, const Vector3T<T>& B)
{
    return A.x*B.x + A.y*B.y + A.z*B.z;
}

/// Returns the cross product (Vektorprodukt) of A and B.
template<class T> inline Vector3T<T> cross(const Vector3T<T>& A, const Vector3T<T>& B)
{
    return Vector3T<T>(A.y*B.z-A.z*B.y,
                       A.z*B.x-A.x*B.z,
                       A.x*B.y-A.y*B.x);
}

/// Returns the length of A.
template<class T> inline T length(const Vector3T<T>& A)
{
    return sqrt(dot(A, A));
}

/// Returns the length of A. This is a specialized version of the generic length<T> function for floats.
template<> inline float length(const Vector3T<float>& A)
{
    return sqrtf(dot(A, A));
}

/// Returns the normalized (unit length) version of A.
/// @throws DivisionByZeroE if length(A)<=Epsilon.
template<class T> inline Vector3T<T> normalize(const Vector3T<T>& A, const T Epsilon)
{
    const T Length=length(A);

    // I'm using <= here rather than only <, so that Epsilon==0 yields a meaningful test (e.g. if T==int).
    if (Length<=Epsilon) throw DivisionByZeroE();

    return A/Length;
}

/// Returns the normalized (unit length) version of A if length(A)>Epsilon, or the (0, 0, 0) vector otherwise.
template<class T> inline Vector3T<T> normalizeOr0(const Vector3T<T>& A, const T Epsilon=0)
{
    const T Length=length(A);

    return (Length>Epsilon) ? scale(A, T(1.0)/Length) : Vector3T<T>(0, 0, 0);
}

template<class T> inline std::string convertToString(const Vector3T<T>& A)
{
    // From MSDN documentation: "digits10 returns the number of decimal digits that the type can represent without loss of precision."
    // For floats, that's usually 6, for doubles, that's usually 15. However, we want to use the number of *significant* decimal digits here,
    // see http://www.open-std.org/JTC1/sc22/wg21/docs/papers/2006/n2005.pdf for details.
    const int sigdigits=std::numeric_limits<T>::digits10 + 3;

    std::ostringstream out;

    out << std::setprecision(sigdigits) << "(" << A.x << ", " << A.y << ", " << A.z << ")";

    return out.str();
}


template<class T> inline std::ostream& operator << (std::ostream& os, const Vector3T<T>& A)
{
    return os << "(" << A.x << ", " << A.y << ", " << A.z << ")";
}


/// Typedef for a Vector3T of floats.
typedef Vector3T<float> Vector3fT;

/// Typedef for a Vector3T of doubles.
typedef Vector3T<double> Vector3dT;
typedef Vector3T<double> VectorT;       // For compatibility with the old VectorT class.

/// Typedef for a Vector3T of ints.
typedef Vector3T<int> Vector3iT;

#endif
