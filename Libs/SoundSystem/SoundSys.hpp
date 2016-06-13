/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SOUNDSYS_SOUNDSYS_INTERFACE_HPP_INCLUDED
#define CAFU_SOUNDSYS_SOUNDSYS_INTERFACE_HPP_INCLUDED

#include "Math3D/Vector3.hpp"


class SoundI;
class SoundShaderT;


/// This is an interface to the sound system.
/// The interface is specified as ABC in order to be able to share the sound system across exe/dll boundaries.
class SoundSysI
{
    public:

    /// Initializes the sound system.
    virtual bool Initialize()=0;

    /// Releases the sound system and removes all sound data from memory.
    virtual void Release()=0;

    /// Determine if the sound system is supported on this platform.
    /// @return Whether or not the sound system is supported.
    virtual bool IsSupported()=0;

    /// Returns the preference number for this sound system, so calling code can decide which sound system to use.
    /// @return Preference number.
    virtual int GetPreferenceNr()=0;

    /// Creates a 2 dimensional sound object using the properties of the passed sound shader.
    /// @param SoundShader Sound shader to use with this sound object.
    virtual SoundI* CreateSound2D(const SoundShaderT* SoundShader)=0;

    /// Creates a 3 dimensional sound object using the properties of the passed sound shader.
    /// @param SoundShader Sound shader to use with this sound object.
    virtual SoundI* CreateSound3D(const SoundShaderT* SoundShader)=0;

    /// Deletes a previously created sound object.
    /// @param Sound The sound object to delete.
    virtual void DeleteSound(SoundI* Sound)=0;

    /// Plays a sound on a channel.
    /// @param Sound The sound object that should be played.
    /// @return true if sound is played on a channel, false if no channel was free (and playing sound have all greater priority
    ///         than this sound).
    virtual bool PlaySound(const SoundI* Sound)=0;

    /// Sets the master volume for this sound system.
    /// @param Volume The new master volume. Value has to be [0, 1] (higher/lower values are clamped).
    virtual void SetMasterVolume(float Volume)=0;

    /// Gets the master volume currently set for this sound system.
    /// @return The current master volume [0, 1].
    virtual float GetMasterVolume()=0;

    /// Updates the position, velocity and orientation of the listener.
    /// @param Position Position of the listener in the 3D Space.
    /// @param Velocity Velocity of the listener.
    /// @param OrientationForward Forward orientation of the listener (unit length vector).
    /// @param OrientationUp Upwards orientation of the listener (unit length vector).
    virtual void UpdateListener(const Vector3dT& Position, const Vector3dT& Velocity, const Vector3fT& OrientationForward, const Vector3fT& OrientationUp)=0;

    /// Upates all channels that are currently being played according to the properties of their sound object.
    virtual void Update()=0;

    /// The virtual destructor makes sure that deleting derived classes via a SoundSysI pointer works properly.
    virtual ~SoundSysI() { }
};

/// A global pointer to the current soundsystem, for common access by all modules that use it.
/// Just set this after you loaded the desired sound DLL to the pointer returned by the DLLs GetSoundSys() function.
/// (And NULL it on unloading the DLL.)
extern SoundSysI* SoundSystem;


#endif
