/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#ifndef _CA_MATH_MATRIX_3X3_HPP_
#define _CA_MATH_MATRIX_3X3_HPP_

#include "Vector3.hpp"


namespace cf
{
    namespace math
    {
        template<class T> class AnglesT;


        /// This class represents a generic 3x3 matrix.
        template<class T>
        class Matrix3x3T
        {
            public:

            /// The default constructor for creating a "1" (identity) matrix.
            Matrix3x3T()
            {
                for (unsigned long i=0; i<3; i++)
                    for (unsigned long j=0; j<3; j++)
                        m[i][j]=(i==j) ? T(1.0) : 0;
            }

            /// Constructor for creating an arbitrary matrix.
            Matrix3x3T(const float M[3][3])
            {
                for (unsigned long i=0; i<3; i++)
                    for (unsigned long j=0; j<3; j++)
                        m[i][j]=T(M[i][j]);
            }

            /// Constructor for creating an arbitrary matrix.
            Matrix3x3T(const double M[3][3])
            {
                for (unsigned long i=0; i<3; i++)
                    for (unsigned long j=0; j<3; j++)
                        m[i][j]=T(M[i][j]);
            }

            /// Constructor for creating an arbitrary matrix.
            Matrix3x3T(T m00, T m01, T m02,
                       T m10, T m11, T m12,
                       T m20, T m21, T m22)
            {
                m[0][0]=m00; m[0][1]=m01; m[0][2]=m02;
                m[1][0]=m10; m[1][1]=m11; m[1][2]=m12;
                m[2][0]=m20; m[2][1]=m21; m[2][2]=m22;
            }

            /// Creates a rotation matrix that corresponds to the three-step rotation described by the Angles.
            /// Refer to the description of the AnglesT class for more information on how the angles are understood.
            /// @param Angles Angles to create the matrix from.
            Matrix3x3T(const AnglesT<T>& Angles);

            /// @name Named constructors.
            //@{
            static Matrix3x3T GetScaleMatrix(T sx, T sy, T sz);                    ///< Returns a scale matrix with scale factors (sx sy sz).
            static Matrix3x3T GetRotateXMatrix(T Angle);                           ///< Returns a rotation matrix about Angle degrees around the x-axis.
            static Matrix3x3T GetRotateYMatrix(T Angle);                           ///< Returns a rotation matrix about Angle degrees around the y-axis.
            static Matrix3x3T GetRotateZMatrix(T Angle);                           ///< Returns a rotation matrix about Angle degrees around the z-axis.
            static Matrix3x3T GetRotateMatrix(T Angle, const Vector3T<T>& Axis);   ///< Returns a rotation matrix about Angle degrees around Axis.
            static Matrix3x3T GetFromAngles_COMPAT(const AnglesT<T>& Angles);      ///< Returns a rotation matrix built from the given angles in a "compatibility-to-old-code" fashion.
            //@}



            /// Returns the i-th row of this matrix.
            T* operator [] (unsigned long i) { assert(i<3); return m[i]; }

            /// Returns the i-th row of this matrix.
            const T* operator [] (unsigned long i) const { assert(i<3); return m[i]; }

            /// Computes M*Other, that is, the matrix product of this and the Other matrix.
            /// @param  Other   The other matrix (right side).
            /// @return The matrix product of this and the Other matrix.
            Matrix3x3T operator * (const Matrix3x3T& Other) const;

            /// Computes M*v, where M is this matrix.
            /// @param  v    A vector.
            /// @return M*v.
            Vector3T<T> operator * (const Vector3T<T>& v) const
            {
                return Mul(v);
            }

            /// Determines if this matrix is equal to Other. Note that this is a bit-wise comparison, no epsilon is taken into account.
            /// @param   Other   The other matrix (right side).
            /// @returns whether this matrix and Other are equal.
            bool operator == (const Matrix3x3T& Other) const
            {
                for (unsigned long i=0; i<3; i++)
                    for (unsigned long j=0; j<3; j++)
                        if (m[i][j]!=Other.m[i][j])
                            return false;

                return true;
            }

            /// Determines if this matrix is not equal to Other. Note that this is a bit-wise comparison, no epsilon is taken into account.
            /// @param   Other   The other matrix (right side).
            /// @returns whether this matrix and Other are not equal.
            bool operator != (const Matrix3x3T& Other) const
            {
                return !(*this==Other);
            }


            void Scale_MS  (T sx, T sy, T sz);  ///< Computes M=M*S, where S=GetScaleMatrix      (sx, sy, sz).
            void Scale_SM  (T sx, T sy, T sz);  ///< Computes M=S*M, where S=GetScaleMatrix      (sx, sy, sz).
            void RotateX_MR(T Angle);           ///< Computes M=M*R, where R=GetRotateXMatrix    (Angle).
            void RotateX_RM(T Angle);           ///< Computes M=R*M, where R=GetRotateXMatrix    (Angle).
            void RotateY_MR(T Angle);           ///< Computes M=M*R, where R=GetRotateYMatrix    (Angle).
            void RotateY_RM(T Angle);           ///< Computes M=R*M, where R=GetRotateYMatrix    (Angle).
            void RotateZ_MR(T Angle);           ///< Computes M=M*R, where R=GetRotateZMatrix    (Angle).
            void RotateZ_RM(T Angle);           ///< Computes M=R*M, where R=GetRotateZMatrix    (Angle).

            /// Returns whether this matrix is equal to Other.
            /// The matrices are considered equal if the element-wise comparison yields no difference larger than Epsilon.
            /// @param Other Matrix to compare to.
            /// @param Epsilon Tolerance value.
            /// @see operator ==
            bool IsEqual(const Matrix3x3T& Other, const T Epsilon=0) const
            {
                for (unsigned long i=0; i<3; i++)
                    for (unsigned long j=0; j<3; j++)
                        if (fabs(m[i][j]-Other.m[i][j]) > Epsilon)
                            return false;

                return true;
            }

            /// Computes M*v, where M is this matrix.
            /// @param  v    A vector.
            /// @return M*v.
            Vector3T<T> Mul(const Vector3T<T>& v) const
            {
                return Vector3T<T>(m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z,
                                   m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z,
                                   m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z);
            }

            /// A shortcut for M.GetTranspose()*v, where M is this matrix.
            /// @param  v    A vector.
            /// @return Mt*v, where Mt is the transpose of this matrix.
            Vector3T<T> MulTranspose(const Vector3T<T>& v) const
            {
                return Vector3T<T>(m[0][0]*v.x + m[1][0]*v.y + m[2][0]*v.z,
                                   m[0][1]*v.x + m[1][1]*v.y + m[2][1]*v.z,
                                   m[0][2]*v.x + m[1][2]*v.y + m[2][2]*v.z);
            }

            /// Computes M*v, where M is this matrix.
            /// @param v     The three-component vector that is multiplied with M.
            /// @param out   The three-component result vector.
            /// @return The three-component result vector M*v is returned via the out parameter.
            void Mul(const T v[3], T out[3]) const
            {
                // We need the Result variable in case that v==out.
                T Result[3]={ m[0][0]*v[0] + m[0][1]*v[1] + m[0][2]*v[2],
                              m[1][0]*v[0] + m[1][1]*v[1] + m[1][2]*v[2],
                              m[2][0]*v[0] + m[2][1]*v[1] + m[2][2]*v[2] };

                for (int i=0; i<3; i++) out[i]=Result[i];
            }

            /// Computes the inverse of this matrix.
            /// @param Result Whether the opration was successfull.
            /// @return If this matrix is invertible, the inverse is returned by this method and *Result, if not at NULL, is set to true.
            ///         Otherwise, an undefined matrix is returned and *Result, if not at NULL, is set to false.
            Matrix3x3T GetInverse(bool* Result=NULL) const;

            /// Returns the transpose of this matrix.
            /// @return The transpose of this matrix.
            Matrix3x3T GetTranspose() const;

            /// Converts the rotation that is described by this matrix to an AnglesT<T> instance that describes the same rotation.
            /// @returns An AnglesT<T> instance that describes the same rotation as this matrix.
            AnglesT<T> ToAngles_COMPAT() const;
         // AnglesT<T> ToAngles() const;    // TODO: Implement the non-compatiblity / "right" / "correct" version...



            /// The matrix values.
            T m[3][3];

            static const Matrix3x3T<T> Identity; ///< Identity matrix.
        };


        /// Typedef for a Matrix3x3T of floats.
        typedef Matrix3x3T<float> Matrix3x3fT;

        /// Typedef for a Matrix3x3T of doubles.
        typedef Matrix3x3T<double> Matrix3x3dT;
    }
}

#endif
