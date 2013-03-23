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
#include "Window.hpp"

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

using namespace cf::GuiSys;


/*************************************/
/*** ComponentBasicsT::WindowNameT ***/
/*************************************/

ComponentBasicsT::WindowNameT::WindowNameT(const char* Name, const std::string& Value, const char* Flags[], ComponentBasicsT& CompBasics)
    : TypeSys::VarT<std::string>(Name, Value, Flags),
      m_CompBasics(CompBasics)
{
}


// The compiler-written copy constructor would copy m_CompBasics from Var.m_CompBasics,
// but we must obviously use the reference to the proper parent instance instead.
ComponentBasicsT::WindowNameT::WindowNameT(const WindowNameT& Var, ComponentBasicsT& CompBasics)
    : TypeSys::VarT<std::string>(Var),
      m_CompBasics(CompBasics)
{
}


namespace
{
    // If the given name was used for the given window, would it be unique among its siblings?
    bool IsNameUnique(WindowT* Win, const std::string& Name)
    {
        if (!Win) return true;
        if (Win->GetParent() == NULL) return true;

        const ArrayT< IntrusivePtrT<WindowT> >& Siblings = Win->GetParent()->GetChildren();

        for (unsigned int SibNr = 0; SibNr < Siblings.Size(); SibNr++)
            if (Siblings[SibNr] != Win && Siblings[SibNr]->GetBasics()->GetWindowName() == Name)
                return false;

        return true;
    }
}


void ComponentBasicsT::WindowNameT::Set(const std::string& v)
{
    // Make sure that m_CompBasics actually refers to the ComponentBasicsT instance that contains us!
    assert(this == &m_CompBasics.m_Name);

    const std::string BaseName = cf::String::ToLuaIdentifier(v);
    std::string       NewName  = BaseName;

    for (unsigned int Count = 1; !IsNameUnique(m_CompBasics.GetWindow(), NewName); Count++)
    {
        std::ostringstream out;

        out << BaseName << "_" << Count;

        NewName = out.str();
    }

    TypeSys::VarT<std::string>::Set(NewName);
}


/************************/
/*** ComponentBasicsT ***/
/************************/

void* ComponentBasicsT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentBasicsT();
}

const luaL_reg ComponentBasicsT::MethodsList[] =
{
    { "__tostring", ComponentBasicsT::toString },
    { NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentBasicsT::TypeInfo(GetComponentTIM(), "ComponentBasicsT", "ComponentBaseT", ComponentBasicsT::CreateInstance, MethodsList);


ComponentBasicsT::ComponentBasicsT()
    : ComponentBaseT(),
      m_Name("Name", "Window", NULL, *this),
      m_Show("Show", true)
{
    GetMemberVars().Add(&m_Name);
    GetMemberVars().Add(&m_Show);
}


ComponentBasicsT::ComponentBasicsT(const ComponentBasicsT& Comp)
    : ComponentBaseT(Comp),
      m_Name(Comp.m_Name, *this),
      m_Show(Comp.m_Show)
{
    GetMemberVars().Add(&m_Name);
    GetMemberVars().Add(&m_Show);
}


ComponentBasicsT* ComponentBasicsT::Clone() const
{
    return new ComponentBasicsT(*this);
}


int ComponentBasicsT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "window basics component");
    return 1;
}
