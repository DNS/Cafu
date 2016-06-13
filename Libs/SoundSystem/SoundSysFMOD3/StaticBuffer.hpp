/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SOUNDSYS_STATIC_BUFFER_HPP_INCLUDED
#define CAFU_SOUNDSYS_STATIC_BUFFER_HPP_INCLUDED

#include "Buffer.hpp"


struct FSOUND_SAMPLE;


/// A static buffer created from an audio file.
/// Static buffers load the audio file completely into memory and can then be
/// used for playback by multiple mixer tracks.
/// This class is basically just a wrapper the FMOD3 sound sample object.
class StaticBufferT : public BufferT
{
    public:

    /// Constructor. Creates a static buffer from an audio file.
    /// @param AudioFile The path to the file this buffer should be created from.
    /// @param Is3DSound Whether the buffer is used as a 3D sound object.
    StaticBufferT(const std::string& AudioFile, bool Is3DSound);

    /// Destructor.
    ~StaticBufferT();

    int AttachToChannel(unsigned int Priority);


    private:

    FSOUND_SAMPLE* m_Sample; ///< The FMOD3 sound sample used with this static buffer.

    // Don't allow use of copy and assignment constructor.
    StaticBufferT(StaticBufferT&);
    StaticBufferT& operator=(const StaticBufferT&);
};

#endif
