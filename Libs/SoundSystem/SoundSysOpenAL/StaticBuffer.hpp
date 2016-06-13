/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SOUNDSYS_STATIC_BUFFER_HPP_INCLUDED
#define CAFU_SOUNDSYS_STATIC_BUFFER_HPP_INCLUDED

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
