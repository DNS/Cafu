/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SOUNDSYS_SOUND_HPP_INCLUDED
#define CAFU_SOUNDSYS_SOUND_HPP_INCLUDED

#include "Math3D/Vector3.hpp"


/// This class represents a sound.
/// Sounds are created and destroyed via the appropriate methods in the sound system.
class SoundI
{
    public:

    /// Plays the sound using the current sound system.
    virtual bool Play()=0;

    /// Stops the sound.
    virtual void Stop()=0;

    /// Pauses the sound.
    virtual void Pause()=0;

    /// Resumes a previously paused sound.
    virtual bool Resume()=0;

    /// Checks if the sound is currently playing.
    virtual bool IsPlaying() const=0;

    /// Checks if the sound is for 3D playback.
    virtual bool Is3D() const=0;

    /// Resets the sound properties to those of the shader it was created from.
    virtual void ResetProperties()=0;

    /// Set the position from which the sound is emanating.
    virtual void SetPosition(const Vector3dT& Position_)=0;

    /// Sets the velocity of the source that emits the sound.
    virtual void SetVelocity(const Vector3dT& Velocity_)=0;

    /// Sets the direction into which the sound source is moving.
    virtual void SetDirection(const Vector3dT& Direction_)=0;

    /// Sets the priority of this sound.
    virtual void SetPriority(unsigned int Priority_)=0;

    /// Sets the volume.
    virtual void SetInnerVolume(float InnerVolume_)=0;

    /// Sets the minimal distance.
    virtual void SetMinDistance(float MinDist_)=0;

    /// Sets the maximal distance.
    virtual void SetMaxDistance(float MaxDist_)=0;

    /// Sets the angle of the inner sound cone.
    virtual void SetInnerConeAngle(float InnerConeAngle_)=0;

    /// Sets the angle of the outer sound cone.
    virtual void SetOuterConeAngle(float OuterConeAngle_)=0;

    /// Sets the volume inside the outer sound cone.
    virtual void SetOuterVolume(float OuterVolume_)=0;

    /// Gets the priority of this sound.
    virtual unsigned int GetPriority() const=0;

    /// Gets the volume.
    virtual float GetInnerVolume() const=0;

    /// Gets the minimal distance.
    virtual float GetMinDistance() const=0;

    /// Gets the maximal distance.
    virtual float GetMaxDistance() const=0;

    /// Gets the angle of the inner sound cone.
    virtual float GetInnerConeAngle() const=0;

    /// Sets the angle of the outer sound cone.
    virtual float GetOuterConeAngle() const=0;

    /// Gets the volume inside the outer sound cone.
    virtual float GetOuterVolume() const=0;

    /// The virtual destructor makes sure that deleting derived classes via a SoundI pointer works properly.
    virtual ~SoundI() { }
};

#endif
