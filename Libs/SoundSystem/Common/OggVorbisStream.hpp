/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_OGG_VORBIS_STREAM_HPP_INCLUDED
#define CAFU_OGG_VORBIS_STREAM_HPP_INCLUDED

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

    /// The constructor. Throws an exception of type std::runtime_error on failure.
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
