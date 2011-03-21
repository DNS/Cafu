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

#ifndef _SOUNDSYS_STREAMING_BUFFER_HPP_
#define _SOUNDSYS_STREAMING_BUFFER_HPP_

#include "Buffer.hpp"


struct FSOUND_STREAM;


/// A streaming buffer created from an audio file.
/// Streaming buffers stream the audio data from a file instead of loading
/// it completely into memory.
/// This class is basically just a wrapper around the FMOD3 sound stream object.
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
    int  AttachToChannel(unsigned int Priority);
    void Rewind();


    private:

    FSOUND_STREAM* m_Stream; ///< The FMOD3 sound stream used with this static buffer.

    // Don't allow use of copy and assignment constructor.
    StreamingBufferT(StreamingBufferT&);
    StreamingBufferT& operator=(const StreamingBufferT&);
};

#endif
