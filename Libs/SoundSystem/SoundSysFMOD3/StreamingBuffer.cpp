/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
