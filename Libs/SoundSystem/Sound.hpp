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


/// Class representing a sound object that can be played.
/// Sound objects are created using a sound system and are then controlled by the calling code that triggers
/// playback, deletes them and adjusts its attributes like the current position, velocity and direction.
/// Creating a sound object trough the sound system is done by passing a sound shader to it that is used as
/// a template for the sound objects properties. These properties can be changed by the calling code at will.
/// It is possible to change aspects of the sound such as the volume, the sound cone, etc.
class SoundI
{
    public:

    /// Plays the sound using the current sound system.
    virtual bool Play()=0;

    /// Stops the sound.
    virtual void Stop()=0;

    /// Pauses the sound.
    virtual void Pause()=0;

    /// Resumes a previously pasued sound.
    virtual bool Resume()=0;

    /// Checks if the sound object is currently playing.
    virtual bool IsPlaying() const=0;

    /// Checks if the sound object is 3 dimensional. Otherwise it is 2 dimensional.
    virtual bool Is3D() const=0;

    /// Resets the sound objects properties to those of the shader it was created from.
    virtual void ResetProperties()=0;

    /// Set the position from which the sound is emanating.
    virtual void SetPosition(const Vector3dT& Position_)=0;

    /// Sets the velocity of the object that emits the sound.
    virtual void SetVelocity(const Vector3dT& Velocity_)=0;

    /// Sets the direction to which the sound is emanating.
    virtual void SetDirection(const Vector3dT& Direction_)=0;

    /// Sets the priority for this sound object.
    virtual void SetPriority(unsigned int Priority_)=0;

    /// Sets the volume for this sound object.
    virtual void SetInnerVolume(float InnerVolume_)=0;

    /// Sets the minimal distance for this sound object.
    virtual void SetMinDistance(float MinDist_)=0;

    /// Sets the maximal distance for this sound object.
    virtual void SetMaxDistance(float MaxDist_)=0;

    /// Sets the angle of the inner sound cone for this sound object.
    virtual void SetInnerConeAngle(float InnerConeAngle_)=0;

    /// Sets the angle of the outer sound cone for this sound object.
    virtual void SetOuterConeAngle(float OuterConeAngle_)=0;

    /// Sets the volume inside the outer sound cone for this sound object.
    virtual void SetOuterVolume(float OuterVolume_)=0;

    /// Gets the priority of this sound object.
    virtual unsigned int GetPriority() const=0;

    /// Gets the volume of this sound object.
    virtual float GetInnerVolume() const=0;

    /// Gets the minimal distance of this sound object.
    virtual float GetMinDistance() const=0;

    /// Gets the maximal distance of this sound object.
    virtual float GetMaxDistance() const=0;

    /// Gets the angle of the inner sound cone of this sound object.
    virtual float GetInnerConeAngle() const=0;

    /// Sets the angle of the outer sound cone of this sound object.
    virtual float GetOuterConeAngle() const=0;

    /// Gets the volume inside the outer sound cone of this sound object.
    virtual float GetOuterVolume() const=0;

    /// The virtual destructor makes sure that deleting derived classes via a SoundI pointer works properly.
    virtual ~SoundI() { }
};

#endif
