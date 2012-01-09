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

/*******************************/
/*** Global Resource Manager ***/
/*******************************/

#include <stdio.h>

#include "_ResourceManager.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Renderer.hpp"

#if defined(_WIN32) && defined (_MSC_VER)
    #if (_MSC_VER<1300)
        #define for if (false) ; else for
    #endif
#endif


ResourceManagerT ResMan;


void ResourceManagerT::Init()
{
    char ParticleName[256];

    PARTICLE_GENERIC1=RenderMats.Size();
    RenderMats.PushBack(MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial("Sprites/Generic1")));

    // These particles are used as animated particles, but note that they are treated like normal, individual, non-animated particles.
    PARTICLE_EXPLOSION1_FRAME1=RenderMats.Size();
    for (unsigned long FrameNr=0; FrameNr<26; FrameNr++)
    { 
        sprintf(ParticleName, "Sprites/expl1/expl_%02lu", FrameNr+1);
        RenderMats.PushBack(MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial(ParticleName)));
    }

    PARTICLE_EXPLOSIONVERT_FRAME1=RenderMats.Size();
    for (unsigned long FrameNr=0; FrameNr<55; FrameNr++)
    {
        sprintf(ParticleName, "Sprites/expl4/expl_%02lu", FrameNr+1);
        RenderMats.PushBack(MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial(ParticleName)));
    }

    PARTICLE_EXPLOSION2_FRAME1=RenderMats.Size();
    for (unsigned long FrameNr=0; FrameNr<27; FrameNr++)
    {
        sprintf(ParticleName, "Sprites/expl3/expl_%02lu", FrameNr+1);
        RenderMats.PushBack(MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial(ParticleName)));
    }

    PARTICLE_WHITESMOKE_FRAME1=RenderMats.Size();
    for (unsigned long FrameNr=0; FrameNr<32; FrameNr++)
    {
        sprintf(ParticleName, "Sprites/smoke1/smoke_%02lu", FrameNr+1);
        RenderMats.PushBack(MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial(ParticleName)));
    }
}


void ResourceManagerT::ShutDown()
{
    for (unsigned long RMNr=0; RMNr<RenderMats.Size(); RMNr++)
        MatSys::Renderer->FreeMaterial(RenderMats[RMNr]);

    RenderMats.Clear();
}
