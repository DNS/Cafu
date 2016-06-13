/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SOUND_SYS_IMPL_HPP_INCLUDED
#define CAFU_SOUND_SYS_IMPL_HPP_INCLUDED

#include "OpenALIncl.hpp"
#include "MixerTrackMan.hpp"
#include "../SoundSys.hpp"
#include "Templates/Array.hpp"
#include <map>


class SoundStreamT;
class BufferManagerT;


/// OpenAL implementation of the sound system.
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


    /// Creates an instance of the OpenAL sound system.
    static SoundSysImplT& GetInstance();


    private:

    // Note that the order in which these two members are destroyed is important: First the mixer track manager needs to be destroyed
    // because it detaches all buffers from currently connected sources. Then all buffers can be deleted by destroying the buffer manager.
    BufferManagerT* m_BufferManager;     ///< Buffer manager used by this sound system.
    MixerTrackManT* m_MixerTrackManager; ///< Mixer track manager used by this sound system.

    ALCdevice*  m_Device;  ///< The device which OpenAL is using to play sounds.
    ALCcontext* m_Context; ///< The context in which the sounds are played.

    bool m_IsInitialized; ///< Whether the sound system has been initialized.

    SoundSysImplT();  ///< Private constructor for the Singleton pattern.
    ~SoundSysImplT(); ///< Private destructor.
};

#endif
