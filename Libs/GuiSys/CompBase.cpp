/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "CompBase.hpp"
#include "AllComponents.hpp"
#include "GuiImpl.hpp"
#include "Window.hpp"
#include "UniScriptState.hpp"
#include "VarVisitorsLua.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GuiSys;


const char* ComponentBaseT::DocClass =
    "This is the base class for the components that a window is composed/aggregated of.\n"
    "Components are the basic building blocks of a window: their composition defines\n"
    "the properties, the behaviour, and thus virtually every aspect of the window.";


ComponentBaseT::ComponentBaseT()
    : m_Window(NULL),
      m_MemberVars()
{
}


ComponentBaseT::ComponentBaseT(const ComponentBaseT& Comp)
    : m_Window(NULL),
      m_MemberVars()
{
}


ComponentBaseT* ComponentBaseT::Clone() const
{
    return new ComponentBaseT(*this);
}


bool ComponentBaseT::CallLuaMethod(const char* MethodName, const char* Signature, ...)
{
    va_list vl;

    va_start(vl, Signature);
    const bool Result = m_Window && m_Window->GetGui().GetScriptState().CallMethod_Impl(IntrusivePtrT<ComponentBaseT>(this), MethodName, 0, Signature, vl);
    va_end(vl);

    return Result;
}


// Note that this method is the twin of Deserialize(), whose implementation it must match.
void ComponentBaseT::Serialize(cf::Network::OutStreamT& Stream) const
{
    const ArrayT<cf::TypeSys::VarBaseT*>& Vars = m_MemberVars.GetArray();

    for (unsigned int VarNr = 0; VarNr < Vars.Size(); VarNr++)
    {
        // Variables with flag "DontSerialize" are typically covered already because another variable co-
        // serializes them, e.g. m_ModelName in ComponentModelT co-serializes m_ModelAnimNr and m_ModelSkinNr.
        // (Alternatively, variables with this flag may just not need to be serialized.)
        if (!Vars[VarNr]->HasFlag("DontSerialize"))
            Vars[VarNr]->Serialize(Stream);
    }

    DoSerialize(Stream);
}


// Note that this method is the twin of Serialize(), whose implementation it must match.
void ComponentBaseT::Deserialize(cf::Network::InStreamT& Stream, bool IsIniting)
{
    const ArrayT<cf::TypeSys::VarBaseT*>& Vars = m_MemberVars.GetArray();

    for (unsigned int VarNr = 0; VarNr < Vars.Size(); VarNr++)
    {
        // Variables with flag "DontSerialize" are typically covered already because another variable co-
        // serializes them, e.g. m_ModelName in ComponentModelT co-serializes m_ModelAnimNr and m_ModelSkinNr.
        // (Alternatively, variables with this flag may just not need to be serialized.)
        if (!Vars[VarNr]->HasFlag("DontSerialize"))
            Vars[VarNr]->Deserialize(Stream);
    }

    DoDeserialize(Stream, IsIniting);
}


void ComponentBaseT::UpdateDependencies(WindowT* Window)
{
    m_Window = Window;
}


void ComponentBaseT::OnClockTickEvent(float t)
{
    // Run the pending value interpolations.
    for (unsigned int INr = 0; INr < m_PendingInterp.Size(); INr++)
    {
        InterpolationT* I = m_PendingInterp[INr];

        // Run this interpolation only if there is no other interpolation that addresses the same target value.
        unsigned int OldINr;

        for (OldINr = 0; OldINr < INr; OldINr++)
            if (m_PendingInterp[OldINr]->Var == I->Var && m_PendingInterp[OldINr]->Suffix == I->Suffix)
                break;

        if (OldINr < INr) continue;


        // Actually run the interpolation I.
        I->CurrentTime += t;

        if (I->CurrentTime >= I->TotalTime)
        {
            // This interpolation reached its end value, so drop it from the pending queue.
            cf::TypeSys::VarVisitorSetFloatT SetFloat(I->Suffix, I->EndValue);

            I->Var->accept(SetFloat);

            delete I;
            m_PendingInterp.RemoveAtAndKeepOrder(INr);
            INr--;
        }
        else
        {
            cf::TypeSys::VarVisitorSetFloatT SetFloat(I->Suffix, I->GetCurrentValue());

            I->Var->accept(SetFloat);
        }
    }
}


namespace
{
    // Returns the variable matching VarName, and a Suffix that indicates whether the whole variable or only a part of it was meant.
    void DetermineVar(const char* VarName, const cf::TypeSys::VarManT& MemberVars, cf::TypeSys::VarBaseT*& Var, unsigned int& Suffix)
    {
        Var    = MemberVars.Find(VarName);
        Suffix = UINT_MAX;

        // Success: VarName was immediately found, so it did not contain a suffix.
        if (Var) return;

        const size_t len = strlen(VarName);
        size_t i;

        for (i = len; i > 0; i--)
            if (VarName[i - 1] == '.')
                break;

        // No '.' found in VarName, so there is nothing we can do but return with Var == NULL.
        if (i == 0) return;

        // If '.' is the last character in VarName, there cannot be a suffix, so return with Var == NULL.
        if (i == len) return;

        i--;    // Adjust i to the position of the dot.

        // The portion before the '.' can be at most 15 characters, so return with Var == NULL if it has 16 or more.
        if (i >= 16) return;

        char TempName[16];
        strncpy(TempName, VarName, i);
        TempName[i] = 0;

        Var = MemberVars.Find(TempName);

        // Could not find the variable even with the suffix stripped from its name, so return with Var == NULL.
        if (!Var) return;

        const char Sfx = VarName[i + 1];

             if (Sfx == 'x' || Sfx == 'r') Suffix = 0;
        else if (Sfx == 'y' || Sfx == 'g') Suffix = 1;
        else if (Sfx == 'z' || Sfx == 'b') Suffix = 2;
        else
        {
            // There is most likely something wrong if we get here, but alas, let it be.
            Suffix = 0;
        }
    }
}


static const cf::TypeSys::MethsDocT META_Get =
{
    "get",
    "Returns the value of an attribute (a member variable) of this class.",
    "any", "(string var_name)"
};

int ComponentBaseT::Get(lua_State* LuaState)
{
    ScriptBinderT                    Binder(LuaState);
    cf::TypeSys::VarVisitorGetToLuaT GetToLua(LuaState);
    IntrusivePtrT<ComponentBaseT>    Comp    = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);
    const char*                      VarName = luaL_checkstring(LuaState, 2);
    const cf::TypeSys::VarBaseT*     Var     = Comp->m_MemberVars.Find(VarName);

    if (!Var)
        return luaL_argerror(LuaState, 2, (std::string("unknown variable \"") + VarName + "\"").c_str());

    Var->accept(GetToLua);
    return GetToLua.GetNumResults();
}


static const cf::TypeSys::MethsDocT META_Set =
{
    "set",
    "Sets an attribute (a member variable) of this class to a new value.",
    "", "(string var_name, any new_value)"
};

int ComponentBaseT::Set(lua_State* LuaState)
{
    ScriptBinderT                      Binder(LuaState);
    cf::TypeSys::VarVisitorSetFromLuaT SetFromLua(LuaState);
    IntrusivePtrT<ComponentBaseT>      Comp    = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);
    const char*                        VarName = luaL_checkstring(LuaState, 2);
    cf::TypeSys::VarBaseT*             Var     = Comp->m_MemberVars.Find(VarName);

    if (!Var)
        return luaL_argerror(LuaState, 2, (std::string("unknown variable \"") + VarName + "\"").c_str());

    Var->accept(SetFromLua);
    return 0;
}


static const cf::TypeSys::MethsDocT META_GetExtraMessage =
{
    "GetExtraMessage",
    "Returns the result of `VarBaseT::GetExtraMessage()` for the given member variable.",
    "string", "(string var_name)"
};

int ComponentBaseT::GetExtraMessage(lua_State* LuaState)
{
    ScriptBinderT                 Binder(LuaState);
    IntrusivePtrT<ComponentBaseT> Comp    = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);
    const char*                   VarName = luaL_checkstring(LuaState, 2);
    cf::TypeSys::VarBaseT*        Var     = Comp->m_MemberVars.Find(VarName);

    if (!Var)
        return luaL_argerror(LuaState, 2, (std::string("unknown variable \"") + VarName + "\"").c_str());

    lua_pushstring(LuaState, Var->GetExtraMessage().c_str());
    return 1;
}


static const cf::TypeSys::MethsDocT META_Interpolate =
{
    "interpolate",
    "Schedules a value for interpolation between a start and end value over a given period of time.\n"
    "Only variables that are floating-point numbers and variables that are tuples whose elements are\n"
    "floating-point numbers can be interpolated. (These are the variables whose underlying C++ type\n"
    "is `float`, `double`, `Vector2fT` or `Vector3fT`.)\n"
    "For variables that are tuples, you must append one of the suffixes `.x`, `.y`, `.z` to determine\n"
    "the first, second or third element for interpolation. Alternatively, the suffixes `.r`, `.g`, `.b`\n"
    "are more naturally used with color tuples, and work exactly alike.",
    "", "(string var_name, number start_value, number end_value, number time)"
};

int ComponentBaseT::Interpolate(lua_State* LuaState)
{
    ScriptBinderT                 Binder(LuaState);
    IntrusivePtrT<ComponentBaseT> Comp    = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);
    const char*                   VarName = luaL_checkstring(LuaState, 2);
    cf::TypeSys::VarBaseT*        Var     = NULL;
    unsigned int                  Suffix  = UINT_MAX;

    DetermineVar(VarName, Comp->m_MemberVars, Var, Suffix);

    if (!Var)
        return luaL_argerror(LuaState, 2, (std::string("unknown variable \"") + VarName + "\"").c_str());

    // Make sure that there are no more than MAX_INTERPOLATIONS interpolations pending for Var already.
    // If so, just delete the oldest ones, which effectively means to skip them (the next youngest interpolation will take over).
    // The purpose is of course to prevent anything from adding arbitrarily many interpolations, eating up memory,
    // which could happen from bad user code (e.g. if the Cafu game code doesn't protect multiple human players from using
    // a GUI simultaneously, mouse cursor "position flickering" might occur on the server, which in turn might trigger the
    // permanent addition of interpolations from OnFocusLose()/OnFocusGain() scripts).
    const unsigned int MAX_INTERPOLATIONS = 10;
    unsigned int       InterpolationCount =  0;

    for (unsigned int INr = Comp->m_PendingInterp.Size(); INr > 0; INr--)
    {
        InterpolationT* I = Comp->m_PendingInterp[INr-1];

        if (I->Var == Var && I->Suffix == Suffix)
            InterpolationCount++;

        if (InterpolationCount > MAX_INTERPOLATIONS)
        {
            delete I;
            Comp->m_PendingInterp.RemoveAtAndKeepOrder(INr-1);
            break;
        }
    }

    // Now add the new interpolation to the pending list.
    InterpolationT* I = new InterpolationT;

    I->Var         = Var;
    I->Suffix      = Suffix;
    I->StartValue  = float(lua_tonumber(LuaState, -3));
    I->EndValue    = float(lua_tonumber(LuaState, -2));
    I->CurrentTime = 0.0f;
    I->TotalTime   = float(lua_tonumber(LuaState, -1)/1000.0);

    Comp->m_PendingInterp.PushBack(I);
    return 0;
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int ComponentBaseT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "base component");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentBaseT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentBaseT();
}

const luaL_Reg ComponentBaseT::MethodsList[] =
{
    { "get",             Get },
    { "set",             Set },
    { "GetExtraMessage", GetExtraMessage },
    { "interpolate",     Interpolate },
    { "__tostring",      toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentBaseT::DocMethods[] =
{
    META_Get,
    META_Set,
    META_GetExtraMessage,
    META_Interpolate,
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentBaseT::TypeInfo(GetComponentTIM(), "GuiSys::ComponentBaseT", NULL /*No base class.*/, ComponentBaseT::CreateInstance, MethodsList, DocClass, DocMethods);
