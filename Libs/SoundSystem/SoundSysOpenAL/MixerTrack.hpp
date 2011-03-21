/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#ifndef _SOUNDSYS_MIXER_TRACK_HPP_
#define _SOUNDSYS_MIXER_TRACK_HPP_

#include "OpenALIncl.hpp"

#include <string>


class SoundImplT;


/// A mixer track represents/encapsulates/abstracs an OpenAL sound source.
/// Playing a sound assigns a mixer track to the sound (and the sounds buffer to the mixer track).
/// A mixer track can only be assigned to and play back one sound at a time.
/// Mixer tracks are a limited resource; they are reused and managed by the MixerTrackManT singleton.
class MixerTrackT
{
    public:

    /// The constructor. Throws an exception of type std::runtime_error on failure.
    MixerTrackT();

    /// The destructor. Stops the sound, detaches, and releases the OpenAL source.
    ~MixerTrackT();

    /// Plays a sound.
    /// The previously assigned sound is stopped and detached, the new sound is attached and played.
    /// @param Sound   The sound to play.
    /// @returns whether the attempt to play the sound was succesful.
    bool Play(SoundImplT* Sound);

    /// Pauses the currently played sound.
    void Pause();

    /// Resumes a previously paused sound.
    void Resume();

    /// Stops the currently played sound and detaches it from this mixer track.
    void StopAndDetach();

    /// Checks if this mixer track is playing the sound currently attached to it.
    /// @return Whether the sound is currently playing.
    bool IsPlaying();

    /// Checks if this mixer track is currently used the sound currently attached to it.
    /// This is more general than IsPlaying() since it also returns mixer tracks as used if the
    /// sound attached to them is not playing but paused. Stopped sounds will result don't mark
    /// a mixer track as used.
    /// @return Whether the mixer track is currently used.
    bool IsUsed();

    /// Returns information about the priority of the currently played sound object.
    /// @return Priority of the currently played sound object. Higher values mean higher priority.
    unsigned int GetPriority();

    /// Updates the mixer track according to the attached sound.
    /// Updated attributes include the position, velocity and direction, as well as shader attributes
    /// like the minimal distance, the volume, etc.
    void Update();

    /// Returns the handle to the OpenAL "source" that is encapsulated by this mixer track.
    ALuint GetOpenALSource() const { return m_SourceHandle; }


    private:

    MixerTrackT(const MixerTrackT&);        ///< Use of the Copy    Constructor is not allowed.
    void operator = (const MixerTrackT&);   ///< Use of the Assignment Operator is not allowed.

    SoundImplT* m_Sound;        ///< The sound that is currently attached (but not necessarily played) to the mixer track.
    ALuint      m_SourceHandle; ///< The OpenAL source handle encapsulated by this mixer track.
};

#endif
