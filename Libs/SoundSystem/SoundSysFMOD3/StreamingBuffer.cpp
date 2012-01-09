/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#include "fmod.h"           // FMOD
#ifdef _WIN32
#include "fmod_errors.h"    // FMOD Error Messages
#endif

#include <iostream>


StreamingBufferT::StreamingBufferT(const std::string& AudioFile, bool Is3DSound)
    : BufferT(AudioFile),
      m_Stream(FSOUND_Stream_Open(AudioFile.c_str(), Is3DSound ? FSOUND_HW3D | FSOUND_FORCEMONO : FSOUND_HW2D, 0, 0))
{
    if (m_Stream==NULL) std::cout << "FMOD3: Error creating stream from '" << AudioFile << "'\n";
}


StreamingBufferT::~StreamingBufferT()
{
    if (m_Stream==NULL) return;

    FSOUND_Stream_Close(m_Stream);
}


int StreamingBufferT::AttachToChannel(unsigned int Priority)
{
    if (m_Stream==NULL) return -1;

    // Note: Streams have always a priority of 256 in FMOD and can never be rejected.
    // The only thing we can do is set the custom priority after a channel has been set.
    int ChannelHandle=FSOUND_Stream_PlayEx(FSOUND_FREE, m_Stream, NULL, true);

    if(FSOUND_SetPriority(ChannelHandle, Priority)==false)
       std::cout << "FMOD3: [Warning] Couldn't set channel priority for '" << FileName << "'\n";

    return ChannelHandle;
}


void StreamingBufferT::Rewind()
{
    FSOUND_Stream_SetPosition(m_Stream, 0);
}
