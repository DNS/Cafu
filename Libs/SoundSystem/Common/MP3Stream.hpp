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

#ifndef _SOUNDSYS_MP3_STREAM_HPP_
#define _SOUNDSYS_MP3_STREAM_HPP_

#include "SoundStream.hpp"


struct mpg123_handle_struct;
typedef struct mpg123_handle_struct mpg123_handle;
namespace cf { namespace FileSys { class InFileI; } }


/// Represents an MP3 stream.
/// Audio data is decompressed and readable as 16 bit enconded mono or stereo raw PCM data.
/// This class uses the mpg123 library to read PCM data from an MP3 file.
class MP3StreamT : public SoundStreamT
{
    public:

    /// The constructor. Throws an exception of type std::runtime_error on failure.
    /// Creates an audio data stream from an MP3 file.
    /// @param FileName The path to the file from which the stream should be created.
    MP3StreamT(const std::string& FileName);

    /// Destructor.
    ~MP3StreamT();

    // SoundStream implementation.
    int          Read(unsigned char* Buffer, unsigned int Size);
    bool         Rewind();
    unsigned int GetChannels();
    unsigned int GetRate();


    private:

    mpg123_handle*        StreamHandle; ///< Handle to the mpg123 stream.
    cf::FileSys::InFileI* StreamFile;   ///< The file handle from which the mp3 data is streamed.

    long Rate;     ///< Sampling rate of this stream in Hz.
    int  Channels; ///< Number of channels in this stream.
};

#endif
