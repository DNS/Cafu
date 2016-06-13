/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "CompTransform.hpp"
#include "AllComponents.hpp"
#include "Entity.hpp"

#include "Math3D/Angles.hpp"
#include "Math3D/Matrix3x3.hpp"
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


void ComponentTransformT::LookAt(const Vector3fT& Pos, unsigned int AxisNr, bool NoPitch)
{
#if 1
    if (AxisNr > 1) AxisNr = 0;             // AxisNr can only be 0 or 1.

    Vector3fT Dir = Pos - m_Origin.Get();   // Pos is in parent-space already.

    if (NoPitch)
        Dir.z = 0.0f;

    const float DirL = length(Dir);

    if (DirL < 0.001f) return;
    Dir /= DirL;

    // Choose Other orthogonal to Dir and in the XY plane.
    Vector3fT   Other(-Dir.y, Dir.x, 0.0f);
    const float OtherL = length(Other);

    if (OtherL < 0.001f)
    {
        Dir   = Vector3fT(0, 0, Dir.z < 0.0f ? -1.0f : 1.0f);
        Other = Vector3fT(0, 1.0f, 0);
    }
    else
    {
        Other /= OtherL;
    }

    cf::math::Matrix3x3fT Mat;

    for (unsigned int i = 0; i < 3; i++)
    {
        Mat[i][AxisNr]     = Dir[i];
        Mat[i][1 - AxisNr] = Other[i];
    }

    if (!NoPitch)
    {
        const Vector3fT zAxis = cross(Dir, Other);

        for (unsigned int i = 0; i < 3; i++)
            Mat[i][2] = zAxis[i];
    }

    SetQuatPS(cf::math::QuaternionfT(Mat));
#else
    /*
     * This code works, but it's too generic for most use cases: While it perfectly aligns
     * the desired axis along the desired direction, it also arbitrarily mutates the other
     * two axes, introducing a "skew" to the orientation that is normally not desired.
     */
    const cf::math::Matrix3x3fT Mat(GetQuatPS());   // Our principal axes, in parent-space.
    const Vector3fT             Axis(Mat[0][AxisNr], Mat[1][AxisNr], Mat[2][AxisNr]);

    if (NoPitch)  // Optional: Project Pos (or rather Dir) into the xy-plane.
    {
        // ...
    }

    // Find the axis and the angle that we have to rotate about in order to map Axis onto Dir.
    // Note that Pos is in parent-space already.
    const Vector3fT Dir      = normalizeOr0(Pos - m_Origin.Get());
    const Vector3fT RotAxis  = cross(Axis, Dir);
    const float     RotAxisL = length(RotAxis);
    const float     RotAngle = acos(clamp(-1.0f, dot(Axis, Dir), 1.0f));

    if (RotAxisL < 0.0001f) return;

    const cf::math::QuaternionfT RotQuat = cf::math::QuaternionfT(RotAxis / RotAxisL, RotAngle);

    SetQuatPS(RotQuat * GetQuatPS());
#endif
}


ComponentTransformT* ComponentTransformT::Clone() const
{
    return new ComponentTransformT(*this);
}


static const cf::TypeSys::MethsDocT META_GetOriginWS =
{
    "GetOriginWS",
    "Returns the origin of the transform (in world-space).",
    "tuple", "()"
};

int ComponentTransformT::GetOriginWS(lua_State* LuaState)
{
    ScriptBinderT                      Binder(LuaState);
    IntrusivePtrT<ComponentTransformT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentTransformT> >(1);
    const Vector3fT                    OriginWS(Comp->GetOriginWS());

    lua_pushnumber(LuaState, OriginWS.x);
    lua_pushnumber(LuaState, OriginWS.y);
    lua_pushnumber(LuaState, OriginWS.z);
    return 3;
}


static const cf::TypeSys::MethsDocT META_SetOriginWS =
{
    "SetOriginWS",
    "Sets the origin of the transform (in world-space).",
    "", "(number x, number y, number z)"
};

int ComponentTransformT::SetOriginWS(lua_State* LuaState)
{
    ScriptBinderT                      Binder(LuaState);
    IntrusivePtrT<ComponentTransformT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentTransformT> >(1);
    Vector3fT OriginWS;

    OriginWS.x = float(luaL_checknumber(LuaState, 2));
    OriginWS.y = float(luaL_checknumber(LuaState, 3));
    OriginWS.z = float(luaL_checknumber(LuaState, 4));

    Comp->SetOriginWS(OriginWS);
    return 0;
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


static const cf::TypeSys::MethsDocT META_LookAt =
{
    "LookAt",
    "Sets the orientation of the transform so that it \"looks at\" the given position.\n"
    "The new orientation is chosen such that the bank angle is always 0 relative to the xy-plane.\n"
    "@param PosX      The target position to look at (x-component).\n"
    "@param PosY      The target position to look at (y-component).\n"
    "@param PosZ      The target position to look at (z-component).\n"
    "@param AxisNr    The \"look axis\", i.e. the number of the axis to orient towards `Pos`:\n"
    "                 0 for the x-axis, 1 for the y-axis.\n"
    "@param NoPitch   If `true`, the pitch angle is kept at 0, and the given axis points towards `Pos`\n"
    "                 only in the XY-Plane and the z-axis points straight up (0, 0, 1).",
    "", "(number PosX, number PosY, number PosZ, integer AxisNr = 0, boolean NoPitch = false)"
};

int ComponentTransformT::LookAt(lua_State* LuaState)
{
    ScriptBinderT                      Binder(LuaState);
    IntrusivePtrT<ComponentTransformT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentTransformT> >(1);

    Vector3fT    Pos;
    unsigned int AxisNr  = 0;
    bool         NoPitch = false;

    Pos.x = float(luaL_checknumber(LuaState, 2));
    Pos.y = float(luaL_checknumber(LuaState, 3));
    Pos.z = float(luaL_checknumber(LuaState, 4));

    if (lua_isnumber(LuaState, 5))
        AxisNr = lua_tointeger(LuaState, 5);

    if (lua_isnumber(LuaState, 6))
        NoPitch = lua_tointeger(LuaState, 6) != 0;
    else if (lua_isboolean(LuaState, 6))
        NoPitch = lua_toboolean(LuaState, 6) != 0;

    Comp->LookAt(Pos, AxisNr, NoPitch);
    return 0;
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
    { "GetOriginWS", GetOriginWS },
    { "SetOriginWS", SetOriginWS },
    { "GetAngles",   GetAngles },
    { "SetAngles",   SetAngles },
    { "GetAxisX",    GetAxisX },
    { "GetAxisY",    GetAxisY },
    { "GetAxisZ",    GetAxisZ },
    { "LookAt",      LookAt },
    { "__tostring",  toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentTransformT::DocMethods[] =
{
    META_GetOriginWS,
    META_SetOriginWS,
    META_GetAngles,
    META_SetAngles,
    META_GetAxisX,
    META_GetAxisY,
    META_GetAxisZ,
    META_LookAt,
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentTransformT::TypeInfo(GetComponentTIM(), "GameSys::ComponentTransformT", "GameSys::ComponentBaseT", ComponentTransformT::CreateInstance, MethodsList, DocClass, DocMethods, NULL, DocVars);
