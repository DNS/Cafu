/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#include "WorldMan.hpp"
#include "ConsoleCommands/ConVar.hpp"


const WorldT* WorldManT::LoadWorld(const char* FileName, bool InitForGraphics, WorldT::ProgressFunctionT ProgressFunction)
{
    for (unsigned long WorldNr=0; WorldNr<Worlds.Size(); WorldNr++)
    {
        WorldInfoT& WI=Worlds[WorldNr];

        if (WI.FileName==FileName)
        {
            assert(WI.RefCount>0);
            WI.RefCount++;
            if (InitForGraphics && !WI.Init4Gfx) InitWorldForGfx(WI);
            return WI.WorldPtr;
        }
    }


    // FileName not found in cache, create a new instance.
    WorldT* NewWorld=new WorldT(FileName, ProgressFunction);    // Must do this here in case WorldT::WorldT() throws.

    Worlds.PushBackEmpty();
    WorldInfoT& WI=Worlds[Worlds.Size()-1];

    WI.FileName=FileName;
    WI.RefCount=1;
    WI.Init4Gfx=false;
    WI.WorldPtr=NewWorld;

    if (InitForGraphics) InitWorldForGfx(WI);

    return WI.WorldPtr;
}


void WorldManT::FreeWorld(const WorldT* World)
{
    for (unsigned long WorldNr=0; WorldNr<Worlds.Size(); WorldNr++)
    {
        WorldInfoT& WI=Worlds[WorldNr];

        if (WI.WorldPtr==World)
        {
            assert(WI.RefCount>0);
            WI.RefCount--;

            if (WI.RefCount==0)
            {
                delete WI.WorldPtr;
                Worlds.RemoveAt(WorldNr);
            }

            return;
        }
    }

    assert(false);
}


WorldManT::~WorldManT()
{
    // Did we really pair each call to LoadWorld() with a call to FreeWorld()?
    assert(Worlds.Size()==0);
}


void WorldManT::InitWorldForGfx(WorldInfoT& WI)
{
    // Assert that this is done only once for any world instance.
    assert(!WI.Init4Gfx);

#if SHL_ENABLED
    WI.WorldPtr->SHLMapMan.InitTextures();
#endif

    static ConVarT r_gamma_lm("cl_gamma_lm", 1.0, ConVarT::FLAG_MAIN_EXE, "The gamma correction value that is applied to lightmaps on the next map load.");

    static ConVarT r_lm_ambient_r("cl_lm_ambient_r", 0, ConVarT::FLAG_MAIN_EXE, "The red component of the lightmaps ambient (==minimum) color.");
    static ConVarT r_lm_ambient_g("cl_lm_ambient_g", 0, ConVarT::FLAG_MAIN_EXE, "The green component of the lightmaps ambient (==minimum) color.");
    static ConVarT r_lm_ambient_b("cl_lm_ambient_b", 0, ConVarT::FLAG_MAIN_EXE, "The blue component of the lightmaps ambient (==minimum) color.");

    assert(r_gamma_lm.GetType()==ConVarT::Double);
    assert(r_lm_ambient_r.GetType()==ConVarT::Integer);

    WI.WorldPtr->LightMapMan.InitTextures(float(r_gamma_lm.GetValueDouble()), r_lm_ambient_r.GetValueInt(), r_lm_ambient_g.GetValueInt(), r_lm_ambient_b.GetValueInt());

    // Note that this world has been inited for graphics already.
    WI.Init4Gfx=true;
}
