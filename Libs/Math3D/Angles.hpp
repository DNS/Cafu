/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2013 Carsten Fuchs Software.

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

#ifndef CAFU_MATH_ANGLES_HPP_INCLUDED
#define CAFU_MATH_ANGLES_HPP_INCLUDED

#include "Vector3.hpp"


namespace cf
{
    namespace math
    {
        template<class T> class Matrix3x3T;
        template<class T> class QuaternionT;


        /*
         * TODO: AnglesT should no longer derive from Vector3T, but rather have a private Vector3T member.
         *       Then, AnglesT should only expose a Heading, Pitch and Bank interface, and nothing with
         *       x, y or z, so that the meaning of things is always clear.
         */

        /*
         * TODO: Some tests would be nice, e.g. these two are enough for testing all combinations:
         *       assert(TestAngles == AnglesT(Matrix3x3T(QuaternionT(TestAngles))))
         *       assert(TestAngles == AnglesT(QuaternionT(Matrix3x3T(TestAngles))))
         */

        /// This class represents a triple of angles.
        /// The class keeps the angles in degrees, not in radians.
        ///
        /// The angles are typically used to describe an orientation. Generally, the orientation is achieved
        /// by composing three elemental rotations about the principal axes by the given angles.
        /// For this purpose, all related code in Cafu (e.g. the matrix and quaternion classes) uses the
        /// following conventions:
        ///
        ///   - As everywhere in Cafu, a right-handed coordinate system is used: the x-axis points right,
        ///     the y-axis points forward, and the z-axis points up.
        ///
        ///   - The rotations are done
        ///       - first about the z-axis (by the "heading" or "yaw" angle),
        ///       - then about the rotated y-axis (by the "pitch" angle),
        ///       - then about the rotated x-axis (by the "bank" or "roll" angle).
        ///     This order is explained in greater detail in the references listed below.
        ///
        ///   - Positive angles rotate counter-clockwise (the "right-hand rule").
        ///
        ///   - North is along the x-axis, West is along the y-axis.
        ///     This does *not* follow the compase-rose analogy (that we had normally preferred), where North is
        ///     along the y-axis, East is along the x-axis, and positive rotation is clockwise, but was accepted
        ///     for conformance with the broader conventions (e.g. DIN 9300) given in the references below.
        ///
        /// References (in German, as the English editions don't have the relevant math):
        ///
        ///   - http://de.wikipedia.org/wiki/Eulersche_Winkel
        ///     Especially section "z, y', x''-Konvention in der Fahrzeugtechnik" has a very well explained
        ///     derivation of a rotation matrix from Euler angles. Note, however, that the resulting matrix
        ///     transforms from world-to-model space, whereas we're more interested in the transpose, in order
        ///     to transform from model-to-world space.
        ///
        ///   - http://de.wikipedia.org/wiki/Roll-Nick-Gier-Winkel
        ///     Section "Z, Y', X'' Konvention" complements the above, where the model-to-world space matrix
        ///     is readily given, and the math for converting back to Euler angles as well.
        template<class T> class AnglesT : public Vector3T<T>
        {
            public:

            /// The default constructor. It initializes all angles to zero.
            AnglesT() : Vector3T<T>() { }

            /// This constructor initializes the angles from x_, y_ and z_ respectively.
            AnglesT(T x_, T y_, T z_) : Vector3T<T>(x_, y_, z_) { }

            /// A constructor for initializing an Angles instance from a Vector3T.
            AnglesT(const Vector3T<T>& v) : Vector3T<T>(v) { }

            /// Constructs a set of angles that describe the same orientation as the given matrix.
            AnglesT(const Matrix3x3T<T>& M);

            /// Constructs a set of angles that describe the same orientation as the given quaternion.
            AnglesT(const QuaternionT<T>& Quat);


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


            private:

            void Init(const Matrix3x3T<T>& M);
        };


        /// Typedef for an AnglesT of floats.
        typedef AnglesT<float> AnglesfT;

        /// Typedef for an Angles of doubles.
        typedef AnglesT<double> AnglesdT;
    }
}

#endif
