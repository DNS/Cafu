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

/******************************/
/*** Monster Maker (Header) ***/
/******************************/

#ifndef _MONSTERMAKER_HPP_
#define _MONSTERMAKER_HPP_

#include "../../BaseEntity.hpp"


class EntityCreateParamsT;


class EntMonsterMakerT : public BaseEntityT
{
    private:

    enum MonsterTypeT { Unknown, CompanyBot, Butterfly, Eagle };

    MonsterTypeT  MonsterType;          // Type of monsters to create
    unsigned long MaxCreate;            // Number of monsters the MonsterMaker can totally create
    float         Delay;                // How often a new monster will be dookied out
    unsigned long MaxAlive;             // Maximum number of live children allowed at one time (new ones will not be made until one dies)
    unsigned long CurrentlyAlive;       // Number of currently alive children
    float         TimeSinceLastMake;    // Time since the last child was made


    public:

    EntMonsterMakerT(const EntityCreateParamsT& Params);

    void Think(float FrameTime, unsigned long ServerFrameNr);


    const cf::TypeSys::TypeInfoT* GetType() const;
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;
};

#endif
