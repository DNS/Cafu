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

#ifndef CAFU_SOUND_IMPL_HPP_INCLUDED
#define CAFU_SOUND_IMPL_HPP_INCLUDED

#include "../Sound.hpp"


class SoundShaderT;
class BufferT;
class MixerTrackT;
class SoundSysImplT;


/// OpenAL implementation of the SoundI interface.
/// @todo Implement copy construtor that takes shader and buffer (+increases references) but sets mixertrack always to NULL.
class SoundImplT : public SoundI
{
    public:

    /// The constructor. Creates a new sound using a specified sound shader and buffer.
    /// If either sound shader or buffer are NULL the sound is invalid, and playing invalid
    /// sound files will fail.
    /// @param SoundSys   The sound system this sound is created in.
    /// @param Is3D_      Whether this sound is created as an 3 dimensional sound or not.
    /// @param Shader_    The sound shader used by this sound.
    /// @param Buffer_    The buffer that contains the audio data played by this sound.
    SoundImplT(SoundSysImplT* SoundSys, bool Is3D_, const SoundShaderT* Shader_=NULL, BufferT* Buffer_=NULL);

    /// The destructor. Deletes the sound by removing it from its current mixer track (if any) and buffer.
    ~SoundImplT();

    // Implement the SoundI interface.
    bool         Play();
    void         Stop();
    void         Pause();
    bool         Resume();
    bool         IsPlaying() const;
    bool         Is3D() const;
    void         ResetProperties();
    void         SetPosition(const Vector3dT& Position_);
    void         SetVelocity(const Vector3dT& Velocity_);
    void         SetDirection(const Vector3dT& Direction_);
    void         SetPriority(unsigned int Priority_);
    void         SetInnerVolume(float InnerVolume_);
    void         SetMinDistance(float MinDist_);
    void         SetMaxDistance(float MaxDist_);
    void         SetInnerConeAngle(float InnerConeAngle_);
    void         SetOuterConeAngle(float OuterConeAngle_);
    void         SetOuterVolume(float OuterVolume_);
    unsigned int GetPriority() const;
    float        GetInnerVolume() const;
    float        GetMinDistance() const;
    float        GetMaxDistance() const;
    float        GetInnerConeAngle() const;
    float        GetOuterConeAngle() const;
    float        GetOuterVolume() const;


    BufferT*            Buffer;         ///< The buffer played by this sound.
    MixerTrackT*        MixerTrack;     ///< The mixertrack on which this sound is played.
    Vector3dT           Position;       ///< Position of this sound in the world (only relevant if sound is 3D).
    Vector3dT           Velocity;       ///< Velocity of the the object that emits this sound (only relevant if sound is 3D).
    Vector3dT           Direction;      ///< Direction of the sound (only relevant if ConeAngle (defined in shader) is smaller than 360 degree and sound is 3D).
    float               InnerVolume;    ///< The volume of this sound (inside its sound cone/at minimal distance). 1.0 meaning 100% volume and 0.0 mute sound.
    float               OuterVolume;    ///< The sounds volume if listener is outside the sound cone.
    float               InnerConeAngle; ///< The inner angle of the cone in which the sound is emited at normal volume.
    float               OuterConeAngle; ///< The outer angle of the cone outside which the sound is emited at outside volume.
    float               MinDistance;    ///< The minimum distance that the sound will cease to continue growing louder at (stays at max. volume).
    float               MaxDistance;    ///< The maximum distance that the sound will cease to attenuate.
    const bool          Is3DSound;      ///< Whether the sound is 2D or 3D.
    unsigned int        Priority;       ///< Priority of the sound. Values <0 mean that the priority is not set and therefore obtained from the sound shader.


    private:

    const SoundShaderT* m_Shader;   ///< The sound shader used by this sound object.
    SoundSysImplT*      m_SoundSys; ///< Pointer to the sound system that created this sound. Needed to control playback trough the sound instead of the soundsystem.
};

#endif
