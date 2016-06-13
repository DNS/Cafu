/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MATH_MISC_HPP_INCLUDED
#define CAFU_MATH_MISC_HPP_INCLUDED


template<class T> class Vector3T;


namespace cf
{
    namespace math
    {
        float  round(float  f);     ///< Rounds the given number to the nearest integer.
        double round(double d);     ///< Rounds the given number to the nearest integer.

        /// Rounds all components of the given vector to the nearest integer.
        template<class T> Vector3T<T> round(const Vector3T<T>& v)
        {
            return Vector3T<T>(round(v.x), round(v.y), round(v.z));
        }
    }
}

#endif
