/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SOUNDSYS_MP3_STREAM_HPP_INCLUDED
#define CAFU_SOUNDSYS_MP3_STREAM_HPP_INCLUDED

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
