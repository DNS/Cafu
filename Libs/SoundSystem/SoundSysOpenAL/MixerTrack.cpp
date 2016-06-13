/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
    const double METERS_PER_WORLD_UNIT = 0.0254;

    float Pos[3]={ float(m_Sound->Position.x * METERS_PER_WORLD_UNIT),
                   float(m_Sound->Position.y * METERS_PER_WORLD_UNIT),
                   float(m_Sound->Position.z * METERS_PER_WORLD_UNIT) };

    float Vel[3]={ float(m_Sound->Velocity.x * METERS_PER_WORLD_UNIT),
                   float(m_Sound->Velocity.y * METERS_PER_WORLD_UNIT),
                   float(m_Sound->Velocity.z * METERS_PER_WORLD_UNIT) };

    float Dir[3]={ float(m_Sound->Direction.x * METERS_PER_WORLD_UNIT),
                   float(m_Sound->Direction.y * METERS_PER_WORLD_UNIT),
                   float(m_Sound->Direction.z * METERS_PER_WORLD_UNIT) };

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
