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

#ifndef _OGG_VORBIS_STREAM_HPP_
#define _OGG_VORBIS_STREAM_HPP_

#include "SoundStream.hpp"

#if defined(_WIN32) && defined(_MSC_VER)
// Turn off warning 4244 for vorbisfile.h: 'Argument': Konvertierung von 'ogg_int64_t' in 'long', moeglicher Datenverlust.
#pragma warning(disable:4244)
#endif
#define OV_EXCLUDE_STATIC_CALLBACKS
#include "vorbis/vorbisfile.h"
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(error:4244)
#endif


namespace cf { namespace FileSys { class InFileI; } }


/// Represents an Ogg Vorbis stream.
/// Audio data is decompressed and readable as 16 bit enconded mono or stereo raw PCM data.
/// This class uses libogg and libvorbis provided by Xiph.org to read PCM data from an Ogg
/// Vorbis file.
class OggVorbisStreamT : public SoundStreamT
{
    public:

    /// Constructor.
    /// Creates an audio data stream from an Ogg Vorbis file.
    /// @param FileName The path to the file from which the stream should be created.
    OggVorbisStreamT(const std::string& FileName);

    /// Destructor.
    ~OggVorbisStreamT();

    // SoundStreamT implementation.
    int          Read(unsigned char* Buffer, unsigned int Size);
    bool         Rewind();
    unsigned int GetChannels();
    unsigned int GetRate();


    private:

    OggVorbis_File        StreamHandle; ///< Handle to the vorbis file structure.
    cf::FileSys::InFileI* StreamFile;   ///< The file handle from which the vorbis data is streamed.
    vorbis_info*          StreamInfo;   ///< Information about the current bitstream read from the ogg vorbis file.

    int                   BitStream;    ///< Number of the current bitstream.
};

#endif
