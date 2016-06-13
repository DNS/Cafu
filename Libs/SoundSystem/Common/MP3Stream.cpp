/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "MP3Stream.hpp"
#include "FileSys/FileMan.hpp"
#include "FileSys/File.hpp"
#include "Templates/Array.hpp"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <stdexcept>

// #ifdef _WIN32
// // Under Windows, the mpg123 functions must be properly declared for DLL import.
// #define LINK_MPG123_DLL
// #endif
#include "mpg123.h"


static ArrayT<cf::FileSys::InFileI*> OpenFiles;


/// Static replace method for the POSIX read function used by MPG123 to read from mp3 files.
/// We use our streamed file here to load data into buffer, so MPG123 works directly with cf::FileSys files.
static ssize_t FileSysRead(int FileDesc, void* Buffer, size_t Count)
{
    cf::FileSys::InFileI* StreamFile=OpenFiles[FileDesc];

    return StreamFile->Read((char*)Buffer, uint32_t(Count));
}


/// Static replace method for the POSIX lseek function used by MPG123 to seek in mp3 files.
/// We use our streamed file here to seek in it, so MPG123 works directly with cf::FileSys files.
static off_t FileSysSeek(int FileDesc, off_t Offset, int Whence)
{
    cf::FileSys::InFileI*         StreamFile=OpenFiles[FileDesc];
    cf::FileSys::FileI::SeekFromT SeekFrom  =cf::FileSys::FileI::FROM_BEGINNING;

    switch (Whence)
    {
        case SEEK_CUR: SeekFrom=cf::FileSys::FileI::FROM_CURRENT_POS; break;
        case SEEK_END: SeekFrom=cf::FileSys::FileI::FROM_END;         break;
    }

    if (StreamFile->Seek(Offset, SeekFrom))
        return off_t(StreamFile->GetPos());

    return -1;
}


/// Small wrapper class around mpg123 to init and exit the library.
class MPG123InstanceT
{
    public:

    MPG123InstanceT()
    {
        const int Result=mpg123_init();

        if (Result!=MPG123_OK)
        {
            std::string ErrorStr=mpg123_plain_strerror(Result);

            throw std::runtime_error("MPG123: Couldn't initialize mpg123 ("+ErrorStr+")");
        }
    }

    ~MPG123InstanceT()
    {
        mpg123_exit();
    }
};


/// Maximum amount of bytes parsed to resync a MP3 stream if sync is lost. If the stream could not be resynced after
/// parsing this many bytes the stream cannot be created.
/// Negative values result in resync attempts until the end of the stream.
static const long ResyncLimit=32768;


MP3StreamT::MP3StreamT(const std::string& FileName)
    : StreamHandle(NULL),
      StreamFile(NULL),
      Rate(0),
      Channels(0)
{
    static MPG123InstanceT MPG123Inst;

    int Result=MPG123_OK;

    // Create MPG123 handle.
    StreamHandle=mpg123_new(NULL, &Result);

    if (StreamHandle==NULL)
    {
        std::string ErrorStr=mpg123_plain_strerror(Result);
        throw std::runtime_error("MPG123: Couldn't open stream handle ("+ErrorStr+")");
    }

    Result=mpg123_param(StreamHandle, MPG123_RESYNC_LIMIT, ResyncLimit, 0);

    if (Result!=MPG123_OK)
    {
        std::string ErrorStr=mpg123_strerror(StreamHandle);
        throw std::runtime_error("MPG123: Couldn't adjust resync limit ("+ErrorStr+")");
    }

    // Replace read and seek function used for filedescriptors, so they work with pointers to FileI objects from the cf::FileSys.
    Result=mpg123_replace_reader(StreamHandle, &FileSysRead, &FileSysSeek);

    if (Result!=MPG123_OK)
    {
        std::string ErrorStr=mpg123_strerror(StreamHandle);
        throw std::runtime_error("MPG123: Couldn't replace reader functions ("+ErrorStr+")");
    }

    StreamFile=cf::FileSys::FileMan->OpenRead(FileName);

    if (!StreamFile)
    {
        throw std::runtime_error("MPG123: Error opening file "+FileName);
    }

    int Encoding=0;
    OpenFiles.PushBack(StreamFile);

    if (mpg123_open_fd(StreamHandle, int(OpenFiles.Size()-1))!=MPG123_OK || mpg123_getformat(StreamHandle, &Rate, &Channels, &Encoding)!=MPG123_OK)
    {
        OpenFiles.DeleteBack();
        std::string ErrorStr=mpg123_strerror(StreamHandle);
        throw std::runtime_error("MPG123: Error opening stream: "+ErrorStr);
    }

    // Signed 16 is the default output format anyways; it would actually be only different if we forced it.
    assert(Encoding==MPG123_ENC_SIGNED_16);

    // Fix output format.
    mpg123_format_none(StreamHandle);
    mpg123_format     (StreamHandle, Rate, Channels, Encoding);
}


MP3StreamT::~MP3StreamT()
{
    mpg123_delete(StreamHandle);

    // Find our StreamFile in the OpenFiles list.
    const int Pos=OpenFiles.Find(StreamFile);

    // Set our entry to NULL.
    if (Pos!=-1) OpenFiles[Pos]=NULL;

    // Remove as many trailing NULL entries as possible, in order to reuse indices and keep the list small.
    while (OpenFiles.Size()>0 && OpenFiles[OpenFiles.Size()-1]==NULL)
        OpenFiles.DeleteBack();

    cf::FileSys::FileMan->Close(StreamFile);
}


int MP3StreamT::Read(unsigned char* Buffer, unsigned int Size)
{
    size_t ReadBytes=0;

    const int Result=mpg123_read(StreamHandle, Buffer, Size, &ReadBytes);

    // MPG123_FORMAT_CHANGED is handled as error here since we fixed the output format above and it shouldn't change here.
    if (Result!=MPG123_OK && Result!=MPG123_DONE)
    {
        std::cout << "MPG123: Error reading from stream: " << mpg123_strerror(StreamHandle) << "\n";
        return -1;
    }

    if (Result==MPG123_DONE)
    {
        std::cout << "MPG123: End of stream reached (Return '" << ReadBytes << "').\n";
    }

    return int(ReadBytes);
}


bool MP3StreamT::Rewind()
{
    assert(StreamFile!=NULL);

    /*if (mpg123_seek(StreamHandle, 0, SEEK_SET)<0)
    {
        std::cout << "MPG123: Error rewinding Stream: " <<  mpg123_strerror(StreamHandle) << "\n";
        return false;
    }*/

    // For some reason mp123_seek doesn't work if the stream has reached its end, so we manually set
    // the file pointer to the files beginning.
    if (!StreamFile->Seek(0, cf::FileSys::FileI::FROM_BEGINNING))
    {
        std::cout << "MPG123: Error rewinding Stream\n";
        return false;
    }

    return true;
}


unsigned int MP3StreamT::GetChannels()
{
    return Channels;
}


unsigned int MP3StreamT::GetRate()
{
    return Rate;
}
