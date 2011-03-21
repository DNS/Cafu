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

#include "MixerTrack.hpp"
#include "SoundImpl.hpp"
#include "../SoundShader.hpp"
#include "Buffer.hpp"

#include <iostream>


MixerTrackT::MixerTrackT()
    : m_Sound(NULL),
      m_SourceHandle(0)
{
    alGenSources(1, &m_SourceHandle);

    int Error=alGetError();

    // If there was an error creating the new source.
    if (Error!=AL_NO_ERROR)
        throw std::runtime_error("Error creating new mixer track: "+TranslateErrorCode(Error));
}


MixerTrackT::~MixerTrackT()
{
    StopAndDetach();
    alDeleteSources(1, &m_SourceHandle);

    assert(alGetError()==AL_NO_ERROR);
}


bool MixerTrackT::Play(SoundImplT* Sound)
{
    StopAndDetach();
    if (Sound==NULL) return false;

    m_Sound=Sound;
    m_Sound->MixerTrack=this;
    m_Sound->Buffer->AttachToMixerTrack(this);

    alSourcePlay(m_SourceHandle);


    const int Error=alGetError();

    if (Error!=AL_NO_ERROR)
        std::cout << "OpenAL: Error playing sound in mixer track (Error: " << TranslateErrorCode(Error) << ")\n";

    return Error==AL_NO_ERROR;
}


void MixerTrackT::Pause()
{
    if (m_Sound==NULL) return;

    alSourcePause(m_SourceHandle);
    assert(alGetError()==AL_NO_ERROR);
}


void MixerTrackT::Resume()
{
    if (m_Sound==NULL) return;

    alSourcePlay(m_SourceHandle);
    assert(alGetError()==AL_NO_ERROR);
}


void MixerTrackT::StopAndDetach()
{
    if (m_Sound==NULL) return;

    // Release this mixer track (OpenAL source) for use by another sound.
    // If we instead wanted to hold on to the sound, we should still detach, then re-attach it,
    // or else the sounds buffer had to re-initialized explicitly, e.g. m_Sound->Buffer->Init();
    alSourceStop(m_SourceHandle);
    // alSourceRewind(m_SourceHandle);

    if (!m_Sound->Buffer->DetachFromMixerTrack(this)) std::cout << "Couldn't detach from buffer.\n";
    m_Sound->MixerTrack=NULL;
    m_Sound=NULL;
}


bool MixerTrackT::IsPlaying()
{
    if (m_Sound==NULL) return false;
    assert(alGetError()==AL_NO_ERROR);

    int SourceState=0;
    alGetSourcei(m_SourceHandle, AL_SOURCE_STATE, &SourceState);

    return SourceState==AL_PLAYING;
}


bool MixerTrackT::IsUsed()
{
    if (m_Sound==NULL) return false;

    int SourceState=0;
    alGetSourcei(m_SourceHandle, AL_SOURCE_STATE, &SourceState);

    return SourceState==AL_PLAYING || SourceState==AL_PAUSED;
}


unsigned int MixerTrackT::GetPriority()
{
    if (m_Sound==NULL) return 0;

    return m_Sound->GetPriority();
}


void MixerTrackT::Update()
{
    if (m_Sound==NULL) return;

    // Update the OpenAL source attributes.
    // Note that this method MixerTrackT::Update() is always called, independent from the source state (AL_INITIAL, AL_PLAYING, AL_STOPPED, ...).
    float Pos[3]={ float(m_Sound->Position.x/1000.0f),
                   float(m_Sound->Position.y/1000.0f),
                   float(m_Sound->Position.z/1000.0f) };

    float Vel[3]={ float(m_Sound->Velocity.x/1000.0f),
                   float(m_Sound->Velocity.y/1000.0f),
                   float(m_Sound->Velocity.z/1000.0f) };

    float Dir[3]={ float(m_Sound->Direction.x/1000.0f),
                   float(m_Sound->Direction.y/1000.0f),
                   float(m_Sound->Direction.z/1000.0f) };

    // TODO: Only make alSource...() calls if m_Sound indicates that it has changed? (E.g. have it have a "m_HasChanged" flag that is set whenever one of its attributes is set.)
    alSourcei (m_SourceHandle, AL_SOURCE_RELATIVE,    m_Sound->Is3D() ? AL_FALSE : AL_TRUE);
    alSourcefv(m_SourceHandle, AL_POSITION,           Pos);
    alSourcefv(m_SourceHandle, AL_VELOCITY,           Vel);
    alSourcefv(m_SourceHandle, AL_DIRECTION,          Dir);
    alSourcef (m_SourceHandle, AL_REFERENCE_DISTANCE, m_Sound->MinDistance);
    alSourcef (m_SourceHandle, AL_MAX_DISTANCE,       m_Sound->MaxDistance);
    alSourcef (m_SourceHandle, AL_GAIN,               m_Sound->InnerVolume);
    alSourcef (m_SourceHandle, AL_CONE_OUTER_GAIN,    m_Sound->OuterVolume);
    alSourcef (m_SourceHandle, AL_CONE_INNER_ANGLE,   m_Sound->InnerConeAngle);
    alSourcef (m_SourceHandle, AL_CONE_OUTER_ANGLE,   m_Sound->OuterConeAngle);

    assert(alGetError()==AL_NO_ERROR);
}
