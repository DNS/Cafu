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

#ifndef _SOUNDSYS_CAPTURE_BUFFER_HPP_
#define _SOUNDSYS_CAPTURE_BUFFER_HPP_

#include "Buffer.hpp"


/// A CaptureBufferT is a BufferT specialization for audio data captured from an OpenAL capture device.
/// The audio data is captured from the device and "streamed" (using buffer queuing) to the OpenAL source.
/// CaptureBufferT instances cannot be shared, each instance can only be used on a single mixer track.
class CaptureBufferT : public BufferT
{
    public:

    /// The constructor.
    /// @param DeviceName   The name of the OpenAL device (as obtained from the ALC_CAPTURE_DEVICE_SPECIFIER list) that is used for capturing.
    /// @param ForceMono    Whether the data from the resource should be reduced to a single channel before use (mono output).
    CaptureBufferT(const std::string& DeviceName, bool ForceMono);

    /// The destructor.
    ~CaptureBufferT();

    // BufferT implementation.
    unsigned int GetChannels() const;
    bool CanShare() const;
    void Update();
    bool AttachToMixerTrack(MixerTrackT* MixerTrack);
    bool DetachFromMixerTrack(MixerTrackT* MixerTrack);


    private:

    const ALenum          m_Format;             ///< The OpenAL data format that we use for both capturing and output.
    ALCdevice*            m_CaptureDevice;      ///< The OpenGL device from which we capture input to provide the PCM data for the buffers.
    ArrayT<unsigned char> m_RawPcmOutputBuffer; ///< This buffer collects the captured (and possibly audio processed) raw PCM data for output until one of the OpenAL output buffers has been processed and is available for reuse.
    ArrayT<ALuint>        m_OutputBuffers;      ///< The buffers that are queued on the source and played alternately with current data from the input.
    ArrayT<ALuint>        m_RecycleBuffers;     ///< The output buffers that are currently available for re-use.
};

#endif
