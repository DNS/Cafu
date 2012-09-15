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

#include "PointLightSource.hpp"
#include "EntityCreateParams.hpp"
#include "Network/State.hpp"
#include "TypeSys.hpp"
#include "UniScriptState.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace GAME_NAME;


// Implement the type info related code.
const cf::TypeSys::TypeInfoT* EntPointLightSourceT::GetType() const
{
    return &TypeInfo;
 // return &EntPointLightSourceT::TypeInfo;
}

void* EntPointLightSourceT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new EntPointLightSourceT(*static_cast<const EntityCreateParamsT*>(&Params));
}

const luaL_Reg EntPointLightSourceT::MethodsList[]=
{
    { "GetColor",  EntPointLightSourceT::GetColor },
    { "SetColor",  EntPointLightSourceT::SetColor },
    { "GetRadius", EntPointLightSourceT::GetRadius },
    { "SetRadius", EntPointLightSourceT::SetRadius },
 // { "__tostring", toString },
    { NULL, NULL }
};

const cf::TypeSys::TypeInfoT EntPointLightSourceT::TypeInfo(GetBaseEntTIM(), "EntPointLightSourceT", "BaseEntityT", EntPointLightSourceT::CreateInstance, MethodsList);


EntPointLightSourceT::EntPointLightSourceT(const EntityCreateParamsT& Params)
    : BaseEntityT(Params,
                  BoundingBox3dT(Vector3dT(), Vector3dT()),
                  0),
      m_Radius(0.0f),
      m_DiffuseColor(0),
      m_SpecularColor(0),
      m_CastsShadows(true)
{
    // Werte die 'PropertyPairs' aus, die von der Basis-Klasse 'BaseEntity' noch nicht ausgewertet wurden!
    for (std::map<std::string, std::string>::const_iterator It=Properties.begin(); It!=Properties.end(); ++It)
    {
        const std::string& Key  =It->first;
        const std::string& Value=It->second;

        if (Key=="light_radius")
        {
            m_Radius=float(atof(Value.c_str()));
        }
        else if (Key=="light_color_diff")
        {
            unsigned long      r, g, b;
            std::istringstream iss(Value);

            iss >> r >> g >> b;

            m_DiffuseColor+= r;
            m_DiffuseColor+=(g <<  8);
            m_DiffuseColor+=(b << 16);
        }
        else if (Key=="light_color_spec")
        {
            unsigned long      r, g, b;
            std::istringstream iss(Value);

            iss >> r >> g >> b;

            m_SpecularColor+= r;
            m_SpecularColor+=(g <<  8);
            m_SpecularColor+=(b << 16);
        }
        else if (Key=="light_casts_shadows")
        {
            m_CastsShadows=atoi(Value.c_str())!=0;
        }
    }

    // printf("Instantiated an EntPointLightSourceT at %f %f %f, r %f, %lu %lu!\n", m_Origin.x, m_Origin.y, m_Origin.z, m_Radius, m_DiffuseColor, m_SpecularColor);
}


void EntPointLightSourceT::DoSerialize(cf::Network::OutStreamT& Stream) const
{
    Stream << m_Radius;
    Stream << m_DiffuseColor;
    Stream << m_SpecularColor;
    Stream << m_CastsShadows;
}


void EntPointLightSourceT::DoDeserialize(cf::Network::InStreamT& Stream)
{
    Stream >> m_Radius;
    Stream >> m_DiffuseColor;
    Stream >> m_SpecularColor;
    Stream >> m_CastsShadows;
}


bool EntPointLightSourceT::GetLightSourceInfo(unsigned long& DiffuseColor, unsigned long& SpecularColor, VectorT& Position, float& Radius, bool& CastsShadows) const
{
    if (m_DiffuseColor==0 && m_SpecularColor==0) return false;

    Position     =m_Origin;
    Radius       =m_Radius;
    DiffuseColor =m_DiffuseColor;
    SpecularColor=m_SpecularColor;
    CastsShadows =m_CastsShadows;

    return true;
}


int EntPointLightSourceT::GetColor(lua_State* LuaState)
{
    cf::ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntPointLightSourceT> Ent=Binder.GetCheckedObjectParam< IntrusivePtrT<EntPointLightSourceT> >(1);

    lua_pushnumber(LuaState, (Ent->m_DiffuseColor >>  0) & 0xFF);
    lua_pushnumber(LuaState, (Ent->m_DiffuseColor >>  8) & 0xFF);
    lua_pushnumber(LuaState, (Ent->m_DiffuseColor >> 16) & 0xFF);

    lua_pushnumber(LuaState, (Ent->m_SpecularColor >>  0) & 0xFF);
    lua_pushnumber(LuaState, (Ent->m_SpecularColor >>  8) & 0xFF);
    lua_pushnumber(LuaState, (Ent->m_SpecularColor >> 16) & 0xFF);

    return 6;
}


int EntPointLightSourceT::SetColor(lua_State* LuaState)
{
    cf::ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntPointLightSourceT> Ent=Binder.GetCheckedObjectParam< IntrusivePtrT<EntPointLightSourceT> >(1);

    const unsigned long dr=((unsigned long)luaL_checknumber(LuaState, 2)) & 0xFF;
    const unsigned long dg=((unsigned long)luaL_checknumber(LuaState, 3)) & 0xFF;
    const unsigned long db=((unsigned long)luaL_checknumber(LuaState, 4)) & 0xFF;

    Ent->m_DiffuseColor =(db << 16) + (dg << 8) + (dr << 0);
    Ent->m_SpecularColor=Ent->m_DiffuseColor;

    if (lua_gettop(LuaState)==7)
    {
        const unsigned long sr=((unsigned long)luaL_checknumber(LuaState, 5)) & 0xFF;
        const unsigned long sg=((unsigned long)luaL_checknumber(LuaState, 6)) & 0xFF;
        const unsigned long sb=((unsigned long)luaL_checknumber(LuaState, 7)) & 0xFF;

        Ent->m_SpecularColor=(sb << 16) + (sg << 8) + (sr << 0);
    }

    return 0;
}


int EntPointLightSourceT::GetRadius(lua_State* LuaState)
{
    cf::ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntPointLightSourceT> Ent=Binder.GetCheckedObjectParam< IntrusivePtrT<EntPointLightSourceT> >(1);

    lua_pushnumber(LuaState, Ent->m_Radius);

    return 1;
}


int EntPointLightSourceT::SetRadius(lua_State* LuaState)
{
    cf::ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntPointLightSourceT> Ent=Binder.GetCheckedObjectParam< IntrusivePtrT<EntPointLightSourceT> >(1);

    Ent->m_Radius=float(luaL_checknumber(LuaState, 2));

    return 0;
}
