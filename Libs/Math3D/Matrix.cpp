/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Matrix.hpp"
#include "Quaternion.hpp"


template<class T> Matrix4x4T<T>::Matrix4x4T(const Vector3T<T>& t, const cf::math::QuaternionT<T>& q, const Vector3T<T>& s)
{
    const T x = q.x;
    const T y = q.y;
    const T z = q.z;
    const T w = q.w;

    m[0][0] = 1 - 2*y*y - 2*z*z;  m[0][1] =     2*x*y - 2*w*z;  m[0][2] =     2*x*z + 2*w*y;  m[0][3] = t.x;
    m[1][0] =     2*x*y + 2*w*z;  m[1][1] = 1 - 2*x*x - 2*z*z;  m[1][2] =     2*y*z - 2*w*x;  m[1][3] = t.y;
    m[2][0] =     2*x*z - 2*w*y;  m[2][1] =     2*y*z + 2*w*x;  m[2][2] = 1 - 2*x*x - 2*y*y;  m[2][3] = t.z;
    m[3][0] =                 0;  m[3][1] =                 0;  m[3][2] =                 0;  m[3][3] =   1;

    for (unsigned int j = 0; j < 3; j++)
        if (s[j] != 1)
            for (unsigned int i = 0; i < 3; i++)
                m[i][j] *= s[j];
}


template<class T> Matrix4x4T<T> Matrix4x4T<T>::GetProjOrthoMatrix(T left, T right, T bottom, T top, T zNear, T zFar)
{
    return Matrix4x4T(
        2 / (right - left),                  0,                   0, -(right + left) / (right - left),
                         0, 2 / (top - bottom),                   0, -(top + bottom) / (top - bottom),
                         0,                  0, -2 / (zFar - zNear), -(zFar + zNear) / (zFar - zNear),
                         0,                  0,                   0,                                1);
}


template<class T> Matrix4x4T<T> Matrix4x4T<T>::GetProjFrustumMatrix(T left, T right, T bottom, T top, T zNear, T zFar)
{
    const T x = (2*zNear) / (right - left);
    const T y = (2*zNear) / (top - bottom);
    const T a = (right + left) / (right - left);
    const T b = (top + bottom) / (top - bottom);

    // If zFar <= zNear, the far plane is assumed to be at infinity (a useful special case for stencil shadow projections).
    const T c = (zNear < zFar) ? -(zFar + zNear) / (zFar - zNear) : -1;
    const T d = (zNear < zFar) ? -(2*zFar*zNear) / (zFar - zNear) : -2*zNear;

    return Matrix4x4T(
        x, 0,  a, 0,
        0, y,  b, 0,
        0, 0,  c, d,
        0, 0, -1, 0);
}


template<class T> Matrix4x4T<T> Matrix4x4T<T>::GetProjPerspectiveMatrix(T fovY, T aspect, T zNear, T zFar)
{
    if (zFar <= zNear)
    {
        // If zFar <= zNear, the far plane is assumed to be at infinity (a useful special case for stencil shadow projections).
        // This code is also used in <http://trac.cafu.de/browser/cafu/trunk/Libs/OpenGL/OpenGLWindow.cpp?rev=100#L137>.
        const T cotanFovY = 1 / tan(fovY * T(3.14159265358979323846 / 360.0));

        return Matrix4x4T(
            cotanFovY / aspect,         0,  0,          0,
                             0, cotanFovY,  0,          0,
                             0,         0, -1, -2 * zNear,
                             0,         0, -1,          0);
    }

    const T ymax =  zNear * tan(fovY * T(3.14159265358979323846 / 360.0));
    const T ymin = -ymax;
    const T xmin =  ymin * aspect;
    const T xmax =  ymax * aspect;

    return GetProjFrustumMatrix(xmin, xmax, ymin, ymax, zNear, zFar);
}


template<class T> Matrix4x4T<T> Matrix4x4T<T>::GetProjPickMatrix(T x, T y, T width, T height, int viewport[4])
{
    // See the OpenGL Programming Guide for a description and CaWE for an example for how the pick matrix is used.
    const T sx = viewport[2] / width;
    const T sy = viewport[3] / height;
    const T tx = (viewport[2] + 2*(viewport[0] - x)) / width;
    const T ty = (viewport[3] + 2*(viewport[1] - y)) / height;

    return Matrix4x4T(sx,  0, 0, tx,
                       0, sy, 0, ty,
                       0,  0, 1,  0,
                       0,  0, 0,  1);
}


template<class T> Matrix4x4T<T> Matrix4x4T<T>::GetTranslateMatrix(const Vector3T<T>& t)
{
    Matrix4x4T M;

    M.m[0][3] = t.x;
    M.m[1][3] = t.y;
    M.m[2][3] = t.z;

    return M;
}


template<class T> Matrix4x4T<T> Matrix4x4T<T>::GetScaleMatrix(T sx, T sy, T sz)
{
    Matrix4x4T M;

    M.m[0][0] = sx;
    M.m[1][1] = sy;
    M.m[2][2] = sz;

    return M;
}


template<class T> Matrix4x4T<T> Matrix4x4T<T>::GetRotateXMatrix(T Angle)
{
    Matrix4x4T M;

    const T RadAngle = Angle * T(3.14159265358979323846 / 180.0);
    const T SinAngle = sin(RadAngle);
    const T CosAngle = cos(RadAngle);

    M.m[1][1] = CosAngle; M.m[1][2] = -SinAngle;
    M.m[2][1] = SinAngle; M.m[2][2] =  CosAngle;

    return M;
}


template<class T> Matrix4x4T<T> Matrix4x4T<T>::GetRotateYMatrix(T Angle)
{
    Matrix4x4T M;

    const T RadAngle = Angle * T(3.14159265358979323846 / 180.0);
    const T SinAngle = sin(RadAngle);
    const T CosAngle = cos(RadAngle);

    M.m[0][0] =  CosAngle; M.m[0][2] = SinAngle;
    M.m[2][0] = -SinAngle; M.m[2][2] = CosAngle;

    return M;
}


template<class T> Matrix4x4T<T> Matrix4x4T<T>::GetRotateZMatrix(T Angle)
{
    Matrix4x4T M;

    const T RadAngle = Angle * T(3.14159265358979323846 / 180.0);
    const T SinAngle = sin(RadAngle);
    const T CosAngle = cos(RadAngle);

    M.m[0][0] = CosAngle; M.m[0][1] = -SinAngle;
    M.m[1][0] = SinAngle; M.m[1][1] =  CosAngle;

    return M;
}


template<class T> Matrix4x4T<T> Matrix4x4T<T>::GetRotateMatrix(T Angle, const Vector3T<T>& Axis)
{
    const T RadAngle = Angle * T(3.14159265358979323846 / 180.0);
    const T s        = sin(RadAngle);
    const T c        = cos(RadAngle);
    const T t        = 1 - c;

    const T tx = t * Axis.x;
    const T ty = t * Axis.y;
    const T tz = t * Axis.z;

    const T sx = s * Axis.x;
    const T sy = s * Axis.y;
    const T sz = s * Axis.z;

    return Matrix4x4T(tx*Axis.x +  c, tx*Axis.y - sz, tx*Axis.z + sy, 0,
                      tx*Axis.y + sz, ty*Axis.y +  c, ty*Axis.z - sx, 0,
                      tx*Axis.z - sy, ty*Axis.z + sx, tz*Axis.z +  c, 0,
                                   0,              0,              0, 1);
}


template<class T> Matrix4x4T<T> Matrix4x4T<T>::operator * (const Matrix4x4T<T>& vm) const
{
    return Matrix4x4T(
        m[0][0]*vm.m[0][0] + m[0][1]*vm.m[1][0] + m[0][2]*vm.m[2][0] + m[0][3]*vm.m[3][0],
        m[0][0]*vm.m[0][1] + m[0][1]*vm.m[1][1] + m[0][2]*vm.m[2][1] + m[0][3]*vm.m[3][1],
        m[0][0]*vm.m[0][2] + m[0][1]*vm.m[1][2] + m[0][2]*vm.m[2][2] + m[0][3]*vm.m[3][2],
        m[0][0]*vm.m[0][3] + m[0][1]*vm.m[1][3] + m[0][2]*vm.m[2][3] + m[0][3]*vm.m[3][3],

        m[1][0]*vm.m[0][0] + m[1][1]*vm.m[1][0] + m[1][2]*vm.m[2][0] + m[1][3]*vm.m[3][0],
        m[1][0]*vm.m[0][1] + m[1][1]*vm.m[1][1] + m[1][2]*vm.m[2][1] + m[1][3]*vm.m[3][1],
        m[1][0]*vm.m[0][2] + m[1][1]*vm.m[1][2] + m[1][2]*vm.m[2][2] + m[1][3]*vm.m[3][2],
        m[1][0]*vm.m[0][3] + m[1][1]*vm.m[1][3] + m[1][2]*vm.m[2][3] + m[1][3]*vm.m[3][3],

        m[2][0]*vm.m[0][0] + m[2][1]*vm.m[1][0] + m[2][2]*vm.m[2][0] + m[2][3]*vm.m[3][0],
        m[2][0]*vm.m[0][1] + m[2][1]*vm.m[1][1] + m[2][2]*vm.m[2][1] + m[2][3]*vm.m[3][1],
        m[2][0]*vm.m[0][2] + m[2][1]*vm.m[1][2] + m[2][2]*vm.m[2][2] + m[2][3]*vm.m[3][2],
        m[2][0]*vm.m[0][3] + m[2][1]*vm.m[1][3] + m[2][2]*vm.m[2][3] + m[2][3]*vm.m[3][3],

        m[3][0]*vm.m[0][0] + m[3][1]*vm.m[1][0] + m[3][2]*vm.m[2][0] + m[3][3]*vm.m[3][0],
        m[3][0]*vm.m[0][1] + m[3][1]*vm.m[1][1] + m[3][2]*vm.m[2][1] + m[3][3]*vm.m[3][1],
        m[3][0]*vm.m[0][2] + m[3][1]*vm.m[1][2] + m[3][2]*vm.m[2][2] + m[3][3]*vm.m[3][2],
        m[3][0]*vm.m[0][3] + m[3][1]*vm.m[1][3] + m[3][2]*vm.m[2][3] + m[3][3]*vm.m[3][3]);
}


template<class T> void Matrix4x4T<T>::Translate_MT(const Vector3dT& vTrans)
{
    const Vector3dT t = Mul1(vTrans);

    m[0][3] = T(t.x);
    m[1][3] = T(t.y);
    m[2][3] = T(t.z);
}


template<class T> void Matrix4x4T<T>::Translate_MT(T tx, T ty, T tz)
{
    m[0][3] = m[0][0]*tx + m[0][1]*ty + m[0][2]*tz + m[0][3];
    m[1][3] = m[1][0]*tx + m[1][1]*ty + m[1][2]*tz + m[1][3];
    m[2][3] = m[2][0]*tx + m[2][1]*ty + m[2][2]*tz + m[2][3];
}


template<class T> void Matrix4x4T<T>::Translate_TM(const Vector3dT& vTrans)
{
    m[0][3] += T(vTrans.x);
    m[1][3] += T(vTrans.y);
    m[2][3] += T(vTrans.z);
}


template<class T> void Matrix4x4T<T>::Scale_MS(T sx, T sy, T sz)
{
    for (unsigned int i = 0; i < 4; i++)
    {
        m[i][0] *= sx;
        m[i][1] *= sy;
        m[i][2] *= sz;
    }
}


template<class T> void Matrix4x4T<T>::Scale_SM(T sx, T sy, T sz)
{
    for (unsigned int j = 0; j < 4; j++)
    {
        m[0][j] *= sx;
        m[1][j] *= sy;
        m[2][j] *= sz;
    }
}


template<class T> void Matrix4x4T<T>::RotateX_MR(T Angle)
{
    const T RadAngle = Angle * T(3.14159265358979323846 / 180.0);
    const T SinAngle = sin(RadAngle);
    const T CosAngle = cos(RadAngle);

    for (unsigned int i = 0; i < 4; i++)
    {
        const T y =  CosAngle*m[i][1] + SinAngle*m[i][2];
        const T z = -SinAngle*m[i][1] + CosAngle*m[i][2];

        m[i][1] = y;
        m[i][2] = z;
    }
}


template<class T> void Matrix4x4T<T>::RotateX_RM(T Angle)
{
    const T RadAngle = Angle * T(3.14159265358979323846 / 180.0);
    const T SinAngle = sin(RadAngle);
    const T CosAngle = cos(RadAngle);

    for (unsigned int j = 0; j < 4; j++)
    {
        const T a = CosAngle*m[1][j] - SinAngle*m[2][j];
        const T b = SinAngle*m[1][j] + CosAngle*m[2][j];

        m[1][j] = a;
        m[2][j] = b;
    }
}


template<class T> void Matrix4x4T<T>::RotateY_MR(T Angle)
{
    const T RadAngle = Angle * T(3.14159265358979323846 / 180.0);
    const T SinAngle = sin(RadAngle);
    const T CosAngle = cos(RadAngle);

    for (unsigned int i = 0; i < 4; i++)
    {
        const T x = CosAngle*m[i][0] - SinAngle*m[i][2];
        const T z = SinAngle*m[i][0] + CosAngle*m[i][2];

        m[i][0] = x;
        m[i][2] = z;
    }
}


template<class T> void Matrix4x4T<T>::RotateY_RM(T Angle)
{
    const T RadAngle = Angle * T(3.14159265358979323846 / 180.0);
    const T SinAngle = sin(RadAngle);
    const T CosAngle = cos(RadAngle);

    for (unsigned int j = 0; j < 4; j++)
    {
        const T a =  CosAngle*m[0][j] + SinAngle*m[2][j];
        const T b = -SinAngle*m[0][j] + CosAngle*m[2][j];

        m[0][j] = a;
        m[2][j] = b;
    }
}


template<class T> void Matrix4x4T<T>::RotateZ_MR(T Angle)
{
    const T RadAngle = Angle * T(3.14159265358979323846 / 180.0);
    const T SinAngle = sin(RadAngle);
    const T CosAngle = cos(RadAngle);

    for (unsigned int i = 0; i < 4; i++)
    {
        const T x =  CosAngle*m[i][0] + SinAngle*m[i][1];
        const T y = -SinAngle*m[i][0] + CosAngle*m[i][1];

        m[i][0] = x;
        m[i][1] = y;
    }
}


template<class T> void Matrix4x4T<T>::RotateZ_RM(T Angle)
{
    const T RadAngle = Angle * T(3.14159265358979323846 / 180.0);
    const T SinAngle = sin(RadAngle);
    const T CosAngle = cos(RadAngle);

    for (unsigned int j = 0; j < 4; j++)
    {
        const T a = CosAngle*m[0][j] - SinAngle*m[1][j];
        const T b = SinAngle*m[0][j] + CosAngle*m[1][j];

        m[0][j] = a;
        m[1][j] = b;
    }
}


template<class T> Matrix4x4T<T> Matrix4x4T<T>::GetInverse(bool* Result) const
{
    // How it's done:
    // AX = I
    // A = this
    // X = the matrix we're looking for
    // I = identity
    T   mat[4][8];
    int rowMap[4];

    if (Result != NULL) Result[0] = false;

    // Copy this matrix into the left half of mat and the identity matrix into the right half, so that "mat=AI".
    for (unsigned int i = 0; i < 4; i++)
    {
        const T* pIn  = m[i];
        T*       pOut = mat[i];

        for (unsigned int j = 0; j < 4; j++)
        {
            pOut[j    ] = pIn[j];
            pOut[j + 4] = (i == j) ? T(1.0) : T(0.0);
        }

        rowMap[i] = i;
    }

    // Use row operations to get to reduced row-echelon form using these rules:
    // 1. Multiply or divide a row by a nonzero number.
    // 2. Add a multiple of one row to another.
    // 3. Interchange two rows.
    for (unsigned int iRow = 0; iRow < 4; iRow++)
    {
        // Find the row with the largest element in this column.
        T   fLargest = T(0.001);
        int iLargest = -1;

        for (int iTest = iRow; iTest < 4; iTest++)
        {
            const T fTest = fabs(mat[rowMap[iTest]][iRow]);

            if (fTest > fLargest)
            {
                iLargest = iTest;
                fLargest = fTest;
            }
        }

        // They're all too small.. sorry.
        if (iLargest == -1) return Matrix4x4T();

        // Swap the rows.
        const int iTemp = rowMap[iLargest];
        rowMap[iLargest] = rowMap[iRow];
        rowMap[iRow] = iTemp;

        T* pRow = mat[rowMap[iRow]];

        // Divide this row by the element.
        const T mul = 1 / pRow[iRow];
        for (unsigned int j = 0; j < 8; j++) pRow[j] *= mul;
        pRow[iRow] = 1;   // Preserve accuracy...

        // Eliminate this element from the other rows using operation 2.
        for (unsigned int i = 0; i < 4; i++)
        {
            if (i == iRow) continue;

            T* pScaleRow = mat[rowMap[i]];

            // Multiply this row by -(iRow*the element).
            const T mul_ = -pScaleRow[iRow];
            for (unsigned int j = 0; j < 8; j++) pScaleRow[j] += pRow[j]*mul_;
            pScaleRow[iRow] = 0;    // Preserve accuracy...
        }
    }

    // The inverse is on the right side of AX now (the identity is on the left).
    Matrix4x4T dst;

    for (unsigned int i = 0; i < 4; i++)
    {
        const T* pIn  = mat[rowMap[i]] + 4;
        T*       pOut = dst.m[i];

        for (unsigned int j = 0; j < 4; j++) pOut[j] = pIn[j];
    }

    if (Result != NULL) Result[0] = true;
    return dst;
}


template<class T> Matrix4x4T<T> Matrix4x4T<T>::GetTranspose() const
{
    Matrix4x4T mt;

    for (unsigned int i = 0; i < 4; i++)
        for (unsigned int j = 0; j < 4; j++)
            mt.m[i][j] = m[j][i];

    return mt;
}


template class Matrix4x4T<float>;
template class Matrix4x4T<double>;
