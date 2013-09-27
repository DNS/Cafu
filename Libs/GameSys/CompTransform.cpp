/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2013 Carsten Fuchs Software.

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

#include "CompTransform.hpp"
#include "AllComponents.hpp"

#include "Math3D/Angles.hpp"
#include "UniScriptState.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GameSys;


namespace
{
    const char* FlagsIsQuat[] = { "IsQuat", NULL };
}


const char* ComponentTransformT::DocClass =
    "This component adds information about the position and orientation of the entity.";


const cf::TypeSys::VarsDocT ComponentTransformT::DocVars[] =
{
    { "Origin",      "The origin of the entity (in the coordinate system of its parent)." },
    { "Orientation", "The orientation of the entity (in the coordinate system of its parent)." },
    { NULL, NULL }
};


ComponentTransformT::ComponentTransformT()
    : ComponentBaseT(),
      m_Origin("Origin", Vector3fT(0.0f, 0.0f, 0.0f)),
      m_Quat("Orientation", Vector3fT(0.0f, 0.0f, 0.0f), FlagsIsQuat)
{
    GetMemberVars().Add(&m_Origin);
    GetMemberVars().Add(&m_Quat);
}


ComponentTransformT::ComponentTransformT(const ComponentTransformT& Comp)
    : ComponentBaseT(Comp),
      m_Origin(Comp.m_Origin),
      m_Quat(Comp.m_Quat)
{
    GetMemberVars().Add(&m_Origin);
    GetMemberVars().Add(&m_Quat);
}


ComponentTransformT* ComponentTransformT::Clone() const
{
    return new ComponentTransformT(*this);
}


static const cf::TypeSys::MethsDocT META_GetAngles =
{
    "GetAngles",
    "Returns the orientation of this entity as a tuple of three angles, measured in degrees:\n"
    "  - heading (yaw),\n"
    "  - pitch,\n"
    "  - bank (roll).",
    "tuple", "()"
};

int ComponentTransformT::GetAngles(lua_State* LuaState)
{
    ScriptBinderT                      Binder(LuaState);
    IntrusivePtrT<ComponentTransformT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentTransformT> >(1);
    const cf::math::AnglesfT           Angles(Comp->GetQuat());

    lua_pushnumber(LuaState, Angles.yaw());
    lua_pushnumber(LuaState, Angles.pitch());
    lua_pushnumber(LuaState, Angles.roll());
    return 3;
}


static const cf::TypeSys::MethsDocT META_SetAngles =
{
    "SetAngles",
    "Sets the orientation of this entity from a set of three angles, measured in degrees:\n"
    "  - heading (yaw),\n"
    "  - pitch,\n"
    "  - bank (roll).",
    "", "(number heading, number pitch=0.0, number bank=0.0)"
};

int ComponentTransformT::SetAngles(lua_State* LuaState)
{
    cf::math::AnglesfT                 Angles;
    ScriptBinderT                      Binder(LuaState);
    IntrusivePtrT<ComponentTransformT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentTransformT> >(1);

    // Note that only the heading is a mandatory parameter. Pitch and bank are optional, they default to 0.
    Angles.yaw()   = float(luaL_checknumber(LuaState, 2));
    Angles.pitch() = float(lua_tonumber(LuaState, 3));
    Angles.roll()  = float(lua_tonumber(LuaState, 4));

    Comp->SetQuat(cf::math::QuaternionfT(Angles));
    return 0;
}


static const cf::TypeSys::MethsDocT META_GetAxisX =
{
    "GetAxisX",
    "Returns the x-axis of the local coordinate system of this entity.\n"
    "The local coordinate system expresses the orientation of the entity.\n"
    "It is relative to the entity's parent.",
    "tuple", "()"
};

int ComponentTransformT::GetAxisX(lua_State* LuaState)
{
    ScriptBinderT                      Binder(LuaState);
    IntrusivePtrT<ComponentTransformT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentTransformT> >(1);
    const cf::math::QuaternionfT       Quat = Comp->GetQuat();

    // This is the same code as in the related Matrix3x3T(Quat) constructor.
    lua_pushnumber(LuaState, 1 - 2 * Quat.y * Quat.y - 2 * Quat.z * Quat.z);
    lua_pushnumber(LuaState,     2 * Quat.x * Quat.y + 2 * Quat.w * Quat.z);
    lua_pushnumber(LuaState,     2 * Quat.x * Quat.z - 2 * Quat.w * Quat.y);
    return 3;
}


static const cf::TypeSys::MethsDocT META_GetAxisY =
{
    "GetAxisY",
    "Returns the y-axis of the local coordinate system of this entity.\n"
    "The local coordinate system expresses the orientation of the entity.\n"
    "It is relative to the entity's parent.",
    "tuple", "()"
};

int ComponentTransformT::GetAxisY(lua_State* LuaState)
{
    ScriptBinderT                      Binder(LuaState);
    IntrusivePtrT<ComponentTransformT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentTransformT> >(1);
    const cf::math::QuaternionfT       Quat = Comp->GetQuat();

    // This is the same code as in the related Matrix3x3T(Quat) constructor.
    lua_pushnumber(LuaState,     2 * Quat.x * Quat.y - 2 * Quat.w * Quat.z);
    lua_pushnumber(LuaState, 1 - 2 * Quat.x * Quat.x - 2 * Quat.z * Quat.z);
    lua_pushnumber(LuaState,     2 * Quat.y * Quat.z + 2 * Quat.w * Quat.x);
    return 3;
}


static const cf::TypeSys::MethsDocT META_GetAxisZ =
{
    "GetAxisZ",
    "Returns the z-axis of the local coordinate system of this entity.\n"
    "The local coordinate system expresses the orientation of the entity.\n"
    "It is relative to the entity's parent.",
    "tuple", "()"
};

int ComponentTransformT::GetAxisZ(lua_State* LuaState)
{
    ScriptBinderT                      Binder(LuaState);
    IntrusivePtrT<ComponentTransformT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentTransformT> >(1);
    const cf::math::QuaternionfT       Quat = Comp->GetQuat();

    // This is the same code as in the related Matrix3x3T(Quat) constructor.
    lua_pushnumber(LuaState,     2 * Quat.x * Quat.z + 2 * Quat.w * Quat.y);
    lua_pushnumber(LuaState,     2 * Quat.y * Quat.z - 2 * Quat.w * Quat.x);
    lua_pushnumber(LuaState, 1 - 2 * Quat.x * Quat.x - 2 * Quat.y * Quat.y);
    return 3;
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__toString",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int ComponentTransformT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "transform component");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentTransformT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentTransformT();
}

const luaL_reg ComponentTransformT::MethodsList[] =
{
    { "GetAngles",  ComponentTransformT::GetAngles },
    { "SetAngles",  ComponentTransformT::SetAngles },
    { "GetAxisX",   ComponentTransformT::GetAxisX },
    { "GetAxisY",   ComponentTransformT::GetAxisY },
    { "GetAxisZ",   ComponentTransformT::GetAxisZ },
    { "__tostring", ComponentTransformT::toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentTransformT::DocMethods[] =
{
    META_GetAngles,
    META_SetAngles,
    META_GetAxisX,
    META_GetAxisY,
    META_GetAxisZ,
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentTransformT::TypeInfo(GetComponentTIM(), "ComponentTransformT", "ComponentBaseT", ComponentTransformT::CreateInstance, MethodsList, DocClass, DocMethods, DocVars);
