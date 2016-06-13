/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "SoundSysImpl.hpp"
#include "SoundImpl.hpp"
#include "Channel.hpp"
#include "Buffer.hpp"
#include "BufferManager.hpp"
#include "fmod.h"           // FMOD
#ifdef _WIN32
#include "fmod_errors.h"    // FMOD Error Messages
#endif
#include "../SoundShader.hpp"
#include <iostream>


SoundSysImplT& SoundSysImplT::GetInstance()
{
    static SoundSysImplT SoundSys;

    return SoundSys;
}


SoundSysImplT::SoundSysImplT()
    : m_BufferManager(BufferManagerT::GetInstance()),
      m_IsInitialized(false)
{
}


SoundSysImplT::~SoundSysImplT()
{
    Release();
}


bool SoundSysImplT::Initialize()
{
    // Sound system is already initialized.
    if (m_IsInitialized) return true;

    // std::cout << "FMOD3: Initializing sound system\n";

    // Initialze FMOD with 44,1 KHz sample rate and with 32 software channels.
    if (!FSOUND_Init(44100, 32, 0)) return false;

    // std::cout << "FMOD3: Got " << FSOUND_GetMaxChannels() << " channels\n";

    // Create as many Channel objects as there are software and hardware channels.
    for (int i=0; i<FSOUND_GetMaxChannels(); i++)
    {
        m_Channels.PushBack(ChannelT(i));
    }

    m_IsInitialized=true;

    return true;
}


void SoundSysImplT::Release()
{
    if (!m_IsInitialized) return;

    // std::cout << "FMOD3: Releasing sound system\n";

    m_BufferManager->ReleaseAll();

    FSOUND_Close();

    m_IsInitialized=false;
}


bool SoundSysImplT::IsSupported()
{
    if (FSOUND_GetVersion()<FMOD_VERSION) return false;

    return true;
}


int SoundSysImplT::GetPreferenceNr()
{
    return 1000;
}


SoundI* SoundSysImplT::CreateSound2D(const SoundShaderT* SoundShader)
{
    // Create "empty" sound object if no sound shader is passed.
    // This is often the case if the game code doesn't check if a sound shader is registered and simply passes the result of
    // SoundShaderManager->GetSoundShader("xxx") to this method.
    if (SoundShader==NULL) return new SoundImplT(this, false);

    BufferT* Buffer=m_BufferManager->GetBuffer(SoundShader->AudioFile, SoundShader->LoadType, false);

    if (Buffer==NULL) return NULL;

    return new SoundImplT(this, false, SoundShader, Buffer);
}


SoundI* SoundSysImplT::CreateSound3D(const SoundShaderT* SoundShader)
{
    // Create "empty" sound object if no sound shader is passed.
    // This is often the case if the game code doesn't check if a sound shader is registered and simply passes the result of
    // SoundShaderManager->GetSoundShader("xxx") to this method.
    if (SoundShader==NULL) return new SoundImplT(this, true);

    BufferT* Buffer=m_BufferManager->GetBuffer(SoundShader->AudioFile, SoundShader->LoadType, true);

    if (Buffer==NULL) return NULL;

    return new SoundImplT(this, true, SoundShader, Buffer);
}


void SoundSysImplT::DeleteSound(SoundI* Sound)
{
    delete Sound;
}


bool SoundSysImplT::PlaySound(const SoundI* Sound)
{
    // We can safely cast SoundI to SoundImplT, since we have created it ourselves.
    SoundImplT* SoundImplTmp=(SoundImplT*)Sound;

    if (SoundImplTmp->Buffer==NULL) return false;

    int Channel=SoundImplTmp->Buffer->AttachToChannel(SoundImplTmp->GetPriority());

    if (Channel==-1) return false;

    // The lower 12 bits are the channel number, so delete upper bits.
    int ChannelNum=Channel&4095;

    assert(ChannelNum<=(int)m_Channels.Size());

    m_Channels[ChannelNum].Sound        =SoundImplTmp;
    m_Channels[ChannelNum].ChannelHandle=Channel;

    SoundImplTmp->ChannelHandle=Channel; // The sound needs also knowledge about its handle for stop/pause/resume functions.

    // Make initial update.
    m_Channels[ChannelNum].Update();

    FSOUND_SetPaused(Channel, false);

    return true;
}


void SoundSysImplT::SetMasterVolume(float Volume)
{
    if (Volume>1.0f) Volume=1.0f;
    if (Volume<0.0f) Volume=0.0f;

    Volume*=255.0f;

    FSOUND_SetSFXMasterVolume(int(Volume));
}


float SoundSysImplT::GetMasterVolume()
{
    return float(FSOUND_GetSFXMasterVolume())/255.0f;
}


void SoundSysImplT::Update()
{
    // Update sounds for all channels.
    for (unsigned long i=0; i<m_Channels.Size(); i++)
    {
        // Only update channel if it is playing.
        if (FSOUND_IsPlaying(m_Channels[i].ChannelHandle))
            m_Channels[i].Update();
    }

    FSOUND_Update(); // Should be called every game frame according to FMOD docu.
}


void SoundSysImplT::UpdateListener(const Vector3dT& Position, const Vector3dT& Velocity, const Vector3fT& OrientationForward, const Vector3fT& OrientationUp)
{
    const double METERS_PER_WORLD_UNIT = 0.0254;

    // Note that y and z coordinate are swapped because FMOD uses another coordinate system than Cafu.

    // Update 3D sound attributes.
    float PositionTriplet[3]={ float(Position.x * METERS_PER_WORLD_UNIT),
                               float(Position.z * METERS_PER_WORLD_UNIT),
                               float(Position.y * METERS_PER_WORLD_UNIT) };
    float VelocityTriplet[3]={ float(Velocity.x * METERS_PER_WORLD_UNIT),
                               float(Velocity.z * METERS_PER_WORLD_UNIT),
                               float(Velocity.y * METERS_PER_WORLD_UNIT) };

    FSOUND_3D_Listener_SetAttributes(PositionTriplet, VelocityTriplet,
                                     OrientationForward.x, OrientationForward.z, OrientationForward.y,
                                     OrientationUp.x, OrientationUp.z, OrientationUp.y);
}
