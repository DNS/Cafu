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
