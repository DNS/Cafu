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

#ifndef _SOUNDSYS_STATIC_BUFFER_HPP_
#define _SOUNDSYS_STATIC_BUFFER_HPP_

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
