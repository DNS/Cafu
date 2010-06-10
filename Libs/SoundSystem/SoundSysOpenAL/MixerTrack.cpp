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

#include "MixerTrack.hpp"
#include "SoundImpl.hpp"
#include "../SoundShader.hpp"
#include "Buffer.hpp"

#include <iostream>


MixerTrackT::MixerTrackT()
    : m_CurrentSound(NULL),
      m_SourceHandle(0)
{
    alGenSources(1, &m_SourceHandle);

    int Error=alGetError();

    // If there was an error creating the new source.
    if (Error!=AL_NO_ERROR)
        throw new CreateErrorE("Error creating new mixer track (Error: "+TranslateErrorCode(Error)+")");
}


MixerTrackT::~MixerTrackT()
{
    // Detach the current sound if one is attached.
    DetachCurrentSound();

    // Delete source.
    alDeleteSources(1, &m_SourceHandle);

    assert(alGetError()==AL_NO_ERROR);
}


bool MixerTrackT::PlaySound(SoundImplT* Sound)
{
    StopCurrent();

    // Attach the new sound to the mixer track.
    assert(Sound!=NULL);

    m_CurrentSound=Sound;
    m_CurrentSound->Buffer->AttachToMixerTrack(this);
    m_CurrentSound->MixerTrack=this;


    Update();   // Initial update for the new sound.

    alSourcePlay(m_SourceHandle);

    const int Error=alGetError();

    if (Error!=AL_NO_ERROR)
        std::cout << "OpenAL: Error playing sound in mixer track (Error: " << TranslateErrorCode(Error) << ")\n";

    return Error==AL_NO_ERROR;
}


void MixerTrackT::StopCurrent()
{
    // Release this mixer track (OpenAL source) for use by another sound.
    // If we instead wanted to hold on to the sound, we should still detach, then re-attach it,
    // or else the sounds buffer had to re-initialized explicitly, e.g. m_CurrentSound->Buffer->Init();
    DetachCurrentSound();
}


void MixerTrackT::PauseCurrent()
{
    if (m_CurrentSound==NULL) return;

    alSourcePause(m_SourceHandle);
    assert(alGetError()==AL_NO_ERROR);
}


void MixerTrackT::ResumeCurrent()
{
    if (m_CurrentSound==NULL) return;

    alSourcePlay(m_SourceHandle);
    assert(alGetError()==AL_NO_ERROR);
}


void MixerTrackT::DetachCurrentSound()
{
    if (m_CurrentSound==NULL) return;

    // Stop current sound from playing, so OpenAL wont get any errors.
    alSourceStop(m_SourceHandle);
    // alSourceRewind(m_SourceHandle); ?

    // Notify the buffer that it is no longer attached to this source.
    if (!m_CurrentSound->Buffer->DetachFromMixerTrack(this)) std::cout << "Couldn't detach from buffer\n";

    // Notify the sound object that it is no longer attached to this source.
    m_CurrentSound->MixerTrack=NULL;

    // Remove the OpenAL buffer from the source.
    alSourcei(m_SourceHandle, AL_BUFFER, 0);

    assert(alGetError()==AL_NO_ERROR);

    m_CurrentSound=NULL;
}


bool MixerTrackT::IsPlaying()
{
    if (m_CurrentSound==NULL) return false;
    assert(alGetError()==AL_NO_ERROR);

    int SourceState=0;
    alGetSourcei(m_SourceHandle, AL_SOURCE_STATE, &SourceState);

    return SourceState==AL_PLAYING;
}


bool MixerTrackT::IsUsed()
{
    if (m_CurrentSound==NULL) return false;
    assert(alGetError()==AL_NO_ERROR);

    int SourceState=0;
    alGetSourcei(m_SourceHandle, AL_SOURCE_STATE, &SourceState);

    return SourceState==AL_PLAYING || SourceState==AL_PAUSED;
}


unsigned int MixerTrackT::GetPriority()
{
    if (m_CurrentSound==NULL) return 0;

    return m_CurrentSound->GetPriority();
}


void MixerTrackT::Update()
{
    if (m_CurrentSound==NULL) return;

    // Update the OpenAL source attributes.
    // Note that this method MixerTrackT::Update() is always called, independent from the source state (AL_INITIAL, AL_PLAYING, AL_STOPPED, ...).
    //DELETE // Note that the AL_LOOPING attribute is exempt from being set here as it is managed at the buffer level.
    float Pos[3]={ float(m_CurrentSound->Position.x/1000.0f),
                   float(m_CurrentSound->Position.y/1000.0f),
                   float(m_CurrentSound->Position.z/1000.0f) };

    float Vel[3]={ float(m_CurrentSound->Velocity.x/1000.0f),
                   float(m_CurrentSound->Velocity.y/1000.0f),
                   float(m_CurrentSound->Velocity.z/1000.0f) };

    float Dir[3]={ float(m_CurrentSound->Direction.x/1000.0f),
                   float(m_CurrentSound->Direction.y/1000.0f),
                   float(m_CurrentSound->Direction.z/1000.0f) };

    // TODO: Only make alSource...() calls if m_CurrentSound indicates that it has changed? (E.g. have it have a "m_HasChanged" flag that is set whenever one of its attributes is set.)
    alSourcei (m_SourceHandle, AL_SOURCE_RELATIVE,    m_CurrentSound->Is3D() ? AL_FALSE : AL_TRUE);
    alSourcefv(m_SourceHandle, AL_POSITION,           Pos);
    alSourcefv(m_SourceHandle, AL_VELOCITY,           Vel);
    alSourcefv(m_SourceHandle, AL_DIRECTION,          Dir);
    alSourcef (m_SourceHandle, AL_REFERENCE_DISTANCE, m_CurrentSound->MinDistance);
    alSourcef (m_SourceHandle, AL_MAX_DISTANCE,       m_CurrentSound->MaxDistance);
    alSourcef (m_SourceHandle, AL_GAIN,               m_CurrentSound->InnerVolume);
    alSourcef (m_SourceHandle, AL_CONE_OUTER_GAIN,    m_CurrentSound->OuterVolume);
    alSourcef (m_SourceHandle, AL_CONE_INNER_ANGLE,   m_CurrentSound->InnerConeAngle);
    alSourcef (m_SourceHandle, AL_CONE_OUTER_ANGLE,   m_CurrentSound->OuterConeAngle);

    assert(alGetError()==AL_NO_ERROR);
}
