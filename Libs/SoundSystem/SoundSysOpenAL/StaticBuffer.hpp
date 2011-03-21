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

#ifndef _SOUNDSYS_STATIC_BUFFER_HPP_
#define _SOUNDSYS_STATIC_BUFFER_HPP_

#include "Buffer.hpp"


/// A StaticBufferT is a BufferT specialization for audio data from a file whose contents fits entirely into memory.
/// StaticBufferT instances can be shared and thus independently be used by multiple mixer tracks at the same time.
class StaticBufferT : public BufferT
{
    public:

    /// The constructor. Throws an exception of type std::runtime_error on failure.
    /// @param FileName    The name of the audio file that this buffer is created from.
    /// @param ForceMono   Whether the data from the resource should be reduced to a single channel before use (mono output).
    StaticBufferT(const std::string& FileName, bool ForceMono);

    /// The destructor.
    ~StaticBufferT();

    // BufferT implementation.
    unsigned int GetChannels() const;
    bool CanShare() const;
    void Update();
    bool AttachToMixerTrack(MixerTrackT* MixerTrack);
    bool DetachFromMixerTrack(MixerTrackT* MixerTrack);


    private:

    ALuint m_Buffer;
};

#endif
