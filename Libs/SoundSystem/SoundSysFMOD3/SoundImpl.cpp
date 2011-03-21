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

#include "SoundImpl.hpp"
#include "SoundSysImpl.hpp"
#include "Buffer.hpp"
#include "BufferManager.hpp"

#include "../SoundShader.hpp"

#include "fmod.h"           // FMOD
#ifdef _WIN32
#include "fmod_errors.h"    // FMOD Error Messages
#endif


SoundImplT::SoundImplT(SoundSysImplT* SoundSys, bool Is3D_, const SoundShaderT* Shader_, BufferT* Buffer_)
    : Buffer(Buffer_),
      ChannelHandle(-1),
      Position(),
      Velocity(),
      Direction(),
      Volume(0.5f),
      MinDistance(0.0f),
      MaxDistance(1000000000.0f),
      Is3DSound(Is3D_),
      Priority(0),
      m_Shader(Shader_),
      m_SoundSys(SoundSys)
{
    // Initialize sound object properties with shader values.
    ResetProperties();
}


SoundImplT::~SoundImplT()
{
    if (Buffer)
        BufferManagerT::GetInstance()->ReleaseBuffer(Buffer);
}


bool SoundImplT::Play()
{
    return m_SoundSys->PlaySound(this);
}


void SoundImplT::Stop()
{
    if (ChannelHandle==-1) return;

    FSOUND_StopSound(ChannelHandle);
    Buffer->Rewind();
}


void SoundImplT::Pause()
{
    if (ChannelHandle==-1) return;

    FSOUND_SetPaused(ChannelHandle, true);
}


bool SoundImplT::Resume()
{
    if (ChannelHandle==-1) return false;

    if (!FSOUND_SetPaused(ChannelHandle, false)) return false;

    return true;
}


bool SoundImplT::IsPlaying() const
{
    if (FSOUND_IsPlaying(ChannelHandle)) return true;

    return false;
}


bool SoundImplT::Is3D() const
{
    return Is3DSound;
}


void SoundImplT::ResetProperties()
{
    if (!m_Shader) return;

    // If sound object has a valid shader use it to reset sound object properties.
    Volume     =m_Shader->InnerVolume;
    MinDistance=m_Shader->MinDistance;
    MaxDistance=m_Shader->MaxDistance;
    Priority   =m_Shader->Priority;
}


void SoundImplT::SetPosition(const Vector3dT& Position_)
{
    Position=Position_;
}


void SoundImplT::SetVelocity(const Vector3dT& Velocity_)
{
    Velocity=Velocity_;
}


void SoundImplT::SetDirection(const Vector3dT& Direction_)
{
    Direction=Direction_;
}


void SoundImplT::SetPriority(unsigned int Priority_)
{
    Priority=Priority_;
}


void SoundImplT::SetInnerVolume(float InnerVolume_)
{
    Volume=InnerVolume_;
}


void SoundImplT::SetMinDistance(float MinDist_)
{
    MinDistance=MinDist_;
}


void SoundImplT::SetMaxDistance(float MaxDist_)
{
    MaxDistance=MaxDist_;
}


void SoundImplT::SetInnerConeAngle(float InnerConeAngle_)
{
    // Not supported with FMOD3.
}


void SoundImplT::SetOuterConeAngle(float OuterConeAngle_)
{
    // Not supported with FMOD3.
}


void SoundImplT::SetOuterVolume(float OuterVolume_)
{
    // Not supported with FMOD3.
}


unsigned int SoundImplT::GetPriority() const
{
    return Priority;
}


float SoundImplT::GetInnerVolume() const
{
    return Volume;
}


float SoundImplT::GetMinDistance() const
{
    return MinDistance;
}


float SoundImplT::GetMaxDistance() const
{
    return MaxDistance;
}


float SoundImplT::GetInnerConeAngle() const
{
    // Not supported with FMOD3.
    return 0.0f;
}


float SoundImplT::GetOuterConeAngle() const
{
    // Not supported with FMOD3.
    return 0.0f;
}


float SoundImplT::GetOuterVolume() const
{
    // Not supported with FMOD3.
    return 0.0f;
}
