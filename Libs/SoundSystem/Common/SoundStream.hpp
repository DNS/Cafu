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

#ifndef CAFU_SOUNDSYS_SOUND_STREAM_HPP_INCLUDED
#define CAFU_SOUNDSYS_SOUND_STREAM_HPP_INCLUDED

#include <string>


/// Represents a 16 Bit encoded mono or stereo raw PCM data stream.
class SoundStreamT
{
    public:

    /// Creates a new sound stream for the specified resource.
    /// @param ResName   The name of the stream resource. ResName can be a file name or the name of an OpenAL capture device (as obtained from the ALC_CAPTURE_DEVICE_SPECIFIER list).
    /// @returns the created sound stream instance. Throws an exception of type std::runtime_error on failure.
    static SoundStreamT* Create(const std::string& ResName);

    /// Reads an amount of bytes from the stream and writes them into the buffer.
    /// @param Buffer Buffer to write the data into.
    /// @param Size Amount of bytes to be read from the stream and write in the buffer.
    /// @return Number of bytes wrote into the buffer. -1 if an error occured during reading.
    virtual int Read(unsigned char* Buffer, unsigned int Size)=0;

    /// Sets the stream back to the beginning.
    virtual bool Rewind()=0;

    /// Returns the number of channels in the current stream.
    /// @return Number of channels in the stream.
    virtual unsigned int GetChannels()=0;

    /// Get the sampling rate of this stream.
    /// @return The sampling rate in Hz.
    virtual unsigned int GetRate()=0;

    /// Virtual destructor to make sure the destructor of the derived class is called.
    virtual ~SoundStreamT() { }
};

#endif
