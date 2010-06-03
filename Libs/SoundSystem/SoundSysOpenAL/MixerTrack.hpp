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

#ifndef _SOUNDSYS_MIXER_TRACK_HPP_
#define _SOUNDSYS_MIXER_TRACK_HPP_

#include "OpenALIncl.hpp"

#include <string>


class SoundImplT;


/// A mixer track to playback sound objects.
/// Mixer tracks are used to playback sounds through the sound system. A mixer track can only play one sound
/// at a time. Since mixer tracks are a limited resource they need to be reused (see MixerTrackManT).
/// This mixer track encapsulates an OpenAL source that is used for playback.
class MixerTrackT
{
    public:

    /// Error that is thrown when the creation of a mixer track fails.
    class CreateErrorE
    {
        public:

        /// Constructor.
        /// @param ErrorMsg This errors information message.
        CreateErrorE(std::string ErrorMsg) : Message(ErrorMsg) { }

        const std::string Message; ///< Information about the error.
    };

    /// Contructor. Generates an OpenAL source.
    MixerTrackT();

    /// Destructor. Detaches the currently attached sound and destroys the OpenAL source.
    ~MixerTrackT();

    /// Play the passed sound object on this mixer track.
    /// If a sound is already attached to the mixer track it is detached before the new sound is played.
    /// @param Sound The sound object to play.
    /// @return Whether the attempt to playback the sound was succesfull.
    bool PlaySound(SoundImplT* Sound);

    /// Stops the currently played sound if any.
    void StopCurrent();

    /// Pauses the currently played sound.
    void PauseCurrent();

    /// Resumes the currently attached sound if it is paused.
    void ResumeCurrent();

    /// Detaches the sound object currently attached to this mixer track.
    /// This is done regardless if it is played (in this case the sound is stopped) or not.
    void DetachCurrentSound();

    /// Checks if this mixer track is playing the sound currently attached to it.
    /// @return Whether the sound is currently playing.
    bool IsPlaying();

    /// Checks if this mixer track is currently used the sound currently attached to it.
    /// This is more general than IsPlaying() since it also returns mixer tracks as used if the
    /// sound attached to them is not playing but paused. Stopped sounds will result don't mark
    /// a mixer track as used.
    /// @return Whether the mixe track is currently used.
    bool IsUsed();

    /// Returns information about the priority of the currently played sound object.
    /// @return Priority of the currently played sound object. Higher values mean higher priority.
    unsigned int GetPriority();

    /// Updates the mixer track according to the information of the currently attached sound object.
    /// Attributes that are update contain the position, velocity, direction as well as shader attributes like
    /// the minimal distance, the volume, etc.
    void Update();

    /// Returns the handle to the OpenAL "source" that is encapsulated by this mixer track.
    ALuint GetOpenALSource() const { return m_SourceHandle; }


    private:

    MixerTrackT(const MixerTrackT&);        ///< Use of the Copy    Constructor is not allowed.
    void operator = (const MixerTrackT&);   ///< Use of the Assignment Operator is not allowed.

    SoundImplT* m_CurrentSound; ///< The sound that is currently attached (but not necessarily played) to the mixer track.
    ALuint      m_SourceHandle; ///< The OpenAL source handle encapsulated by this mixer track.
};

#endif
