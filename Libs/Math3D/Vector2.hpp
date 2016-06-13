/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MATH_VECTOR2_HPP_INCLUDED
#define CAFU_MATH_VECTOR2_HPP_INCLUDED

#include "Errors.hpp"

#include <cassert>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>


/// This class represents a 2-dimensional vector.
template<class T>
class Vector2T
{
    public:

    T x;    ///< The x-component of this vector.
    T y;    ///< The y-component of this vector.



    /// The default constructor. It initializes all components to zero.
    Vector2T() : x(0), y(0) { }

    /// This constructor initializes the components from x_ and y_ respectively.
    Vector2T(T x_, T y_) : x(x_), y(y_) { }

    /// This constructor initializes the components from an array of (at least) two Ts.
    template<class C> explicit Vector2T(const C Values[]) : x(T(Values[0])), y(T(Values[1])) { }



    /// Returns true if the vector is valid, that is, all components are non-NANs.
    bool IsValid() const
    {
        return true;
    }

    /// Component access by index number (0 to 1) rather than by name.
    /// @param Index Index of the component to access. Can only be 0 or 1 (for x or y).
    /// @throws InvalidOperationE if Index is not 0 or 1.
    T& operator [] (unsigned int Index)
    {
        switch (Index)
        {
            case 0: return x;
            case 1: return y;
            default: throw InvalidOperationE();
        }
    }

    /// Component access by index number (0 to 1) rather than by name.
    /// @param Index Index of the component to access. Can only be 0 or 1 (for x or y).
    /// @throws InvalidOperationE if Index is not 0 or 1.
    const T& operator [] (unsigned int Index) const
    {
        switch (Index)
        {
            case 0: return x;
            case 1: return y;
            default: throw InvalidOperationE();
        }
    }



    /// Gets this Vector2T<T> as a Vector2T<float>, so that the cast is explicitly visible in user code.
    Vector2T<float> AsVectorOfFloat() const
    {
#ifdef _WIN32
        assert(typeid(T)!=typeid(float));
#endif

        return Vector2T<float>(float(x), float(y));
    }

    /// Gets this Vector2T<T> as a Vector2T<double>, so that the cast is explicitly visible in user code.
    Vector2T<double> AsVectorOfDouble() const
    {
#ifdef _WIN32
        assert(typeid(T)!=typeid(double));
#endif

        return Vector2T<double>(double(x), double(y));
    }



    /// @name Group of const inspector methods. They are all const and thus do not modify this object.
    //@{

    /// Returns the square of the length of this vector.
    T GetLengthSqr() const { return x*x + y*y; }

    /// Returns whether this vector is equal to B within tolerance Epsilon, that is, whether it is geometrically closer to B than Epsilon.
    /// @param B Vector to compare to.
    /// @param Epsilon Tolerance value.
    /// @see operator ==
    bool IsEqual(const Vector2T<T>& B, const T Epsilon) const
    {
        return (*this-B).GetLengthSqr() <= Epsilon*Epsilon;
    }

    /// Returns a copy of this vector scaled by s, that is, the scalar product (Skalarmultiplikation) of this vector and s.
    /// @param s Scale factor to scale this vector by.
    /// @see  Also see the operator *, which does exactly the same.
    Vector2T<T> GetScaled(const T s) const
    {
        return Vector2T<T>(x*s, y*s);
    }

    /// Returns a copy of this vector non-uniformely scaled by S.
    Vector2T<T> GetScaled(const Vector2T<T>& S) const
    {
        return Vector2T<T>(x*S.x, y*S.y);
    }

    //@}



    /// @name Group of (constructive) binary operators that do not modify their operands.
    //@{

    /// Returns whether this vector and B are truly (bit-wise) identical.
    /// Use this operator with care, as it comes *without* any epsilon threshold for taking rounding errors into account.
    /// @param B Vector to compare to.
    /// @see IsEqual()
    bool operator == (const Vector2T<T>& B) const
    {
        return x==B.x &&
               y==B.y;
    }

    /// Returns whether this vector and B are not equal (bit-wise).
    /// Use this operator with care, as it comes *without* any epsilon threshold for taking rounding errors into account.
    /// @param B Vector to compare to.
    /// @see IsEqual()
    bool operator != (const Vector2T<T>& B) const
    {
        return x!=B.x ||
               y!=B.y;
    }

    /// Returns the sum of this Vector2T<T> and B.
    Vector2T<T> operator + (const Vector2T<T>& B) const
    {
        return Vector2T<T>(x+B.x, y+B.y);
    }

    /// Returns the difference between this Vector2T<T> and B.
    Vector2T<T> operator - (const Vector2T<T>& B) const
    {
        return Vector2T<T>(x-B.x, y-B.y);
    }

    /// The unary minus operator. B=-A is quasi identical with B=A.GetScaled(-1).
    Vector2T<T> operator - () const
    {
        return Vector2T<T>(-x, -y);
    }

    /// Returns a copy of this vector scaled by s, that is, the scalar product (Skalarmultiplikation) of this vector and s.
    /// @param s Factor to multiply this vector with.
    /// @see GetScaled(), which does exactly the same.
    Vector2T<T> operator * (const T s) const
    {
        return GetScaled(s);
    }

    /// Returns a copy of this vector divided by s, that is, the scalar product (Skalarmultiplikation) of this vector and 1/s.
    Vector2T<T> operator / (const T s) const
    {
        // Cannot multiply by the reciprocal, because that won't work with integers.
        return Vector2T<T>(x/s, y/s);
    }

    /// Returns the dot product (Skalarprodukt) of this vector and B.
    T dot(const Vector2T<T>& B) const
    {
        return x*B.x + y*B.y;
    }

    //@}



    /// @name Group of operators that modify this vector.
    //@{

    /// Adds B to this vector.
    Vector2T<T>& operator += (const Vector2T<T>& B)
    {
        x+=B.x;
        y+=B.y;

        return *this;
    }

    /// Subtracts B from this vector.
    Vector2T<T>& operator -= (const Vector2T<T>& B)
    {
        x-=B.x;
        y-=B.y;

        return *this;
    }

    /// Scales this vector by s.
    Vector2T<T>& operator *= (const T s)
    {
        x*=s;
        y*=s;

        return *this;
    }

    /// Divides this vector by s. Assumes that s is not 0.
    Vector2T<T>& operator /= (const T s)
    {
        // Cannot multiply by the reciprocal, because that won't work with integers.
        x/=s;
        y/=s;

        return *this;
    }

    //@}
};


/// Returns A scaled by r, that is, the scalar product (Skalarmultiplikation) of A and r.
template<class T> inline Vector2T<T> scale(const Vector2T<T>& A, const T r)
{
    return Vector2T<T>(A.x*r, A.y*r);
}

/// Returns A, non-uniformely scaled by R.
template<class T> inline Vector2T<T> scale(const Vector2T<T>& A, const Vector2T<T>& R)
{
    return Vector2T<T>(A.x*R.x, A.y*R.y);
}

/// Returns the dot product (Skalarprodukt) of A and B.
template<class T> inline T dot(const Vector2T<T>& A, const Vector2T<T>& B)
{
    return A.x*B.x + A.y*B.y;
}

/// Returns the length of A.
template<class T> inline T length(const Vector2T<T>& A)
{
    return sqrt(dot(A, A));
}

/// Returns the length of A. This is a specialized version of the generic length<T> function for floats.
template<> inline float length(const Vector2T<float>& A)
{
    return sqrtf(dot(A, A));
}

/// Returns the normalized (unit length) version of A.
/// @throws DivisionByZeroE if length(A)<=Epsilon.
template<class T> inline Vector2T<T> normalize(const Vector2T<T>& A, const T Epsilon)
{
    const T Length=length(A);

    // I'm using <= here rather than only <, so that Epsilon==0 yields a meaningful test (e.g. if T==int).
    if (Length<=Epsilon) throw DivisionByZeroE();

    return A/Length;
}

/// Returns the normalized (unit length) version of A if length(A)>Epsilon, or the (0, 0, 0) vector otherwise.
template<class T> inline Vector2T<T> normalizeOr0(const Vector2T<T>& A, const T Epsilon=0)
{
    const T Length=length(A);

    return (Length>Epsilon) ? scale(A, T(1.0)/Length) : Vector2T<T>(0, 0, 0);
}

template<class T> inline std::string convertToString(const Vector2T<T>& A)
{
    // From MSDN documentation: "digits10 returns the number of decimal digits that the type can represent without loss of precision."
    // For floats, that's usually 6, for doubles, that's usually 15. However, we want to use the number of *significant* decimal digits here,
    // see http://www.open-std.org/JTC1/sc22/wg21/docs/papers/2006/n2005.pdf for details.
    const int sigdigits=std::numeric_limits<T>::digits10 + 3;

    std::ostringstream out;

    out << std::setprecision(sigdigits) << "(" << A.x << ", " << A.y << ")";

    return out.str();
}


template<class T> inline std::ostream& operator << (std::ostream& os, const Vector2T<T>& A)
{
    return os << "(" << A.x << ", " << A.y << ")";
}


typedef Vector2T<float>  Vector2fT;
typedef Vector2T<double> Vector2dT;

#endif
