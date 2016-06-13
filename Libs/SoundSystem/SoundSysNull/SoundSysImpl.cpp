/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "SoundSysImpl.hpp"
#include "SoundImpl.hpp"
#include <iostream>


SoundSysImplT& SoundSysImplT::GetInstance()
{
    static SoundSysImplT SoundSys;

    return SoundSys;
}


SoundSysImplT::SoundSysImplT()
{
}


bool SoundSysImplT::Initialize()
{
    return true;
}


void SoundSysImplT::Release()
{
}


bool SoundSysImplT::IsSupported()
{
    return true;
}


int SoundSysImplT::GetPreferenceNr()
{
    return 1;
}


SoundI* SoundSysImplT::CreateSound2D(const SoundShaderT* SoundShader)
{
    return new SoundT();
}


SoundI* SoundSysImplT::CreateSound3D(const SoundShaderT* SoundShader)
{
    return new SoundT();
}


void SoundSysImplT::DeleteSound(SoundI* Sound)
{
    delete Sound;
}


bool SoundSysImplT::PlaySound(const SoundI* Sound)
{
    return true;
}


void SoundSysImplT::SetMasterVolume(float Volume)
{
}


float SoundSysImplT::GetMasterVolume()
{
    return 0.0f;
}


void SoundSysImplT::Update()
{
}


void SoundSysImplT::UpdateListener(const Vector3dT& Position, const Vector3dT& Velocity, const Vector3fT& OrientationForward, const Vector3fT& OrientationUp)
{
}
