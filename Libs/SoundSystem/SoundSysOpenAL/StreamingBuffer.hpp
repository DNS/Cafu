/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SOUNDSYS_STREAMING_BUFFER_HPP_INCLUDED
#define CAFU_SOUNDSYS_STREAMING_BUFFER_HPP_INCLUDED

#include "Buffer.hpp"


class SoundStreamT;


/// A StreamingBufferT is a BufferT specialization for audio data from a device or file whose contents is not kept in memory all at once.
/// Instead, the audio data is streamed from the resource and piecewise queued on the OpenAL source.
/// StreamingBufferT instances cannot be shared, each instance can only be used on a single mixer track.
class StreamingBufferT : public BufferT
{
    public:

    /// The constructor. Throws an exception of type std::runtime_error on failure.
    /// @param ResName     The name of the audio resource that this buffer is created from. ResName can be a file name or the name of an OpenAL capture device (as obtained from the ALC_CAPTURE_DEVICE_SPECIFIER list).
    /// @param ForceMono   Whether the data from the resource should be reduced to a single channel before use (mono output).
    StreamingBufferT(const std::string& ResName, bool ForceMono);

    /// The destructor.
    ~StreamingBufferT();

    // BufferT implementation.
    unsigned int GetChannels() const;
    bool CanShare() const;
    void Update();
    bool AttachToMixerTrack(MixerTrackT* MixerTrack);
    bool DetachFromMixerTrack(MixerTrackT* MixerTrack);


    private:

    /// Fills the given buffers with new stream data and queues them on the mixer track (the OpenAL source).
    /// If the stream has no more data, only the required buffers are processed and the m_EndReached flag is set.
    unsigned int FillAndQueue(const ArrayT<ALuint>& Buffers);

    SoundStreamT*  m_Stream;    ///< The stream that provides the PCM data for the buffers.
    ArrayT<ALuint> m_Buffers;   ///< The buffers that are queued on the source and played alternately with current data from the stream.
};

#endif
