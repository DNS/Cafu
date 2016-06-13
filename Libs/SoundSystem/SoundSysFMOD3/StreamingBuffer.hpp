/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SOUNDSYS_STREAMING_BUFFER_HPP_INCLUDED
#define CAFU_SOUNDSYS_STREAMING_BUFFER_HPP_INCLUDED

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
