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

#ifndef CAFU_SOUNDSYS_MIXER_TRACK_MAN_HPP_INCLUDED
#define CAFU_SOUNDSYS_MIXER_TRACK_MAN_HPP_INCLUDED

#include "Templates/Array.hpp"


class MixerTrackT;


/// The mixer track manager manages the limited mixer tracks that are needed to playback a sound object.
class MixerTrackManT
{
    public:

    /// Returns an instance to the global mixer track manager.
    static MixerTrackManT* GetInstance();

    /// Interface to get a free mixer track that can be used to playback a sound object.
    /// The method checks all currently available mixer tracks for one that is no longer used (sound object
    /// attached to it is no longer playing) and returns it. If no "free" mixer track is found it attempts
    /// to create a new one and returns it.
    /// @param Priority The priority of this request for a mixer track. Usually one should use the priority of the
    ///        sound object this mixer track is intended for.
    /// @return The mixer track that can be used for playback or NULL if no mixer track could be optained.
    MixerTrackT* GetMixerTrack(unsigned int Priority=0);

    /// Deletes all mixer tracks that are currently unused (have no playing sound file attached to them).
    void CleanUp();

    /// Calls Update() for all mixer tracks.
    void UpdateAll();

    /// Releases all mixer tracks.
    void ReleaseAll();




    private:

    ArrayT<MixerTrackT*> MixerTracks; ///< The mixer tracks managed by this mixer track manager.

    /// Private constructor for singleton pattern.
    MixerTrackManT();

    /// Destructor. Deletes all mixer tracks that are managed by this manager.
    ~MixerTrackManT();
};

#endif
