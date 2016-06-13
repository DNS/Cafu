/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SOUND_IMPL_HPP_INCLUDED
#define CAFU_SOUND_IMPL_HPP_INCLUDED

#include "../Sound.hpp"


class SoundSysImplT;
class SoundShaderT;
class BufferT;


/// FMOD3 implementation of the sound object.
class SoundImplT : public SoundI
{
    public:

    /// Constructor. Creates a new sound object using a specified sound shader and buffer.
    /// If either sound shader or buffer are NULL the sound object is invalid and playing invalid
    /// sound files will fail.
    /// @param SoundSys The sound system this sound object is created in.
    /// @param Is3D_ Whether this sound is created as an 3 dimensional sound or not.
    /// @param Shader_ The sound shader used by this sound object.
    /// @param Buffer_ The sound buffer that contains the audio data played by this sound object.
    SoundImplT(SoundSysImplT* SoundSys, bool Is3D_, const SoundShaderT* Shader_=NULL, BufferT* Buffer_=NULL);

    /// Destructor.
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

    BufferT*            Buffer;         ///< The buffer played by this sound object.
    int                 ChannelHandle;  ///< Handle to the channel this sound is currently played on or -1. Note that the ChannelHandle once set is never set back to -1, even if the sound isn't attached to that channel anymore. This is handled internally by FMOD and a sound can never access a channel it isn't attached to.
    Vector3dT           Position;       ///< Position of this sound in the world (only relevant if sound is 3D).
    Vector3dT           Velocity;       ///< Velocity of the the object that emits this sound (only relevant if sound is 3D).
    Vector3dT           Direction;      ///< Direction of the sound (only relevant if ConeAngle (defined in shader) is smaller than 360 degree and sound is 3D).
    float               Volume;         ///< The volume of this sound (inside its sound cone/at minimal distance). 1.0 meaning 100% volume and 0.0 mute sound.
    float               MinDistance;    ///< The minimum distance that the sound will cease to continue growing louder at (stays at max. volume).
    float               MaxDistance;    ///< The maximum distance that the sound will cease to attenuate.
    const bool          Is3DSound;      ///< Whether the sound is 2D or 3D.
    unsigned int        Priority;       ///< Priority of the sound object. Values <0 mean that the priority is not set and therefore obtained from the sound shader.

    private:

    const SoundShaderT* m_Shader;   ///< The sound shader used by this sound object.
    SoundSysImplT*      m_SoundSys; ///< Pointer to the sound system that created this sound. Needed to control playback trough the sound instead of the soundsystem.
};

#endif
