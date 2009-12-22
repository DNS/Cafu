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
#include "OpenALIncl.hpp"


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

    /// Constructor.
    /// Creates a streaming buffer from an audio file.
    /// @param AudioFile Path to the audio file from which the buffer should be created.
    /// @param Is3DSound Whether the buffer is used as a 3D sound object.
    StreamingBufferT(const std::string& AudioFile, bool Is3DSound);

    /// Destructor.
    ~StreamingBufferT();

    // BufferT implementation.
    void Update();
    void Rewind();
    bool IsStream();
    bool AttachToMixerTrack(MixerTrackT* MixerTrack);
    bool DetachFromMixerTrack(MixerTrackT* MixerTrack);


    private:

    ArrayT<ALuint> m_CurrentBuffers; ///< Currently used buffers (needed in this array for deletion in stream destructor).
    SoundStreamT*  m_Stream;         ///< Sound stream used for this buffer.
    ALenum         m_OutputFormat;   ///< OpenAL output format used for stream buffers.
    bool           m_EndReached;     ///< Stream has reached the end, don't update anymore.

    // Don't allow use of copy and assignment constructor.
    StreamingBufferT(StreamingBufferT&);
    StreamingBufferT& operator=(const StreamingBufferT&);
};

#endif
