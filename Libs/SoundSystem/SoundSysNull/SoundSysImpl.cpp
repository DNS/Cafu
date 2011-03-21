/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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
