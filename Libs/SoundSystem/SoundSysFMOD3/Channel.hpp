/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CHANNEL_HPP_INCLUDED
#define CAFU_CHANNEL_HPP_INCLUDED


class SoundImplT;


/// This is a wrapper around a FMOD3 sound channel.
/// It links a FMOD3 sound channel handle to a sound object and provides the means
/// to update the channel according to the sound objects current volume, position, etc.
class ChannelT
{
    public:

    /// Constructor. Creates a new channel.
    /// @param ChannelHandle_ FMOD3 channdel handle used for this channel. Can be changed at anytime once the object is created.
    /// @param Sound_ Sound object used for this channel. Can be changed at anytime once the object is created.
    ChannelT(int ChannelHandle_=-1, const SoundImplT* Sound_=0); //XXX Using NULL here results in compiler error: 'NULL': nichtdeklarierter Bezeichner

    /// Updates the FMOD3 channel handle properties according to the current sound object.
    void Update();

    int               ChannelHandle; ///< The current FMOD3 channel handle used with this channel.
    const SoundImplT* Sound;         ///< The current sound object used with this channel.
};

#endif
