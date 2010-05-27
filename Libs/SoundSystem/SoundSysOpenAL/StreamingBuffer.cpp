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

#include "StreamingBuffer.hpp"
#include "MixerTrack.hpp"

#include "../Common/SoundStream.hpp"

#include <iostream>


StreamingBufferT::StreamingBufferT(const std::string& AudioFile, bool Is3DSound)
    : BufferT(AudioFile, Is3DSound),
      m_Stream(SoundStreamT::Create(AudioFile)),
      m_Buffers(),
      m_OutputFormat(AL_FORMAT_STEREO16),
      m_EndReached(false)
{
    if (m_Stream==NULL) std::cout << "OpenAL: Error creating streamed sound buffer '" << AudioFile << "'\n";
    assert(m_Stream!=NULL);

    m_Buffers.PushBackEmptyExact(5);
    alGenBuffers(m_Buffers.Size(), &m_Buffers[0]);

    // If the stream has only one channel or if the stream should be played 3D we have to set the output format to mono.
    if (m_Stream->GetChannels()==1 || m_Is3DSound) m_OutputFormat=AL_FORMAT_MONO16;
}


StreamingBufferT::~StreamingBufferT()
{
    alDeleteBuffers(m_Buffers.Size(), &m_Buffers[0]);

    const int Error=alGetError();
    if (Error!=AL_NO_ERROR) std::cout << "OpenAL: Error deleting buffer: " << TranslateErrorCode(Error) << "\n";

    // Close and delete stream.
    delete m_Stream;

    assert(alGetError()==AL_NO_ERROR);
}


void StreamingBufferT::FillAndQueue(const ArrayT<ALuint>& Buffers)
{
    static const unsigned int RAW_PCM_BUFFER_SIZE=65536;            ///< Size in bytes of raw PCM data exchange buffer.
    static unsigned char      RawPcmBuffer[RAW_PCM_BUFFER_SIZE];    ///< The buffer for transferring raw PCM data from the stream to the OpenAL buffers.

    for (unsigned long BufNr=0; BufNr<Buffers.Size(); BufNr++)
    {
        int ReadBytes=m_Stream->Read(RawPcmBuffer, RAW_PCM_BUFFER_SIZE);

        if (ReadBytes<=0)
        {
            if (ReadBytes<0) std::cout << __FUNCTION__ << ": Error reading stream " << m_FileName << ".\n";

            m_EndReached=true;
            break;
        }

        if (m_Stream->GetChannels()>1 && m_Is3DSound)
            ReadBytes=ConvertToMono(RawPcmBuffer, ReadBytes);

        alBufferData(Buffers[BufNr], m_OutputFormat, RawPcmBuffer, ReadBytes, m_Stream->GetRate());

        // (Re-)Queue the filled buffers on the mixer track (the OpenAL source).
        alSourceQueueBuffers(m_MixerTracks[0]->m_SourceHandle, 1, &Buffers[BufNr]);
    }
}


void StreamingBufferT::Update()
{
    if (m_EndReached) return;

    assert(m_MixerTracks[0]!=NULL);
    static ArrayT<ALuint> RecycleBuffers;

    {
        int NumRecycle=0;
        alGetSourcei(m_MixerTracks[0]->m_SourceHandle, AL_BUFFERS_PROCESSED, &NumRecycle);

        if (NumRecycle==0) return;

        RecycleBuffers.Overwrite();
        RecycleBuffers.PushBackEmpty(NumRecycle);
    }

    alSourceUnqueueBuffers(m_MixerTracks[0]->m_SourceHandle, RecycleBuffers.Size(), &RecycleBuffers[0]);
    FillAndQueue(RecycleBuffers);

    // If all bufferes were completely processed, the playback was finished and stopped, so restart it now.
    if (RecycleBuffers.Size()==m_Buffers.Size())
        alSourcePlay(m_MixerTracks[0]->m_SourceHandle);

    assert(alGetError()==AL_NO_ERROR);
}


void StreamingBufferT::Rewind()
{
    m_Stream->Rewind();
    m_EndReached=false;
}


bool StreamingBufferT::IsStream()
{
    return true;
}


bool StreamingBufferT::AttachToMixerTrack(MixerTrackT* MixerTrack)
{
    // Stream buffers are unique and can only be attached to one source.
    if (m_MixerTracks.Size()>0) return false;
    m_MixerTracks.PushBack(MixerTrack);

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

    FillAndQueue(m_Buffers);

    assert(alGetError()==AL_NO_ERROR);
    return true;
}


bool StreamingBufferT::DetachFromMixerTrack(MixerTrackT* MixerTrack)
{
    assert(m_MixerTracks.Size()==1);

    if (m_MixerTracks[0]!=MixerTrack) return false;

    m_MixerTracks.Overwrite();
    return true;
}
