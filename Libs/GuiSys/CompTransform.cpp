/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "CompTransform.hpp"
#include "AllComponents.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GuiSys;


const char* ComponentTransformT::DocClass =
    "This component adds information about the position and size of the window.";


const cf::TypeSys::VarsDocT ComponentTransformT::DocVars[] =
{
    { "Pos",      "The position of the top-left corner of the window, relative to its parent." },
    { "Size",     "The size of the window." },
    { "Rotation", "The angle in degrees by how much this entire window is rotated." },
    { NULL, NULL }
};


ComponentTransformT::ComponentTransformT()
    : ComponentBaseT(),
      m_Pos("Pos", Vector2fT(0.0f, 0.0f)),
      m_Size("Size", Vector2fT(80.0f, 60.0f)),
      m_RotAngle("Rotation", 0.0f)
{
    GetMemberVars().Add(&m_Pos);
    GetMemberVars().Add(&m_Size);
    GetMemberVars().Add(&m_RotAngle);
}


ComponentTransformT::ComponentTransformT(const ComponentTransformT& Comp)
    : ComponentBaseT(Comp),
      m_Pos(Comp.m_Pos),
      m_Size(Comp.m_Size),
      m_RotAngle(Comp.m_RotAngle)
{
    GetMemberVars().Add(&m_Pos);
    GetMemberVars().Add(&m_Size);
    GetMemberVars().Add(&m_RotAngle);
}


ComponentTransformT* ComponentTransformT::Clone() const
{
    return new ComponentTransformT(*this);
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
    { "__tostring", toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentTransformT::DocMethods[] =
{
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentTransformT::TypeInfo(GetComponentTIM(), "GuiSys::ComponentTransformT", "GuiSys::ComponentBaseT", ComponentTransformT::CreateInstance, MethodsList, DocClass, DocMethods, NULL, DocVars);
