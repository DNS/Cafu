/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#include "Rotation.hpp"
#include "Angles.hpp"    // For degrees to radian conversion.


using namespace cf::math;


template<class T> const Matrix3x3T<T>& RotationT<T>::GetRotMat() const
{
    if (m_IsRotMatValid) return m_RotMat;

    // This is rotation about an arbitrary axis with quaternions.
    // There are several places in the internet that show how this code is derived, e.g. at
    // http://www.cprogramming.com/tutorial/3d/quaternions.html or http://answers.google.com/answers/threadview?id=361441
    const T a=AnglesT<T>::DegToRad(m_Angle) * T(0.5);
    const T s=sin(a);
    const T c=cos(a);

    const T x=m_Axis.x * s;
    const T y=m_Axis.y * s;
    const T z=m_Axis.z * s;

    const T x2=x + x;
    const T y2=y + y;
    const T z2=z + z;

    const T xx=x * x2;
    const T xy=x * y2;
    const T xz=x * z2;

    const T yy=y * y2;
    const T yz=y * z2;
    const T zz=z * z2;

    const T wx=c * x2;
    const T wy=c * y2;
    const T wz=c * z2;

    m_RotMat.m[0][0]=1.0f - (yy + zz);
    m_RotMat.m[1][0]=xy - wz;
    m_RotMat.m[2][0]=xz + wy;

    m_RotMat.m[0][1]=xy + wz;
    m_RotMat.m[1][1]=1.0f - (xx + zz);
    m_RotMat.m[2][1]=yz - wx;

    m_RotMat.m[0][2]=xz - wy;
    m_RotMat.m[1][2]=yz + wx;
    m_RotMat.m[2][2]=1.0f - (xx + yy);

    m_IsRotMatValid=true;
    return m_RotMat;
}


template class RotationT<float>;
template class RotationT<double>;
