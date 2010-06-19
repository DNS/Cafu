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

#ifndef _SOUNDSYS_CAPTURE_STREAM_HPP_
#define _SOUNDSYS_CAPTURE_STREAM_HPP_

#include "OpenALIncl.hpp"
#include "../Common/SoundStream.hpp"


/// This class represents a stream whose data is obtained from an OpenAL capture device.
class CaptureStreamT : public SoundStreamT
{
    public:

    /// The constructor.
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
