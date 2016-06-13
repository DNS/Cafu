/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Camera.hpp"
#include "Options.hpp"

#include <math.h>


CameraT::CameraT()
    : Pos(),
      Angles(),
      ViewDirLength(64.0f),
      VerticalFOV(67.5f),
      NearPlaneDist(8.0f),
      FarPlaneDist(Options.view3d.BackPlane),
      m_MatrixPos(),
      m_MatrixAngles(),
      m_Matrix()
{
}


Vector3fT CameraT::GetXAxis() const
{
    const MatrixT& M=GetMatrix();

    return Vector3fT(M[0][0], M[0][1], M[0][2]);
}


Vector3fT CameraT::GetYAxis() const
{
    const MatrixT& M=GetMatrix();

    return Vector3fT(M[1][0], M[1][1], M[1][2]);
}


Vector3fT CameraT::GetZAxis() const
{
    const MatrixT& M=GetMatrix();

    return Vector3fT(M[2][0], M[2][1], M[2][2]);
}


const MatrixT& CameraT::GetMatrix() const
{
    if (Pos!=m_MatrixPos || Angles!=m_MatrixAngles)
    {
        m_MatrixPos   =Pos;
        m_MatrixAngles=Angles;
        m_Matrix      =//trixT::GetRotateYMatrix(Angles.roll())*    // Roll is always at 0°.
                       MatrixT::GetRotateXMatrix(Angles.pitch())*
                       MatrixT::GetRotateZMatrix(Angles.yaw())*
                       MatrixT::GetTranslateMatrix(-Pos);
    }

    return m_Matrix;
}


void CameraT::SetLookAtPos(const Vector3fT& LookAtPos)
{
    const Vector3fT ViewDir=LookAtPos-Pos;  // This will be our new y-axis: normalizeOr0(ViewDir)==GetYAxis().

    // See http://en.wikipedia.org/wiki/Atan2 for an excellent article about the atan2() function.
    Angles.yaw()=(ViewDir.x==0.0f && ViewDir.y==0.0f) ? 0.0f : cf::math::AnglesfT::RadToDeg(atan2(ViewDir.x, ViewDir.y));

    // The pitch is the angle between ViewDir and ViewDirXY, that is, acos(dot(ViewDir/length(ViewDir), ViewDirXY/length(ViewDirXY))).
    // This can be rearranged (parts cancel each other out) to yield the usual definition of the cosine ("Ankathete durch Hypothenuse"),
    // but it doesn't properly take the sign into account. Therefore, we rather employ the tangent:
    const float Ankathete   =length(Vector3fT(ViewDir.x, ViewDir.y, 0.0f));     // Adjacent leg.
    const float Gegenkathete=ViewDir.z;                                         // Opposite leg.

    Angles.pitch()=(Ankathete==0.0f && Gegenkathete==0.0f) ? 0.0f : -cf::math::AnglesfT::RadToDeg(atan2(Gegenkathete, Ankathete));

    // Make sure that both the yaw and the pitch are in their proper ranges.
    LimitAngles();
}


void CameraT::LimitAngles()
{
    // Make sure that 0° <= Angles.yaw() < 360°.
    // Could use fmod(), but it doesn't work as desired with negative values of Angles.yaw().
    while (Angles.yaw()>=360.0f) Angles.yaw()-=360.0f;
    while (Angles.yaw()<   0.0f) Angles.yaw()+=360.0f;

    // Clamp Angles.pitch() to the interval from -90° to +90°.
    if (Angles.pitch()> 90.0f) Angles.pitch()= 90.0f;
    if (Angles.pitch()<-90.0f) Angles.pitch()=-90.0f;
}
