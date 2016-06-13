/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
      m_ShadowType("ShadowType", VarShadowTypeT::NONE)
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
      m_ShadowType(Comp.m_ShadowType)
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

const cf::TypeSys::TypeInfoT ComponentPointLightT::TypeInfo(GetComponentTIM(), "GameSys::ComponentPointLightT", "GameSys::ComponentLightT", ComponentPointLightT::CreateInstance, MethodsList, DocClass, DocMethods, NULL, DocVars);
