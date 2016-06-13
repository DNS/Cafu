/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "SoundSysImpl.hpp"
#include "SoundImpl.hpp"
#include "Buffer.hpp"
#include "BufferManager.hpp"
#include "MixerTrack.hpp"
#include "AL/alut.h"
#include "../Common/SoundStream.hpp"
#include "../SoundShader.hpp"

#include <iostream>


SoundSysImplT& SoundSysImplT::GetInstance()
{
    static SoundSysImplT SoundSys;

    return SoundSys;
}


SoundSysImplT::SoundSysImplT()
    : m_BufferManager(BufferManagerT::GetInstance()),
      m_MixerTrackManager(MixerTrackManT::GetInstance()),
      m_Device(NULL),
      m_Context(NULL),
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

    // std::cout << "OpenAL: Initializing sound system\n";

    // If enumeration extension is present print found devices.
    if (alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT"))
    {
        // Enumerate the available output (playback) devices.
        std::cout << "Available OpenAL output devices:\n";

        const char* DeviceNames  =alcGetString(NULL, ALC_DEVICE_SPECIFIER);
        const char* DefaultDevice=alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);

        size_t       Offset=0;
        unsigned int DeviceNum=1;

        while (DeviceNames[Offset]!='\0')
        {
            const std::string PrintName=&DeviceNames[Offset];
            Offset+=PrintName.length()+1;   // Jump to next device name.
            std::cout << "\t" << DeviceNum << ". " << PrintName << (PrintName==DefaultDevice ? " [default]" : "") << "\n";
            DeviceNum++;
        }


        // Enumerate the available input (capture) devices.
        std::cout << "Available OpenAL capture devices:\n";

        DeviceNames  =alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
        DefaultDevice=alcGetString(NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);

        Offset=0;
        DeviceNum=1;

        while (DeviceNames[Offset]!='\0')
        {
            const std::string PrintName=&DeviceNames[Offset];
            Offset+=PrintName.length()+1;   // Jump to next device name.
            std::cout << "\t" << DeviceNum << ". " << PrintName << (PrintName==DefaultDevice ? " [default]" : "") << "\n";
            DeviceNum++;
        }
    }

    if (alcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT"))
    {
        // Enumerate the available output (playback) devices with the ALC_ENUMERATE_ALL_EXT extension.
        std::cout << "Available OpenAL output devices with the ALC_ENUMERATE_ALL_EXT extension:\n";

        const char* DeviceNames  =alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
        const char* DefaultDevice=alcGetString(NULL, ALC_DEFAULT_ALL_DEVICES_SPECIFIER);

        size_t       Offset=0;
        unsigned int DeviceNum=1;

        while (DeviceNames[Offset]!='\0')
        {
            const std::string PrintName=&DeviceNames[Offset];
            Offset+=PrintName.length()+1;   // Jump to next device name.
            std::cout << "\t" << DeviceNum << ". " << PrintName << (PrintName==DefaultDevice ? " [default]" : "") << "\n";
            DeviceNum++;
        }
    }

    // Use default device.
    m_Device=alcOpenDevice(NULL);

    if (m_Device==NULL)
    {
        std::cout << "OpenAL: Couldn't open default device (Error: " << TranslateErrorCode(alGetError()) << ")\n";
        return false;
    }

    // std::cout << "OpenAL: Opened default device.\n";

    // const char* Extensions=alcGetString(m_Device, ALC_EXTENSIONS);

    // std::cout << "OpenAL: Available extensions: " << (Extensions!=NULL ? Extensions : "no extensions found") << "\n";

    m_Context=alcCreateContext(m_Device, NULL);

    if (m_Context==NULL)
    {
        std::cout << "OpenAL: Couldn't open context on device (Error: " << TranslateErrorCode(alGetError()) << ")\n";
        return false;
    }

    alcMakeContextCurrent(m_Context);

    // Initialize ALUT.
    if (!alutInitWithoutContext(NULL, NULL))
    {
        std::cout << "OpenAL: Couldn't initialize ALUT library\n";
        alcMakeContextCurrent(NULL);
        alcDestroyContext(m_Context);
        alcCloseDevice(m_Device);
        assert(alGetError()==AL_NO_ERROR);
        return false;
    }

    // const char* SupportedBuffer=alutGetMIMETypes(ALUT_LOADER_BUFFER);
    // const char* SupportedMemory=alutGetMIMETypes(ALUT_LOADER_MEMORY);

    // std::cout << "OpenAL: Supported MIME types (Buffer): '" << (SupportedBuffer!=NULL ? SupportedBuffer : "none") << "'\n";
    // std::cout << "OpenAL: Supported MIME types (Memory): '" << (SupportedMemory!=NULL ? SupportedMemory : "none") << "'\n";

    assert(alGetError()==AL_NO_ERROR);

    m_IsInitialized=true;

    return true;
}


void SoundSysImplT::Release()
{
    // Sound system hasn't been initialized.
    if (!m_IsInitialized) return;

    // std::cout << "OpenAL: Releasing sound system\n";

    m_MixerTrackManager->ReleaseAll();
    m_BufferManager->ReleaseAll();

    alcMakeContextCurrent(NULL);
    alcDestroyContext(m_Context);
    alcCloseDevice(m_Device);
    // assert(alGetError()==AL_NO_ERROR);   // Commented out because it causes me trouble in Linux right now. TODO/FIXME!
    alutExit();

    m_IsInitialized=false;
}


bool SoundSysImplT::IsSupported()
{
    // Open default device to check if OpenAL is supported.
    m_Device=alcOpenDevice(NULL);

    if (m_Device==NULL) return false;

    alcCloseDevice(m_Device);
    m_Device=NULL;

    return true;
}


int SoundSysImplT::GetPreferenceNr()
{
    return 1500;
}


SoundI* SoundSysImplT::CreateSound2D(const SoundShaderT* SoundShader)
{
    // Create "empty" sound object if no sound shader is passed.
    // This is often the case if the game code doesn't check if a sound shader is registered and simply passes the result of
    // SoundShaderManager->GetSoundShader("xxx") to this method.
    if (SoundShader==NULL) return new SoundImplT(this, false);

    try
    {
        return new SoundImplT(this, false, SoundShader,
                              m_BufferManager->GetBuffer(SoundShader->AudioFile, false /*ForceMono*/, SoundShader->LoadType));
    }
    catch (const std::runtime_error& RE)
    {
        std::cout << __FUNCTION__ << ": " << RE.what();
    }

    return NULL;
}


SoundI* SoundSysImplT::CreateSound3D(const SoundShaderT* SoundShader)
{
    // Create "empty" sound object if no sound shader is passed.
    // This is often the case if the game code doesn't check if a sound shader is registered and simply passes the result of
    // SoundShaderManager->GetSoundShader("xxx") to this method.
    if (SoundShader==NULL) return new SoundImplT(this, true);

    try
    {
        return new SoundImplT(this, true, SoundShader,
                              m_BufferManager->GetBuffer(SoundShader->AudioFile, true /*ForceMono*/, SoundShader->LoadType));
    }
    catch (const std::runtime_error& RE)
    {
        std::cout << __FUNCTION__ << ": " << RE.what();
    }

    return NULL;
}


void SoundSysImplT::DeleteSound(SoundI* Sound)
{
    delete Sound;
}


bool SoundSysImplT::PlaySound(const SoundI* Sound)
{
    // We can safely cast SoundI to SoundT, since we have created it ourselves.
    assert(dynamic_cast<const SoundImplT*>(Sound)!=NULL);
    SoundImplT* SoundTmp=(SoundImplT*)Sound;

    // Only play valid sounds.
    if (SoundTmp->Buffer==NULL) return false;

    MixerTrackT* MixerTrack=m_MixerTrackManager->GetMixerTrack();

    if (MixerTrack==NULL) return false;

    return MixerTrack->Play(SoundTmp);
}


void SoundSysImplT::SetMasterVolume(float Volume)
{
    if (Volume>1.0f) Volume=1.0f;
    if (Volume<0.0f) Volume=0.0f;

    alListenerf(AL_GAIN, Volume);

    assert(alGetError()==AL_NO_ERROR);
}


float SoundSysImplT::GetMasterVolume()
{
    float MasterVolume=0.0f;

    alGetListenerf(AL_GAIN, &MasterVolume);

    return MasterVolume;
}


void SoundSysImplT::Update()
{
    m_BufferManager->UpdateAll();
    m_MixerTrackManager->UpdateAll();
}


void SoundSysImplT::UpdateListener(const Vector3dT& Position, const Vector3dT& Velocity, const Vector3fT& OrientationForward, const Vector3fT& OrientationUp)
{
    // Update 3D sound attributes.
    const double METERS_PER_WORLD_UNIT = 0.0254;

    float PositionTriplet[3]  ={ float(Position.x * METERS_PER_WORLD_UNIT),
                                 float(Position.y * METERS_PER_WORLD_UNIT),
                                 float(Position.z * METERS_PER_WORLD_UNIT) };
    float VelocityTriplet[3]  ={ float(Velocity.x * METERS_PER_WORLD_UNIT),
                                 float(Velocity.y * METERS_PER_WORLD_UNIT),
                                 float(Velocity.z * METERS_PER_WORLD_UNIT) };
    float OrientationSextet[6]={ OrientationForward.x,
                                 OrientationForward.y,
                                 OrientationForward.z,
                                 OrientationUp.x,
                                 OrientationUp.y,
                                 OrientationUp.z };

    alListenerfv(AL_POSITION,    PositionTriplet  );
    alListenerfv(AL_VELOCITY,    VelocityTriplet  );
    alListenerfv(AL_ORIENTATION, OrientationSextet);

    assert(alGetError()==AL_NO_ERROR);
}
