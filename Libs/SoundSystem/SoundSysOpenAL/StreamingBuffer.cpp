/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "StreamingBuffer.hpp"
#include "MixerTrack.hpp"

#include "../Common/SoundStream.hpp"

#include <iostream>


StreamingBufferT::StreamingBufferT(const std::string& ResName, bool ForceMono)
    : BufferT(ResName, ForceMono),
      m_Stream(SoundStreamT::Create(ResName)),      // Throws an exception of type std::runtime_error on failure.
      m_Buffers()
{
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

        // Later: Feed newly read samples through the digital signal processor...
        ;

        alBufferData(Buffers[BufNr], OutputFormat, RawPcmBuffer, ReadBytes, m_Stream->GetRate());

        // (Re-)Queue the filled buffers on the mixer track (the OpenAL source).
        alSourceQueueBuffers(m_MixerTracks[0]->GetOpenALSource(), 1, &Buffers[BufNr]);
    }

    return BufNr;
}


void StreamingBufferT::Update()
{
    if (m_MixerTracks.Size()!=1 || m_MixerTracks[0]==NULL) return;

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

    // Let the stream know that we're done for now with regular calls to Read().
    m_Stream->Rewind();

    // Remove all our buffers from this source.
    alSourcei(MixerTrack->GetOpenALSource(), AL_BUFFER, 0);

    m_MixerTracks.Overwrite();
    return true;
}
