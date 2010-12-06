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

#ifndef _CA_4X4_MATRIX_HPP_
#define _CA_4X4_MATRIX_HPP_

#include "Vector3.hpp"


namespace cf { namespace math { template<class T> class QuaternionT; } }


/// This class represents a generic 4x4 matrix.
/// It has special helper methods for affine geometric transformations in 3D, but can be used for general purposes.
/// Contrary to earlier test versions, it stores the fourth row explicitly, so that easy compatibility with
/// OpenGL matrices is given and no problems occur with general-case use (e.g. as projection matrix etc.).
class MatrixT
{
    public:

    /// The default constructor for creating a "1" (identity) matrix.
    MatrixT()
    {
        for (int i=0; i<4; i++)
            for (int j=0; j<4; j++)
                m[i][j]=(i==j) ? 1.0f : 0.0f;
    }

    /// Constructor for creating an arbitrary matrix.
    MatrixT(const float M[4][4])
    {
        for (int i=0; i<4; i++)
            for (int j=0; j<4; j++)
                m[i][j]=M[i][j];
    }

    /// Constructor for creating an arbitrary matrix.
    MatrixT(const double M[4][4])
    {
        for (int i=0; i<4; i++)
            for (int j=0; j<4; j++)
                m[i][j]=float(M[i][j]);
    }

    /// Constructor for creating an arbitrary matrix.
    MatrixT(float m00, float m01, float m02, float m03,
            float m10, float m11, float m12, float m13,
            float m20, float m21, float m22, float m23,
            float m30, float m31, float m32, float m33)
    {
        m[0][0]=m00; m[0][1]=m01; m[0][2]=m02; m[0][3]=m03;
        m[1][0]=m10; m[1][1]=m11; m[1][2]=m12; m[1][3]=m13;
        m[2][0]=m20; m[2][1]=m21; m[2][2]=m22; m[2][3]=m23;
        m[3][0]=m30; m[3][1]=m31; m[3][2]=m32; m[3][3]=m33;
    }

    /// Constructor for creating a matrix from a quaternion and a translation.
    MatrixT(const cf::math::QuaternionT<float>& Quat, const Vector3fT& Tl);

    /// \name Named constructors.
    //@{
    static MatrixT GetProjOrthoMatrix(float left, float right, float bottom, float top, float zNear, float zFar);   ///< Returns a matrix for orthographic projection.
    static MatrixT GetProjFrustumMatrix(float left, float right, float bottom, float top, float zNear, float zFar); ///< Returns a matrix for perspective projection. If zFar <= zNear, the far plane is assumed to be at infinity (a useful special case for stencil shadow projections).
    static MatrixT GetProjPerspectiveMatrix(float fovY, float aspect, float zNear, float zFar);                     ///< Returns a matrix for perspective projection. If zFar <= zNear, the far plane is assumed to be at infinity (a useful special case for stencil shadow projections).
    static MatrixT GetProjPickMatrix(float x, float y, float width, float height, int viewport[4]);                 ///< Returns a matrix for picking, i.e. the same matrix that gluPickMatrix() uses.

    static MatrixT GetTranslateMatrix(const Vector3fT& t);              ///< Returns a translate matrix about t.
    static MatrixT GetScaleMatrix(float sx, float sy, float sz);        ///< Returns a scale matrix with scale factors (sx sy sz).
    static MatrixT GetRotateXMatrix(float Angle);                       ///< Returns a rotation matrix about Angle degrees around the x-axis.
    static MatrixT GetRotateYMatrix(float Angle);                       ///< Returns a rotation matrix about Angle degrees around the y-axis.
    static MatrixT GetRotateZMatrix(float Angle);                       ///< Returns a rotation matrix about Angle degrees around the z-axis.
    static MatrixT GetRotateMatrix(float Angle, const Vector3fT& Axis); ///< Returns a rotation matrix about Angle degrees around Axis.
    //@}



    /// Returns the i-th row of this matrix.
    float* operator [] (unsigned long i) { assert(i<4); return m[i]; }

    /// Returns the i-th row of this matrix.
    const float* operator [] (unsigned long i) const { assert(i<4); return m[i]; }

    /// Computes M*Other, that is, the matrix product of this and the Other matrix.
    /// @param  Other   The other matrix (right side).
    /// @return The matrix product of this and the Other matrix.
    MatrixT operator * (const MatrixT& Other) const;

    void Translate_MT(const VectorT& Trans);            ///< Computes M=M*T, where T=GetTranslationMatrix(Trans). Assumes that the last row is (0 0 0 1).
    void Translate_MT(float tx, float ty, float tz);    ///< Computes M=M*T, where T=GetTranslationMatrix(Trans). Assumes that the last row is (0 0 0 1).
    void Translate_TM(const VectorT& Trans);            ///< Computes M=T*M, where T=GetTranslationMatrix(Trans). Assumes that the last row is (0 0 0 1).
    void Scale_MS    (float sx, float sy, float sz);    ///< Computes M=M*S, where S=GetScaleMatrix      (sx, sy, sz).
    void Scale_SM    (float sx, float sy, float sz);    ///< Computes M=S*M, where S=GetScaleMatrix      (sx, sy, sz).
    void RotateX_MR  (float Angle);                     ///< Computes M=M*R, where R=GetRotateXMatrix    (Angle).
    void RotateX_RM  (float Angle);                     ///< Computes M=R*M, where R=GetRotateXMatrix    (Angle).
    void RotateY_MR  (float Angle);                     ///< Computes M=M*R, where R=GetRotateYMatrix    (Angle).
    void RotateY_RM  (float Angle);                     ///< Computes M=R*M, where R=GetRotateYMatrix    (Angle).
    void RotateZ_MR  (float Angle);                     ///< Computes M=M*R, where R=GetRotateZMatrix    (Angle).
    void RotateZ_RM  (float Angle);                     ///< Computes M=R*M, where R=GetRotateZMatrix    (Angle).

    /// Computes M*v, where M is this matrix.
    /// The w-component of v is assumed to be 0 (v being a direction vector, not a point).
    /// The last row of M is assumed to be (0 0 0 1).
    /// That means that only the rotation (and scale) of M is applied to v.
    /// @param  v    A direction vector.
    /// @return M*v. The w-component of the returned vector is implied to be 0.
    VectorT Mul0(const VectorT& v) const
    {
        return VectorT(m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z,
                       m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z,
                       m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z);
    }

    // Same as above, but with float.
    /// @param  v    A direction vector.
    /// @return M*v. The w-component of the returned vector is implied to be 0.
    Vector3fT Mul0(const Vector3fT& v) const
    {
        return Vector3fT(m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z,
                         m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z,
                         m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z);
    }

    /// Computes M*v, where M is this matrix.
    /// The w-component of v is assumed to be 1 (v being a point, not a direction vector).
    /// The last row of M is assumed to be (0 0 0 1).
    /// That means that both the rotation (and scale) *and* the translation of M is applied to v.
    /// @param  v    A point.
    /// @return M*v. The w-component of the returned vector is implied to be 1.
    VectorT Mul1(const VectorT& v) const
    {
        return VectorT(m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z + m[0][3],
                       m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z + m[1][3],
                       m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z + m[2][3]);
    }

    // Same as above, but with float.
    /// @param  v    A point.
    /// @return M*v. The w-component of the returned vector is implied to be 1.
    Vector3fT Mul1(const Vector3fT& v) const
    {
        return Vector3fT(m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z + m[0][3],
                         m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z + m[1][3],
                         m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z + m[2][3]);
    }

    /// Computes M*v, where M is this matrix.
    /// The w-component of v is assumed to be 1 (v being a point, not a direction vector).
    /// The last row of M is assumed to be (0 0 0 1).
    /// That means that both the rotation (and scale) *and* the translation of M is applied to v.
    /// @param  v    A point.
    /// @return M*v. The w-component of the returned vector is implied to be 1.
    Vector3fT Mul_xyz1(const Vector3fT& v) const
    {
        return Vector3fT(m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z + m[0][3],
                         m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z + m[1][3],
                         m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z + m[2][3]);
    }

    /// Computes M*v, where M is this matrix.
    /// The w-component of v is assumed to be 1 (v being a point, not a direction vector).
    /// The last row of M is assumed to be (0 0 0 1).
    /// That means that both the rotation (and scale) *and* the translation of M is applied to v.
    /// @param  v   A point.
    /// @param  out The result of M*v.
    void Mul_xyz1(const float v[3], float out[3]) const
    {
        // We need the Result variable in case that v==out.
        float Result[3]={ m[0][0]*v[0] + m[0][1]*v[1] + m[0][2]*v[2] + m[0][3],
                          m[1][0]*v[0] + m[1][1]*v[1] + m[1][2]*v[2] + m[1][3],
                          m[2][0]*v[0] + m[2][1]*v[1] + m[2][2]*v[2] + m[2][3] };

        for (int i=0; i<3; i++) out[i]=Result[i];
    }

    /// Computes M*v, where M is this matrix.
    /// The w-component of v is assumed to be 1 (v being a point, not a direction vector).
    /// The resulting 4-tuple is divided by the w-component so that only the xyz components must be returned.
    /// @param  v    A point.
    /// @return M*v. The w-component of the returned vector is implied to be 1.
    Vector3fT ProjectPoint(const Vector3fT& v) const
    {
        Vector3fT ProjPoint=Mul1(v);
        float     v_w      =m[3][0]*v.x + m[3][1]*v.y + m[3][2]*v.z + m[3][3];

        return (v_w!=0.0f) ? scale(ProjPoint, 1.0f/v_w) : ProjPoint;
    }

    /// Computes M*v, where M is this matrix.
    /// @param v     The four-component vector that is multiplied with M.
    /// @param out   The four-component result vector.
    /// @return The four-component result vector M*v is returned via the out parameter.
    void Mul(const float v[4], float out[4]) const
    {
        // We need the Result variable in case that v==out.
        float Result[4]={ m[0][0]*v[0] + m[0][1]*v[1] + m[0][2]*v[2] + m[0][3]*v[3],
                          m[1][0]*v[0] + m[1][1]*v[1] + m[1][2]*v[2] + m[1][3]*v[3],
                          m[2][0]*v[0] + m[2][1]*v[1] + m[2][2]*v[2] + m[2][3]*v[3],
                          m[3][0]*v[0] + m[3][1]*v[1] + m[3][2]*v[2] + m[3][3]*v[3] };

        for (int i=0; i<4; i++) out[i]=Result[i];
    }

    /// Returns whether this matrix is equal to Other.
    /// The matrices are considered equal if the element-wise comparison yields no difference larger than Epsilon.
    /// @param Other Matrix to compare to.
    /// @param Epsilon Tolerance value.
    bool IsEqual(const MatrixT& Other, const float Epsilon=0) const
    {
        for (unsigned long i=0; i<4; i++)
            for (unsigned long j=0; j<4; j++)
                if (fabs(m[i][j]-Other.m[i][j]) > Epsilon)
                    return false;

        return true;
    }

    /// If this matrix represents a rigid transformation (rotation and translation only, no scale, shear, etc.),
    /// this is a faster shortcut for GetInverse().Mul1(v).
    /// It employs the transpose of the rotational part for inverting the rotation, and properly accounts for the translation.
    /// @param v DOCTODO
    Vector3fT InvXForm(Vector3fT v) const
    {
        v.x-=m[0][3];
        v.y-=m[1][3];
        v.z-=m[2][3];

        return Vector3fT(m[0][0]*v.x + m[1][0]*v.y + m[2][0]*v.z,
                         m[0][1]*v.x + m[1][1]*v.y + m[2][1]*v.z,
                         m[0][2]*v.x + m[1][2]*v.y + m[2][2]*v.z);
    }

    /// Computes the inverse of this matrix.
    /// @param Result Whether the inversion was successfull.
    /// @return If this matrix is invertible, the inverse is returned by this method and *Result, if not at NULL, is set to true.
    ///         Otherwise, an undefined matrix is returned and *Result, if not at NULL, is set to false.
    MatrixT GetInverse(bool* Result=NULL) const;

    /// Returns the transpose of this matrix.
    /// @return The transpose of this matrix.
    MatrixT GetTranspose() const;



    /// The matrix values.
    float m[4][4];
};

#endif
