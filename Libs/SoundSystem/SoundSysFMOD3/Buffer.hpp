/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SOUNDSYS_BUFFER_HPP_INCLUDED
#define CAFU_SOUNDSYS_BUFFER_HPP_INCLUDED

#include "Templates/Array.hpp"

#include <string>


/// A sound buffer created from an audio file.
/// This basic version contains all attributes that are common for both a static buffer and a streaming buffer
/// and provides a general interface, so the calling code doesn't need to distinguish between different buffer
/// types.
class BufferT
{
    public:

    /// Constructor.
    BufferT(const std::string& FileName_="") : References(0), FileName(FileName_) { }

    /// Virtual destructor so the proper destructor of the underlying buffer is called.
    virtual ~BufferT() { }

    /// Attaches this buffer to a channel.
    /// The buffer automatically gets channel using the FMOD sound system.
    /// @param Priority The priority of this buffer (0-255). If all channels are currently in use the FMOD3
    ///        priority system decides which channel is stopped and cleared for this new buffer.
    /// @return The channel handle to which this buffer is now attached. -1 on failure.
    virtual int AttachToChannel(unsigned int Priority)=0;

    /// Rewinds this buffer if it is a stream.
    virtual void Rewind() {};

    unsigned int References; ///< Number of references to this buffer (e.g. how many sound objects use this buffer).


    protected:

    std::string FileName; ///< Name of this file this buffer was created from.


    private:

    // Don't allow use of copy and assignment constructor.
    BufferT(BufferT&);
    BufferT& operator=(const BufferT&);
};

#endif
