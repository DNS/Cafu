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

#include "SoundImpl.hpp"
#include "MixerTrack.hpp"
#include "Buffer.hpp"
#include "BufferManager.hpp"
#include "SoundSysImpl.hpp"

#include "../SoundShader.hpp"

#include <iostream>


SoundImplT::SoundImplT(SoundSysImplT* SoundSys, bool Is3D_, const SoundShaderT* Shader_, BufferT* Buffer_)
    : Buffer(Buffer_),
      MixerTrack(NULL),
      Position(),
      Velocity(),
      Direction(),
      InnerVolume(0.5f),
      OuterVolume(0.0f),
      InnerConeAngle(360.0f),
      OuterConeAngle(0.0f),
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
    if (Buffer!=NULL)
    {
        if (MixerTrack!=NULL)
            MixerTrack->DetachCurrentSound();

        BufferManagerT::GetInstance()->ReleaseBuffer(Buffer);
    }
}


bool SoundImplT::Play()
{
    // If the sound is still attached to a mixer track, use this mixer track to play the sound.
    if (MixerTrack) return MixerTrack->PlaySound(this);

    // If the sound hasn't a mixer track yet, play it trough the sound system (which gets a free
    // mixer track from the mixer track manager).
    return m_SoundSys->PlaySound(this);
}


void SoundImplT::Stop()
{
    if (MixerTrack) MixerTrack->StopCurrent();
}


void SoundImplT::Pause()
{
    if (MixerTrack) MixerTrack->PauseCurrent();
}


bool SoundImplT::Resume()
{
    if (MixerTrack)
    {
        MixerTrack->ResumeCurrent();
        return true;
    }

    return false;
}


bool SoundImplT::IsPlaying() const
{
    if (MixerTrack)
        return MixerTrack->IsPlaying();

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
    InnerVolume   =m_Shader->InnerVolume;
    OuterVolume   =m_Shader->OuterVolume;
    InnerConeAngle=m_Shader->InnerConeAngle;
    OuterConeAngle=m_Shader->OuterConeAngle;
    MinDistance   =m_Shader->MinDistance;
    MaxDistance   =m_Shader->MaxDistance;
    Priority      =m_Shader->Priority;
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
    InnerVolume=InnerVolume_;
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
    InnerConeAngle=InnerConeAngle_;
}


void SoundImplT::SetOuterConeAngle(float OuterConeAngle_)
{
    OuterConeAngle=OuterConeAngle_;
}


void SoundImplT::SetOuterVolume(float OuterVolume_)
{
    OuterVolume=OuterVolume_;
}


unsigned int SoundImplT::GetPriority() const
{
    return Priority;
}


float SoundImplT::GetInnerVolume() const
{
    return InnerVolume;
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
    return InnerConeAngle;
}


float SoundImplT::GetOuterConeAngle() const
{
    return OuterConeAngle;
}


float SoundImplT::GetOuterVolume() const
{
    return OuterVolume;
}
