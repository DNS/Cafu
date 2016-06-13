/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_WORLD_MANAGER_HPP_INCLUDED
#define CAFU_WORLD_MANAGER_HPP_INCLUDED

#include "World.hpp"


class WorldManT
{
    public:

    const WorldT* LoadWorld(const char* FileName, ModelManagerT& ModelMan, cf::GuiSys::GuiResourcesT& GuiRes, bool InitForGraphics, WorldT::ProgressFunctionT ProgressFunction=NULL);
    void FreeWorld(const WorldT* World);

    ~WorldManT();


    private:

    struct WorldInfoT
    {
        std::string   FileName;
        unsigned long RefCount;
        bool          Init4Gfx;
        WorldT*       WorldPtr;
    };

    void InitWorldForGfx(WorldInfoT& WI);

    ArrayT<WorldInfoT> Worlds;
};

#endif
