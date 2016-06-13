/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SOUND_SYS_IMPL_HPP_INCLUDED
#define CAFU_SOUND_SYS_IMPL_HPP_INCLUDED

#include "../SoundSys.hpp"
#include "Templates/Array.hpp"

#include <map>


class  ChannelT;
class  BufferManagerT;
struct SoundFileT;
struct FSOUND_STREAM;
struct FSOUND_SAMPLE;


/// FMOD3 implementation of the sound system.
class SoundSysImplT : public SoundSysI
{
    public:

    // Implement the SoundSysI interface.
    bool Initialize();
    void Release();

    bool IsSupported();
    int GetPreferenceNr();

    SoundI* CreateSound2D(const SoundShaderT* SoundShader);
    SoundI* CreateSound3D(const SoundShaderT* SoundShader);
    void DeleteSound(SoundI* Sound);

    bool PlaySound(const SoundI* Sound);

    void  SetMasterVolume(float Volume);
    float GetMasterVolume();

    void Update();
    void UpdateListener(const Vector3dT& Position, const Vector3dT& Velocity, const Vector3fT& OrientationForward, const Vector3fT& OrientationUp);


    /// Creates an instance of the FMOD3 sound system.
    static SoundSysImplT& GetInstance();


    private:

    ArrayT<ChannelT> m_Channels; ///< Array of all channels that the soundsystem can use.

    BufferManagerT* m_BufferManager; ///< The buffer manager that manages FMOD sound samples and streams.

    bool m_IsInitialized; ///< Whether the sound system has been initialized.

    SoundSysImplT(); ///< Private constructor for the Singleton pattern.
    ~SoundSysImplT(); ///< Private destructor.
};

#endif
