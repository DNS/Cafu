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

/******************/
/*** 4x4 Matrix ***/
/******************/

#include "Matrix.hpp"

#if defined(_WIN32) && defined (_MSC_VER)
    #if (_MSC_VER<1300)
        #define for if (false) ; else for
    #endif
#endif


MatrixT MatrixT::GetProjOrthoMatrix(float left, float right, float bottom, float top, float zNear, float zFar)
{
    return MatrixT(
        2.0f/(right-left),              0.0f,               0.0f, -(right+left)/(right-left),
                     0.0f, 2.0f/(top-bottom),               0.0f, -(top+bottom)/(top-bottom),
                     0.0f,              0.0f, -2.0f/(zFar-zNear), -(zFar+zNear)/(zFar-zNear),
                     0.0f,              0.0f,               0.0f,                       1.0f);
}


MatrixT MatrixT::GetProjFrustumMatrix(float left, float right, float bottom, float top, float zNear, float zFar)
{
    const float x= (2.0f*zNear)/(right-left);
    const float y= (2.0f*zNear)/(top-bottom);
    const float a= (right+left)/(right-left);
    const float b= (top+bottom)/(top-bottom);
    const float c=-(zFar+zNear)/( zFar-zNear);
    const float d=-(2.0f*zFar*zNear)/(zFar-zNear);

    return MatrixT(
        x,    0.0f, a,     0.0f,
        0.0f, y,    b,     0.0f,
        0.0f, 0.0f, c,     d,
        0.0f, 0.0f, -1.0f, 0.0f);
}


MatrixT MatrixT::GetProjPerspectiveMatrix(float fovY, float aspect, float zNear, float zFar)
{
    const float ymax= zNear*float(tan(fovY*3.14159265358979323846f/360.0f));
    const float ymin=-ymax;
    const float xmin= ymin*aspect;
    const float xmax= ymax*aspect;

    return GetProjFrustumMatrix(xmin, xmax, ymin, ymax, zNear, zFar);
}


MatrixT MatrixT::GetProjPickMatrix(float x, float y, float width, float height, int viewport[4])
{
    // See the OpenGL Programming Guide for a description and CaWE for an example for how the pick matrix is used.
    const float sx=viewport[2] / width;
    const float sy=viewport[3] / height;
    const float tx=(viewport[2] + 2.0f * (viewport[0] - x)) / width;
    const float ty=(viewport[3] + 2.0f * (viewport[1] - y)) / height;

    return MatrixT( sx, 0.0, 0.0f,   tx,
                   0.0,  sy, 0.0f,   ty,
                   0.0, 0.0, 1.0f, 0.0f,
                   0.0, 0.0, 0.0f, 1.0f);
}


MatrixT MatrixT::GetTranslateMatrix(const Vector3fT& t)
{
    MatrixT M;

    M.m[0][3]=t.x;
    M.m[1][3]=t.y;
    M.m[2][3]=t.z;

    return M;
}


MatrixT MatrixT::GetScaleMatrix(float sx, float sy, float sz)
{
    MatrixT M;

    M.m[0][0]=sx;
    M.m[1][1]=sy;
    M.m[2][2]=sz;

    return M;
}


MatrixT MatrixT::GetRotateXMatrix(float Angle)
{
    MatrixT M;

    const float RadAngle=Angle*3.14159265358979323846f/180.0f;
    const float SinAngle=float(sin(RadAngle));
    const float CosAngle=float(cos(RadAngle));

    M.m[1][1]=CosAngle; M.m[1][2]=-SinAngle;
    M.m[2][1]=SinAngle; M.m[2][2]= CosAngle;

    return M;
}


MatrixT MatrixT::GetRotateYMatrix(float Angle)
{
    MatrixT M;

    const float RadAngle=Angle*3.14159265358979323846f/180.0f;
    const float SinAngle=float(sin(RadAngle));
    const float CosAngle=float(cos(RadAngle));

    M.m[0][0]= CosAngle; M.m[0][2]=SinAngle;
    M.m[2][0]=-SinAngle; M.m[2][2]=CosAngle;

    return M;
}


MatrixT MatrixT::GetRotateZMatrix(float Angle)
{
    MatrixT M;

    const float RadAngle=Angle*3.14159265358979323846f/180.0f;
    const float SinAngle=float(sin(RadAngle));
    const float CosAngle=float(cos(RadAngle));

    M.m[0][0]=CosAngle; M.m[0][1]=-SinAngle;
    M.m[1][0]=SinAngle; M.m[1][1]= CosAngle;

    return M;
}


MatrixT MatrixT::GetRotateMatrix(float Angle, const Vector3fT& Axis)
{
    const float RadAngle=Angle*3.14159265358979323846f/180.0f;
    const float s       =float(sin(RadAngle));
    const float c       =float(cos(RadAngle));
    const float t       =1.0f-c;

    const float tx=t*Axis.x; const float ty=t*Axis.y; const float tz=t*Axis.z;
    const float sx=s*Axis.x; const float sy=s*Axis.y; const float sz=s*Axis.z;

    return MatrixT(tx*Axis.x +  c, tx*Axis.y - sz, tx*Axis.z + sy, 0.0f,
                   tx*Axis.y + sz, ty*Axis.y +  c, ty*Axis.z - sx, 0.0f,
                   tx*Axis.z - sy, ty*Axis.z + sx, tz*Axis.z +  c, 0.0f,
                             0.0f,           0.0f,           0.0f, 1.0f);
}


MatrixT MatrixT::operator * (const MatrixT& vm) const
{
    return MatrixT(
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


void MatrixT::Translate_MT(const VectorT& vTrans)
{
    const VectorT t=Mul1(vTrans);

    m[0][3]=float(t.x);
    m[1][3]=float(t.y);
    m[2][3]=float(t.z);
}


void MatrixT::Translate_MT(float tx, float ty, float tz)
{
    m[0][3]=m[0][0]*tx + m[0][1]*ty + m[0][2]*tz + m[0][3];
    m[1][3]=m[1][0]*tx + m[1][1]*ty + m[1][2]*tz + m[1][3];
    m[2][3]=m[2][0]*tx + m[2][1]*ty + m[2][2]*tz + m[2][3];
}


void MatrixT::Translate_TM(const VectorT& vTrans)
{
    m[0][3]+=float(vTrans.x);
    m[1][3]+=float(vTrans.y);
    m[2][3]+=float(vTrans.z);
}


void MatrixT::Scale_MS(float sx, float sy, float sz)
{
    for (int i=0; i<4; i++)
    {
        m[i][0]*=sx;
        m[i][1]*=sy;
        m[i][2]*=sz;
    }
}


void MatrixT::Scale_SM(float sx, float sy, float sz)
{
    for (int j=0; j<4; j++)
    {
        m[0][j]*=sx;
        m[1][j]*=sy;
        m[2][j]*=sz;
    }
}


void MatrixT::RotateX_MR(float Angle)
{
    const float RadAngle=Angle*3.14159265358979323846f/180.0f;
    const float SinAngle=float(sin(RadAngle));
    const float CosAngle=float(cos(RadAngle));

    for (int i=0; i<4; i++)
    {
        float y= CosAngle*m[i][1] + SinAngle*m[i][2];
        float z=-SinAngle*m[i][1] + CosAngle*m[i][2];

        m[i][1]=y;
        m[i][2]=z;
    }
}


void MatrixT::RotateX_RM(float Angle)
{
    const float RadAngle=Angle*3.14159265358979323846f/180.0f;
    const float SinAngle=float(sin(RadAngle));
    const float CosAngle=float(cos(RadAngle));

    for (int j=0; j<4; j++)
    {
        float a=CosAngle*m[1][j] - SinAngle*m[2][j];
        float b=SinAngle*m[1][j] + CosAngle*m[2][j];

        m[1][j]=a;
        m[2][j]=b;
    }
}


void MatrixT::RotateY_MR(float Angle)
{
    const float RadAngle=Angle*3.14159265358979323846f/180.0f;
    const float SinAngle=float(sin(RadAngle));
    const float CosAngle=float(cos(RadAngle));

    for (int i=0; i<4; i++)
    {
        float x=CosAngle*m[i][0] - SinAngle*m[i][2];
        float z=SinAngle*m[i][0] + CosAngle*m[i][2];

        m[i][0]=x;
        m[i][2]=z;
    }
}


void MatrixT::RotateY_RM(float Angle)
{
    const float RadAngle=Angle*3.14159265358979323846f/180.0f;
    const float SinAngle=float(sin(RadAngle));
    const float CosAngle=float(cos(RadAngle));

    for (int j=0; j<4; j++)
    {
        float a= CosAngle*m[0][j] + SinAngle*m[2][j];
        float b=-SinAngle*m[0][j] + CosAngle*m[2][j];

        m[0][j]=a;
        m[2][j]=b;
    }
}


void MatrixT::RotateZ_MR(float Angle)
{
    const float RadAngle=Angle*3.14159265358979323846f/180.0f;
    const float SinAngle=float(sin(RadAngle));
    const float CosAngle=float(cos(RadAngle));

    for (int i=0; i<4; i++)
    {
        float x= CosAngle*m[i][0] + SinAngle*m[i][1];
        float y=-SinAngle*m[i][0] + CosAngle*m[i][1];

        m[i][0]=x;
        m[i][1]=y;
    }
}


void MatrixT::RotateZ_RM(float Angle)
{
    const float RadAngle=Angle*3.14159265358979323846f/180.0f;
    const float SinAngle=float(sin(RadAngle));
    const float CosAngle=float(cos(RadAngle));

    for (int j=0; j<4; j++)
    {
        float a=CosAngle*m[0][j] - SinAngle*m[1][j];
        float b=SinAngle*m[0][j] + CosAngle*m[1][j];

        m[0][j]=a;
        m[1][j]=b;
    }
}


MatrixT MatrixT::GetInverse(bool* Result) const
{
    // How it's done:
    // AX = I
    // A = this
    // X = the matrix we're looking for
    // I = identity
    float mat[4][8];
    int   rowMap[4];

    if (Result!=NULL) Result[0]=false;

    // Copy this matrix into the left half of mat and the identity matrix into the right half, so that "mat=AI".
    for (int i=0; i<4; i++)
    {
        const float* pIn =m[i];
        float*       pOut=mat[i];

        for (int j=0; j<4; j++)
        {
            pOut[j  ]=pIn[j];
            pOut[j+4]=(i==j) ? 1.0f : 0.0f;
        }

        rowMap[i]=i;
    }

    // Use row operations to get to reduced row-echelon form using these rules:
    // 1. Multiply or divide a row by a nonzero number.
    // 2. Add a multiple of one row to another.
    // 3. Interchange two rows.
    for (int iRow=0; iRow<4; iRow++)
    {
        // Find the row with the largest element in this column.
        float fLargest = 0.001f;
        int   iLargest = -1;

        for (int iTest=iRow; iTest<4; iTest++)
        {
            float fTest=(float)fabs(mat[rowMap[iTest]][iRow]);

            if (fTest>fLargest)
            {
                iLargest = iTest;
                fLargest = fTest;
            }
        }

        // They're all too small.. sorry.
        if (iLargest==-1) return MatrixT();

        // Swap the rows.
        int iTemp = rowMap[iLargest];
        rowMap[iLargest] = rowMap[iRow];
        rowMap[iRow] = iTemp;

        float* pRow = mat[rowMap[iRow]];

        // Divide this row by the element.
        const float mul=1.0f/pRow[iRow];
        for (int j=0; j<8; j++) pRow[j]*=mul;
        pRow[iRow]=1.0f;    // Preserve accuracy...

        // Eliminate this element from the other rows using operation 2.
        for (int i=0; i<4; i++)
        {
            if (i==iRow) continue;

            float* pScaleRow = mat[rowMap[i]];

            // Multiply this row by -(iRow*the element).
            const float mul=-pScaleRow[iRow];
            for (int j=0; j<8; j++) pScaleRow[j]+=pRow[j]*mul;
            pScaleRow[iRow]=0.0f;   // Preserve accuracy...
        }
    }

    // The inverse is on the right side of AX now (the identity is on the left).
    MatrixT dst;

    for (int i=0; i<4; i++)
    {
        const float* pIn =mat[rowMap[i]]+4;
        float*       pOut=dst.m[i];

        for (int j=0; j<4; j++) pOut[j]=pIn[j];
    }

    if (Result!=NULL) Result[0]=true;
    return dst;
}


MatrixT MatrixT::GetTranspose() const
{
    MatrixT mt;

    for (int i=0; i<4; i++)
        for (int j=0; j<4; j++)
            mt.m[i][j]=m[j][i];

    return mt;
}
