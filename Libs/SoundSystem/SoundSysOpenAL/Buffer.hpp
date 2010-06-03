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

#ifndef _SOUNDSYS_BUFFER_HPP_
#define _SOUNDSYS_BUFFER_HPP_

#include "Templates/Array.hpp"

#include <string>


class MixerTrackT;


/// A sound buffer created from an audio file.
/// This basic version contains all attributes that are common for both a static buffer and a streaming buffer
/// and provides a general interface, so the calling code doesn't need to distinguish between different buffer
/// types.
//
// TODO 1: Remove Is3DSound, instead use IsMono or ForceMono / MakeMono or even more flexibly, the number of channels.
//         Maybe even better, have the AL_FORMAT_* that is also passed to alBufferData() as a ctor param and keep it as a class member.
// TODO 2: Remove IsStream(), instead use something like "CanPlayOnMultipleMixerTracks" or something.
class BufferT
{
    public:

    /// Constructor.
    BufferT(const std::string& FileName="", bool Is3DSound=true) : References(0), m_FileName(FileName), m_Is3DSound(Is3DSound) { }

    /// Virtual destructor so the proper destructor of the underlying buffer is called.
    virtual ~BufferT() { }

    /// Returns the name of the resource underlying this buffer (usually a file name).
    const std::string& GetName() const { return m_FileName; }

    /// Returns whether this buffer is for 2D or 3D playback... but also see the TODO above.
    bool Is3D() const { return m_Is3DSound; }

    /// Updates the buffer (this is only relevant for streaming buffers).
    virtual void Update()=0;

    /// Rewinds the buffer (this is only relevant for streaming buffers).
    virtual void Rewind()=0;

    /// Provides information if the underlying buffer is a streaming buffer.
    /// @return Whether the buffer is a stream.
    virtual bool IsStream() const=0;

    /// Attaches the buffer to a mixer track, so the mixer track can play this buffer.
    /// Note that depending on the underlying buffer it is possible to attach one buffer to multiple mixer tracks.
    /// As for streaming buffers this is not possible since they are unique and can only be attached to one mixer
    /// track.
    /// @param MixerTrack The mixer track this buffer should be attached to.
    /// @return Whether the buffer could be attached to the mixer track.
    virtual bool AttachToMixerTrack(MixerTrackT* MixerTrack)=0;

    /// Detaches the buffer from a mixer track.
    /// This method informs the mixer track as well as the buffer of the change.
    /// @param MixerTrack The mixer track this buffer should be attached from.
    /// @return Whether the buffer could be attached to the mixer track. False means usually that this buffer wasn't
    ///         attached to the passed mixer track at all.
    virtual bool DetachFromMixerTrack(MixerTrackT* MixerTrack)=0;


    unsigned int References; ///< Number of references to this buffer (e.g. how many sound objects use this buffer).


    protected:

    std::string          m_FileName;    ///< Name of this file this buffer was created from.
    ArrayT<MixerTrackT*> m_MixerTracks; ///< Mixer tracks this buffer is currently attached to.
    bool                 m_Is3DSound;   ///< Determines how the buffer is created and attached to a source.

    /// Converts signed 16 bit raw PCM data from stereo to mono.
    /// @param Buffer The PCM data to convert to mono.
    /// @param Size Size of the data in bytes.
    /// @return The resulting buffer size.
    unsigned int ConvertToMono(unsigned char* Buffer, unsigned int Size);


    private:

    // Don't allow use of copy and assignment constructor.
    BufferT(BufferT&);
    BufferT& operator=(const BufferT&);
};

#endif
