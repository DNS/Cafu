/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "CaptureStream.hpp"
#include "Templates/Array.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <stdexcept>


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


CaptureStreamT::CaptureStreamT(const std::string& DeviceName, ALenum Format, unsigned int SampleFrq)
    : m_FORMAT(Format),
      m_SAMPLE_FRQ(SampleFrq),
      m_MAX_CAPTURE_BUFFER_SAMPLES(SampleFrq*4),
      m_CaptureDevice(NULL),
      m_IsCapturing(false),
      m_ZeroSamples(0)
{
    // m_MAX_CAPTURE_BUFFER_SAMPLES is the size of the ring buffer that OpenAL is to use for capturing,
    // given in number of samples and initialized above to keep 4 seconds worth of data.
    // We always try to keep this buffer about half filled in order to avoid buffer over- or underflow.
    m_CaptureDevice=alcCaptureOpenDevice(
        DeviceName.empty() ? NULL : DeviceName.c_str(),
        m_SAMPLE_FRQ,
        m_FORMAT,
        m_MAX_CAPTURE_BUFFER_SAMPLES);

    if (m_CaptureDevice==NULL)
        throw std::runtime_error("Error opening capture device "+DeviceName);
}


CaptureStreamT::~CaptureStreamT()
{
    if (m_IsCapturing)
    {
        alcCaptureStop(m_CaptureDevice);
        m_IsCapturing=false;
    }

    alcCaptureCloseDevice(m_CaptureDevice);
}


unsigned int CaptureStreamT::GetNumCaptureSamples() const
{
    int NumCaptureSamples=0;
    alcGetIntegerv(m_CaptureDevice, ALC_CAPTURE_SAMPLES, 1, &NumCaptureSamples);

    return NumCaptureSamples;
}


void CaptureStreamT::ReduceSamplesTo(unsigned int NumLeft)
{
    const unsigned int NumNow=GetNumCaptureSamples();

    if (NumNow>NumLeft)
    {
        ArrayT<unsigned char> Discard;

        Discard.PushBackEmpty((NumNow-NumLeft)*BytesPerSample(m_FORMAT));
        alcCaptureSamples(m_CaptureDevice, &Discard[0], NumNow-NumLeft);
    }
}


void CaptureStreamT::PrintDebug() const
{
    const unsigned int NumNow=GetNumCaptureSamples();

    std::cout << m_ZeroSamples << "+" << NumNow << " = " << m_ZeroSamples+NumNow
              << " of " << m_MAX_CAPTURE_BUFFER_SAMPLES << " samples in buffer ("
              << float(m_ZeroSamples+NumNow)/m_MAX_CAPTURE_BUFFER_SAMPLES*100.0f << "%).\n";
}


int CaptureStreamT::Read(unsigned char* Buffer, unsigned int Size)
{
    const unsigned int MAX_WRITE_SAMPLES=Size/BytesPerSample(m_FORMAT);

    // If this is the first read after initialization or a rewind, start capturing now.
    // When capturing has been stopped before and is restarted here, GetNumCaptureSamples() should return 0,
    // just as if it had been started for the first time. See Rewind() for more information.
    if (!m_IsCapturing)
    {
        alcCaptureStart(m_CaptureDevice);
        m_IsCapturing=true;
    }

    // std::cout << "\n" << __FUNCTION__ << "():\n" << "        "; PrintDebug();


    // If there are not enough "live" capture samples available, fill-in zeros until
    // the buffer is half full. This gives the device the chance to capture more data.
    if (GetNumCaptureSamples() < m_MAX_CAPTURE_BUFFER_SAMPLES/8)
    {
        m_ZeroSamples=m_MAX_CAPTURE_BUFFER_SAMPLES/2-GetNumCaptureSamples();

        // std::cout << "  + ... "; PrintDebug();
    }

    // If the capture buffer is too full, we have probably lost data already.
    // In order to prevent this from happening over and over again, reduce it to half its size.
    if (m_ZeroSamples+GetNumCaptureSamples() >= m_MAX_CAPTURE_BUFFER_SAMPLES)
    {
        m_ZeroSamples=0;
        ReduceSamplesTo(m_MAX_CAPTURE_BUFFER_SAMPLES/2 + MAX_WRITE_SAMPLES);

        // std::cout << "  - ... "; PrintDebug();
    }


    // Write the leading zero samples.
    unsigned int SamplesWritten=std::min(m_ZeroSamples, MAX_WRITE_SAMPLES);

    memset(Buffer, 0, SamplesWritten*BytesPerSample(m_FORMAT));
    m_ZeroSamples-=SamplesWritten;

    // Write the samples with real, live, captured data.
    if (GetNumCaptureSamples()>0 && SamplesWritten<MAX_WRITE_SAMPLES)
    {
        const unsigned int NumLive=std::min(GetNumCaptureSamples(), MAX_WRITE_SAMPLES-SamplesWritten);

        alcCaptureSamples(m_CaptureDevice, &Buffer[SamplesWritten*BytesPerSample(m_FORMAT)], NumLive);
        SamplesWritten+=NumLive;
    }

    // std::cout << "        " << SamplesWritten << " samples written,\n" << "     =  "; PrintDebug();

    return SamplesWritten*BytesPerSample(m_FORMAT);
}


bool CaptureStreamT::Rewind()
{
    // Captured input cannot be rewound...
    if (m_IsCapturing)
    {
        alcCaptureStop(m_CaptureDevice);
        m_IsCapturing=false;

        // Remove all remaining capture samples from the capture device, so that when capturing is restarted,
        // no stale samples are left. According to the OpenAL spec, this should happen automatically:
        // "The amount of audio samples available after restarting a stopped capture device is reset to zero.",
        // but not all implementations seem to conform.
        ReduceSamplesTo(0);
    }

    m_ZeroSamples=0;
    return true;
}


unsigned int CaptureStreamT::GetChannels()
{
    return m_FORMAT==AL_FORMAT_MONO16 ? 1 : 2;
}


unsigned int CaptureStreamT::GetRate()
{
    return m_SAMPLE_FRQ;
}
