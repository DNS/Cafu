/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#ifndef _SOUNDSYS_BUFFER_HPP_
#define _SOUNDSYS_BUFFER_HPP_

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
