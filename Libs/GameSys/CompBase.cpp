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

#include "CompBase.hpp"
#include "AllComponents.hpp"
#include "Entity.hpp"
#include "Interpolator.hpp"
#include "World.hpp"

#include "ConsoleCommands/ConVar.hpp"
#include "UniScriptState.hpp"
#include "VarVisitorsLua.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GameSys;


namespace
{
    ConVarT clientApproxNPCs("clientApproxNPCs", true, ConVarT::FLAG_MAIN_EXE,
        "Toggles whether origins and other values are interpolated over client frames in order to bridge the larger intervals between server frames.");
}


const char* ComponentBaseT::DocClass =
    "This is the base class for the components that a game entity is composed/aggregated of.\n"
    "Components are the basic building blocks of an entity: their composition defines\n"
    "the properties, the behaviour, and thus virtually every aspect of the entity.";


ComponentBaseT::ComponentBaseT()
    : m_Entity(NULL),
      m_MemberVars(),
      m_PendingInterp(),
      m_ClientApprox()
{
}


ComponentBaseT::ComponentBaseT(const ComponentBaseT& Comp)
    : m_Entity(NULL),
      m_MemberVars(),
      m_PendingInterp(),
      m_ClientApprox()
{
}


ComponentBaseT* ComponentBaseT::Clone() const
{
    return new ComponentBaseT(*this);
}


ComponentBaseT::~ComponentBaseT()
{
    // Note that the m_MemberVars, the m_PendingInterp and the m_ClientApprox all keep references to
    // VarT<T> instances in the derives classes -- which have already been destructed before we get here.
    // Even though we're not dereferencing these variables here, but it would be safer if we had some
    // Cleanup() method that all derived class dtors called.

    for (unsigned int INr = 0; INr < m_PendingInterp.Size(); INr++)
        delete m_PendingInterp[INr];

    for (unsigned int caNr = 0; caNr < m_ClientApprox.Size(); caNr++)
        delete m_ClientApprox[caNr];
}


bool ComponentBaseT::CallLuaMethod(const char* MethodName, int NumExtraArgs, const char* Signature, ...)
{
    va_list vl;

    va_start(vl, Signature);
    const bool Result = m_Entity && m_Entity->GetWorld().GetScriptState().CallMethod_Impl(IntrusivePtrT<ComponentBaseT>(this), MethodName, NumExtraArgs, Signature, vl);
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

    // Deserialization has brought new reference values for interpolated values.
    for (unsigned int caNr = 0; caNr < m_ClientApprox.Size(); caNr++)
    {
        if (IsIniting || !clientApproxNPCs.GetValueBool())
        {
            m_ClientApprox[caNr]->ReInit();
        }
        else
        {
            m_ClientApprox[caNr]->NotifyOverwriteUpdate();
        }
    }

    // Call this after updating the interpolator updates above, so that code
    // that implements DoDeserialize() deals with the latest values.
    DoDeserialize(Stream);
}


void ComponentBaseT::UpdateDependencies(EntityT* Entity)
{
    m_Entity = Entity;
}


void ComponentBaseT::OnServerFrame(float t)
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

    DoServerFrame(t);
}


void ComponentBaseT::OnClientFrame(float t)
{
    // Note that it is up to human player code to setup interpolation for "other"
    // player entities, and to *not* set it up for the "local" player entity.
    if (clientApproxNPCs.GetValueBool())
        for (unsigned int caNr = 0; caNr < m_ClientApprox.Size(); caNr++)
            m_ClientApprox[caNr]->Interpolate(t);

    // TODO: Do we have to run the pending value interpolations here as well?
    DoClientFrame(t);
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


static const cf::TypeSys::MethsDocT META_GetEntity =
{
    "GetEntity",
    "Returns the entity that this component is a part of (or `nil` if the component is currently \"stand-alone\", not a part of any entity).",
    "EntityT", "()"
};

int ComponentBaseT::GetEntity(lua_State* LuaState)
{
    ScriptBinderT                 Binder(LuaState);
    IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    Binder.Push(IntrusivePtrT<EntityT>(Comp->GetEntity()));
    return 1;
}


static const cf::TypeSys::MethsDocT META_InitClientApprox =
{
    "InitClientApprox",
    "Registers the given attribute (a member variable) of this class for interpolation over client frames in order\n"
    "to bridge the larger intervals between server frames.\n"
    "This method only works with variables whose related C++ type is `float`, `double`, `Vector2fT` or `Vector3fT`,\n"
    "and is typically used with ComponentTransform::Origin and ComponentTransform::Orientation. For example:\n"
    "\\code{.lua}\n"
    "    Butterfly.Trafo:InitClientApprox(\"Origin\")\n"
    "    Butterfly.Trafo:InitClientApprox(\"Orientation\")\n"
    "\\endcode\n",
    "", "(string VarName)"
};

int ComponentBaseT::InitClientApprox(lua_State* LuaState)
{
    ScriptBinderT                 Binder(LuaState);
    IntrusivePtrT<ComponentBaseT> Comp    = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);
    const char*                   VarName = luaL_checkstring(LuaState, 2);
    cf::TypeSys::VarBaseT*        Var     = Comp->m_MemberVars.Find(VarName);

    if (!Var)
        return luaL_argerror(LuaState, 2, (std::string("unknown variable \"") + VarName + "\"").c_str());

    // TODO: Only do this if we're in a client world!
    // No need to do it on the server, in CaWE, or the map compile tools.

    for (unsigned int caNr = 0; caNr < Comp->m_ClientApprox.Size(); caNr++)
        if (Var == Comp->m_ClientApprox[caNr]->GetVar())
            return luaL_argerror(LuaState, 2, (std::string("variable \"") + VarName + "\" is already approximated").c_str());

    VarVisitorGetApproxT VarVisGetApprox;
    Var->accept(VarVisGetApprox);
    ApproxBaseT* Approx = VarVisGetApprox.TransferApprox();

    if (!Approx)
        return luaL_argerror(LuaState, 2, (std::string("cannot approximate variable \"") + VarName + "\"").c_str());

    Comp->m_ClientApprox.PushBack(Approx);
    return 0;
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__toString",
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
    { "get",              ComponentBaseT::Get },
    { "set",              ComponentBaseT::Set },
    { "GetExtraMessage",  ComponentBaseT::GetExtraMessage },
    { "interpolate",      ComponentBaseT::Interpolate },
    { "GetEntity",        ComponentBaseT::GetEntity },
    { "InitClientApprox", ComponentBaseT::InitClientApprox },
    { "__tostring",       ComponentBaseT::toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentBaseT::DocMethods[] =
{
    META_Get,
    META_Set,
    META_GetExtraMessage,
    META_Interpolate,
    META_GetEntity,
    META_InitClientApprox,
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentBaseT::TypeInfo(GetComponentTIM(), "ComponentBaseT", NULL /*No base class.*/, ComponentBaseT::CreateInstance, MethodsList, DocClass, DocMethods);
