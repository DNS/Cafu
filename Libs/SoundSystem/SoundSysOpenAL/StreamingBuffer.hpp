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

#ifndef _SOUNDSYS_STREAMING_BUFFER_HPP_
#define _SOUNDSYS_STREAMING_BUFFER_HPP_

#include "Buffer.hpp"


class SoundStreamT;


/// A streaming buffer created from an audio file.
/// Streaming buffers stream the audio data from a file instead of loading
/// it completely into memory.
/// Note that internally a streaming buffer consists of a number of OpenAL
/// buffers that are created and deleted on demand.
/// Also streaming buffers can only be attached to one mixer track at a time.
/// This is due to the streaming nature of streaming buffers (one buffer cannot
/// have different playback positions).
class StreamingBufferT : public BufferT
{
    public:

    /// The constructor.
    /// @param FileName    The name of the audio file that this buffer is created from.
    /// @param ForceMono   Whether the data from the resource should be reduced to a single channel before use (mono output).
    StreamingBufferT(const std::string& FileName, bool ForceMono);

    /// The destructor.
    ~StreamingBufferT();

    // BufferT implementation.
    unsigned int GetChannels() const;
    void Update();
    void Rewind();
    bool IsStream() const;
    bool AttachToMixerTrack(MixerTrackT* MixerTrack);
    bool DetachFromMixerTrack(MixerTrackT* MixerTrack);


    private:

    /// Fills the given buffers with new stream data and queues them on the mixer track (the OpenAL source).
    /// If the stream has no more data, only the required buffers are processed and the m_EndReached flag is set.
    void FillAndQueue(const ArrayT<ALuint>& Buffers);

    SoundStreamT*  m_Stream;        ///< The stream that provides the PCM data for the buffers.
    ArrayT<ALuint> m_Buffers;       ///< The buffers that are queued on the source and played alternately with current data from the stream.
    bool           m_EndReached;    ///< Stream has reached the end, don't update anymore.
};

#endif
