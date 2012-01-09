/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

Cafu is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

Cafu is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Cafu. If not, see <http://www.gnu.org/licenses/>.

For support and more information about Cafu, visit us at <http://www.cafu.de>.
=================================================================================
*/

#ifndef _CA_MATH_MISC_HPP_
#define _CA_MATH_MISC_HPP_


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
