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

#include "Quaternion.hpp"
#include "Matrix3x3.hpp"


using namespace cf::math;


template<class T> QuaternionT<T>::QuaternionT(const Matrix3x3T<T>& Mat)
    : x(0), y(0), z(0), w(1)
{
    // The idea is from <http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q55>,
    // but we use our own approach for finding the operative factor.
    const T t[]={ 0,
                  T(1.0) + Mat[0][0] + Mat[1][1] + Mat[2][2],
                  T(1.0) + Mat[0][0] - Mat[1][1] - Mat[2][2],
                  T(1.0) + Mat[1][1] - Mat[0][0] - Mat[2][2],
                  T(1.0) + Mat[2][2] - Mat[0][0] - Mat[1][1] };

    unsigned int Max=0;

    for (unsigned int i=1; i<5; i++)
        if (t[i]>t[Max]) Max=i;

    const T s=T(2.0) * sqrt(t[Max]);

    if (Max==1)
    {
        x = (Mat[2][1] - Mat[1][2]) / s;
        y = (Mat[0][2] - Mat[2][0]) / s;
        z = (Mat[1][0] - Mat[0][1]) / s;
        w = T(0.25) * s;
    }
    else if (Max==2)
    {
        x = T(0.25) * s;
        y = (Mat[1][0] + Mat[0][1]) / s;
        z = (Mat[0][2] + Mat[2][0]) / s;
        w = (Mat[2][1] - Mat[1][2]) / s;
    }
    else if (Max==3)
    {
        x = (Mat[1][0] + Mat[0][1]) / s;
        y = T(0.25) * s;
        z = (Mat[2][1] + Mat[1][2]) / s;
        w = (Mat[0][2] - Mat[2][0]) / s;
    }
    else if (Max==4)
    {
        x = (Mat[0][2] + Mat[2][0]) / s;
        y = (Mat[2][1] + Mat[1][2]) / s;
        z = T(0.25) * s;
        w = (Mat[1][0] - Mat[0][1]) / s;
    }
}


template<class T> QuaternionT<T> QuaternionT<T>::Euler(const T Pitch, const T Yaw, const T Roll)
{
    // See <http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q60> for details.
    const T SinPitch=sin(T(0.5)*Pitch);
    const T CosPitch=cos(T(0.5)*Pitch);
    const T SinYaw  =sin(T(0.5)*Yaw);
    const T CosYaw  =cos(T(0.5)*Yaw);
    const T SinRoll =sin(T(0.5)*Roll);
    const T CosRoll =cos(T(0.5)*Roll);

    return QuaternionT(
        SinRoll*CosPitch*CosYaw - CosRoll*SinPitch*SinYaw,
        CosRoll*SinPitch*CosYaw + SinRoll*CosPitch*SinYaw,
        CosRoll*CosPitch*SinYaw - SinRoll*SinPitch*CosYaw,
        CosRoll*CosPitch*CosYaw + SinRoll*SinPitch*SinYaw);
}


template class QuaternionT<float>;
template class QuaternionT<double>;
