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

#include <iostream>


static const unsigned int SAMPLE_FRQ=44100;
static const unsigned int MAX_RAW_PCM_BUFFER_BYTES=65536;                   ///< Size in bytes of raw PCM buffers (input and output).
static const unsigned int MAX_CAPTURE_SAMPLES=MAX_RAW_PCM_BUFFER_BYTES/4;   ///< Size of capture buffer in sample frames (16-bit stereo).


CaptureBufferT::CaptureBufferT(const std::string& DeviceName, bool ForceMono)
    : BufferT(DeviceName, ForceMono),
      m_Format(ForceMono ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16),
      m_CaptureDevice(NULL)
{
    m_CaptureDevice=alcCaptureOpenDevice(
        DeviceName.empty() ? NULL : DeviceName.c_str(),
        SAMPLE_FRQ,
        m_Format,
        MAX_CAPTURE_SAMPLES);

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


void CaptureBufferT::Update()
{
    if (!m_CaptureDevice) return;

    int NumCaptureSamples=0;
    alcGetIntegerv(m_CaptureDevice, ALC_CAPTURE_SAMPLES, 1, &NumCaptureSamples);

    if (NumCaptureSamples>0)
    {
        static unsigned char RawPcmInputBuffer[MAX_RAW_PCM_BUFFER_BYTES];

        // TODO: Can implement this *without* the RawPcmInputBuffer, capture directly into m_RawPcmOutputBuffer instead!
        alcCaptureSamples(m_CaptureDevice, RawPcmInputBuffer, NumCaptureSamples);

        // Later: Feed newly read samples through digital signal processor...
        ;

        // Append (processed samples/) newly read samples to output buffer.
        const unsigned long MAX_OUTPUT_BUFFER_BYTES=MAX_RAW_PCM_BUFFER_BYTES*2;
        const unsigned long NumCopyBytes=std::min(NumCaptureSamples*4ul, MAX_OUTPUT_BUFFER_BYTES-m_RawPcmOutputBuffer.Size());

        m_RawPcmOutputBuffer.PushBackEmpty(NumCopyBytes);
        memcpy(&m_RawPcmOutputBuffer[m_RawPcmOutputBuffer.Size()-NumCopyBytes], RawPcmInputBuffer, NumCopyBytes);
    }

    // ...
    {
        int NumRecycle=0;
        alGetSourcei(m_MixerTracks[0]->GetOpenALSource(), AL_BUFFERS_PROCESSED, &NumRecycle);

        assert(m_RecycleBuffers.Size()+NumRecycle <= m_OutputBuffers.Size());

        if (NumRecycle>0)
        {
            m_RecycleBuffers.PushBackEmpty(NumRecycle);
            alSourceUnqueueBuffers(m_MixerTracks[0]->GetOpenALSource(), NumRecycle, &m_RecycleBuffers[m_RecycleBuffers.Size()-NumRecycle]);
        }
    }

    if (m_RecycleBuffers.Size()>0 && m_RawPcmOutputBuffer.Size()>4096)
    {
        const unsigned int NumOutBytes=m_RawPcmOutputBuffer.Size();

        alBufferData(m_RecycleBuffers[0], m_Format, &m_RawPcmOutputBuffer[0], NumOutBytes, SAMPLE_FRQ);
        m_RawPcmOutputBuffer.Overwrite();

        // (Re-)Queue the filled buffers on the mixer track (the OpenAL source).
        alSourceQueueBuffers(m_MixerTracks[0]->GetOpenALSource(), 1, &m_RecycleBuffers[0]);
        m_RecycleBuffers.RemoveAtAndKeepOrder(0);

        // If all buffers were completely processed, the playback was finished and stopped; restart it now.
        int SourceState=0;
        alGetSourcei(m_MixerTracks[0]->GetOpenALSource(), AL_SOURCE_STATE, &SourceState);
        if (SourceState!=AL_PLAYING) alSourcePlay(m_MixerTracks[0]->GetOpenALSource());
    }

    assert(alGetError()==AL_NO_ERROR);
}


void CaptureBufferT::Rewind()
{
    // Captured input cannot be rewound...
}


bool CaptureBufferT::IsStream() const
{
    return true;
}


bool CaptureBufferT::AttachToMixerTrack(MixerTrackT* MixerTrack)
{
    if (!m_CaptureDevice) return false;

    // Capture buffers are unique and can only be attached to one source.
    if (m_MixerTracks.Size()>0) return false;
    m_MixerTracks.PushBack(MixerTrack);

    std::cout << "OpenAL: Starting audio capture from device \"" << GetName() << "\".\n";
    alcCaptureStart(m_CaptureDevice);

    m_RecycleBuffers=m_OutputBuffers;   // All output buffers are initially available for immediate (re-)use!

    assert(alGetError()==AL_NO_ERROR);
    return true;
}


bool CaptureBufferT::DetachFromMixerTrack(MixerTrackT* MixerTrack)
{
    assert(m_MixerTracks.Size()==1);
    if (m_MixerTracks[0]!=MixerTrack) return false;

    std::cout << "OpenAL: Stopping audio capture.\n";
    alcCaptureStop(m_CaptureDevice);

    m_MixerTracks.Overwrite();
    return true;
}
