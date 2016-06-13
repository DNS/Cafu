/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SOUNDSYS_BUFFER_HPP_INCLUDED
#define CAFU_SOUNDSYS_BUFFER_HPP_INCLUDED

#include "OpenALIncl.hpp"
#include "Templates/Array.hpp"

#include <string>


class MixerTrackT;


/// A BufferT encapsulates an audio resource such as a file or a capture device.
/// It is responsible for managing the OpenAL buffer(s) in an OpenAL source,
/// such as creating and filling the buffer(s), assigning or queuing them to the source, etc.
class BufferT
{
    public:

    /// The constructor.
    /// @param ResName     The name of the resource (file or capture device) that this buffer is created from.
    /// @param ForceMono   Whether the data from the resource should be reduced to a single channel before use (mono output).
    BufferT(const std::string& ResName, bool ForceMono);

    /// The virtual destructor, so that derived classes can safely be deleted via a BufferT (base class) pointer.
    virtual ~BufferT() { }

    /// Returns the name of the resource (file or capture device) that this buffer was created from.
    const std::string& GetName() const { return m_ResName; }

    /// Returns whether the data from the resource is reduced to a single channel before use (mono output).
    bool ForcesMono() const { return m_ForceMono; }

    /// Returns the number of audio channels in this buffer (1 is mono, 2 is stereo).
    virtual unsigned int GetChannels() const=0;

    /// Returns whether this buffer can be attached to multiple mixer tracks (resource sharing).
    virtual bool CanShare() const=0;

    /// Updates the buffer.
    virtual void Update()=0;

    /// Attaches the buffer to a mixer track, so the mixer track can play this buffer.
    /// Note that depending on the underlying buffer it is possible to attach one buffer to multiple mixer tracks.
    /// As for streaming buffers this is not possible since they are unique and can only be attached to one mixer
    /// track.
    /// @param MixerTrack The mixer track this buffer should be attached to.
    /// @return Whether the buffer could be attached to the mixer track.
    virtual bool AttachToMixerTrack(MixerTrackT* MixerTrack)=0;

    /// Detaches the buffer from a mixer track.
    /// This method informs the mixer track as well as the buffer of the change.
    /// @param MixerTrack The mixer track this buffer should be attached from.
    /// @return Whether the buffer could be attached to the mixer track. False means usually that this buffer wasn't
    ///         attached to the passed mixer track at all.
    virtual bool DetachFromMixerTrack(MixerTrackT* MixerTrack)=0;


    unsigned int References; ///< Number of references to this buffer (e.g. how many sound objects use this buffer).


    protected:

    ArrayT<MixerTrackT*> m_MixerTracks; ///< Mixer tracks this buffer is currently attached to.

    /// Converts signed 16 bit raw PCM data from stereo to mono.
    /// @param Buffer The PCM data to convert to mono.
    /// @param Size Size of the data in bytes.
    /// @return The resulting buffer size.
    unsigned int ConvertToMono(unsigned char* Buffer, unsigned int Size);


    private:

    BufferT(const BufferT&);            ///< Use of the Copy    Constructor is not allowed.
    void operator = (const BufferT&);   ///< Use of the Assignment Operator is not allowed.

    const std::string m_ResName;        ///< Name of the resource (file or capture device) that this buffer was created from.
    const bool        m_ForceMono;      ///< Whether the data from the resource is reduced to a single channel before use (mono output).
};

#endif
