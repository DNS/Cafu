/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MATH_MATRIX_4X4_HPP_INCLUDED
#define CAFU_MATH_MATRIX_4X4_HPP_INCLUDED

#include "Vector3.hpp"


namespace cf { namespace math { template<class T> class QuaternionT; } }


/// This class represents a generic 4x4 matrix.
/// It has special helper methods for affine geometric transformations in 3D, but can be used for general purposes.
/// Contrary to earlier test versions, it stores the fourth row explicitly, so that easy compatibility with
/// OpenGL matrices is given and no problems occur with general-case use (e.g. as projection matrix etc.).
/// @nosubgrouping
template<class T>
class Matrix4x4T
{
    public:

    /// The default constructor for creating a "1" (identity) matrix.
    Matrix4x4T()
    {
        for (unsigned int i = 0; i < 4; i++)
            for (unsigned int j = 0; j < 4; j++)
                m[i][j] = (i == j) ? T(1.0) : 0;
    }

    /// Constructor for creating an arbitrary matrix.
    Matrix4x4T(const float M[4][4])
    {
        for (unsigned int i = 0; i < 4; i++)
            for (unsigned int j = 0; j < 4; j++)
                m[i][j] = T(M[i][j]);
    }

    /// Constructor for creating an arbitrary matrix.
    Matrix4x4T(const double M[4][4])
    {
        for (unsigned int i = 0; i < 4; i++)
            for (unsigned int j = 0; j < 4; j++)
                m[i][j] = T(M[i][j]);
    }

    /// Constructor for creating an arbitrary matrix.
    Matrix4x4T(T m00, T m01, T m02, T m03,
               T m10, T m11, T m12, T m13,
               T m20, T m21, T m22, T m23,
               T m30, T m31, T m32, T m33)
    {
        m[0][0] = m00; m[0][1] = m01; m[0][2] = m02; m[0][3] = m03;
        m[1][0] = m10; m[1][1] = m11; m[1][2] = m12; m[1][3] = m13;
        m[2][0] = m20; m[2][1] = m21; m[2][2] = m22; m[2][3] = m23;
        m[3][0] = m30; m[3][1] = m31; m[3][2] = m32; m[3][3] = m33;
    }

    /// Constructor for creating a matrix from a translation, a quaternion, and an optional scale.
    ///
    /// The resulting matrix is equal to the matrix product T*R*S of three matrices T, R and S, where
    /// T is the translation matrix that corresponds to the given translation vector @c t,
    /// R is the rotation matrix that is constructed from the given quaternion @c q, and
    /// S is the scale matrix that corresponds to the given scale @c s.
    ///
    /// The resulting matrix is of the form:
    /// @code
    ///     x1*s.x   x2*s.y   x3*s.z   t.x
    ///     y1*s.x   y2*s.y   y3*s.z   t.y
    ///     z1*s.x   z2*s.y   z3*s.z   t.z
    ///          0        0        0     1
    /// @endcode
    ///
    /// @param t   The translation that is expressed in the matrix.
    /// @param q   The quaternion that describes the rotation that is expressed in the matrix.
    /// @param s   The scale that is expressed in the matrix.
    Matrix4x4T(const Vector3T<T>& t, const cf::math::QuaternionT<T>& q, const Vector3T<T>& s = Vector3T<T>(1, 1, 1));

    /// \name Named constructors
    /// @{
    static Matrix4x4T GetProjOrthoMatrix(T left, T right, T bottom, T top, T zNear, T zFar);    ///< Returns a matrix for orthographic projection.
    static Matrix4x4T GetProjFrustumMatrix(T left, T right, T bottom, T top, T zNear, T zFar);  ///< Returns a matrix for perspective projection. If zFar <= zNear, the far plane is assumed to be at infinity (a useful special case for stencil shadow projections).
    static Matrix4x4T GetProjPerspectiveMatrix(T fovY, T aspect, T zNear, T zFar);              ///< Returns a matrix for perspective projection. If zFar <= zNear, the far plane is assumed to be at infinity (a useful special case for stencil shadow projections).
    static Matrix4x4T GetProjPickMatrix(T x, T y, T width, T height, int viewport[4]);          ///< Returns a matrix for picking, i.e. the same matrix that gluPickMatrix() uses.

    static Matrix4x4T GetTranslateMatrix(const Vector3T<T>& t);             ///< Returns a translate matrix about t.
    static Matrix4x4T GetScaleMatrix(T sx, T sy, T sz);                     ///< Returns a scale matrix with scale factors (sx sy sz).
    static Matrix4x4T GetRotateXMatrix(T Angle);                            ///< Returns a rotation matrix about Angle degrees around the x-axis.
    static Matrix4x4T GetRotateYMatrix(T Angle);                            ///< Returns a rotation matrix about Angle degrees around the y-axis.
    static Matrix4x4T GetRotateZMatrix(T Angle);                            ///< Returns a rotation matrix about Angle degrees around the z-axis.
    static Matrix4x4T GetRotateMatrix(T Angle, const Vector3T<T>& Axis);    ///< Returns a rotation matrix about Angle degrees around Axis.
    /// @}



    /// Returns the i-th row of this matrix.
    T* operator [] (unsigned int i) { assert(i < 4); return m[i]; }

    /// Returns the i-th row of this matrix.
    const T* operator [] (unsigned int i) const { assert(i < 4); return m[i]; }

    /// Computes M*Other, that is, the matrix product of this and the Other matrix.
    /// @param  Other   The other matrix (right side).
    /// @return The matrix product of this and the Other matrix.
    Matrix4x4T operator * (const Matrix4x4T& Other) const;

    void Translate_MT(const Vector3dT& Trans);  ///< Computes M=M*T, where T=GetTranslationMatrix(Trans). Assumes that the last row is (0 0 0 1).
    void Translate_MT(T tx, T ty, T tz);        ///< Computes M=M*T, where T=GetTranslationMatrix(Trans). Assumes that the last row is (0 0 0 1).
    void Translate_TM(const Vector3dT& Trans);  ///< Computes M=T*M, where T=GetTranslationMatrix(Trans). Assumes that the last row is (0 0 0 1).
    void Scale_MS    (T sx, T sy, T sz);        ///< Computes M=M*S, where S=GetScaleMatrix      (sx, sy, sz).
    void Scale_SM    (T sx, T sy, T sz);        ///< Computes M=S*M, where S=GetScaleMatrix      (sx, sy, sz).
    void RotateX_MR  (T Angle);                 ///< Computes M=M*R, where R=GetRotateXMatrix    (Angle).
    void RotateX_RM  (T Angle);                 ///< Computes M=R*M, where R=GetRotateXMatrix    (Angle).
    void RotateY_MR  (T Angle);                 ///< Computes M=M*R, where R=GetRotateYMatrix    (Angle).
    void RotateY_RM  (T Angle);                 ///< Computes M=R*M, where R=GetRotateYMatrix    (Angle).
    void RotateZ_MR  (T Angle);                 ///< Computes M=M*R, where R=GetRotateZMatrix    (Angle).
    void RotateZ_RM  (T Angle);                 ///< Computes M=R*M, where R=GetRotateZMatrix    (Angle).

    /// Computes M*v, where M is this matrix.
    /// The w-component of v is assumed to be 0 (v being a direction vector, not a point).
    /// The last row of M is assumed to be (0 0 0 1).
    /// That means that only the rotation (and scale) of M is applied to v.
    /// @param  v    A direction vector.
    /// @return M*v. The w-component of the returned vector is implied to be 0.
    Vector3dT Mul0(const Vector3dT& v) const
    {
        return Vector3dT(m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z,
                         m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z,
                         m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z);
    }

    // Same as above, but with float.
    /// @param  v    A direction vector.
    /// @return M*v. The w-component of the returned vector is implied to be 0.
    Vector3fT Mul0(const Vector3fT& v) const
    {
        return Vector3fT(float(m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z),
                         float(m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z),
                         float(m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z));
    }

    /// Computes M*v, where M is this matrix.
    /// The w-component of v is assumed to be 1 (v being a point, not a direction vector).
    /// The last row of M is assumed to be (0 0 0 1).
    /// That means that both the rotation (and scale) *and* the translation of M is applied to v.
    /// @param  v    A point.
    /// @return M*v. The w-component of the returned vector is implied to be 1.
    Vector3dT Mul1(const Vector3dT& v) const
    {
        return Vector3dT(m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z + m[0][3],
                         m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z + m[1][3],
                         m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z + m[2][3]);
    }

    // Same as above, but with float.
    /// @param  v    A point.
    /// @return M*v. The w-component of the returned vector is implied to be 1.
    Vector3fT Mul1(const Vector3fT& v) const
    {
        return Vector3fT(float(m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z + m[0][3]),
                         float(m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z + m[1][3]),
                         float(m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z + m[2][3]));
    }

    /// Computes M*v, where M is this matrix.
    /// The w-component of v is assumed to be 1 (v being a point, not a direction vector).
    /// The last row of M is assumed to be (0 0 0 1).
    /// That means that both the rotation (and scale) *and* the translation of M is applied to v.
    /// @param  v    A point.
    /// @return M*v. The w-component of the returned vector is implied to be 1.
    Vector3T<T> Mul_xyz1(const Vector3T<T>& v) const
    {
        return Vector3T<T>(m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z + m[0][3],
                           m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z + m[1][3],
                           m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z + m[2][3]);
    }

    /// Computes M*v, where M is this matrix.
    /// The w-component of v is assumed to be 1 (v being a point, not a direction vector).
    /// The last row of M is assumed to be (0 0 0 1).
    /// That means that both the rotation (and scale) *and* the translation of M is applied to v.
    /// @param  v   A point.
    /// @param  out The result of M*v.
    void Mul_xyz1(const T v[3], T out[3]) const
    {
        // We need the Result variable in case that v == out.
        const T Result[3] = { m[0][0]*v[0] + m[0][1]*v[1] + m[0][2]*v[2] + m[0][3],
                              m[1][0]*v[0] + m[1][1]*v[1] + m[1][2]*v[2] + m[1][3],
                              m[2][0]*v[0] + m[2][1]*v[1] + m[2][2]*v[2] + m[2][3] };

        for (unsigned int i = 0; i < 3; i++) out[i] = Result[i];
    }

    /// Computes M*v, where M is this matrix.
    /// The w-component of v is assumed to be 1 (v being a point, not a direction vector).
    /// The resulting 4-tuple is divided by the w-component so that only the xyz components must be returned.
    /// @param  v    A point.
    /// @return M*v. The w-component of the returned vector is implied to be 1.
    Vector3T<T> ProjectPoint(const Vector3T<T>& v) const
    {
        const Vector3T<T> ProjPoint = Mul1(v);
        const T           v_w       = m[3][0]*v.x + m[3][1]*v.y + m[3][2]*v.z + m[3][3];

        return (v_w != 0) ? scale(ProjPoint, 1 / v_w) : ProjPoint;
    }

    /// Computes M*v, where M is this matrix.
    /// @param v     The four-component vector that is multiplied with M.
    /// @param out   The four-component result vector.
    /// @return The four-component result vector M*v is returned via the out parameter.
    void Mul(const T v[4], T out[4]) const
    {
        // We need the Result variable in case that v == out.
        const T Result[4] = { m[0][0]*v[0] + m[0][1]*v[1] + m[0][2]*v[2] + m[0][3]*v[3],
                              m[1][0]*v[0] + m[1][1]*v[1] + m[1][2]*v[2] + m[1][3]*v[3],
                              m[2][0]*v[0] + m[2][1]*v[1] + m[2][2]*v[2] + m[2][3]*v[3],
                              m[3][0]*v[0] + m[3][1]*v[1] + m[3][2]*v[2] + m[3][3]*v[3] };

        for (unsigned int i = 0; i < 4; i++) out[i] = Result[i];
    }

    /// Returns whether this matrix is equal to Other.
    /// The matrices are considered equal if the element-wise comparison yields no difference larger than Epsilon.
    /// @param Other Matrix to compare to.
    /// @param Epsilon Tolerance value.
    bool IsEqual(const Matrix4x4T& Other, const T Epsilon = 0) const
    {
        for (unsigned int i = 0; i < 4; i++)
            for (unsigned int j = 0; j < 4; j++)
                if (fabs(m[i][j] - Other.m[i][j]) > Epsilon)
                    return false;

        return true;
    }

    /// If this matrix represents a rigid transformation (rotation and translation only, no scale, shear, etc.),
    /// this is a faster shortcut for GetInverse().Mul1(v).
    /// It employs the transpose of the rotational part for inverting the rotation, and properly accounts for the translation.
    /// @param v DOCTODO
    Vector3T<T> InvXForm(Vector3T<T> v) const
    {
        v.x -= m[0][3];
        v.y -= m[1][3];
        v.z -= m[2][3];

        return Vector3T<T>(m[0][0]*v.x + m[1][0]*v.y + m[2][0]*v.z,
                           m[0][1]*v.x + m[1][1]*v.y + m[2][1]*v.z,
                           m[0][2]*v.x + m[1][2]*v.y + m[2][2]*v.z);
    }

    /// Computes the inverse of this matrix.
    /// @param Result Whether the inversion was successful.
    /// @return If this matrix is invertible, the inverse is returned by this method and *Result, if not at NULL, is set to true.
    ///         Otherwise, an undefined matrix is returned and *Result, if not at NULL, is set to false.
    Matrix4x4T GetInverse(bool* Result = NULL) const;

    /// Returns the transpose of this matrix.
    /// @return The transpose of this matrix.
    Matrix4x4T GetTranspose() const;



    /// The matrix values.
    T m[4][4];
};


/// Typedef for a Matrix4x4T of floats.
typedef Matrix4x4T<float> Matrix4x4fT;
typedef Matrix4x4T<float> MatrixT;      // The original MatrixT type for backwards-compatibility with old code.

/// Typedef for a Matrix4x4T of doubles.
typedef Matrix4x4T<double> Matrix4x4dT;

#endif
