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

#ifndef _SOUNDSYS_SOUND_STREAM_HPP_
#define _SOUNDSYS_SOUND_STREAM_HPP_

#include <string>


/// Represents a 16 Bit encoded mono or stereo raw PCM data stream.
class SoundStreamT
{
    public:

    /// General sound stream exception described by an error message.
    /// Thrown when the creation of a stream fails or any opration on
    /// an existing stream (read etc.).
    class ExceptionT
    {
        public:

        /// Constructor.
        /// @param Error_ This errors information message.
        ExceptionT(const std::string& Error_) { Error=Error_; }

        /// Get the error message associated with this exception.
        /// @return Error message.
        std::string GetError() { return Error; }


        private:

        std::string Error;
    };

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

    /// Creates a new sound stream of a defined type.
    /// @param FileName Path to the file from which the stream should be created.
    /// @return Created sound stream object or NULL if not successful.
    static SoundStreamT* Create(const std::string& FileName);

    /// Virtual destructor to make sure the destructor of the derived class is called.
    virtual ~SoundStreamT() { }
};

#endif
