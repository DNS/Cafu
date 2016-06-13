/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
