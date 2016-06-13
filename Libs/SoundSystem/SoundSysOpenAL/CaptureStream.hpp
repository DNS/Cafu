/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SOUNDSYS_CAPTURE_STREAM_HPP_INCLUDED
#define CAFU_SOUNDSYS_CAPTURE_STREAM_HPP_INCLUDED

#include "OpenALIncl.hpp"
#include "../Common/SoundStream.hpp"


/// This class represents a stream whose data is obtained from an OpenAL capture device.
class CaptureStreamT : public SoundStreamT
{
    public:

    /// The constructor. Throws an exception of type std::runtime_error on failure.
    /// @param DeviceName   The name of the OpenAL device (as obtained from the ALC_CAPTURE_DEVICE_SPECIFIER list) that is used for capturing.
    /// @param Format       The data format in which the samples are captured, kept and returned). Must be AL_FORMAT_MONO16 or AL_FORMAT_STEREO16.
    /// @param SampleFrq    The frequency the device is sampled with.
    CaptureStreamT(const std::string& DeviceName, ALenum Format=AL_FORMAT_STEREO16, unsigned int SampleFrq=44100);

    /// The destructor.
    ~CaptureStreamT();

    // SoundStreamT implementation.
    int          Read(unsigned char* Buffer, unsigned int Size);
    bool         Rewind();
    unsigned int GetChannels();
    unsigned int GetRate();


    private:

    unsigned int GetNumCaptureSamples() const;          ///< Returns the number of currently available samples in the OpenAL capture buffer. This is just a wrapper around alcGetIntegerv(m_CaptureDevice, ALC_CAPTURE_SAMPLES, ...).
    void         ReduceSamplesTo(unsigned int NumLeft); ///< Reduces the number of samples in the OpenAL capture buffer so that no more than NumLeft samples are left.
    void         PrintDebug() const;                    ///< Prints some debug output to the standard output stream.

    const ALenum       m_FORMAT;                        ///< The OpenAL data format that we use for both capturing and output.
    const unsigned int m_SAMPLE_FRQ;                    ///< The frequency in Hz with which the capture device is sampled. Defaults to 44100 Hz.
    const unsigned int m_MAX_CAPTURE_BUFFER_SAMPLES;    ///< The size of the ring buffer that OpenAL is to use for capturing, given in number of samples. We always try to keep this buffer about half filled in order to avoid buffer over- or underflow.
    ALCdevice*         m_CaptureDevice;                 ///< The OpenGL device from which we capture input to provide the PCM data for the buffers.
    bool               m_IsCapturing;                   ///< True while we're actually capturing (between calls to alcCaptureStart() and alcCaptureStop()).
    unsigned int       m_ZeroSamples;                   ///< The number of "zero" samples that are currently filled in to cover for buffer underflow.
};

#endif
