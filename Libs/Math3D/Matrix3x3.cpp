/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Matrix3x3.hpp"
#include "Angles.hpp"
#include "Quaternion.hpp"


using namespace cf::math;


// The static identity instance of this matrix type.
template<class T> const Matrix3x3T<T> Matrix3x3T<T>::Identity;


template<class T> Matrix3x3T<T>::Matrix3x3T(const AnglesT<T>& Angles)
{
    const T RadH = AnglesT<T>::DegToRad(Angles.yaw());
    const T SinH = sin(RadH);
    const T CosH = cos(RadH);

    const T RadP = AnglesT<T>::DegToRad(Angles.pitch());
    const T SinP = sin(RadP);
    const T CosP = cos(RadP);

    const T RadB = AnglesT<T>::DegToRad(Angles.roll());
    const T SinB = sin(RadB);
    const T CosB = cos(RadB);

    // Refer to http://de.wikipedia.org/wiki/Rotationsmatrix for the correct start matrices.
    m[0][0] = CosP*CosH;  m[0][1] = SinB*SinP*CosH - CosB*SinH;  m[0][2] = CosB*SinP*CosH + SinB*SinH;
    m[1][0] = CosP*SinH;  m[1][1] = SinB*SinP*SinH + CosB*CosH;  m[1][2] = CosB*SinP*SinH - SinB*CosH;
    m[2][0] = -SinP;      m[2][1] = SinB*CosP;                   m[2][2] = CosB*CosP;

    // This is not a check for accuracy, but for principal correctness of the above computations.
    assert(this->IsEqual(
        GetRotateZMatrix(Angles.yaw()) *
        GetRotateYMatrix(Angles.pitch()) *
        GetRotateXMatrix(Angles.roll()),
        0.001f));
}


template<class T> Matrix3x3T<T>::Matrix3x3T(const QuaternionT<T>& Quat)
{
    // This is the same code as in the related MatrixT constructor.
    const T x = Quat.x;
    const T y = Quat.y;
    const T z = Quat.z;
    const T w = Quat.w;

    m[0][0] = 1 - 2 * y * y - 2 * z * z;   m[0][1] =     2 * x * y - 2 * w * z;   m[0][2] =     2 * x * z + 2 * w * y;
    m[1][0] =     2 * x * y + 2 * w * z;   m[1][1] = 1 - 2 * x * x - 2 * z * z;   m[1][2] =     2 * y * z - 2 * w * x;
    m[2][0] =     2 * x * z - 2 * w * y;   m[2][1] =     2 * y * z + 2 * w * x;   m[2][2] = 1 - 2 * x * x - 2 * y * y;
}


template<class T> Matrix3x3T<T> Matrix3x3T<T>::GetScaleMatrix(T sx, T sy, T sz)
{
    Matrix3x3T<T> M;

    M.m[0][0]=sx;
    M.m[1][1]=sy;
    M.m[2][2]=sz;

    return M;
}


template<class T> Matrix3x3T<T> Matrix3x3T<T>::GetRotateXMatrix(T Angle)
{
    Matrix3x3T<T> M;

    const T RadAngle=AnglesT<T>::DegToRad(Angle);
    const T SinAngle=sin(RadAngle);
    const T CosAngle=cos(RadAngle);

    // Refer to http://de.wikipedia.org/wiki/Rotationsmatrix especially for the correctness of the signs.
    M.m[1][1]=CosAngle; M.m[1][2]=-SinAngle;
    M.m[2][1]=SinAngle; M.m[2][2]= CosAngle;

    return M;
}


template<class T> Matrix3x3T<T> Matrix3x3T<T>::GetRotateYMatrix(T Angle)
{
    Matrix3x3T<T> M;

    const T RadAngle=AnglesT<T>::DegToRad(Angle);
    const T SinAngle=sin(RadAngle);
    const T CosAngle=cos(RadAngle);

    // Refer to http://de.wikipedia.org/wiki/Rotationsmatrix especially for the correctness of the signs.
    M.m[0][0]= CosAngle; M.m[0][2]=SinAngle;
    M.m[2][0]=-SinAngle; M.m[2][2]=CosAngle;

    return M;
}


template<class T> Matrix3x3T<T> Matrix3x3T<T>::GetRotateZMatrix(T Angle)
{
    Matrix3x3T<T> M;

    const T RadAngle=AnglesT<T>::DegToRad(Angle);
    const T SinAngle=sin(RadAngle);
    const T CosAngle=cos(RadAngle);

    // Refer to http://de.wikipedia.org/wiki/Rotationsmatrix especially for the correctness of the signs.
    M.m[0][0]=CosAngle; M.m[0][1]=-SinAngle;
    M.m[1][0]=SinAngle; M.m[1][1]= CosAngle;

    return M;
}


template<class T> Matrix3x3T<T> Matrix3x3T<T>::GetRotateMatrix(T Angle, const Vector3T<T>& Axis)
{
    const T RadAngle=AnglesT<T>::DegToRad(Angle);
    const T s       =sin(RadAngle);
    const T c       =cos(RadAngle);
    const T t       =T(1.0)-c;

    const T Ax=Axis.x;
    const T Ay=Axis.y;
    const T Az=Axis.z;

    const T tx=t*Ax; const T ty=t*Ay; const T tz=t*Az;
    const T sx=s*Ax; const T sy=s*Ay; const T sz=s*Az;

    return Matrix3x3T<T>(tx*Ax +  c, tx*Ay - sz, tx*Az + sy,
                         tx*Ay + sz, ty*Ay +  c, ty*Az - sx,
                         tx*Az - sy, ty*Az + sx, tz*Az +  c);
}


// This code is kept for compatibility reasons with old CaWE code.
// It rotates around the axes in a different order than the Matrix3x3T(const AnglesT<T>& Angles) ctor above.
template<class T> Matrix3x3T<T> Matrix3x3T<T>::GetFromAngles_COMPAT(const AnglesT<T>& Angles)
{
    // !!!
    // !!!  This method is only for compatibility with old code in CaWE!
    // !!!  It uses a different understanding for the axes than what is defined at the AnglesT class
    // !!!  (i.e. y and z swapped, z negated) and should therefore NOT be used in new code,
    // !!!  and eventually be replaced by equivalent code that implements the proper axes orientations!
    // !!!
    // !!!  (Funnily enough, it seems to turn out that this is not so old and obsolete as initially thought,
    // !!!   it's only the order or names of the *angles* that is different from our new, "canonical"
    // !!!   method, not the order or orientation or something of the actual *transformations*!)
    // !!!
    enum { PITCH=0, YAW, ROLL };

    const T RadH=AnglesT<T>::DegToRad(Angles[YAW]);
    const T SinH=sin(RadH);
    const T CosH=cos(RadH);

    const T RadP=AnglesT<T>::DegToRad(Angles[PITCH]);
    const T SinP=sin(RadP);
    const T CosP=cos(RadP);

    const T RadB=AnglesT<T>::DegToRad(Angles[ROLL]);
    const T SinB=sin(RadB);
    const T CosB=cos(RadB);

    return Matrix3x3T<T>(CosP*CosH,   SinB*SinP*CosH + CosB*-SinH,   CosB*SinP*CosH + -SinB*-SinH,
                         CosP*SinH,   SinB*SinP*SinH + CosB*CosH,    CosB*SinP*SinH + -SinB*CosH,
                        -SinP,        SinB*CosP,                     CosB*CosP);
}


template<class T> Matrix3x3T<T> Matrix3x3T<T>::operator * (const Matrix3x3T<T>& vm) const
{
    return Matrix3x3T<T>(
        m[0][0]*vm.m[0][0] + m[0][1]*vm.m[1][0] + m[0][2]*vm.m[2][0],
        m[0][0]*vm.m[0][1] + m[0][1]*vm.m[1][1] + m[0][2]*vm.m[2][1],
        m[0][0]*vm.m[0][2] + m[0][1]*vm.m[1][2] + m[0][2]*vm.m[2][2],

        m[1][0]*vm.m[0][0] + m[1][1]*vm.m[1][0] + m[1][2]*vm.m[2][0],
        m[1][0]*vm.m[0][1] + m[1][1]*vm.m[1][1] + m[1][2]*vm.m[2][1],
        m[1][0]*vm.m[0][2] + m[1][1]*vm.m[1][2] + m[1][2]*vm.m[2][2],

        m[2][0]*vm.m[0][0] + m[2][1]*vm.m[1][0] + m[2][2]*vm.m[2][0],
        m[2][0]*vm.m[0][1] + m[2][1]*vm.m[1][1] + m[2][2]*vm.m[2][1],
        m[2][0]*vm.m[0][2] + m[2][1]*vm.m[1][2] + m[2][2]*vm.m[2][2]);
}


template<class T> void Matrix3x3T<T>::Scale_MS(T sx, T sy, T sz)
{
    for (unsigned long i=0; i<3; i++)
    {
        m[i][0]*=sx;
        m[i][1]*=sy;
        m[i][2]*=sz;
    }
}


template<class T> void Matrix3x3T<T>::Scale_SM(T sx, T sy, T sz)
{
    for (unsigned long j=0; j<3; j++)
    {
        m[0][j]*=sx;
        m[1][j]*=sy;
        m[2][j]*=sz;
    }
}


template<class T> void Matrix3x3T<T>::RotateX_MR(T Angle)
{
    const T RadAngle=AnglesT<T>::DegToRad(Angle);
    const T SinAngle=sin(RadAngle);
    const T CosAngle=cos(RadAngle);

    for (unsigned long i=0; i<3; i++)
    {
        const T y= CosAngle*m[i][1] + SinAngle*m[i][2];
        const T z=-SinAngle*m[i][1] + CosAngle*m[i][2];

        m[i][1]=y;
        m[i][2]=z;
    }
}


template<class T> void Matrix3x3T<T>::RotateX_RM(T Angle)
{
    const T RadAngle=AnglesT<T>::DegToRad(Angle);
    const T SinAngle=sin(RadAngle);
    const T CosAngle=cos(RadAngle);

    for (unsigned long j=0; j<3; j++)
    {
        const T a=CosAngle*m[1][j] - SinAngle*m[2][j];
        const T b=SinAngle*m[1][j] + CosAngle*m[2][j];

        m[1][j]=a;
        m[2][j]=b;
    }
}


template<class T> void Matrix3x3T<T>::RotateY_MR(T Angle)
{
    const T RadAngle=AnglesT<T>::DegToRad(Angle);
    const T SinAngle=sin(RadAngle);
    const T CosAngle=cos(RadAngle);

    for (unsigned long i=0; i<3; i++)
    {
        const T x=CosAngle*m[i][0] - SinAngle*m[i][2];
        const T z=SinAngle*m[i][0] + CosAngle*m[i][2];

        m[i][0]=x;
        m[i][2]=z;
    }
}


template<class T> void Matrix3x3T<T>::RotateY_RM(T Angle)
{
    const T RadAngle=AnglesT<T>::DegToRad(Angle);
    const T SinAngle=sin(RadAngle);
    const T CosAngle=cos(RadAngle);

    for (unsigned long j=0; j<3; j++)
    {
        const T a= CosAngle*m[0][j] + SinAngle*m[2][j];
        const T b=-SinAngle*m[0][j] + CosAngle*m[2][j];

        m[0][j]=a;
        m[2][j]=b;
    }
}


template<class T> void Matrix3x3T<T>::RotateZ_MR(T Angle)
{
    const T RadAngle=AnglesT<T>::DegToRad(Angle);
    const T SinAngle=sin(RadAngle);
    const T CosAngle=cos(RadAngle);

    for (unsigned long i=0; i<3; i++)
    {
        const T x= CosAngle*m[i][0] + SinAngle*m[i][1];
        const T y=-SinAngle*m[i][0] + CosAngle*m[i][1];

        m[i][0]=x;
        m[i][1]=y;
    }
}


template<class T> void Matrix3x3T<T>::RotateZ_RM(T Angle)
{
    const T RadAngle=AnglesT<T>::DegToRad(Angle);
    const T SinAngle=sin(RadAngle);
    const T CosAngle=cos(RadAngle);

    for (unsigned long j=0; j<3; j++)
    {
        const T a=CosAngle*m[0][j] - SinAngle*m[1][j];
        const T b=SinAngle*m[0][j] + CosAngle*m[1][j];

        m[0][j]=a;
        m[1][j]=b;
    }
}


template<class T> Matrix3x3T<T> Matrix3x3T<T>::GetInverse(bool* Result) const
{
    // How it's done:
    // AX = I
    // A = this
    // X = the matrix we're looking for
    // I = identity
    T   mat[3][6];
    int rowMap[3];

    if (Result!=NULL) Result[0]=false;

    // Copy this matrix into the left half of mat and the identity matrix into the right half, so that "mat=AI".
    for (unsigned long i=0; i<3; i++)
    {
        const T* pIn =m[i];
        T*       pOut=mat[i];

        for (unsigned long j=0; j<3; j++)
        {
            pOut[j  ]=pIn[j];
            pOut[j+3]=(i==j) ? T(1.0) : T(0.0);
        }

        rowMap[i]=i;
    }

    // Use row operations to get to reduced row-echelon form using these rules:
    // 1. Multiply or divide a row by a nonzero number.
    // 2. Add a multiple of one row to another.
    // 3. Interchange two rows.
    for (int iRow=0; iRow<3; iRow++)
    {
        // Find the row with the largest element in this column.
        T   fLargest = T(0.001);
        int iLargest = -1;

        for (int iTest=iRow; iTest<3; iTest++)
        {
            const T fTest=fabs(mat[rowMap[iTest]][iRow]);

            if (fTest>fLargest)
            {
                iLargest = iTest;
                fLargest = fTest;
            }
        }

        // They're all too small.. sorry.
        if (iLargest==-1) return Matrix3x3T();

        // Swap the rows.
        int iTemp = rowMap[iLargest];
        rowMap[iLargest] = rowMap[iRow];
        rowMap[iRow] = iTemp;

        T* pRow = mat[rowMap[iRow]];

        // Divide this row by the element.
        const T mul=T(1.0)/pRow[iRow];
        for (int j=0; j<6; j++) pRow[j]*=mul;
        pRow[iRow]=T(1.0);    // Preserve accuracy...

        // Eliminate this element from the other rows using operation 2.
        for (int i=0; i<3; i++)
        {
            if (i==iRow) continue;

            T* pScaleRow = mat[rowMap[i]];

            // Multiply this row by -(iRow*the element).
            const T mul_=-pScaleRow[iRow];
            for (int j=0; j<6; j++) pScaleRow[j]+=pRow[j]*mul_;
            pScaleRow[iRow]=0.0;   // Preserve accuracy...
        }
    }

    // The inverse is on the right side of AX now (the identity is on the left).
    Matrix3x3T<T> dst;

    for (int i=0; i<3; i++)
    {
        const T* pIn =mat[rowMap[i]]+3;
        T*       pOut=dst.m[i];

        for (int j=0; j<3; j++) pOut[j]=pIn[j];
    }

    if (Result!=NULL) Result[0]=true;
    return dst;
}


template<class T> Matrix3x3T<T> Matrix3x3T<T>::GetTranspose() const
{
    Matrix3x3T<T> mt;

    for (unsigned long i=0; i<3; i++)
        for (unsigned long j=0; j<3; j++)
            mt.m[i][j]=m[j][i];

    return mt;
}


template<class T> AnglesT<T> Matrix3x3T<T>::ToAngles_COMPAT() const
{
    // !!!
    // !!!  This method is only for compatibility with old code in CaWE!
    // !!!  It uses a different understanding for the axes than what is defined at the AnglesT class
    // !!!  (i.e. y and z swapped, z negated) and should therefore NOT be used in new code,
    // !!!  and eventually be replaced by equivalent code that implements the proper axes orientations!
    // !!!
    enum { PITCH=0, YAW, ROLL };

    // Extract the basis vectors from the matrix.
    const Vector3T<T> forward(m[0][0], m[1][0], m[2][0]);   // First  column, x-axis.
    const Vector3T<T> left   (m[0][1], m[1][1], m[2][1]);   // Second column.
    const Vector3T<T> up     (m[0][2], m[1][2], m[2][2]);   // Third  column.

    const T    xyDist=sqrt(forward[0]*forward[0] + forward[1]*forward[1]);
    AnglesT<T> Angles;

    if (xyDist>0.001f)
    {
        Angles[YAW  ]=AnglesT<T>::RadToDeg(atan2( forward[1], forward[0]));
        Angles[PITCH]=AnglesT<T>::RadToDeg(atan2(-forward[2], xyDist));
        Angles[ROLL ]=AnglesT<T>::RadToDeg(atan2( left[2], up[2]));
    }
    else
    {
        // The forward vector points mostly along the z-axis,
        // which means that we have a Gimbal Lock, http://de.wikipedia.org/wiki/Gimbal_Lock
        Angles[YAW  ]=AnglesT<T>::RadToDeg(atan2(-left[0], left[1]));
        Angles[PITCH]=AnglesT<T>::RadToDeg(atan2(-forward[2], xyDist));
        Angles[ROLL ]=0;    // No roll, because one degree of freedom has been lost (that is, yaw==roll).
    }

    assert(GetFromAngles_COMPAT(Angles).IsEqual(*this, 0.001f));
    return Angles;
}


template class Matrix3x3T<float>;
template class Matrix3x3T<double>;
