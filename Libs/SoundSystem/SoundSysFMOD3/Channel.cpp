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

    float Pos[3]={ float(Sound->Position.x/1000.0),
                   float(Sound->Position.z/1000.0),
                   float(Sound->Position.y/1000.0) };

    float Vel[3]={ float(Sound->Velocity.x/1000.0),
                   float(Sound->Velocity.z/1000.0),
                   float(Sound->Velocity.y/1000.0) };

    // Note that direction and sound cones are ignored because they are not supported by FMOD.

    if (!FSOUND_3D_SetAttributes(ChannelHandle, Pos, Vel))
        std::cout << "FMOD3: Error setting channel position and velocity\n";

    if (!FSOUND_3D_SetMinMaxDistance(ChannelHandle, Sound->MinDistance, Sound->MaxDistance))
        std::cout << "FMOD3: Error setting channel min/max distance\n";
}
