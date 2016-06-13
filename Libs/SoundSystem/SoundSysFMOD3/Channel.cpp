/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Channel.hpp"
#include "SoundImpl.hpp"

#include "../SoundShader.hpp"

#include "fmod.h"           // FMOD
#ifdef _WIN32
#include "fmod_errors.h"    // FMOD Error Messages
#endif

#include <iostream>


ChannelT::ChannelT(int ChannelHandle_, const SoundImplT* Sound_)
    : ChannelHandle(ChannelHandle_),
      Sound(Sound_)
{
}


void ChannelT::Update()
{
    if (ChannelHandle==-1 || Sound==NULL) return;

    // Translate volume.
    unsigned int Volume=(unsigned int)(Sound->Volume*255);

    if(!FSOUND_SetVolume(ChannelHandle, Volume>255 ? 255 : Volume))
        std::cout << "FMOD3: Error setting channel volume\n";

    // Only make 3D adjustments if the sound really is a 3D sound.
    if (!Sound->Is3D()) return;

    // Note that y and z coordinate are swapped because FMOD uses another coordinate system than Cafu.
    const double METERS_PER_WORLD_UNIT = 0.0254;

    float Pos[3]={ float(Sound->Position.x * METERS_PER_WORLD_UNIT),
                   float(Sound->Position.z * METERS_PER_WORLD_UNIT),
                   float(Sound->Position.y * METERS_PER_WORLD_UNIT) };

    float Vel[3]={ float(Sound->Velocity.x * METERS_PER_WORLD_UNIT),
                   float(Sound->Velocity.z * METERS_PER_WORLD_UNIT),
                   float(Sound->Velocity.y * METERS_PER_WORLD_UNIT) };

    // Note that direction and sound cones are ignored because they are not supported by FMOD.

    if (!FSOUND_3D_SetAttributes(ChannelHandle, Pos, Vel))
        std::cout << "FMOD3: Error setting channel position and velocity\n";

    if (!FSOUND_3D_SetMinMaxDistance(ChannelHandle, Sound->MinDistance, Sound->MaxDistance))
        std::cout << "FMOD3: Error setting channel min/max distance\n";
}
