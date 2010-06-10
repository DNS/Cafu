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


StreamingBufferT::StreamingBufferT(const std::string& FileName, bool ForceMono)
    : BufferT(FileName, ForceMono),
      m_Stream(SoundStreamT::Create(FileName)),
      m_Buffers()
{
    if (m_Stream==NULL) std::cout << "OpenAL: Error creating streamed sound buffer '" << FileName << "'\n";
    assert(m_Stream!=NULL);

    m_Buffers.PushBackEmptyExact(5);
    alGenBuffers(m_Buffers.Size(), &m_Buffers[0]);
}


StreamingBufferT::~StreamingBufferT()
{
    alDeleteBuffers(m_Buffers.Size(), &m_Buffers[0]);

    const int Error=alGetError();
    if (Error!=AL_NO_ERROR) std::cout << "OpenAL: Error deleting buffer: " << TranslateErrorCode(Error) << "\n";

    delete m_Stream;
}


unsigned int StreamingBufferT::GetChannels() const
{
    return ForcesMono() ? 1 : m_Stream->GetChannels();
}


bool StreamingBufferT::CanShare() const
{
    return false;
}


unsigned int StreamingBufferT::FillAndQueue(const ArrayT<ALuint>& Buffers)
{
    static const unsigned int RAW_PCM_BUFFER_SIZE=65536;            ///< Size in bytes of raw PCM data exchange buffer.
    static unsigned char      RawPcmBuffer[RAW_PCM_BUFFER_SIZE];    ///< The buffer for transferring raw PCM data from the stream to the OpenAL buffers.
    unsigned long             BufNr=0;

    for (BufNr=0; BufNr<Buffers.Size(); BufNr++)
    {
        ALenum OutputFormat=(m_Stream->GetChannels()==1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
        int    ReadBytes   =m_Stream->Read(RawPcmBuffer, RAW_PCM_BUFFER_SIZE);

        // if (ReadBytes==0 && shader->loops)
        // {
        //     m_Stream->Rewind();
        //     ReadBytes=m_Stream->Read(RawPcmBuffer, RAW_PCM_BUFFER_SIZE);
        // }

        if (ReadBytes<=0)
        {
            if (ReadBytes<0) std::cout << __FUNCTION__ << ": Error reading stream " << GetName() << ".\n";
            break;
        }

        if (ForcesMono() && OutputFormat==AL_FORMAT_STEREO16)
        {
            OutputFormat=AL_FORMAT_MONO16;
            ReadBytes   =ConvertToMono(RawPcmBuffer, ReadBytes);
        }

        alBufferData(Buffers[BufNr], OutputFormat, RawPcmBuffer, ReadBytes, m_Stream->GetRate());

        // (Re-)Queue the filled buffers on the mixer track (the OpenAL source).
        alSourceQueueBuffers(m_MixerTracks[0]->GetOpenALSource(), 1, &Buffers[BufNr]);
    }

    //DELETE // When Update() is not called frequently enough, our buffer queue may run empty before the end of the stream has been reached.
    //DELETE // In order to prevent OpenAL from stopping the source (that is, putting it in state AL_STOPPED) in that case,
    //DELETE // we activate looping. As such, looping acts as a safeguard against unintentional buffer underrun.
    //DELETE // Looping is deactivated when the end of the stream has been reached (and the shader specifies no looping, see above).
    //DELETE alSourcei(m_MixerTracks[0]->GetOpenALSource(), AL_LOOPING, m_EndReached ? AL_FALSE : AL_TRUE);
    return BufNr;
}


void StreamingBufferT::Update()
{
    if (m_MixerTracks.Size()!=1 || m_MixerTracks[0]==NULL) return;

    //DELETE // Deactivate looping before the buffer queue is queried and manipulated.
    //DELETE // If looping is active, the AL_BUFFERS_PROCESSED query below always returns 0.
    //DELETE // Looping is re-activated in FillAndQueue().
    //DELETE alSourcei(m_MixerTracks[0]->GetOpenALSource(), AL_LOOPING, AL_FALSE);

    int NumRecycle=0;
    alGetSourcei(m_MixerTracks[0]->GetOpenALSource(), AL_BUFFERS_PROCESSED, &NumRecycle);

    if (NumRecycle==0) return;

    static ArrayT<ALuint> RecycleBuffers;
    RecycleBuffers.Overwrite();
    RecycleBuffers.PushBackEmpty(NumRecycle);

    alSourceUnqueueBuffers(m_MixerTracks[0]->GetOpenALSource(), RecycleBuffers.Size(), &RecycleBuffers[0]);

    const unsigned int NumNewBuffers=FillAndQueue(RecycleBuffers);

    // When Update() is not called frequently enough, our buffer queue may run empty before the end of the stream has been reached.
    // (Note that Update() is always called, independently from the state (AL_INITIAL, AL_PLAYING, AL_STOPPED, ...) the source is currently in!)
    // Now automatically restart the playback if necessary:
    // 1. Initially, before the stream playback is started, we may or may not have buffers queued on the source,
    //    but none of them has been processed yet: NumRecycle==0 above, the playback is not auto-restarted here.
    // 2. When the stream is playing, everything runs normally and from the fact that we have queued more buffers
    //    with new data, we can auto-restart the playback if necessary.
    // 3. When the stream ends, the remaining buffers are processed and retrieved, but no buffers are refilled and requeued.
    //    Eventually, all buffers have been retrieved and NumRecycle==0 again.
    // x. When playback is stopped in mid-play, the stopping code must also make sure that we are reinitialized,
    //    reverting to case 1. The mixer tracks accomplish this by detaching and re-attaching us appropriately.
    if (NumNewBuffers>0)
    {
        int SourceState=0;
        alGetSourcei(m_MixerTracks[0]->GetOpenALSource(), AL_SOURCE_STATE, &SourceState);

        if (SourceState==AL_STOPPED)
            alSourcePlay(m_MixerTracks[0]->GetOpenALSource());
    }

    assert(alGetError()==AL_NO_ERROR);
}


bool StreamingBufferT::AttachToMixerTrack(MixerTrackT* MixerTrack)
{
    // Stream buffers are unique and can only be attached to one source.
    if (m_MixerTracks.Size()>0) return false;
    m_MixerTracks.PushBack(MixerTrack);

    alSourcei(MixerTrack->GetOpenALSource(), AL_BUFFER, 0);
    m_Stream->Rewind();
    FillAndQueue(m_Buffers);

    assert(alGetError()==AL_NO_ERROR);
    return true;
}


bool StreamingBufferT::DetachFromMixerTrack(MixerTrackT* MixerTrack)
{
    if (m_MixerTracks.Size()!=1 || m_MixerTracks[0]!=MixerTrack) return false;

    //DELETE // Reset looping to the default setting (deactivated) again.
    //DELETE alSourcei(MixerTrack->GetOpenALSource(), AL_LOOPING, AL_FALSE);

    // Remove all our buffers from this source.
    alSourcei(MixerTrack->GetOpenALSource(), AL_BUFFER, 0);

    m_MixerTracks.Overwrite();
    return true;
}
