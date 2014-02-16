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
#include "Entity.hpp"

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
    "This component adds information about the position and orientation of its entity.\n"
    "Positions and orientations can be measured relative to several distinct spaces:\n"
    "\n"
    "world-space\n"
    "  : The global and \"absolute\" coordinate space that also exists when nothing else does.\n"
    "\n"
    "entity-space\n"
    "  : The local coordinate system of the entity. It is defined by the entity's transform component relative\n"
    "    to the entity's parent-space. The term \"model-space\" can be used synonymously with \"entity-space\".\n"
    "\n"
    "parent-space\n"
    "  : The entity-space of an entity's parent.\n"
    "    If an entity has no parent entity, this is the same as world-space.\n"
    "\n"
    "Although transform components can theoretically and technically exist without being attached to an entity,\n"
    "in practice this distinction is not made. Every entity has exactly one built-in transform component, and\n"
    "terms like \"the origin of the transform\" and \"the origin of the entity\" are used synonymously.";


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


Vector3fT ComponentTransformT::GetOriginWS() const
{
    IntrusivePtrT<const EntityT> Ent = GetEntity();

    if (Ent == NULL)
        return m_Origin.Get();

    Ent = Ent->GetParent();

    if (Ent == NULL)
        return m_Origin.Get();

    MatrixT Mat(Ent->GetTransform()->GetOriginPS(), Ent->GetTransform()->GetQuatPS());

    for (Ent = Ent->GetParent(); Ent != NULL; Ent = Ent->GetParent())
        if (!Ent->GetTransform()->IsIdentity())
            Mat = MatrixT(Ent->GetTransform()->GetOriginPS(), Ent->GetTransform()->GetQuatPS()) * Mat;

    // Mat now transforms from parent-space to world-space, so multiply m_Origin with it.
    return Mat.Mul1(m_Origin.Get());
}


void ComponentTransformT::SetOriginWS(const Vector3fT& OriginWS)
{
    // This is largely analogous to GetOriginWS().
    IntrusivePtrT<const EntityT> Ent = GetEntity();

    if (Ent == NULL)
    {
        m_Origin.Set(OriginWS);
        return;
    }

    Ent = Ent->GetParent();

    if (Ent == NULL)
    {
        m_Origin.Set(OriginWS);
        return;
    }

    MatrixT Mat(Ent->GetTransform()->GetOriginPS(), Ent->GetTransform()->GetQuatPS());

    for (Ent = Ent->GetParent(); Ent != NULL; Ent = Ent->GetParent())
        if (!Ent->GetTransform()->IsIdentity())
            Mat = MatrixT(Ent->GetTransform()->GetOriginPS(), Ent->GetTransform()->GetQuatPS()) * Mat;

    // The inverse of Mat now transforms from world-space to parent-space, so multiply OriginWS with it.
    m_Origin.Set(Mat.InvXForm(OriginWS));
}


const cf::math::QuaternionfT ComponentTransformT::GetQuatWS() const
{
    cf::math::QuaternionfT Quat = GetQuatPS();

    if (GetEntity() == NULL)
        return Quat;

    for (IntrusivePtrT<const EntityT> P = GetEntity()->GetParent(); P != NULL; P = P->GetParent())
        if (P->GetTransform()->m_Quat.Get() != Vector3fT())
            Quat = P->GetTransform()->GetQuatPS() * Quat;

    // Quat describes the rotation from entity-space to world-space.
    return Quat;
}


void ComponentTransformT::SetQuatWS(const cf::math::QuaternionfT& QuatWS)
{
    IntrusivePtrT<const EntityT> Ent = GetEntity();

    if (Ent == NULL)
    {
        m_Quat.Set(QuatWS.GetXYZ());
        return;
    }

    Ent = Ent->GetParent();

    if (Ent == NULL)
    {
        m_Quat.Set(QuatWS.GetXYZ());
        return;
    }

    cf::math::QuaternionfT Quat = Ent->GetTransform()->GetQuatPS();

    for (Ent = Ent->GetParent(); Ent != NULL; Ent = Ent->GetParent())
        if (Ent->GetTransform()->m_Quat.Get() != Vector3fT())
            Quat = Ent->GetTransform()->GetQuatPS() * Quat;

    // Quat describes the rotation from parent-space to world-space, that is:
    //     QuatWS = Quat * GetQuatPS()
    // As we have to find "GetQuatPS()", left-multiply both sides with the inverse of Quat.
    m_Quat.Set((Quat.GetConjugate() * QuatWS).GetXYZ());
}


MatrixT ComponentTransformT::GetEntityToWorld() const
{
    MatrixT ModelToWorld(GetOriginPS(), GetQuatPS());

    if (!GetEntity())
        return ModelToWorld;

    for (IntrusivePtrT<const EntityT> P = GetEntity()->GetParent(); P != NULL; P = P->GetParent())
        if (!P->GetTransform()->IsIdentity())
            ModelToWorld = MatrixT(P->GetTransform()->GetOriginPS(), P->GetTransform()->GetQuatPS()) * ModelToWorld;

    return ModelToWorld;
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
    "  - bank (roll).\n"
    "The angles are relative to the coordinate system of the parent entity.",
    "tuple", "()"
};

int ComponentTransformT::GetAngles(lua_State* LuaState)
{
    ScriptBinderT                      Binder(LuaState);
    IntrusivePtrT<ComponentTransformT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentTransformT> >(1);
    const cf::math::AnglesfT           Angles(Comp->GetQuatPS());

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
    "  - bank (roll).\n"
    "The angles are relative to the coordinate system of the parent entity.",
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

    Comp->SetQuatPS(cf::math::QuaternionfT(Angles));
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
    const cf::math::QuaternionfT       Quat = Comp->GetQuatPS();

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
    const cf::math::QuaternionfT       Quat = Comp->GetQuatPS();

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
    const cf::math::QuaternionfT       Quat = Comp->GetQuatPS();

    // This is the same code as in the related Matrix3x3T(Quat) constructor.
    lua_pushnumber(LuaState,     2 * Quat.x * Quat.z + 2 * Quat.w * Quat.y);
    lua_pushnumber(LuaState,     2 * Quat.y * Quat.z - 2 * Quat.w * Quat.x);
    lua_pushnumber(LuaState, 1 - 2 * Quat.x * Quat.x - 2 * Quat.y * Quat.y);
    return 3;
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
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

const luaL_Reg ComponentTransformT::MethodsList[] =
{
    { "GetAngles",  GetAngles },
    { "SetAngles",  SetAngles },
    { "GetAxisX",   GetAxisX },
    { "GetAxisY",   GetAxisY },
    { "GetAxisZ",   GetAxisZ },
    { "__tostring", toString },
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

const cf::TypeSys::TypeInfoT ComponentTransformT::TypeInfo(GetComponentTIM(), "GameSys::ComponentTransformT", "GameSys::ComponentBaseT", ComponentTransformT::CreateInstance, MethodsList, DocClass, DocMethods, DocVars);
