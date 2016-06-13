/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Quaternion.hpp"
#include "Angles.hpp"
#include "Matrix3x3.hpp"


using namespace cf::math;


template<class T> QuaternionT<T>::QuaternionT(const AnglesT<T>& Angles)
{
    // See http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q60 for details.
    const T SinYaw   = sin(T(0.5) * AnglesT<T>::DegToRad(Angles.yaw()));
    const T CosYaw   = cos(T(0.5) * AnglesT<T>::DegToRad(Angles.yaw()));
    const T SinPitch = sin(T(0.5) * AnglesT<T>::DegToRad(Angles.pitch()));
    const T CosPitch = cos(T(0.5) * AnglesT<T>::DegToRad(Angles.pitch()));
    const T SinRoll  = sin(T(0.5) * AnglesT<T>::DegToRad(Angles.roll()));
    const T CosRoll  = cos(T(0.5) * AnglesT<T>::DegToRad(Angles.roll()));

    x = SinRoll*CosPitch*CosYaw - CosRoll*SinPitch*SinYaw;
    y = CosRoll*SinPitch*CosYaw + SinRoll*CosPitch*SinYaw;
    z = CosRoll*CosPitch*SinYaw - SinRoll*SinPitch*CosYaw;
    w = CosRoll*CosPitch*CosYaw + SinRoll*SinPitch*SinYaw;
}


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


template<class T> QuaternionT<T>::QuaternionT(const Vector3T<T>& Axis, const T Angle)
{
    // Convert the rotation axis and angle to a quaternion as described at
    // http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q56
    assert(::length(Axis) > T(0.99) && ::length(Axis) < T(1.01));

    const T sin_a = sin(T(0.5) * Angle);
    const T cos_a = cos(T(0.5) * Angle);

    x = Axis.x * sin_a;
    y = Axis.y * sin_a;
    z = Axis.z * sin_a;
    w = cos_a;

    assert(IsNormal(T(0.01)));
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
