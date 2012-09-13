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

#ifndef CAFU_POINTLIGHTSOURCE_HPP_INCLUDED
#define CAFU_POINTLIGHTSOURCE_HPP_INCLUDED

#include "BaseEntity.hpp"


class EntityCreateParamsT;
struct luaL_Reg;


class EntPointLightSourceT : public BaseEntityT
{
    public:

    EntPointLightSourceT(const EntityCreateParamsT& Params);

    bool GetLightSourceInfo(unsigned long& DiffuseColor, unsigned long& SpecularColor, VectorT& Position, float& Radius, bool& CastsShadows) const;


    const cf::TypeSys::TypeInfoT* GetType() const;
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

    void DoSerialize(cf::Network::OutStreamT& Stream) const;    // Override the BaseEntityT base class method.
    void DoDeserialize(cf::Network::InStreamT& Stream);         // Override the BaseEntityT base class method.


    float    m_Radius;
    uint32_t m_DiffuseColor;
    uint32_t m_SpecularColor;
    bool     m_CastsShadows;


    // Script methods (to be called from the map/entity Lua scripts).
    static int GetColor(lua_State* LuaState);
    static int SetColor(lua_State* LuaState);
    static int GetRadius(lua_State* LuaState);
    static int SetRadius(lua_State* LuaState);

    static const luaL_Reg MethodsList[];
};

#endif
