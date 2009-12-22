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


static const unsigned int NumStreamBuffers=5;     ///< Number of OpenAL buffers used internally for streamed buffers.
static const unsigned int BufferSize      =65536; ///< Size in bytes of each OpenAL buffer. It is not recommended setting this value lower than 4096.


// TODO force mono stream creation if Is3DSound==false.
StreamingBufferT::StreamingBufferT(const std::string& AudioFile, bool Is3DSound)
    : BufferT(AudioFile, Is3DSound),
      m_Stream(SoundStreamT::Create(AudioFile)),
      m_OutputFormat(AL_FORMAT_STEREO16),
      m_EndReached(false)
{
    if (m_Stream==NULL) std::cout << "OpenAL: Error creating streamed sound buffer '" << AudioFile << "'\n";

    assert(m_Stream!=NULL);

    // If the stream has only one channel or if the stream should be played 3D we have to set the output format to mono.
    if (m_Stream->GetChannels()==1 || m_Is3DSound) m_OutputFormat=AL_FORMAT_MONO16;
}


StreamingBufferT::~StreamingBufferT()
{
    // Delete stream buffers.
    for (unsigned int i=0; i<m_CurrentBuffers.Size(); i++)
        alDeleteBuffers(1, &m_CurrentBuffers[i]);

    int Error=alGetError();
    if (Error!=AL_NO_ERROR) std::cout << "OpenAL: Error deleting buffer: " << TranslateErrorCode(Error) << "\n";

    // Close and delete stream.
    delete m_Stream;

    assert(alGetError()==AL_NO_ERROR);
}


void StreamingBufferT::Update()
{
    // Only update if the stream hasn't reached the end.
    if (m_EndReached) return;

    assert(m_MixerTracks[0]!=NULL);

    int BuffersProcessed=0;

    alGetSourcei(m_MixerTracks[0]->m_SourceHandle, AL_BUFFERS_PROCESSED, &BuffersProcessed);

    // Only update if there are buffers to update.
    if (BuffersProcessed==0) return;

    ALuint* BuffersToDelete=new ALuint[BuffersProcessed];

    alSourceUnqueueBuffers(m_MixerTracks[0]->m_SourceHandle, BuffersProcessed, BuffersToDelete);

    alDeleteBuffers(BuffersProcessed, BuffersToDelete);

    ALuint* NewBuffers=new ALuint[BuffersProcessed];

    // Generate new Buffers for stream.
    alGenBuffers(BuffersProcessed, NewBuffers);

    // Remove processed buffers from MusicBufferArray and add created buffers.
    for (int i=0; i<BuffersProcessed; i++)
    {
        m_CurrentBuffers.RemoveAtAndKeepOrder(0);
        m_CurrentBuffers.PushBack(NewBuffers[i]);
    }

    unsigned char* ByteBuffer=new unsigned char[BufferSize];
    int            ReadBytes =0;

    int i=0;
    // int SourceState=-1;

    // Fill music buffers.
    for (i=0; i<BuffersProcessed; i++)
    {
        ReadBytes=m_Stream->Read(ByteBuffer, BufferSize);

        if (ReadBytes<0)
        {
            std::cout << "OpenAL: Error creating music buffer.\n";
            break;
        }

        if (ReadBytes==0)
        {
            m_EndReached=true;
        }

        if (m_Stream->GetChannels()>1 && m_Is3DSound) ReadBytes=ConvertToMono(ByteBuffer, ReadBytes);

        alBufferData(NewBuffers[i], m_OutputFormat, ByteBuffer, ReadBytes, m_Stream->GetRate());
    }

    // There was an error during buffer processing.
    if (i<BuffersProcessed)
    {
        delete[] ByteBuffer;
        return;
    }

    delete[] ByteBuffer;

    // Enqueue buffers if any created.
    alSourceQueueBuffers(m_MixerTracks[0]->m_SourceHandle, BuffersProcessed, NewBuffers);

    // If all buffers had been processed, retrigger playing since the source has stopped.
    if (BuffersProcessed==int(NumStreamBuffers)) alSourcePlay(m_MixerTracks[0]->m_SourceHandle);

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

    ALuint* StreamBuffers=new ALuint[NumStreamBuffers];

    // Create stream buffers.
    alGenBuffers(NumStreamBuffers, StreamBuffers);

    unsigned char* ByteBuffer=new unsigned char[BufferSize];
    int            ReadBytes =0;

    // Prepare first stream buffers.
    for (unsigned int i=0; i<NumStreamBuffers; i++)
    {
        // Read data for each stream buffer.
        ReadBytes=m_Stream->Read(ByteBuffer, BufferSize);

        // FIXME For now we assert that there are at least 5 buffers to read.
        // FIXME Later we have to check return values from mpg123_read and change format, end prematurely etc.
        if (ReadBytes<0)
        {
            std::cout << "OpenAL: Error creating music buffer.\n";
            //StopMusic();
            //return false;
        }

        if (m_Stream->GetChannels()>1 && m_Is3DSound) ReadBytes=ConvertToMono(ByteBuffer, ReadBytes);

        alBufferData(StreamBuffers[i], m_OutputFormat, ByteBuffer, ReadBytes, m_Stream->GetRate());

        // Add buffers to array for deletion on Update() or destruct.
        m_CurrentBuffers.PushBack(StreamBuffers[i]);
    }

    delete[] ByteBuffer;

    // Attach first stream buffers (created in constructor).
    alSourceQueueBuffers(MixerTrack->m_SourceHandle, NumStreamBuffers, StreamBuffers);

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

    assert(alGetError()==AL_NO_ERROR);
    return true;
}


bool StreamingBufferT::DetachFromMixerTrack(MixerTrackT* MixerTrack)
{
    assert(m_MixerTracks.Size()==1);

    if (m_MixerTracks[0]!=MixerTrack) return false;

    m_MixerTracks.Clear();

    return true;
}
