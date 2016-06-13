/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "StaticBuffer.hpp"
#include "MixerTrack.hpp"

#include "../Common/SoundStream.hpp"
#include "Templates/Array.hpp"
#include "String.hpp"

#include "AL/alut.h"

#include <iostream>


StaticBufferT::StaticBufferT(const std::string& FileName, bool ForceMono)
    : BufferT(FileName, ForceMono),
      m_Buffer(AL_NONE)
{
    // If the file name ends with .ogg or .mp3 we use MP3StreamT or OggVorbisStreamT to read all PCM data of the file into a local buffer.
    if (cf::String::EndsWith(FileName, ".mp3") || cf::String::EndsWith(FileName, ".ogg"))
    {
        SoundStreamT* Stream=SoundStreamT::Create(FileName);    // Throws an exception of type std::runtime_error on failure.
        ArrayT<unsigned char> StreamBuffer;

        while (true)
        {
            const int     MAX_READ_BYTES=65536;
            unsigned char ReadBuffer[MAX_READ_BYTES];
            const int     ReadBytes=Stream->Read(ReadBuffer, MAX_READ_BYTES);

            if (ReadBytes<=0) break;

            for (int i=0; i<ReadBytes; i++)
                StreamBuffer.PushBack(ReadBuffer[i]);
        }

        // Finally copy the stream buffer into the OpenAL buffer.
        ALenum OutputFormat=(Stream->GetChannels()==1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
        int    TotalReadBytes=StreamBuffer.Size();

        if (ForceMono && OutputFormat==AL_FORMAT_STEREO16)
        {
            OutputFormat  =AL_FORMAT_MONO16;
            TotalReadBytes=ConvertToMono(&StreamBuffer[0], TotalReadBytes);
        }

        alGenBuffers(1, &m_Buffer);
        alBufferData(m_Buffer, OutputFormat, &StreamBuffer[0], TotalReadBytes, Stream->GetRate());

        delete Stream;
    }
    else
    {
        // Other files are handled by ALUT.
        // TODO: Use alutLoadMemoryFromFile() instead, reduce to mono if necessary!
        m_Buffer=alutCreateBufferFromFile(FileName.c_str());

        if (m_Buffer==AL_NONE) std::cout << "OpenAL: Error creating static sound buffer '" << FileName << "'" << " Error: " << alutGetErrorString(alutGetError()) << "\n";
    }

    assert(alGetError()==AL_NO_ERROR);
}


StaticBufferT::~StaticBufferT()
{
    assert(References==0);
    assert(m_MixerTracks.Size()==0);

    alDeleteBuffers(1, &m_Buffer);

    int Error=alGetError();
    if (Error!=AL_NO_ERROR) std::cout << "OpenAL: Error deleting buffer: " << TranslateErrorCode(Error) << "\n";
}


unsigned int StaticBufferT::GetChannels() const
{
    ALint NumChannels;

    alGetBufferi(m_Buffer, AL_CHANNELS, &NumChannels);
    return NumChannels;
}


bool StaticBufferT::CanShare() const
{
    return true;
}


void StaticBufferT::Update()
{
    // Static buffers are always up to date.
}


bool StaticBufferT::AttachToMixerTrack(MixerTrackT* MixerTrack)
{
    // Only add mixer track if it is not yet in the array.
    if (m_MixerTracks.Find(MixerTrack)==-1) m_MixerTracks.PushBack(MixerTrack);

    // Attach our buffer to the source.
    alSourcei(MixerTrack->GetOpenALSource(), AL_BUFFER, m_Buffer);
    return true;
}


bool StaticBufferT::DetachFromMixerTrack(MixerTrackT* MixerTrack)
{
    const int Position=m_MixerTracks.Find(MixerTrack);
    if (Position==-1) return false;

    // Remove our buffer from this source.
    alSourcei(MixerTrack->GetOpenALSource(), AL_BUFFER, 0);

    m_MixerTracks.RemoveAtAndKeepOrder(Position);
    return true;
}
