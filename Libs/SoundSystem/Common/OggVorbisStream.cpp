/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "OggVorbisStream.hpp"
#include "FileSys/FileMan.hpp"
#include "FileSys/File.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>


// Static callback method for the IO read function used by libvorbis to read from ogg files.
// We use our streamed file here to load data into buffer, so libvorbis works directly with cf::FileSys files.
// ElementSize: Size in bytes of one element to read.
// NumRead:     Number of elements to read.
static size_t FileSysRead(void* Buffer, size_t ElementSize, size_t NumRead, void* DataSource)
{
    cf::FileSys::InFileI* StreamFile=(cf::FileSys::InFileI*) DataSource;

    return StreamFile->Read((char*)Buffer, uint32_t(ElementSize*NumRead));
}


// Static callback method for the IO seek function used by libvorbis to seek in ogg files.
// We use our streamed file here to seek in a file, so libvorbis works directly with cf::FileSys files.
static int FileSysSeek(void* DataSource, ogg_int64_t Offset, int Whence)
{
    cf::FileSys::InFileI* StreamFile=(cf::FileSys::InFileI*) DataSource;

    cf::FileSys::FileI::SeekFromT SeekFrom=cf::FileSys::FileI::FROM_BEGINNING;
    switch (Whence)
    {
        case SEEK_CUR: SeekFrom=cf::FileSys::FileI::FROM_CURRENT_POS; break;
        case SEEK_END: SeekFrom=cf::FileSys::FileI::FROM_END;         break;
    }

    if (StreamFile->Seek((int)Offset, SeekFrom))
        return int(StreamFile->GetPos());

    return -1;
}


// Static callback method for the IO tell function used by libvorbis to tell the current position in ogg files.
// We use our streamed file here to tell the position, so libvorbis works directly with cd::FileSys files.
static long FileSysTell(void* DataSource)
{
    cf::FileSys::InFileI* StreamFile=(cf::FileSys::InFileI*) DataSource;

    return long(StreamFile->GetPos());
}


static std::string ErrorToStr(int ErrorCode);


OggVorbisStreamT::OggVorbisStreamT(const std::string& FileName)
    : StreamHandle(),
      StreamFile(NULL),
      StreamInfo(NULL),
      BitStream(-1)
{
    // Create struct of custom callback methods, so libvorbis works with cf::FileSys.
    ov_callbacks CallBacks;

    CallBacks.read_func =&FileSysRead;
    CallBacks.seek_func =&FileSysSeek;
    CallBacks.close_func=NULL;          // Not set since we want to close the streamed file ourselves using the file manager.
    CallBacks.tell_func =&FileSysTell;

    StreamFile=cf::FileSys::FileMan->OpenRead(FileName);


    int Result=ov_test_callbacks(StreamFile, &StreamHandle, NULL, 0, CallBacks);;

    if (Result!=0)
        throw std::runtime_error("OggVorbis: File is no OggVorbis file ("+ErrorToStr(Result)+")");

    Result=ov_test_open(&StreamHandle);

    if (Result!=0)
        throw std::runtime_error("OggVorbis: Couldn't open stream ("+ErrorToStr(Result)+")");

    // Read rate and channels from the current bitstream.
    StreamInfo=ov_info(&StreamHandle, BitStream);

    if (StreamInfo==NULL)
        throw std::runtime_error("OggVorbis: Couldn't access stream information");
}


OggVorbisStreamT::~OggVorbisStreamT()
{
    ov_clear(&StreamHandle);

    cf::FileSys::FileMan->Close(StreamFile);
}


int OggVorbisStreamT::Read(unsigned char* Buffer, unsigned int Size)
{
    // libvorbis implementation doesn't actually write Size bytes from the stream in to the buffer when ov_read is called.
    // Size is just a maximum that will never be exceeded.
    // To make sure we read Size bytes from the stream into the buffer, we need to call ov_read until Size bytes are read.
             int Result   =0;
    unsigned int ReadBytes=0;

    int  CurrentBitStream=BitStream;
    long CurrentRate     =StreamInfo->rate;

    while (ReadBytes<Size)
    {
        Result=ov_read(&StreamHandle, (char*)&Buffer[ReadBytes], Size-ReadBytes, 0, 2, 1, &BitStream);

        if (Result<0)
        {
            std::cout << "OggVorbis: Couldn't read from stream (Error: " << ErrorToStr(Result) << ")\n";
            return Result;
        }

        if (Result==0) return ReadBytes;

        ReadBytes+=Result;

        if (CurrentBitStream!=BitStream)
        {
            StreamInfo=ov_info(&StreamHandle, BitStream);
            CurrentBitStream=BitStream;

            if (CurrentRate!=StreamInfo->rate)
            {
                CurrentRate=StreamInfo->rate;
                std::cout << "OggVorbis: Warning! Ogg file contains multiple streams with different sample rate, playback errors may occur...\n";
            }
        }
    }

    return ReadBytes;
}


bool OggVorbisStreamT::Rewind()
{
    int Result=ov_time_seek(&StreamHandle, 0.0);

    if (Result!=0)
    {
        std::cout << "OggVorbis: Couldn't rewind stream (Error: " << ErrorToStr(Result) << ")\n";
        return false;
    }

    return true;
}


unsigned int OggVorbisStreamT::GetChannels()
{
    if (StreamInfo==NULL) return 0;

    return StreamInfo->channels;
}


unsigned int OggVorbisStreamT::GetRate()
{
    if (StreamInfo==NULL) return 0;

    return StreamInfo->rate;
}


static std::string ErrorToStr(int ErrorCode)
{
    switch(ErrorCode)
    {
        case OV_EREAD:      return "OV_EREAD";
        case OV_ENOTVORBIS: return "OV_ENOTVORBIS";
        case OV_EVERSION:   return "OV_EVERSION";
        case OV_EBADHEADER: return "OV_EBADHEADER";
        case OV_EFAULT:     return "OV_EFAULT";
        case OV_HOLE:       return "OV_HOLE";
        case OV_EBADLINK:   return "OV_EBADLINK";
        case OV_ENOSEEK:    return "OV_ENOSEEK";
        case OV_EINVAL:     return "OV_EINVAL";
    }

    return "Unknown error";
}
