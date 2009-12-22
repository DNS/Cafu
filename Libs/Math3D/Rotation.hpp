/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#ifndef _CF_MATH_ROTATION_HPP_
#define _CF_MATH_ROTATION_HPP_

#include "Matrix3x3.hpp"
#include "Vector3.hpp"


namespace cf
{
    namespace math
    {
        /// This class describes a rotation about an arbitrary origin and an arbitrary axis.
        /// The mathematical background is simple, see for example the lecture notes of my computer graphics
        /// course in winter semester 2001/2002, D:\\Uni\\Computergrafik\\Lecture02_AffineTransforms.pdf page 16.
        template<class T> class RotationT
        {
            public:

            /// The default constructor. It creates a rotation that represents a "null" rotation.
            RotationT()
                : m_Origin(),
                  m_Axis(),
                  m_Angle(0),
                  m_IsRotMatValid(true),    // Ok, because m_RotMat is inited as the identity matrix.
                  m_RotMat()
            {
            }

            /// The constructor for creating a rotation.
            /// @param Origin   The origin (center) of the rotation.
            /// @param Axis     The axis of the rotation.
            /// @param Angle    The Euler angle (in degrees) for the rotation.
            RotationT(const Vector3T<T>& Origin, const Vector3T<T>& Axis, const T& Angle)
                : m_Origin(Origin),
                  m_Axis(Axis),
                  m_Angle(Angle),
                  m_IsRotMatValid(false),
                  m_RotMat()
            {
            }

            /// Gets the origin of this rotation.
            const Vector3T<T>&   GetOrigin() const { return m_Origin; }

            /// Gets the rotation axis.
            const Vector3T<T>&   GetAxis() const   { return m_Axis;   }

            /// Gets the rotation angle.
            T                    GetAngle() const  { return m_Angle;  }

            /// Gets the rotation matrix.
            const Matrix3x3T<T>& GetRotMat() const;

            /// Rotates a vector with this rotation.
            Vector3T<T> GetRotated(const Vector3T<T>& A) const { return GetRotMat()*(A-m_Origin) + m_Origin; }

            /// Same as GetRotated.
            Vector3T<T> operator * (const Vector3T<T>& A) const { return GetRotated(A); }


            private:

            Vector3T<T>           m_Origin;
            Vector3T<T>           m_Axis;
            T                     m_Angle;

            mutable bool          m_IsRotMatValid;  ///< Whether the m_RotMat member is valid (matches the m_Axis and m_Angle).
            mutable Matrix3x3T<T> m_RotMat;         ///< The 3x3 matrix that corresponds to the rotation about m_Axis about m_Angle degrees. Note that this always translates about the (0, 0, 0) origin, it does *not* take m_Origin into account!
        };


        /// Typedef for a RotationT of floats.
        typedef RotationT<float> RotationfT;

        /// Typedef for a Rotation of doubles.
        typedef RotationT<double> RotationdT;
    }
}

#endif
