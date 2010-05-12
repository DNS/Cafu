/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#ifndef _SOUNDSYS_SOUND_HPP_
#define _SOUNDSYS_SOUND_HPP_

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
