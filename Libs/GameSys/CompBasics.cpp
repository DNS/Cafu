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

#include "CompBasics.hpp"
#include "AllComponents.hpp"
#include "Entity.hpp"

#include "String.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

#if defined(_WIN32) && defined(_MSC_VER)
    // Turn off warning C4355: 'this' : used in base member initializer list.
    #pragma warning(disable:4355)
#endif

using namespace cf::GameSys;


/*************************************/
/*** ComponentBasicsT::EntityNameT ***/
/*************************************/

ComponentBasicsT::EntityNameT::EntityNameT(const char* Name, const std::string& Value, const char* Flags[], ComponentBasicsT& CompBasics)
    : TypeSys::VarT<std::string>(Name, Value, Flags),
      m_CompBasics(CompBasics)
{
}


// The compiler-written copy constructor would copy m_CompBasics from Var.m_CompBasics,
// but we must obviously use the reference to the proper parent instance instead.
ComponentBasicsT::EntityNameT::EntityNameT(const EntityNameT& Var, ComponentBasicsT& CompBasics)
    : TypeSys::VarT<std::string>(Var),
      m_CompBasics(CompBasics)
{
}


namespace
{
    // If the given name was used for the given entity, would it be unique among its siblings?
    bool IsNameUnique(EntityT* Ent, const std::string& Name)
    {
        if (!Ent) return true;
        if (Ent->GetParent() == NULL) return true;

        const ArrayT< IntrusivePtrT<EntityT> >& Siblings = Ent->GetParent()->GetChildren();

        for (unsigned int SibNr = 0; SibNr < Siblings.Size(); SibNr++)
            if (Siblings[SibNr] != Ent && Siblings[SibNr]->GetBasics()->GetEntityName() == Name)
                return false;

        return true;
    }
}


// The deserialization of network messages on the client can cause a member variable
// of a component to be `Set()` very frequently, and often to the same value as before.
//
// Consequently, setting a variable to the same value must be dealt with as efficiently
// as possible (for performance), and free of unwanted side effects (for correctness).
void ComponentBasicsT::EntityNameT::Set(const std::string& v)
{
    // Make sure that m_CompBasics actually refers to the ComponentBasicsT instance that contains us!
    assert(this == &m_CompBasics.m_Name);

    // No change? Then skip re-establishing that the name is valid.
    if (Get() == v) return;

    const std::string BaseName = cf::String::ToLuaIdentifier(v);
    std::string       NewName  = BaseName;

    for (unsigned int Count = 1; !IsNameUnique(m_CompBasics.GetEntity(), NewName); Count++)
    {
        std::ostringstream out;

        out << BaseName << "_" << Count;

        NewName = out.str();
    }

    TypeSys::VarT<std::string>::Set(NewName);
}


/*************************************/
/*** ComponentBasicsT::EntityShowT ***/
/*************************************/

ComponentBasicsT::EntityShowT::EntityShowT(const char* Name, const bool& Value, const char* Flags[], ComponentBasicsT& CompBasics)
    : TypeSys::VarT<bool>(Name, Value, Flags),
      m_CompBasics(CompBasics)
{
}


// The compiler-written copy constructor would copy m_CompBasics from Var.m_CompBasics,
// but we must obviously use the reference to the proper parent instance instead.
ComponentBasicsT::EntityShowT::EntityShowT(const EntityShowT& Var, ComponentBasicsT& CompBasics)
    : TypeSys::VarT<bool>(Var),
      m_CompBasics(CompBasics)
{
}


void ComponentBasicsT::EntityShowT::Set(const bool& v)
{
    // Make sure that m_CompBasics actually refers to the ComponentBasicsT instance that contains us!
    assert(this == &m_CompBasics.m_Show);

    if (Get() == v) return;

    TypeSys::VarT<bool>::Set(v);

    // Call `OnShow()` only after the new value has been set.
    m_CompBasics.CallLuaMethod("OnShow", 0);
}


/************************/
/*** ComponentBasicsT ***/
/************************/

const char* ComponentBasicsT::DocClass =
    "This component adds the basics of the entity (its name and the \"is shown?\" and \"is static?\" flags).";


const cf::TypeSys::VarsDocT ComponentBasicsT::DocVars[] =
{
    { "Name",   "The name of the entity. Entity names must be valid Lua script identifiers and unique among their siblings." },
    { "Show",   "Is this entity currently shown?" },
    { "Static", "Are the map primitives of this entity fixed and immovable, never moving around in the game world?" },
    { NULL, NULL }
};


ComponentBasicsT::ComponentBasicsT()
    : ComponentBaseT(),
      m_Name("Name", "Entity", NULL, *this),
      m_Show("Show", true, NULL, *this),
      m_Static("Static", false)
{
    GetMemberVars().Add(&m_Name);
    GetMemberVars().Add(&m_Show);
    GetMemberVars().Add(&m_Static);
}


ComponentBasicsT::ComponentBasicsT(const ComponentBasicsT& Comp)
    : ComponentBaseT(Comp),
      m_Name(Comp.m_Name, *this),
      m_Show(Comp.m_Show, *this),
      m_Static(Comp.m_Static)
{
    GetMemberVars().Add(&m_Name);
    GetMemberVars().Add(&m_Show);
    GetMemberVars().Add(&m_Static);
}


ComponentBasicsT* ComponentBasicsT::Clone() const
{
    return new ComponentBasicsT(*this);
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int ComponentBasicsT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "entity basics component");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentBasicsT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentBasicsT();
}

const luaL_Reg ComponentBasicsT::MethodsList[] =
{
    { "__tostring", toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentBasicsT::DocMethods[] =
{
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentBasicsT::TypeInfo(GetComponentTIM(), "GameSys::ComponentBasicsT", "GameSys::ComponentBaseT", ComponentBasicsT::CreateInstance, MethodsList, DocClass, DocMethods, DocVars);
