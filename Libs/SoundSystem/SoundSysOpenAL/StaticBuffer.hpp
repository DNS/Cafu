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

#ifndef _SOUNDSYS_STATIC_BUFFER_HPP_
#define _SOUNDSYS_STATIC_BUFFER_HPP_

#include "Buffer.hpp"
#include "OpenALIncl.hpp"


/// A static buffer created from an audio file.
/// Static buffers load the audio file completely into memory and can then be
/// used for playback by multiple mixer tracks.
/// This class is basically just a wrapper around the OpenAL buffer which is
/// one buffer that is fully loaded into memory on creation.
/// Additional code covers the creation of the OpenAL buffer from different file
/// types by this classes constructor.
class StaticBufferT : public BufferT
{
    public:

    /// Constructor.
    /// Creates a static buffer from an audio file.
    /// @param AudioFile Path to the audio file from which the buffer should be created.
    /// @param Is3DSound Whether the buffer is used as a 3D sound object (audio data has to be mono).
    StaticBufferT(const std::string& AudioFile, bool Is3DSound);

    /// Destructor.
    ~StaticBufferT();

    // BufferT implementation.
    void Update();
    void Rewind();
    bool IsStream() const;

    bool AttachToMixerTrack(MixerTrackT* MixerTrack);
    bool DetachFromMixerTrack(MixerTrackT* MixerTrack);


    private:

    ALuint m_Buffer;

    StaticBufferT(const StaticBufferT&);        ///< Use of the Copy    Constructor is not allowed.
    void operator = (const StaticBufferT&);     ///< Use of the Assignment Operator is not allowed.
};

#endif
