/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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


/*************************************/
/*** ComponentBasicsT::WindowShowT ***/
/*************************************/

ComponentBasicsT::WindowShowT::WindowShowT(const char* Name, const bool& Value, const char* Flags[], ComponentBasicsT& CompBasics)
    : TypeSys::VarT<bool>(Name, Value, Flags),
      m_CompBasics(CompBasics)
{
}


// The compiler-written copy constructor would copy m_CompBasics from Var.m_CompBasics,
// but we must obviously use the reference to the proper parent instance instead.
ComponentBasicsT::WindowShowT::WindowShowT(const WindowShowT& Var, ComponentBasicsT& CompBasics)
    : TypeSys::VarT<bool>(Var),
      m_CompBasics(CompBasics)
{
}


void ComponentBasicsT::WindowShowT::Set(const bool& v)
{
    // Make sure that m_CompBasics actually refers to the ComponentBasicsT instance that contains us!
    assert(this == &m_CompBasics.m_Show);

    if (v == Get()) return;

    TypeSys::VarT<bool>::Set(v);

    // Call `OnShow()` only after the new value has been set.
    m_CompBasics.CallLuaMethod("OnShow");
}


/************************/
/*** ComponentBasicsT ***/
/************************/

const char* ComponentBasicsT::DocClass =
    "This component adds the basics of the window (its name and the \"is shown?\" flag).";


const cf::TypeSys::VarsDocT ComponentBasicsT::DocVars[] =
{
    { "Name", "The name of the window. Window names must be valid Lua script identifiers and unique among their siblings." },
    { "Show", "Is this window currently shown?" },
    { NULL, NULL }
};


ComponentBasicsT::ComponentBasicsT()
    : ComponentBaseT(),
      m_Name("Name", "Window", NULL, *this),
      m_Show("Show", true, NULL, *this)
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

    lua_pushfstring(LuaState, "window basics component");
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

const cf::TypeSys::MethsDocT ComponentBasicsT::DocCallbacks[] =
{
    { "OnShow",
      "This method is called when the value of the component's `Show` member has changed.",
      "", "" },
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentBasicsT::TypeInfo(
    GetComponentTIM(),
    "GuiSys::ComponentBasicsT",
    "GuiSys::ComponentBaseT",
    ComponentBasicsT::CreateInstance,
    MethodsList,
    DocClass,
    DocMethods,
    DocCallbacks,
    DocVars);
