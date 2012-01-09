/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#ifndef _SOUND_SYS_IMPL_HPP_
#define _SOUND_SYS_IMPL_HPP_

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
