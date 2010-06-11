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

#include "CaptureBuffer.hpp"
#include "MixerTrack.hpp"

#include "../Common/SoundStream.hpp"

#include <cstring>
#include <iostream>


static const unsigned int SAMPLE_FRQ=44100;
static const unsigned int MAX_RAW_PCM_BUFFER_BYTES=65536;   ///< Size in bytes of raw PCM buffers (input and output).


static unsigned int BytesPerSample(ALenum Format)
{
    switch (Format)
    {
        case AL_FORMAT_MONO8:    return 1;
        case AL_FORMAT_MONO16:   return 2;
        case AL_FORMAT_STEREO8:  return 2;
        case AL_FORMAT_STEREO16: return 4;
    }

    return 4;
}


CaptureBufferT::CaptureBufferT(const std::string& DeviceName, bool ForceMono)
    : BufferT(DeviceName, ForceMono),
      m_Format(ForceMono ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16),
      m_CaptureDevice(NULL)
{
    m_CaptureDevice=alcCaptureOpenDevice(
        DeviceName.empty() ? NULL : DeviceName.c_str(),
        SAMPLE_FRQ,
        m_Format,
        MAX_RAW_PCM_BUFFER_BYTES/BytesPerSample(m_Format));

    if (m_CaptureDevice==NULL) std::cout << "OpenAL: Error opening capture device '" << DeviceName << "'\n";
    assert(m_CaptureDevice!=NULL);

    m_OutputBuffers.PushBackEmptyExact(5);
    alGenBuffers(m_OutputBuffers.Size(), &m_OutputBuffers[0]);
}


CaptureBufferT::~CaptureBufferT()
{
    alDeleteBuffers(m_OutputBuffers.Size(), &m_OutputBuffers[0]);

    const int Error=alGetError();
    if (Error!=AL_NO_ERROR) std::cout << "OpenAL: Error deleting buffer: " << TranslateErrorCode(Error) << "\n";

    if (m_CaptureDevice) alcCaptureCloseDevice(m_CaptureDevice);

    assert(alGetError()==AL_NO_ERROR);
}


unsigned int CaptureBufferT::GetChannels() const
{
    return m_Format==AL_FORMAT_MONO16 ? 1 : 2;
}


bool CaptureBufferT::CanShare() const
{
    return false;
}


void CaptureBufferT::Update()
{
    if (!m_CaptureDevice) return;
    if (m_MixerTracks.Size()!=1 || m_MixerTracks[0]==NULL) return;


    int NumCaptureSamples=0;
    alcGetIntegerv(m_CaptureDevice, ALC_CAPTURE_SAMPLES, 1, &NumCaptureSamples);

    // Read any available capture samples, optionally run DSP on them, and store them in our raw PCM output buffer.
    if (NumCaptureSamples>0)
    {
        static unsigned char RawPcmInputBuffer[MAX_RAW_PCM_BUFFER_BYTES];

        // TODO: Can implement this *without* the RawPcmInputBuffer, capture directly into m_RawPcmOutputBuffer instead!
        alcCaptureSamples(m_CaptureDevice, RawPcmInputBuffer, NumCaptureSamples);

        // Later: Feed newly read samples through the digital signal processor...
        ;

        // Append (processed samples/) newly read samples to output buffer.
        const unsigned long MAX_OUTPUT_BUFFER_BYTES=MAX_RAW_PCM_BUFFER_BYTES*2;
        const unsigned long NumCopyBytes=std::min((unsigned long) NumCaptureSamples*BytesPerSample(m_Format),
                                                  MAX_OUTPUT_BUFFER_BYTES-m_RawPcmOutputBuffer.Size());

        if (NumCopyBytes>0)   // Accessing m_RawPcmOutputBuffer at index m_RawPcmOutputBuffer.Size()-0 is the problem...
        {
            m_RawPcmOutputBuffer.PushBackEmpty(NumCopyBytes);
            memcpy(&m_RawPcmOutputBuffer[m_RawPcmOutputBuffer.Size()-NumCopyBytes], RawPcmInputBuffer, NumCopyBytes);
        }

    }

    // std::cout << "\n1. captured " << NumCaptureSamples << " new samples, m_RawPcmOutputBuffer size "
    //           << m_RawPcmOutputBuffer.Size() << " / " << MAX_RAW_PCM_BUFFER_BYTES*2 << "\n";


    // Add any processed buffers to those available for recycling.
    int NumRecycle=0;
    alGetSourcei(m_MixerTracks[0]->GetOpenALSource(), AL_BUFFERS_PROCESSED, &NumRecycle);

    if (NumRecycle>0)
    {
        assert(m_RecycleBuffers.Size()+NumRecycle <= m_OutputBuffers.Size());

        m_RecycleBuffers.PushBackEmpty(NumRecycle);
        alSourceUnqueueBuffers(m_MixerTracks[0]->GetOpenALSource(), NumRecycle, &m_RecycleBuffers[m_RecycleBuffers.Size()-NumRecycle]);
    }

    // std::cout << "2. buffers processed " << NumRecycle << ", m_RecycleBuffers.Size() " << m_RecycleBuffers.Size() << "\n";


    // If enough data has been captured, enqueue it in one of the available buffers.
    if (m_RecycleBuffers.Size()>0)
    {
        if (m_RawPcmOutputBuffer.Size()>4096)
        {
            alBufferData(m_RecycleBuffers[0], m_Format, &m_RawPcmOutputBuffer[0], m_RawPcmOutputBuffer.Size(), SAMPLE_FRQ);
            m_RawPcmOutputBuffer.Overwrite();

            // Enqueue the filled buffers on the mixer track (the OpenAL source).
            alSourceQueueBuffers(m_MixerTracks[0]->GetOpenALSource(), 1, &m_RecycleBuffers[0]);
            m_RecycleBuffers.RemoveAtAndKeepOrder(0);

            // std::cout << "3. enqueued OpenAL buffer\n";
        }
        else
        {
            unsigned char ZerosBuffer[MAX_RAW_PCM_BUFFER_BYTES];

            memset(ZerosBuffer, 0, MAX_RAW_PCM_BUFFER_BYTES);
            for (unsigned long BufNr=0; BufNr<m_RecycleBuffers.Size(); BufNr++)
                alBufferData(m_RecycleBuffers[BufNr], m_Format, ZerosBuffer, MAX_RAW_PCM_BUFFER_BYTES, SAMPLE_FRQ);

            // Enqueue the filled buffers on the mixer track (the OpenAL source).
            alSourceQueueBuffers(m_MixerTracks[0]->GetOpenALSource(), m_RecycleBuffers.Size(), &m_RecycleBuffers[0]);
            m_RecycleBuffers.Overwrite();

            // std::cout << "3. enqueued 0'ed OpenAL buffers.\n";
        }

        // When Update() is not called frequently enough, our buffer queue may run empty before the end of the stream has been reached.
        // (Note that Update() is always called, independently from the state (AL_INITIAL, AL_PLAYING, AL_STOPPED, ...) the source is currently in!)
        // Now automatically restart the playback if necessary:
        // 1. Initially, before the captured data playback is started, we may or may not have buffers queued on the source,
        //    but none of them has been processed yet: NumRecycle==0 above, the playback is not auto-restarted here.
        // 2. When the capture data is playing, everything runs normally and from the fact that we have queued more buffers
        //    with new data, we can auto-restart the playback if necessary.
        // 3. Unlike a stream, capturing data from a device never reaches a natural end where it might stop all by itself.
        // x. When playback is stopped in mid-play, the stopping code must also make sure that we are reinitialized,
        //    reverting to case 1. The mixer tracks accomplish this by detaching and re-attaching us appropriately.
        if (NumRecycle>0 /*&& NumNewBuffers>0*/)    // NumNewBuffers==1 in this context.
        {
            int SourceState=0;
            alGetSourcei(m_MixerTracks[0]->GetOpenALSource(), AL_SOURCE_STATE, &SourceState);

            if (SourceState==AL_STOPPED)
                alSourcePlay(m_MixerTracks[0]->GetOpenALSource());
        }
    }

    assert(alGetError()==AL_NO_ERROR);
}


bool CaptureBufferT::AttachToMixerTrack(MixerTrackT* MixerTrack)
{
    if (!m_CaptureDevice) return false;

    // Capture buffers cannot be shared - they can only be attached to a single source.
    if (m_MixerTracks.Size()>0) return false;
    m_MixerTracks.PushBack(MixerTrack);

    alSourcei(MixerTrack->GetOpenALSource(), AL_BUFFER, 0);
    m_RawPcmOutputBuffer.Overwrite();
    m_RecycleBuffers=m_OutputBuffers;   // All output buffers are initially available for immediate (re-)use!
    Update();

    std::cout << "OpenAL: Starting audio capture from device \"" << GetName() << "\".\n";
    alcCaptureStart(m_CaptureDevice);

    assert(alGetError()==AL_NO_ERROR);
    return true;
}


bool CaptureBufferT::DetachFromMixerTrack(MixerTrackT* MixerTrack)
{
    if (m_MixerTracks.Size()!=1 || m_MixerTracks[0]!=MixerTrack) return false;

    std::cout << "OpenAL: Stopping audio capture.\n";
    alcCaptureStop(m_CaptureDevice);

    // Remove all our buffers from this source.
    alSourcei(MixerTrack->GetOpenALSource(), AL_BUFFER, 0);

    m_MixerTracks.Overwrite();
    return true;
}
