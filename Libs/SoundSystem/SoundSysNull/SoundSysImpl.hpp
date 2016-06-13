/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SOUND_SYS_IMPL_HPP_INCLUDED
#define CAFU_SOUND_SYS_IMPL_HPP_INCLUDED

#include "../SoundSys.hpp"


/// NULL implementation of the sound system.
/// This implementation is used for systems that don't have any audio hardware or where audio
/// output is not desired (e.g. game servers).
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


    /// Creates an instance of the NULL sound system.
    static SoundSysImplT& GetInstance();


    private:

    SoundSysImplT(); ///< Private constructor for the Singleton pattern.
};

#endif
