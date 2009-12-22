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

#include "StaticBuffer.hpp"
#include "MixerTrack.hpp"

#include "../Common/SoundStream.hpp"
#include "Templates/Array.hpp"
#include "String.hpp"

#include "AL/alut.h"

#include <iostream>


static const unsigned int InitBufferSize=65536;


StaticBufferT::StaticBufferT(const std::string& AudioFile, bool Is3DSound)
    : BufferT(AudioFile, Is3DSound),
      m_Buffer(AL_NONE)
{
    // If the file ends on .ogg or .mp3 we use MP3StreamT or OggVorbisStreamT to read all PCM data of the file into a local buffer.
    if (cf::String::EndsWith(AudioFile, ".mp3") || cf::String::EndsWith(AudioFile, ".ogg"))
    {
        // TODO Force mono stream if Is3DSound (stereo streams cannot be played 3 dimensional).
        SoundStreamT* Stream=SoundStreamT::Create(AudioFile);

        if (Stream!=NULL)
        {
            int FreeBytes     =InitBufferSize; // Bytes still free in this buffer.
            int ReadBytes     =0;              // Number of bytes read from the input stream on last read.
            int TotalReadBytes=0;              // Total number of bytes read from the input stream.

            ArrayT<unsigned char> LocalBuffer;

            LocalBuffer.PushBackEmpty(InitBufferSize);

            ReadBytes=Stream->Read(&LocalBuffer[0], FreeBytes);

            while(ReadBytes>0)
            {
                assert(ReadBytes<=FreeBytes);

                if (ReadBytes==FreeBytes)
                {
                    LocalBuffer.PushBackEmpty(InitBufferSize);
                    FreeBytes=InitBufferSize;
                }
                else
                {
                    FreeBytes-=ReadBytes;
                }

                TotalReadBytes+=ReadBytes;
                ReadBytes=Stream->Read(&LocalBuffer[TotalReadBytes], FreeBytes);
            }

            // Reached end of stream, copy streamdata into OpenAL buffer.
            if (ReadBytes==0)
            {
                ALenum OutputFormat=AL_FORMAT_STEREO16;

                if (Stream->GetChannels()==1 || Is3DSound) OutputFormat=AL_FORMAT_MONO16;
                if (Stream->GetChannels() >1 && Is3DSound) TotalReadBytes=ConvertToMono(&LocalBuffer[0], TotalReadBytes);

                alGenBuffers(1, &m_Buffer);

                alBufferData(m_Buffer, OutputFormat, &LocalBuffer[0], TotalReadBytes, Stream->GetRate());
            }

            delete   Stream;
            LocalBuffer.Clear();
        }
        else
        {
            std::cout << "OpenAL: Error creating static buffer from file '" << AudioFile << "' Error: Stream could not be opened\n";
        }
    }
    else
    {
        // Other files are handled by ALUT.
        // TODO is it possible to let ALUT create mono PCM from stereo wavs?
        m_Buffer=alutCreateBufferFromFile(AudioFile.c_str());

        if (m_Buffer==AL_NONE) std::cout << "OpenAL: Error creating static sound buffer '" << AudioFile << "'" << " Error: " << alutGetErrorString(alutGetError()) << "\n";
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


void StaticBufferT::Update()
{
    // Nothing to do for static buffers.
}


void StaticBufferT::Rewind()
{
    // Nothing to do for static buffers.
}


bool StaticBufferT::IsStream()
{
    return false;
}


bool StaticBufferT::AttachToMixerTrack(MixerTrackT* MixerTrack)
{
    // Only add mixer track if it is not yet in the array.
    if (m_MixerTracks.Find(MixerTrack)==-1) m_MixerTracks.PushBack(MixerTrack);

    if (!m_Is3DSound)
    {
        alSourcei (MixerTrack->m_SourceHandle, AL_SOURCE_RELATIVE, AL_TRUE);
        alSource3f(MixerTrack->m_SourceHandle, AL_POSITION, 0.0f, 0.0f, 0.0f);
    }
    else
    {
        // Explicitly reset to non-relative source positioning.
        alSourcei (MixerTrack->m_SourceHandle, AL_SOURCE_RELATIVE, AL_FALSE);
    }

    alSourcei(MixerTrack->m_SourceHandle, AL_BUFFER, m_Buffer); // Attach buffer to source.

    return true;
}


bool StaticBufferT::DetachFromMixerTrack(MixerTrackT* MixerTrack)
{
    int Position=m_MixerTracks.Find(MixerTrack);

    if (Position==-1) return false;

    m_MixerTracks.RemoveAtAndKeepOrder(Position);

    return true;
}
