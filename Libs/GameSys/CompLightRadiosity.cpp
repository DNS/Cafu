/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2014 Carsten Fuchs Software.

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

#include "CompLightRadiosity.hpp"
#include "AllComponents.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GameSys;


const char* ComponentRadiosityLightT::DocClass =
    "This component adds a radiosity point light source to its entity.\n"
    "Radiosity lights are preprocessed and their effects are baked into lightmaps.\n"
    "Consequently, radiosity lights are static, and script-updating them at run-time has no effect.\n"
    "However, their results look very natural and their performance at run-time is very good.";


const cf::TypeSys::VarsDocT ComponentRadiosityLightT::DocVars[] =
{
    { "Color",     "The color of the emitted light." },
    { "Intensity", "The intensity (per color-component) of the light source in watt per steradian (W/sr)." },
    { "ConeAngle", "The size of the cone in which light is emitted (in degrees; use 360.0 for a spherical light)." },
    { NULL, NULL }
};


namespace
{
    const char* FlagsIsColor[] = { "IsColor", NULL };
}


ComponentRadiosityLightT::ComponentRadiosityLightT()
    : ComponentLightT(),
      m_Color("Color", Vector3fT(1.0f, 0.95f, 0.8f), FlagsIsColor),
      m_Intensity("Intensity", 100.0f),
      m_ConeAngle("ConeAngle", 360.0f)
{
    GetMemberVars().Add(&m_Color);
    GetMemberVars().Add(&m_Intensity);
    GetMemberVars().Add(&m_ConeAngle);
}


ComponentRadiosityLightT::ComponentRadiosityLightT(const ComponentRadiosityLightT& Comp)
    : ComponentLightT(Comp),
      m_Color(Comp.m_Color),
      m_Intensity(Comp.m_Intensity),
      m_ConeAngle(Comp.m_ConeAngle)
{
    GetMemberVars().Add(&m_Color);
    GetMemberVars().Add(&m_Intensity);
    GetMemberVars().Add(&m_ConeAngle);
}


ComponentRadiosityLightT* ComponentRadiosityLightT::Clone() const
{
    return new ComponentRadiosityLightT(*this);
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int ComponentRadiosityLightT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "radiosity light component");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentRadiosityLightT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentRadiosityLightT();
}

const luaL_Reg ComponentRadiosityLightT::MethodsList[] =
{
    { "__tostring", toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentRadiosityLightT::DocMethods[] =
{
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentRadiosityLightT::TypeInfo(GetComponentTIM(), "GameSys::ComponentRadiosityLightT", "GameSys::ComponentLightT", ComponentRadiosityLightT::CreateInstance, MethodsList, DocClass, DocMethods, DocVars);
