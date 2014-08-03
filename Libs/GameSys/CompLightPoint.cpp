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

#include "CompLightPoint.hpp"
#include "AllComponents.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GameSys;


/********************************************/
/*** ComponentPointLightT::VarShadowTypeT ***/
/********************************************/

ComponentPointLightT::VarShadowTypeT::VarShadowTypeT(const char* Name, const int& Value, const char* Flags[])
    : TypeSys::VarT<int>(Name, Value, Flags)
{
}


void ComponentPointLightT::VarShadowTypeT::GetChoices(ArrayT<std::string>& Strings, ArrayT<int>& Values) const
{
    Strings.PushBack("none");           Values.PushBack(NONE);
    Strings.PushBack("Stencil (hard)"); Values.PushBack(STENCIL);
}


/****************************/
/*** ComponentPointLightT ***/
/****************************/

const char* ComponentPointLightT::DocClass =
    "This component adds a dynamic point light source to its entity.";


const cf::TypeSys::VarsDocT ComponentPointLightT::DocVars[] =
{
    { "On",         "Switch the light source on or off." },
    { "Color",      "The color of the emitted light." },
    { "Radius",     "The distance up to which the light may be perceived." },
    { "ShadowType", "The type of the shadows that are cast by this light source." },
    { NULL, NULL }
};


namespace
{
    const char* FlagsIsColor[] = { "IsColor", NULL };
}


ComponentPointLightT::ComponentPointLightT()
    : ComponentLightT(),
      m_On("On", true),
      m_Color("Color", Vector3fT(1.0f, 0.95f, 0.8f), FlagsIsColor),
      m_Radius("Radius", 128.0f),
      m_ShadowType("ShadowType", VarShadowTypeT::NONE),
      m_UseClientEffects(false),
      m_ClientColor(m_Color.Get()),
      m_ClientRadius(m_Radius.Get())
{
    GetMemberVars().Add(&m_On);
    GetMemberVars().Add(&m_Color);
    GetMemberVars().Add(&m_Radius);
    GetMemberVars().Add(&m_ShadowType);
}


ComponentPointLightT::ComponentPointLightT(const ComponentPointLightT& Comp)
    : ComponentLightT(Comp),
      m_On(Comp.m_On),
      m_Color(Comp.m_Color),
      m_Radius(Comp.m_Radius),
      m_ShadowType(Comp.m_ShadowType),
      m_UseClientEffects(false),
      m_ClientColor(m_Color.Get()),
      m_ClientRadius(m_Radius.Get())
{
    GetMemberVars().Add(&m_On);
    GetMemberVars().Add(&m_Color);
    GetMemberVars().Add(&m_Radius);
    GetMemberVars().Add(&m_ShadowType);
}


ComponentPointLightT* ComponentPointLightT::Clone() const
{
    return new ComponentPointLightT(*this);
}


BoundingBox3fT ComponentPointLightT::GetCullingBB() const
{
    const float r = m_Radius.Get();

    // A light source is typically not seen by itself, but its "effects" are.
    return BoundingBox3fT(Vector3fT(-r, -r, -r), Vector3fT(r, r, r));
}


void ComponentPointLightT::DoClientFrame(float t)
{
    bool Result = false;

    const bool HaveClEff = CallLuaMethod("ClientEffect", 0, "f>bffff", t,
        &Result, &m_ClientColor.x, &m_ClientColor.y, &m_ClientColor.z, &m_ClientRadius);

    m_UseClientEffects = HaveClEff && Result;
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int ComponentPointLightT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "point light component");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentPointLightT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentPointLightT();
}

const luaL_Reg ComponentPointLightT::MethodsList[] =
{
    { "__tostring", toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentPointLightT::DocMethods[] =
{
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentPointLightT::TypeInfo(GetComponentTIM(), "GameSys::ComponentPointLightT", "GameSys::ComponentLightT", ComponentPointLightT::CreateInstance, MethodsList, DocClass, DocMethods, DocVars);
