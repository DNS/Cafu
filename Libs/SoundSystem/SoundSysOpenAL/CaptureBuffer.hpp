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
#include "OpenALIncl.hpp"


class SoundStreamT;


/// A buffer that is similar to a StreamingBufferT but that obtains its data by capturing
/// input from an OpenAL capture device (rather than by decoding a file data stream).
/// The captured input is streamed into a set of streaming buffers for playback.
///
/// Unlike streaming buffers (but very much like static buffers), capture buffers can be
/// attached to several mixer tracks simultaneously (as they cannot be rewound).
class CaptureBufferT : public BufferT
{
    public:

    /// The constructor.
    /// @param DeviceName   The name of the OpenAL device (as obtained from the ALC_CAPTURE_DEVICE_SPECIFIER list) used for capturing.
    /// @param Is3DSound    Whether the buffer is used as a 3D sound object.
    CaptureBufferT(const std::string& DeviceName, bool Is3DSound);

    /// The destructor.
    ~CaptureBufferT();

    // BufferT implementation.
    void Update();
    void Rewind();
    bool IsStream() const;
    bool AttachToMixerTrack(MixerTrackT* MixerTrack);
    bool DetachFromMixerTrack(MixerTrackT* MixerTrack);


    private:

    CaptureBufferT(const CaptureBufferT&);      ///< Use of the Copy    Constructor is not allowed.
    void operator = (const CaptureBufferT&);    ///< Use of the Assignment Operator is not allowed.

    ALCdevice*            m_CaptureDevice;      ///< The OpenGL device from which we capture input to provide the PCM data for the buffers.
    ArrayT<unsigned char> m_RawPcmOutputBuffer; ///< This buffer collects the captured (and possibly audio processed) raw PCM data for output until one of the OpenAL output buffers has been processed and is available for reuse.
    ArrayT<ALuint>        m_OutputBuffers;      ///< The buffers that are queued on the source and played alternately with current data from the input.
    ArrayT<ALuint>        m_RecycleBuffers;     ///< The output buffers that are currently available for re-use.
    ALenum                m_OutputFormat;       ///< OpenAL output format used for stream buffers.
};

#endif
