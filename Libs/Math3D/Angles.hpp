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

#ifndef _CF_MATH_ANGLES_HPP_
#define _CF_MATH_ANGLES_HPP_

#include "Vector3.hpp"


namespace cf
{
    namespace math
    {
        /// This class represents a triple of angles that are specified in degrees.
        ///
        /// The angles are usually understood as independent rotations about the x-, y- and z-axis, respectively,
        /// where in a right-handed coordinate system the x-axis points right, the y-axis points forward, and the z-axis points up.
        /// Therefore, the x-axis angle is aliased by the name "pitch" (Nicken),
        ///            the y-axis angle is aliased by the name "roll" (Rollen),
        ///        and the z-axis angle is aliased by the name "yaw" (Gieren).
        /// The convention is to obtain the corresponding rotation matrix by first rotating about the z-axis (yaw), then rotating
        /// the new local (relative) system about the x-axis (pitch), and finally rotating the local system about the y-axis (roll).
        template<class T> class AnglesT : public Vector3T<T>
        {
            public:

            /// The default constructor. It initializes all angles to zero.
            AnglesT() : Vector3T<T>() { }

            /// This constructor initializes the angles from x_, y_ and z_ respectively.
            AnglesT(T x_, T y_, T z_) : Vector3T<T>(x_, y_, z_) { }

            /// A constructor for initializing an Angles instance from a Vector3T.
            AnglesT(const Vector3T<T>& v) : Vector3T<T>(v) { }


            // The "this->" cannot be omitted here, or else g++ 4.2.3 complains that "'x' was not declared in this scope".
            T& pitch() { return this->x; }              ///< Provides the alias for the angle of rotation around the x-axis.
            const T& pitch() const { return this->x; }  ///< Provides the alias for the angle of rotation around the x-axis.

            T& roll() { return this->y; }               ///< Provides the alias for the angle of rotation around the y-axis.
            const T& roll() const { return this->y; }   ///< Provides the alias for the angle of rotation around the y-axis.

            T& yaw() { return this->z; }                ///< Provides the alias for the angle of rotation around the z-axis.
            const T& yaw() const { return this->z; }    ///< Provides the alias for the angle of rotation around the z-axis.


            static const double PI;                                         ///< This is PI.
            static T RadToDeg(const T Angle) { return Angle*T(180.0/PI); }  ///< Converts the given angle from radians to degrees.
            static T DegToRad(const T Angle) { return Angle*T(PI/180.0); }  ///< Converts the given angle from degrees to radians.
        };


        /// Typedef for an AnglesT of floats.
        typedef AnglesT<float> AnglesfT;

        /// Typedef for an Angles of doubles.
        typedef AnglesT<double> AnglesdT;
    }
}

#endif
