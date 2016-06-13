/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "StaticBuffer.hpp"

#include "fmod.h"           // FMOD
#ifdef _WIN32
#include "fmod_errors.h"    // FMOD Error Messages
#endif

#include <iostream>


static const unsigned int InitBufferSize=65536;


StaticBufferT::StaticBufferT(const std::string& AudioFile, bool Is3DSound)
    : BufferT(AudioFile),
      m_Sample(FSOUND_Sample_Load(FSOUND_UNMANAGED, AudioFile.c_str(), Is3DSound ? FSOUND_HW3D | FSOUND_FORCEMONO : FSOUND_HW2D, 0, 0))
{
    if (m_Sample==NULL) std::cout << "FMOD3: Error creating sample from '" << AudioFile << "'\n";
}


StaticBufferT::~StaticBufferT()
{
    assert(References==0);

    if (m_Sample==NULL) return;

    FSOUND_Sample_Free(m_Sample);
}


int StaticBufferT::AttachToChannel(unsigned int Priority)
{
    if (m_Sample==NULL) return -1;

    // First we need to set the priority of our sample to the one passed to this method.
    // This ensures that FMOD will automatically kick a channel with lower priority.
    FSOUND_Sample_SetDefaults(m_Sample, -1, -1, -1, Priority>255 ? 255 : Priority); // FMOD only supports priorities between 0 and 255.

    return FSOUND_PlaySoundEx(FSOUND_FREE, m_Sample, NULL, true);
}
