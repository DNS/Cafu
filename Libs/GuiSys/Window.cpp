/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#include "Window.hpp"
#include "WindowCreateParams.hpp"
#include "CompBase.hpp"
#include "GuiImpl.hpp"
#include "GuiResources.hpp"
#include "ConsoleCommands/Console.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "TypeSys.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

#include <assert.h>


using namespace cf::GuiSys;


// Note that we cannot simply replace this method with a global TypeInfoManT instance,
// because it is called during global static initialization time. The TIM instance being
// embedded in the function guarantees that it is properly initialized before first use.
cf::TypeSys::TypeInfoManT& cf::GuiSys::GetWindowTIM()
{
    static cf::TypeSys::TypeInfoManT TIM;

    return TIM;
}


/*** Begin of TypeSys related definitions for this class. ***/

void* WindowT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new WindowT(*static_cast<const cf::GuiSys::WindowCreateParamsT*>(&Params));
}

const luaL_reg WindowT::MethodsList[]=
{
    { "set",             WindowT::Set },
    { "get",             WindowT::Get },
    { "interpolate",     WindowT::Interpolate },
    { "SetName",         WindowT::SetName },
    { "GetName",         WindowT::GetName },
    { "AddChild",        WindowT::AddChild },
    { "RemoveChild",     WindowT::RemoveChild },
    { "GetParent",       WindowT::GetParent },
    { "GetChildren",     WindowT::GetChildren },
    { "GetTransform",    WindowT::GetTransform },
    { "AddComponent",    WindowT::AddComponent },
    { "RemoveComponent", WindowT::RmvComponent },
    { "GetComponents",   WindowT::GetComponents },
    { "GetComponent",    WindowT::GetComponent },
    { "__tostring",      WindowT::toString },
    { NULL, NULL }
};

const cf::TypeSys::TypeInfoT WindowT::TypeInfo(GetWindowTIM(), "WindowT", NULL /*No base class.*/, WindowT::CreateInstance, MethodsList);

/*** End of TypeSys related definitions for this class. ***/


WindowT::WindowT(const WindowCreateParamsT& Params)
    : Time(0.0f),
      ShowWindow(true),
      m_Gui(Params.Gui),
      m_ExtData(NULL),
      m_Parent(NULL),
      m_Children(),
      m_Name(""),
      m_Transform(new ComponentTransformT()),
      m_Components()
{
    m_Transform->SetSize(Vector2fT(80, 60));

    // This is currently required, because this ctor is also used now for windows created
    // by Lua script (not only for windows created in C++, using "new WindowT()")...
    FillMemberVars();
}


WindowT::WindowT(const WindowT& Window, bool Recursive)
    : Time(Window.Time),
      ShowWindow(Window.ShowWindow),
      m_Gui(Window.m_Gui),
      m_ExtData(NULL   /* Clone() it?? */),
      m_Parent(NULL),
      m_Children(),
      m_Name(Window.m_Name),
      m_Transform(Window.GetTransform()->Clone()),
      m_Components()
{
    // Copy-create all components first.
    m_Components.PushBackEmptyExact(Window.GetComponents().Size());

    for (unsigned int CompNr = 0; CompNr < Window.GetComponents().Size(); CompNr++)
        m_Components[CompNr] = Window.GetComponents()[CompNr]->Clone();

    // Now that all components have been copied, have them resolve their dependencies among themselves.
    for (unsigned int CompNr = 0; CompNr < m_Components.Size(); CompNr++)
        m_Components[CompNr]->UpdateDependencies(this);

    FillMemberVars();

    if (Recursive)
    {
        for (unsigned long ChildNr=0; ChildNr<Window.m_Children.Size(); ChildNr++)
        {
            m_Children.PushBack(Window.m_Children[ChildNr]->Clone(Recursive));
            m_Children[ChildNr]->m_Parent=this;
        }
    }
}


WindowT* WindowT::Clone(bool Recursive) const
{
    return new WindowT(*this, Recursive);
}


WindowT::~WindowT()
{
    // Cannot have a parent any more. Otherwise, the parent still had
    // us as a child, and we should not have gotten here for destruction.
    assert(m_Parent == NULL);

    // Cleanly disconnect our children.
    for (unsigned long ChildNr=0; ChildNr<m_Children.Size(); ChildNr++)
        m_Children[ChildNr]->m_Parent = NULL;

    m_Children.Clear();

    // Delete the external data.
    delete m_ExtData;
    m_ExtData=NULL;

    // Delete the components.
    for (unsigned int CompNr = 0; CompNr < m_Components.Size(); CompNr++)
    {
        // No one else should still have a pointer to m_Components[CompNr] at this point.
        // Nevertheless, it is still possible that m_Components[CompNr]->GetRefCount() > 1,
        // for example if a script still keeps a reference to the component, or has kept
        // one that is not yet garbage collected.
        // To be safe, make sure that the component no longer refers back to this window.
        m_Components[CompNr]->UpdateDependencies(NULL);
    }

    m_Components.Clear();
}


void WindowT::SetExtData(ExtDataT* ExtData)
{
    delete m_ExtData;
    m_ExtData=ExtData;
}


namespace
{
    bool IsNameUnique(WindowT* Win, const ArrayT< IntrusivePtrT<WindowT> >& Siblings)
    {
        for (unsigned long SibNr = 0; SibNr < Siblings.Size(); SibNr++)
            if (Siblings[SibNr] != Win && Siblings[SibNr]->GetName() == Win->GetName())
                return false;

        return true;
    }
}


void WindowT::SetName(const std::string& Name)
{
    m_Name = Name;

    if (m_Parent)
    {
        // We need a true copy of Name here, because if Name is a reference to m_Name
        // (consider e.g. SetName(GetName())), the loop below will not properly work.
        const std::string BaseName = Name;

        for (unsigned int Count = 1; !IsNameUnique(this, m_Parent->m_Children); Count++)
        {
            std::ostringstream out;

            out << BaseName << "_" << Count;

            m_Name = out.str();
        }
    }
}


bool WindowT::AddChild(IntrusivePtrT<WindowT> Child, unsigned long Pos)
{
    assert(Child->m_Parent == NULL);
    if (Child->m_Parent != NULL)    // A child window must be a root node...
        return false;   // luaL_argerror(LuaState, 2, "child window already has a parent, use RemoveChild() first");

    assert(Child != GetRoot());
    if (Child == GetRoot())         // ... but not the root of the hierarchy it is inserted into.
        return false;   // luaL_argerror(LuaState, 2, "a window cannot be made a child of itself");

    m_Children.InsertAt(std::min(m_Children.Size(), Pos), Child);
    Child->m_Parent = this;

    // Make sure that the childs name is unique among its siblings.
    Child->SetName(Child->GetName());
    return true;
}


bool WindowT::RemoveChild(IntrusivePtrT<WindowT> Child)
{
    assert(Child->m_Parent == this);
    if (Child->m_Parent != this)
        return false;   // luaL_argerror(LuaState, 2, "window is the child of another parent");

    const int Index = m_Children.Find(Child);

    assert(Index >= 0);
    if (Index < 0)
        return false;   // return luaL_argerror(LuaState, 2, "window not found among the children of its parent");

    m_Children.RemoveAtAndKeepOrder(Index);
    Child->m_Parent = NULL;
    return true;
}


void WindowT::GetChildren(ArrayT< IntrusivePtrT<WindowT> >& Chld, bool Recurse) const
{
#ifdef DEBUG
    // Make sure that there are no cycles in the hierarchy of children.
    for (unsigned long ChildNr=0; ChildNr<m_Children.Size(); ChildNr++)
        assert(Chld.Find(m_Children[ChildNr]) == -1);
#endif

    Chld.PushBack(m_Children);

    if (!Recurse) return;

    for (unsigned long ChildNr=0; ChildNr<m_Children.Size(); ChildNr++)
        m_Children[ChildNr]->GetChildren(Chld, Recurse);
}


IntrusivePtrT<WindowT> WindowT::GetRoot()
{
    WindowT* Root = this;

    while (Root->m_Parent)
        Root = Root->m_Parent;

    return Root;
}


IntrusivePtrT<ComponentBaseT> WindowT::GetComponent(const std::string& TypeName, unsigned int n) const
{
    for (unsigned int CompNr = 0; CompNr < m_Components.Size(); CompNr++)
        if (m_Components[CompNr]->GetName() == TypeName)
        {
            if (n == 0) return m_Components[CompNr];
            n--;
        }

    return NULL;
}


bool WindowT::AddComponent(IntrusivePtrT<ComponentBaseT> Comp, unsigned long Index)
{
    if (Comp->GetWindow()) return false;
    assert(m_Components.Find(Comp) < 0);

    m_Components.InsertAt(std::min(Index, m_Components.Size()), Comp);

    // Have the components re-resolve their dependencies among themselves.
    for (unsigned int CompNr = 0; CompNr < m_Components.Size(); CompNr++)
        m_Components[CompNr]->UpdateDependencies(this);

    return true;
}


void WindowT::DeleteComponent(unsigned long CompNr)
{
    // Let the component know that it is no longer a part of this window.
    m_Components[CompNr]->UpdateDependencies(NULL);

    m_Components.RemoveAtAndKeepOrder(CompNr);

    // Have the remaining components re-resolve their dependencies among themselves.
    for (unsigned int CompNr = 0; CompNr < m_Components.Size(); CompNr++)
        m_Components[CompNr]->UpdateDependencies(this);
}


void WindowT::GetAbsolutePos(float& x, float& y) const
{
#if 1
    x = m_Transform->GetPos().x;
    y = m_Transform->GetPos().y;

    for (const WindowT* P = m_Parent; P; P = P->m_Parent)
    {
        x += P->m_Transform->GetPos().x;
        y += P->m_Transform->GetPos().y;
    }
#else
    // Recursive implementation:
    if (Parent==NULL)
    {
        x = m_Transform->GetPos().x;
        y = m_Transform->GetPos().y;
        return;
    }

    float px;
    float py;

    // We have a parent, so get it's absolute position first, then add our relative position.
    Parent->GetAbsolutePos(px, py);

    x = px + m_Transform->GetPos().x;
    y = py + m_Transform->GetPos().y;
#endif
}


IntrusivePtrT<WindowT> WindowT::Find(const std::string& WantedName)
{
    if (WantedName == m_Name) return this;

    // Recursively see if any of the children has the desired name.
    for (unsigned long ChildNr=0; ChildNr<m_Children.Size(); ChildNr++)
    {
        IntrusivePtrT<WindowT> Win=m_Children[ChildNr]->Find(WantedName);

        if (Win!=NULL) return Win;
    }

    return NULL;
}


IntrusivePtrT<WindowT> WindowT::Find(float x, float y, bool OnlyVisible)
{
    if (OnlyVisible && !ShowWindow) return NULL;

    // Children are on top of their parents and (currently) not clipped to the parent rectangle, so we should search them first.
    for (unsigned long ChildNr=0; ChildNr<m_Children.Size(); ChildNr++)
    {
        // Search the children in reverse order, because if they overlap,
        // those that are drawn last appear topmost, and so we want to find them first.
        IntrusivePtrT<WindowT> Found=m_Children[m_Children.Size()-1-ChildNr]->Find(x, y, OnlyVisible);

        if (Found!=NULL) return Found;
    }

    // Okay, the point (x, y) is not inside any of the children, now check this window.
    float AbsX1, AbsY1;

    GetAbsolutePos(AbsX1, AbsY1);

    const float SizeX = m_Transform->GetSize().x;
    const float SizeY = m_Transform->GetSize().y;

    return (x<AbsX1 || y<AbsY1 || x>AbsX1+SizeX || y>AbsY1+SizeY) ? NULL : this;
}


void WindowT::Render() const
{
    if (!ShowWindow) return;

    float x1;
    float y1;

    GetAbsolutePos(x1, y1);


    // Render the components.
    {
        MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);

        // Set the coordinate origin to the top-left corner of our window.
        if (m_Transform->GetRotAngle() == 0)
        {
            MatSys::Renderer->Translate(MatSys::RendererI::MODEL_TO_WORLD, x1, y1, 0.0f);
        }
        else
        {
            MatSys::Renderer->Translate(MatSys::RendererI::MODEL_TO_WORLD, x1 + m_Transform->GetSize().x/2.0f, y1 + m_Transform->GetSize().y/2.0f, 0.0f);
            MatSys::Renderer->RotateZ  (MatSys::RendererI::MODEL_TO_WORLD, m_Transform->GetRotAngle());
            MatSys::Renderer->Translate(MatSys::RendererI::MODEL_TO_WORLD,     -m_Transform->GetSize().x/2.0f,     -m_Transform->GetSize().y/2.0f, 0.0f);
        }

        // Render components in the proper order -- bottom-up.
        for (unsigned long CompNr = m_Components.Size(); CompNr > 0; CompNr--)
            m_Components[CompNr-1]->Render();

        MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    }


    // Save the current matrices.
    if (m_Transform->GetRotAngle() != 0)
    {
        MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);

        MatSys::Renderer->Translate(MatSys::RendererI::MODEL_TO_WORLD, x1+m_Transform->GetSize().x/2.0f, y1+m_Transform->GetSize().y/2.0f, 0.0f);
        MatSys::Renderer->RotateZ  (MatSys::RendererI::MODEL_TO_WORLD, m_Transform->GetRotAngle());
        MatSys::Renderer->Translate(MatSys::RendererI::MODEL_TO_WORLD, -(x1+m_Transform->GetSize().x/2.0f), -(y1+m_Transform->GetSize().y/2.0f), 0.0f);
    }


    // Render the children.
    for (unsigned long ChildNr=0; ChildNr<m_Children.Size(); ChildNr++)
    {
        m_Children[ChildNr]->Render();
    }

    // Give the external data class a chance to render additional items.
    // E.g. if m_ExtData is used in a GUI editor, it might render selection borders etc.
    if (m_ExtData) m_ExtData->Render();

    if (m_Transform->GetRotAngle() != 0)
    {
        // Restore the previously active matrices.
        MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    }
}


bool WindowT::OnInputEvent(const CaKeyboardEventT& KE)
{
    // Forward the event to the components.
    for (unsigned int CompNr = 0; CompNr < m_Components.Size(); CompNr++)
        if (m_Components[CompNr]->OnInputEvent(KE))
            return true;

    return false;
}


bool WindowT::OnInputEvent(const CaMouseEventT& ME, float PosX, float PosY)
{
    // Derived classes that do *not* handle this event should return WindowT::OnInputEvent(ME)
    // (the base class result) rather than simply false. This gives the base class a chance to handle the event.

#if 0
    // This should only hold for (in wxWidgets-terms) "command events", not for events specific to the window (like mouse events):

    // If the base class has no use for the event itself either, it will propagate it to the parent windows.
    // Here, in WindowT, being at the root of the inheritance hierachy, we have no use for the ME event,
    // so just propagate it up to the parent(s).
    if (Parent==NULL) return false;
    return Parent->OnInputEvent(ME, PosX, PosY);
#else
    // Forward the event to the components.
    for (unsigned int CompNr = 0; CompNr < m_Components.Size(); CompNr++)
        if (m_Components[CompNr]->OnInputEvent(ME, PosX, PosY))
            return true;

    return false;
#endif
}


bool WindowT::OnClockTickEvent(float t)
{
    // Forward the event to the components.
    for (unsigned int CompNr = 0; CompNr < m_Components.Size(); CompNr++)
        m_Components[CompNr]->OnClockTickEvent(t);

    // float OldTime=Time;

    Time+=t;

    // See if we have to run any OnTime scripts.
    // ;

    // Run the pending value interpolations.
    for (unsigned long INr=0; INr<m_PendingInterp.Size(); INr++)
    {
        InterpolationT* I=m_PendingInterp[INr];

        // Run this interpolation only if there is no other interpolation that addresses the same target value.
        unsigned long OldINr;

        for (OldINr=0; OldINr<INr; OldINr++)
            if (m_PendingInterp[OldINr]->Value == I->Value)
                break;

        if (OldINr<INr) continue;


        // Actually run the interpolation I.
        I->CurrentTime+=t;

        if (I->CurrentTime >= I->TotalTime)
        {
            // This interpolation reached its end value, so drop it from the pending queue.
            I->Value[0]=I->EndValue;

            delete I;
            m_PendingInterp.RemoveAtAndKeepOrder(INr);
            INr--;
        }
        else
        {
            I->UpdateValue();
        }
    }

    return true;
}


bool WindowT::CallLuaMethod(const char* MethodName, const char* Signature, ...)
{
    va_list vl;

    va_start(vl, Signature);
    const bool Result=m_Gui.m_ScriptState.CallMethod(IntrusivePtrT<WindowT>(this), MethodName, Signature, vl);
    va_end(vl);

    return Result;
}


void WindowT::FillMemberVars()
{
    MemberVars["time"]=MemberVarT(Time);
    MemberVars["show"]=MemberVarT(ShowWindow);
}


/**********************************************/
/*** Impementation of Lua binding functions ***/
/**********************************************/

int WindowT::Set(lua_State* LuaState)
{
    ScriptBinderT     Binder(LuaState);
    IntrusivePtrT<WindowT> Win=Binder.GetCheckedObjectParam< IntrusivePtrT<WindowT> >(1);
    std::string       VarName=luaL_checkstring(LuaState, 2);
    const MemberVarT& Var    =Win->MemberVars[VarName];

    if (VarName == "rect")
    {
        Win->m_Transform->SetPos (Vector2fT(float(lua_tonumber(LuaState, 3)), float(lua_tonumber(LuaState, 4))));
        Win->m_Transform->SetSize(Vector2fT(float(lua_tonumber(LuaState, 5)), float(lua_tonumber(LuaState, 6))));

        return 0;
    }

    if (Var.Member==NULL)
    {
        // Bad argument "VarName".
        luaL_argerror(LuaState, 2, (std::string("unknown field '")+VarName+"'").c_str());
        return 0;
    }

    switch (Var.Type)
    {
        case MemberVarT::TYPE_FLOAT:
            ((float*)Var.Member)[0]=float(lua_tonumber(LuaState, 3));
            break;

        case MemberVarT::TYPE_FLOAT2:
            ((float*)Var.Member)[0]=float(lua_tonumber(LuaState, 3));
            ((float*)Var.Member)[1]=float(lua_tonumber(LuaState, 4));
            break;

        case MemberVarT::TYPE_FLOAT4:
            ((float*)Var.Member)[0]=float(lua_tonumber(LuaState, 3));
            ((float*)Var.Member)[1]=float(lua_tonumber(LuaState, 4));
            ((float*)Var.Member)[2]=float(lua_tonumber(LuaState, 5));
            ((float*)Var.Member)[3]=float(lua_tonumber(LuaState, 6));
            break;

        case MemberVarT::TYPE_INT:
            ((int*)Var.Member)[0]=lua_tointeger(LuaState, 3);
            break;

        case MemberVarT::TYPE_BOOL:
            // I also want to treat the number 0 as false, not just "false" and "nil".
            if (lua_isnumber(LuaState, 3)) ((bool*)Var.Member)[0]=lua_tointeger(LuaState, 3)!=0;
                                      else ((bool*)Var.Member)[0]=lua_toboolean(LuaState, 3)!=0;
            break;

        case MemberVarT::TYPE_STRING:
        {
            const char* s=lua_tostring(LuaState, 3);

            ((std::string*)Var.Member)[0]=(s!=NULL) ? s : "";
            break;
        }
    }

    return 0;
}


int WindowT::Get(lua_State* LuaState)
{
    ScriptBinderT     Binder(LuaState);
    IntrusivePtrT<WindowT> Win=Binder.GetCheckedObjectParam< IntrusivePtrT<WindowT> >(1);
    std::string       VarName=luaL_checkstring(LuaState, 2);
    const MemberVarT& Var    =Win->MemberVars[VarName];

    if (Var.Member==NULL)
    {
        // Bad argument "VarName".
        luaL_argerror(LuaState, 2, (std::string("unknown field '")+VarName+"'").c_str());
        return 0;
    }

    switch (Var.Type)
    {
        case MemberVarT::TYPE_FLOAT:
            lua_pushnumber(LuaState, ((float*)Var.Member)[0]);
            return 1;

        case MemberVarT::TYPE_FLOAT2:
            lua_pushnumber(LuaState, ((float*)Var.Member)[0]);
            lua_pushnumber(LuaState, ((float*)Var.Member)[1]);
            return 2;

        case MemberVarT::TYPE_FLOAT4:
            lua_pushnumber(LuaState, ((float*)Var.Member)[0]);
            lua_pushnumber(LuaState, ((float*)Var.Member)[1]);
            lua_pushnumber(LuaState, ((float*)Var.Member)[2]);
            lua_pushnumber(LuaState, ((float*)Var.Member)[3]);
            return 4;

        case MemberVarT::TYPE_INT:
            lua_pushinteger(LuaState, ((int*)Var.Member)[0]);
            return 1;

        case MemberVarT::TYPE_BOOL:
            lua_pushboolean(LuaState, ((bool*)Var.Member)[0]);
            return 1;

        case MemberVarT::TYPE_STRING:
            lua_pushstring(LuaState, ((std::string*)Var.Member)[0].c_str());
            return 1;
    }

    return 0;
}


int WindowT::Interpolate(lua_State* LuaState)
{
    ScriptBinderT     Binder(LuaState);
    IntrusivePtrT<WindowT> Win=Binder.GetCheckedObjectParam< IntrusivePtrT<WindowT> >(1);
    std::string       VarName=luaL_checkstring(LuaState, 2);
    const MemberVarT& Var    =Win->MemberVars[VarName];

    if (Var.Member==NULL)
    {
        // Bad argument "VarName".
        luaL_argerror(LuaState, 2, (std::string("unknown field '")+VarName+"'").c_str());
        return 0;
    }

    switch (Var.Type)
    {
        case MemberVarT::TYPE_FLOAT:
        {
            const unsigned long MAX_INTERPOLATIONS=10;
            unsigned long       InterpolationCount=0;

            // Make sure that there are no more than MAX_INTERPOLATIONS interpolations pending for Var already.
            // If so, just delete the oldest ones, which effectively means to skip them (the next youngest interpolation will take over).
            // The purpose is of course to prevent anything from adding arbitrarily many interpolations, eating up memory,
            // which could happen from bad user code (e.g. if the Cafu game code doesn't protect multiple human players from using
            // a GUI simultaneously, mouse cursor "position flickering" might occur on the server, which in turn might trigger the
            // permanent addition of interpolations from OnFocusLose()/OnFocusGain() scripts).
            for (unsigned long INr=Win->m_PendingInterp.Size(); INr>0; INr--)
            {
                InterpolationT* I=Win->m_PendingInterp[INr-1];

                if (I->Value==(float*)Var.Member) InterpolationCount++;

                if (InterpolationCount>MAX_INTERPOLATIONS)
                {
                    delete I;
                    Win->m_PendingInterp.RemoveAtAndKeepOrder(INr-1);
                    break;
                }
            }

            // Now add the new interpolation to the pending list.
            InterpolationT* I=new InterpolationT;

            I->Value      =(float*)Var.Member;
            I->StartValue =float(lua_tonumber(LuaState, 3));
            I->EndValue   =float(lua_tonumber(LuaState, 4));
            I->CurrentTime=0.0f;
            I->TotalTime  =float(lua_tonumber(LuaState, 5)/1000.0);

            Win->m_PendingInterp.PushBack(I);
            break;
        }

        default:
        {
            luaL_argerror(LuaState, 2, (std::string("Cannot interpolate over field '")+VarName+"', it is not of type 'float'.").c_str());
            break;
        }
    }

    return 0;
}


int WindowT::GetName(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<WindowT> Win=Binder.GetCheckedObjectParam< IntrusivePtrT<WindowT> >(1);

    lua_pushstring(LuaState, Win->GetName().c_str());
    return 1;
}


int WindowT::SetName(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<WindowT> Win=Binder.GetCheckedObjectParam< IntrusivePtrT<WindowT> >(1);

    Win->SetName(luaL_checkstring(LuaState, 2));
    return 0;
}


int WindowT::AddChild(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<WindowT> Win  =Binder.GetCheckedObjectParam< IntrusivePtrT<WindowT> >(1);
    IntrusivePtrT<WindowT> Child=Binder.GetCheckedObjectParam< IntrusivePtrT<WindowT> >(2);

    if (Child->m_Parent!=NULL)      // A child window must be a root node...
        return luaL_argerror(LuaState, 2, "child window already has a parent, use RemoveChild() first");

    if (Child==Win->GetRoot())      // ... but not the root of the hierarchy it is inserted into.
        return luaL_argerror(LuaState, 2, "a window cannot be made a child of itself");

    Win->m_Children.PushBack(Child);
    Child->m_Parent = Win.get();

    // Make sure that the childs name is unique among its siblings.
    Child->SetName(Child->GetName());
    return 0;
}


int WindowT::RemoveChild(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<WindowT> Parent=Binder.GetCheckedObjectParam< IntrusivePtrT<WindowT> >(1);
    IntrusivePtrT<WindowT> Child =Binder.GetCheckedObjectParam< IntrusivePtrT<WindowT> >(2);

    if (Child->m_Parent != Parent.get())
        return luaL_argerror(LuaState, 2, "window is the child of another parent");

    const int Index=Parent->m_Children.Find(Child);

    if (Index<0)
        return luaL_argerror(LuaState, 2, "window not found among the children of its parent");

    Parent->m_Children.RemoveAtAndKeepOrder(Index);
    Child->m_Parent=NULL;

    return 0;
}


int WindowT::GetParent(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<WindowT> Win=Binder.GetCheckedObjectParam< IntrusivePtrT<WindowT> >(1);

    if (Win->m_Parent)
    {
        // Be careful not to push the raw Win->m_Parent pointer here.
        Binder.Push(IntrusivePtrT<WindowT>(Win->m_Parent));
    }
    else
    {
        lua_pushnil(LuaState);
    }

    return 1;
}


int WindowT::GetChildren(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<WindowT> Win=Binder.GetCheckedObjectParam< IntrusivePtrT<WindowT> >(1);

    lua_newtable(LuaState);

    for (unsigned long ChildNr=0; ChildNr<Win->m_Children.Size(); ChildNr++)
    {
        Binder.Push(Win->m_Children[ChildNr]);
        lua_rawseti(LuaState, -2, ChildNr+1);
    }

    return 1;
}


int WindowT::GetTransform(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<WindowT> Win = Binder.GetCheckedObjectParam< IntrusivePtrT<WindowT> >(1);

    Binder.Push(Win->GetTransform());
    return 1;
}


int WindowT::AddComponent(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<WindowT> Win = Binder.GetCheckedObjectParam< IntrusivePtrT<WindowT> >(1);

    for (int i = 2; i <= lua_gettop(LuaState); i++)
    {
        IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(i);

        if (Comp->GetWindow())
            return luaL_argerror(LuaState, i, "the component is already a part of a window");

        assert(Win->m_Components.Find(Comp) < 0);

        Win->m_Components.PushBack(Comp);
    }

    // Now that the whole set of components has been added,
    // have the components re-resolve their dependencies among themselves.
    for (unsigned int CompNr = 0; CompNr < Win->m_Components.Size(); CompNr++)
        Win->m_Components[CompNr]->UpdateDependencies(Win.get());

    return 0;
}


int WindowT::RmvComponent(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<WindowT>        Win  = Binder.GetCheckedObjectParam< IntrusivePtrT<WindowT> >(1);
    IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(2);

    const int Index = Win->m_Components.Find(Comp);

    if (Index < 0)
        return luaL_argerror(LuaState, 2, "component not found in this window");

    Win->DeleteComponent(Index);
    return 0;
}


int WindowT::GetComponents(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<WindowT> Win = Binder.GetCheckedObjectParam< IntrusivePtrT<WindowT> >(1);

    lua_newtable(LuaState);

    for (unsigned long CompNr = 0; CompNr < Win->m_Components.Size(); CompNr++)
    {
        Binder.Push(Win->m_Components[CompNr]);
        lua_rawseti(LuaState, -2, CompNr+1);
    }

    return 1;
}


int WindowT::GetComponent(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<WindowT>        Win  = Binder.GetCheckedObjectParam< IntrusivePtrT<WindowT> >(1);
    IntrusivePtrT<ComponentBaseT> Comp = Win->GetComponent(luaL_checkstring(LuaState, 2), lua_tointeger(LuaState, 3));

    if (Comp == NULL) lua_pushnil(LuaState);
                 else Binder.Push(Comp);

    return 1;
}


int WindowT::toString(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<WindowT> Win=Binder.GetCheckedObjectParam< IntrusivePtrT<WindowT> >(1);

    lua_pushfstring(LuaState, "A gui window with name \"%s\".", Win->GetName().c_str());
    return 1;
}
