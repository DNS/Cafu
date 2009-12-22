/*****************************/
/*** Integer Matrix (Code) ***/
/*****************************/


#include "IntMatrix.hpp"


IntMatrixT::IntMatrixT()
{
    for (char i=0; i<=3; i++)
        for (char j=0; j<=3; j++)
            m[i][j]=(i==j) ? 1:0;
}


IntMatrixT IntMatrixT::GetTranslationMatrix(int tx, int ty, int tz)
{
    IntMatrixT TranslationMatrix;

    TranslationMatrix.m[0][3]=tx;
    TranslationMatrix.m[1][3]=ty;
    TranslationMatrix.m[2][3]=tz;

    return TranslationMatrix;
}


IntMatrixT IntMatrixT::GetRotationXMatrix(int Angle)
{
    IntMatrixT RotXMatrix;
    int        SinAngle=0;
    int        CosAngle=0;

    switch (Angle)
    {
        case -360: SinAngle= 0; CosAngle= 1; break;
        case -270: SinAngle= 1; CosAngle= 0; break;
        case -180: SinAngle= 0; CosAngle=-1; break;
        case - 90: SinAngle=-1; CosAngle= 0; break;
        case    0: SinAngle= 0; CosAngle= 1; break;
        case   90: SinAngle= 1; CosAngle= 0; break;
        case  180: SinAngle= 0; CosAngle=-1; break;
        case  270: SinAngle=-1; CosAngle= 0; break;
        case  360: SinAngle= 0; CosAngle= 1; break;
    }

    RotXMatrix.m[1][1]= CosAngle;
    RotXMatrix.m[2][2]= CosAngle;
    RotXMatrix.m[1][2]=-SinAngle;
    RotXMatrix.m[2][1]= SinAngle;

    return RotXMatrix;
}


IntMatrixT IntMatrixT::GetRotationYMatrix(int Angle)
{
    IntMatrixT RotYMatrix;
    int        SinAngle=0;
    int        CosAngle=0;

    switch (Angle)
    {
        case -360: SinAngle= 0; CosAngle= 1; break;
        case -270: SinAngle= 1; CosAngle= 0; break;
        case -180: SinAngle= 0; CosAngle=-1; break;
        case - 90: SinAngle=-1; CosAngle= 0; break;
        case    0: SinAngle= 0; CosAngle= 1; break;
        case   90: SinAngle= 1; CosAngle= 0; break;
        case  180: SinAngle= 0; CosAngle=-1; break;
        case  270: SinAngle=-1; CosAngle= 0; break;
        case  360: SinAngle= 0; CosAngle= 1; break;
    }

    RotYMatrix.m[0][0]= CosAngle;
    RotYMatrix.m[2][2]= CosAngle;
    RotYMatrix.m[0][2]= SinAngle;
    RotYMatrix.m[2][0]=-SinAngle;

    return RotYMatrix;
}


IntMatrixT IntMatrixT::GetRotationZMatrix(int Angle)
{
    IntMatrixT RotZMatrix;
    int        SinAngle=0;
    int        CosAngle=0;

    switch (Angle)
    {
        case -360: SinAngle= 0; CosAngle= 1; break;
        case -270: SinAngle= 1; CosAngle= 0; break;
        case -180: SinAngle= 0; CosAngle=-1; break;
        case - 90: SinAngle=-1; CosAngle= 0; break;
        case    0: SinAngle= 0; CosAngle= 1; break;
        case   90: SinAngle= 1; CosAngle= 0; break;
        case  180: SinAngle= 0; CosAngle=-1; break;
        case  270: SinAngle=-1; CosAngle= 0; break;
        case  360: SinAngle= 0; CosAngle= 1; break;
    }

    RotZMatrix.m[0][0]= CosAngle;
    RotZMatrix.m[1][1]= CosAngle;
    RotZMatrix.m[0][1]=-SinAngle;
    RotZMatrix.m[1][0]= SinAngle;

    return RotZMatrix;
}


IntMatrixT operator * (const IntMatrixT& A, const IntMatrixT& B)
{
    IntMatrixT Result;

    for (char i=0; i<=2; i++)
    {
        Result.m[i][0]=A.m[i][0]*B.m[0][0] + A.m[i][1]*B.m[1][0] + A.m[i][2]*B.m[2][0] + A.m[i][3]*B.m[3][0];
        Result.m[i][1]=A.m[i][0]*B.m[0][1] + A.m[i][1]*B.m[1][1] + A.m[i][2]*B.m[2][1] + A.m[i][3]*B.m[3][1];
        Result.m[i][2]=A.m[i][0]*B.m[0][2] + A.m[i][1]*B.m[1][2] + A.m[i][2]*B.m[2][2] + A.m[i][3]*B.m[3][2];
        Result.m[i][3]=A.m[i][0]*B.m[0][3] + A.m[i][1]*B.m[1][3] + A.m[i][2]*B.m[2][3] + A.m[i][3]*B.m[3][3];
    }

    return Result;
}


bool operator == (const IntMatrixT& A, const IntMatrixT& B)
{
    for (char i=0; i<=3; i++)
        for (char j=0; j<=3; j++)
            if (A.m[i][j]!=B.m[i][j]) return false;

    return true;
}
