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

/*****************************/
/*** Hand Grenade (Header) ***/
/*****************************/

#ifndef _HANDGRENADE_HPP_
#define _HANDGRENADE_HPP_

#include "../../BaseEntity.hpp"


class EntityCreateParamsT;
class SoundI;


class EntHandGrenadeT : public BaseEntityT
{
    public:

    EntHandGrenadeT(const EntityCreateParamsT& Params);
    ~EntHandGrenadeT();

    void Think(float FrameTime, unsigned long ServerFrameNr);

    void ProcessEvent(char EventID);
    bool GetLightSourceInfo(unsigned long& DiffuseColor, unsigned long& SpecularColor, VectorT& Position, float& Radius, bool& CastsShadows) const;
    void Draw(bool FirstPersonView, float LodDist) const;


    const cf::TypeSys::TypeInfoT* GetType() const;
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

    SoundI* FireSound;
};

#endif
