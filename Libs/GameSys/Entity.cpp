/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Entity.hpp"
#include "AllComponents.hpp"
#include "CompBase.hpp"
#include "EntityCreateParams.hpp"
#include "World.hpp"

#include "Network/State.hpp"
#include "TypeSys.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

#include <cassert>

#if defined(_WIN32) && _MSC_VER<1600
#include "pstdint.h"            // Paul Hsieh's portable implementation of the stdint.h header.
#else
#include <stdint.h>
#endif


using namespace cf::GameSys;


// Note that we cannot simply replace this method with a global TypeInfoManT instance,
// because it is called during global static initialization time. The TIM instance being
// embedded in the function guarantees that it is properly initialized before first use.
cf::TypeSys::TypeInfoManT& cf::GameSys::GetGameSysEntityTIM()
{
    static cf::TypeSys::TypeInfoManT TIM;

    return TIM;
}


const char* EntityT::DocClass =
    "An entity is the basic element in a game world.\n"
    "\n"
    "Entity can be hierarchically arranged in parent/child relationships, e.g. a player that rides a car.\n"
    "\n"
    "An entity is a separate unit that is self-contained and has its own identity, but has very little own features.\n"
    "Instead, an entity contains a set of components, each of which implements a specific feature for the entity.";


EntityT::EntityT(const EntityCreateParamsT& Params)
    : m_World(Params.World),
      m_ID(m_World.GetNextEntityID(Params.GetID())),
      m_Parent(NULL),
      m_Children(),
      m_App(NULL),
      m_Basics(new ComponentBasicsT()),
      m_Transform(new ComponentTransformT()),
      m_Components()
{
    m_Basics->UpdateDependencies(this);
    m_Transform->UpdateDependencies(this);

    // m_Transform->SetOrigin(Vector3fT(0.0f, 0.0f, 0.0f));
}


EntityT::EntityT(const EntityT& Entity, bool Recursive)
    : m_World(Entity.m_World),
      m_ID(m_World.GetNextEntityID()),
      m_Parent(NULL),
      m_Children(),
      m_App(NULL),
      m_Basics(Entity.GetBasics()->Clone()),
      m_Transform(Entity.GetTransform()->Clone()),
      m_Components()
{
    // Copy-create all components first.
    if (Entity.GetApp() != NULL)
        m_App = Entity.GetApp()->Clone();

    m_Components.PushBackEmptyExact(Entity.GetComponents().Size());

    for (unsigned int CompNr = 0; CompNr < Entity.GetComponents().Size(); CompNr++)
        m_Components[CompNr] = Entity.GetComponents()[CompNr]->Clone();

    // Now that all components have been copied, have them resolve their dependencies among themselves.
    UpdateAllDependencies();

    // Recursively copy the children.
    if (Recursive)
    {
        for (unsigned long ChildNr=0; ChildNr<Entity.m_Children.Size(); ChildNr++)
        {
            m_Children.PushBack(new EntityT(*Entity.m_Children[ChildNr], Recursive));
            m_Children[ChildNr]->m_Parent=this;
        }
    }
}


EntityT::~EntityT()
{
    // Cannot have a parent any more. Otherwise, the parent still had
    // us as a child, and we should not have gotten here for destruction.
    assert(m_Parent == NULL);

    // Cleanly disconnect our children.
    for (unsigned long ChildNr=0; ChildNr<m_Children.Size(); ChildNr++)
        m_Children[ChildNr]->m_Parent = NULL;

    m_Children.Clear();

    // Delete the components.
    for (unsigned int CompNr = 0; CompNr < m_Components.Size(); CompNr++)
    {
        // No one else should still have a pointer to m_Components[CompNr] at this point.
        // Nevertheless, it is still possible that m_Components[CompNr]->GetRefCount() > 1,
        // for example if a script still keeps a reference to the component, or has kept
        // one that is not yet garbage collected.
        // To be safe, make sure that the component no longer refers back to this entity.
        m_Components[CompNr]->UpdateDependencies(NULL);
    }

    m_Components.Clear();

    if (!m_App.IsNull()) m_App->UpdateDependencies(NULL);
    m_Basics->UpdateDependencies(NULL);
    m_Transform->UpdateDependencies(NULL);
}


bool EntityT::AddChild(IntrusivePtrT<EntityT> Child, unsigned long Pos)
{
    assert(Child->m_Parent == NULL);
    if (Child->m_Parent != NULL)    // A child entity must be a root node...
        return false;   // luaL_argerror(LuaState, 2, "child entity already has a parent, use RemoveChild() first");

    assert(Child != GetRoot());
    if (Child == GetRoot())         // ... but not the root of the hierarchy it is inserted into.
        return false;   // luaL_argerror(LuaState, 2, "an entity cannot be made a child of itself");

    m_Children.InsertAt(std::min(m_Children.Size(), Pos), Child);
    Child->m_Parent = this;

    // Make sure that the childs name is unique among its siblings.
    // The method must be tricked in order to prevent its no-change shortcut erroneously skipping the checks.
    const std::string ChildName = Child->GetBasics()->GetEntityName();

    Child->GetBasics()->SetEntityName("");
    Child->GetBasics()->SetEntityName(ChildName);

    return true;
}


bool EntityT::RemoveChild(IntrusivePtrT<EntityT> Child)
{
    assert(Child->m_Parent == this);
    if (Child->m_Parent != this)
        return false;   // luaL_argerror(LuaState, 2, "entity is the child of another parent");

    const int Index = m_Children.Find(Child);

    assert(Index >= 0);
    if (Index < 0)
        return false;   // return luaL_argerror(LuaState, 2, "entity not found among the children of its parent");

    m_Children.RemoveAtAndKeepOrder(Index);
    Child->m_Parent = NULL;
    return true;
}


void EntityT::GetChildren(ArrayT< IntrusivePtrT<EntityT> >& Chld, bool Recurse) const
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


void EntityT::GetAll(ArrayT< IntrusivePtrT<EntityT> >& List)
{
    List.PushBack(this);

    for (unsigned int ChildNr = 0; ChildNr < m_Children.Size(); ChildNr++)
        m_Children[ChildNr]->GetAll(List);
}


IntrusivePtrT<EntityT> EntityT::GetRoot()
{
    EntityT* Root = this;

    while (Root->m_Parent)
        Root = Root->m_Parent;

    return Root;
}


void EntityT::SetApp(IntrusivePtrT<ComponentBaseT> App)
{
    if (m_App == App) return;

    if (!m_App.IsNull()) m_App->UpdateDependencies(NULL);

    m_App = App;

    UpdateAllDependencies();
}


IntrusivePtrT<ComponentBaseT> EntityT::GetComponent(const std::string& TypeName, unsigned int n) const
{
    if (m_App != NULL && TypeName == m_App->GetName())
    {
        if (n == 0) return m_App;
        n--;
    }

    if (TypeName == m_Basics->GetName())
    {
        if (n == 0) return m_Basics;
        n--;
    }

    if (TypeName == m_Transform->GetName())
    {
        if (n == 0) return m_Transform;
        n--;
    }

    for (unsigned int CompNr = 0; CompNr < m_Components.Size(); CompNr++)
        if (m_Components[CompNr]->GetName() == TypeName)
        {
            if (n == 0) return m_Components[CompNr];
            n--;
        }

    return NULL;
}


IntrusivePtrT<ComponentBaseT> EntityT::GetComponent(unsigned int n) const
{
    if (m_App != NULL)
    {
        if (n == 0) return m_App;
        n--;
    }

    if (n == 0) return m_Basics;
    n--;

    if (n == 0) return m_Transform;
    n--;

    return n < m_Components.Size() ? m_Components[n] : NULL;
}


bool EntityT::AddComponent(IntrusivePtrT<ComponentBaseT> Comp, unsigned long Index)
{
    if (Comp->GetEntity()) return false;
    assert(m_Components.Find(Comp) < 0);

    m_Components.InsertAt(std::min(Index, m_Components.Size()), Comp);

    // Have the components re-resolve their dependencies among themselves.
    UpdateAllDependencies();

    return true;
}


void EntityT::DeleteComponent(unsigned long CompNr)
{
    // Let the component know that it is no longer a part of this entity.
    m_Components[CompNr]->UpdateDependencies(NULL);

    m_Components.RemoveAtAndKeepOrder(CompNr);

    // Have the remaining components re-resolve their dependencies among themselves.
    UpdateAllDependencies();
}


IntrusivePtrT<EntityT> EntityT::FindID(unsigned int WantedID)
{
    if (WantedID == m_ID) return this;

    // Recursively see if any of the children has the desired name.
    for (unsigned int ChildNr = 0; ChildNr < m_Children.Size(); ChildNr++)
    {
        IntrusivePtrT<EntityT> Ent = m_Children[ChildNr]->FindID(WantedID);

        if (Ent != NULL) return Ent;
    }

    return NULL;
}


IntrusivePtrT<EntityT> EntityT::Find(const std::string& WantedName)
{
    if (WantedName == m_Basics->GetEntityName()) return this;

    // Recursively see if any of the children has the desired name.
    for (unsigned long ChildNr=0; ChildNr<m_Children.Size(); ChildNr++)
    {
        IntrusivePtrT<EntityT> Ent = m_Children[ChildNr]->Find(WantedName);

        if (Ent != NULL) return Ent;
    }

    return NULL;
}


void EntityT::FindByComponent(const std::string& TypeName, ArrayT< IntrusivePtrT<EntityT> >& Result)
{
    if (GetComponent(TypeName) != NULL)
        Result.PushBack(this);

    for (unsigned int ChildNr = 0; ChildNr < m_Children.Size(); ChildNr++)
        m_Children[ChildNr]->FindByComponent(TypeName, Result);
}


bool EntityT::Has(IntrusivePtrT<EntityT> Ent) const
{
    while (Ent != NULL)
    {
        if (Ent == this) return true;

        Ent = Ent->GetParent();
    }

    return false;
}


BoundingBox3fT EntityT::GetCullingBB(bool WorldSpace) const
{
    BoundingBox3fT BB;

    // We know that m_Basics and m_Transform have no visual representation, but m_App may have.
    if (m_App != NULL)
    {
        const BoundingBox3fT CompBB = m_App->GetCullingBB();

        if (CompBB.IsInited())
            BB += CompBB;
    }

    for (unsigned int CompNr = 0; CompNr < m_Components.Size(); CompNr++)
    {
        const BoundingBox3fT CompBB = m_Components[CompNr]->GetCullingBB();

        if (CompBB.IsInited())
            BB += CompBB;
    }

    if (!WorldSpace) return BB;
    if (!BB.IsInited()) return BB;

    // Transform BB from entity-space to world-space.
    Vector3fT     Corners[8];
    const MatrixT etw = m_Transform->GetEntityToWorld();

    BB.GetCornerVertices(Corners);
    BB = BoundingBox3fT(etw.Mul1(Corners[0]));

    for (unsigned int i = 1; i < 8; i++)
        BB += etw.Mul1(Corners[i]);

    return BB;
}


// Note that this method is the twin of Deserialize(), whose implementation it must match.
void EntityT::Serialize(cf::Network::OutStreamT& Stream, bool WithChildren) const
{
    m_Basics->Serialize(Stream);
    m_Transform->Serialize(Stream);
    if (m_App != NULL) m_App->Serialize(Stream);       // Don't serialize anything that is application-specific, but see below for why we do it anyway.

    // Serialize the "custom" components.
    assert(m_Components.Size() < 256);
    Stream << uint8_t(m_Components.Size());

    for (unsigned int CompNr = 0; CompNr < m_Components.Size(); CompNr++)
    {
        assert(m_Components[CompNr]->GetType()->TypeNr < 256);
        Stream << uint8_t(m_Components[CompNr]->GetType()->TypeNr);

        m_Components[CompNr]->Serialize(Stream);
    }

    // Recursively serialize the children (if requested).
    Stream << WithChildren;

    if (WithChildren)
    {
        Stream << uint32_t(m_Children.Size());

        for (unsigned int ChildNr = 0; ChildNr < m_Children.Size(); ChildNr++)
            m_Children[ChildNr]->Serialize(Stream, WithChildren);
    }
}


// Note that this method is the twin of Serialize(), whose implementation it must match.
void EntityT::Deserialize(cf::Network::InStreamT& Stream, bool IsIniting)
{
    m_Basics->Deserialize(Stream, IsIniting);
    m_Transform->Deserialize(Stream, IsIniting);

    // This is really not what we want:
    //   1. Normally, we shouldn't serialize anything that is application-specific.
    //   2. Breaks our implicit convention to handle m_App before m_Basics and m_Transform.
    //   3. What if m_App != NULL on the server and == NULL on the client, or vice versa?
    // However, we need it so that CompGameEntityT can update its clip models, just like and ComponentCollisionModelT
    // (and thus it must be done *after* deserializing m_Transform).
    // The right solution would be to add some PostDeserialize() method instead (and some PostThink() as well).
    if (m_App != NULL) m_App->Deserialize(Stream, IsIniting);


    // Deserialize the "custom" components.
    uint8_t NumComponents = 0;
    Stream >> NumComponents;

    while (m_Components.Size() > NumComponents)
    {
        // Remove any extra components, updating the dependencies as required.
        // (This is not efficient whenever multiple components are removed...)
        DeleteComponent(m_Components.Size() - 1);
    }

    for (unsigned int CompNr = 0; CompNr < NumComponents; CompNr++)
    {
        uint8_t CompTypeNr = 0;
        Stream >> CompTypeNr;

        bool IsNew = false;

        if (CompNr >= m_Components.Size())
        {
            // Note that if `TI == NULL`, there really is not much that we can do.
            const cf::TypeSys::TypeInfoT* TI = GetComponentTIM().FindTypeInfoByNr(CompTypeNr);
            IntrusivePtrT<ComponentBaseT> Comp(static_cast<ComponentBaseT*>(TI->CreateInstance(cf::TypeSys::CreateParamsT())));

            // Add the component, updating the dependencies as required.
            // (This is not efficient whenever multiple components are added...)
            AddComponent(Comp);
            IsNew = true;
        }

        if (m_Components[CompNr]->GetType()->TypeNr != CompTypeNr)
        {
            // Note that if `TI == NULL`, there really is not much that we can do.
            const cf::TypeSys::TypeInfoT* TI = GetComponentTIM().FindTypeInfoByNr(CompTypeNr);
            IntrusivePtrT<ComponentBaseT> Comp(static_cast<ComponentBaseT*>(TI->CreateInstance(cf::TypeSys::CreateParamsT())));

            // Replace the component, updating the dependencies as required.
            // (This is not efficient whenever multiple components are replaced...)
            DeleteComponent(CompNr);
            AddComponent(Comp, CompNr);
            IsNew = true;
        }

        m_Components[CompNr]->Deserialize(Stream, IsIniting);

        if (IsNew)
        {
            // The component was newly added to an entity that exists in a live map.
            // Consequently, we must run the post-load stuff here.
            m_Components[CompNr]->OnPostLoad(false);
            m_Components[CompNr]->CallLuaMethod("OnInit", 0);
        }
    }


    // Recursively deserialize the children (if requested).
    bool WithChildren = false;
    Stream >> WithChildren;

    if (WithChildren)
    {
        uint32_t NumChildren = 0;
        Stream >> NumChildren;

        while (m_Children.Size() > NumChildren)
            m_Children.DeleteBack();

        for (unsigned int ChildNr = 0; ChildNr < NumChildren; ChildNr++)
        {
            if (ChildNr >= m_Children.Size())
            {
                IntrusivePtrT<EntityT> NewEnt = new EntityT(EntityCreateParamsT(m_World));

                m_Children.PushBack(NewEnt);
            }

            m_Children[ChildNr]->Deserialize(Stream, IsIniting);
        }
    }
}


bool EntityT::RenderComponents(bool FirstPersonView, float LodDist) const
{
    bool b = false;

    /*
     * Note that operator || short-circuits! Therefore:
     *
     * b = b || Render();   // Wrong, possibly doesn't call Render()!
     * b = Render() || b;   // Ok, calls Render().
     */

    // Render the "custom" components in the proper order -- bottom-up.
    for (unsigned long CompNr = m_Components.Size(); CompNr > 0; CompNr--)
        b = m_Components[CompNr-1]->Render(FirstPersonView, LodDist) || b;

    // Render the "fixed" components.
    // b = m_Transform->Render(FirstPersonView, LodDist) || b;
    // b = m_Basics->Render(FirstPersonView, LodDist) || b;
    if (m_App != NULL) b = m_App->Render(FirstPersonView, LodDist) || b;

    return b;
}


void EntityT::OnServerFrame(float t)
{
    // Forward the event to the "fixed" components (or else they cannot interpolate).
    if (m_App != NULL) m_App->OnServerFrame(t);
    m_Basics->OnServerFrame(t);
    m_Transform->OnServerFrame(t);

    // Forward the event to the "custom" components.
    for (unsigned int CompNr = 0; CompNr < m_Components.Size(); CompNr++)
        m_Components[CompNr]->OnServerFrame(t);
}


void EntityT::OnClientFrame(float t)
{
    // Forward the event to the "fixed" components (or else they cannot interpolate).
    if (m_App != NULL) m_App->OnClientFrame(t);
    m_Basics->OnClientFrame(t);
    m_Transform->OnClientFrame(t);

    // Forward the event to the "custom" components.
    for (unsigned int CompNr = 0; CompNr < m_Components.Size(); CompNr++)
        m_Components[CompNr]->OnClientFrame(t);
}


bool EntityT::CallLuaMethod(const char* MethodName, int NumExtraArgs, const char* Signature, ...)
{
    va_list vl;

    va_start(vl, Signature);
    const bool Result=m_World.GetScriptState().CallMethod_Impl(IntrusivePtrT<EntityT>(this), MethodName, NumExtraArgs, Signature, vl);
    va_end(vl);

    return Result;
}


void EntityT::UpdateAllDependencies()
{
    if (m_App != NULL)
        m_App->UpdateDependencies(this);

    m_Basics->UpdateDependencies(this);
    m_Transform->UpdateDependencies(this);

    for (unsigned int CompNr = 0; CompNr < m_Components.Size(); CompNr++)
        m_Components[CompNr]->UpdateDependencies(this);
}


/***********************************************/
/*** Implementation of Lua binding functions ***/
/***********************************************/

static const cf::TypeSys::MethsDocT META_GetID =
{
    "GetID",
    "Returns the ID of this entity.\n"
    "The ID is unique in the world, and is used (in C++ code) to unambiguously identify\n"
    "the entity in network messages and as entity index number into `.cw` world files.\n",
    "number", "()"
};

int EntityT::GetID(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntityT> Ent = Binder.GetCheckedObjectParam< IntrusivePtrT<EntityT> >(1);

    lua_pushinteger(LuaState, Ent->GetID());
    return 1;
}


static const cf::TypeSys::MethsDocT META_AddChild =
{
    "AddChild",
    "This method adds the given entity to the children of this entity.",
    "", "(entity child)"
};

int EntityT::AddChild(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntityT> Ent   = Binder.GetCheckedObjectParam< IntrusivePtrT<EntityT> >(1);
    IntrusivePtrT<EntityT> Child = Binder.GetCheckedObjectParam< IntrusivePtrT<EntityT> >(2);

    if (Child->m_Parent != NULL)    // A child entity must be a root node...
        return luaL_argerror(LuaState, 2, "child entity already has a parent, use RemoveChild() first");

    if (Child == Ent->GetRoot())    // ... but not the root of the hierarchy it is inserted into.
        return luaL_argerror(LuaState, 2, "an entity cannot be made a child of itself");

    Ent->m_Children.PushBack(Child);
    Child->m_Parent = Ent.get();

    // Make sure that the childs name is unique among its siblings.
    // The method must be tricked in order to prevent its no-change shortcut erroneously skipping the checks.
    const std::string ChildName = Child->GetBasics()->GetEntityName();

    Child->GetBasics()->SetEntityName("");
    Child->GetBasics()->SetEntityName(ChildName);

    return 0;
}


static const cf::TypeSys::MethsDocT META_RemoveChild =
{
    "RemoveChild",
    "This method removes the given entity from the children of this entity.\n"
    "@param child   The entity that is to be removed from the children of this entity.",
    "", "(entity child)"
};

int EntityT::RemoveChild(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntityT> Parent=Binder.GetCheckedObjectParam< IntrusivePtrT<EntityT> >(1);
    IntrusivePtrT<EntityT> Child =Binder.GetCheckedObjectParam< IntrusivePtrT<EntityT> >(2);

    if (Child->m_Parent != Parent.get())
        return luaL_argerror(LuaState, 2, "entity is the child of another parent");

    const int Index=Parent->m_Children.Find(Child);

    if (Index<0)
        return luaL_argerror(LuaState, 2, "entity not found among the children of its parent");

    Parent->m_Children.RemoveAtAndKeepOrder(Index);
    Child->m_Parent=NULL;

    return 0;
}


static const cf::TypeSys::MethsDocT META_GetParent =
{
    "GetParent",
    "This method returns the parent of this entity (or `nil` if there is no parent).",
    "EntityT", "()"
};

int EntityT::GetParent(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntityT> Ent = Binder.GetCheckedObjectParam< IntrusivePtrT<EntityT> >(1);

    if (Ent->m_Parent)
    {
        // Be careful not to push the raw Ent->m_Parent pointer here.
        Binder.Push(IntrusivePtrT<EntityT>(Ent->m_Parent));
    }
    else
    {
        lua_pushnil(LuaState);
    }

    return 1;
}


static const cf::TypeSys::MethsDocT META_GetRoot =
{
    "GetRoot",
    "Returns the top-most parent of this entity, that is, the root of the hierarchy that this entity is in.",
    "EntityT", "()"
};

int EntityT::GetRoot(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntityT> Ent = Binder.GetCheckedObjectParam< IntrusivePtrT<EntityT> >(1);

    Binder.Push(Ent->GetRoot());
    return 1;
}


static const cf::TypeSys::MethsDocT META_GetChildren =
{
    "GetChildren",
    "This method returns an array of the children of this entity.",
    "table", "()"
};

int EntityT::GetChildren(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntityT> Ent = Binder.GetCheckedObjectParam< IntrusivePtrT<EntityT> >(1);

    lua_newtable(LuaState);

    for (unsigned long ChildNr=0; ChildNr<Ent->m_Children.Size(); ChildNr++)
    {
        Binder.Push(Ent->m_Children[ChildNr]);
        lua_rawseti(LuaState, -2, ChildNr+1);
    }

    return 1;
}


static const cf::TypeSys::MethsDocT META_FindByID =
{
    "FindByID",
    "Finds the entity with the given ID in the hierachy tree of this entity.\n"
    "Use `GetRoot():Find(xy)` in order to search the entire world for the entity with ID `xy`.\n"
    "@param ID   The ID of the entity that is to be found.\n"
    "@returns The entity with the desired ID, or `nil` if no entity with this ID exists.\n",
    "EntityT", "(number ID)"
};

int EntityT::FindByID(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntityT> Ent = Binder.GetCheckedObjectParam< IntrusivePtrT<EntityT> >(1);
    IntrusivePtrT<EntityT> Res = Ent->FindID(luaL_checkint(LuaState, 2));

    if (Res != NULL)
    {
        Binder.Push(Res);
    }
    else
    {
        lua_pushnil(LuaState);
    }

    return 1;
}


static const cf::TypeSys::MethsDocT META_FindByName =
{
    "FindByName",
    "Finds the entity with the given name in the hierachy tree of this entity.\n"
    "Use `GetRoot():Find(\"xy\")` in order to search the entire world for the entity with name `xy`.\n"
    "@param Name   The name of the entity that is to be found.\n"
    "@returns The entity with the desired name, or `nil` if no entity with this name exists.\n",
    "EntityT", "(string Name)"
};

int EntityT::FindByName(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntityT> Ent = Binder.GetCheckedObjectParam< IntrusivePtrT<EntityT> >(1);
    IntrusivePtrT<EntityT> Res = Ent->Find(luaL_checkstring(LuaState, 2));

    if (Res != NULL)
    {
        Binder.Push(Res);
    }
    else
    {
        lua_pushnil(LuaState);
    }

    return 1;
}


static const cf::TypeSys::MethsDocT META_FindByComponent =
{
    "FindByComponent",
    "Finds all entities in the hierachy tree of this entity that have at least one component of the given (type) name.\n"
    "Use `GetRoot():FindByComponent(\"xy\")` in order to search the entire world for entities with component `xy`.\n"
    "\n"
    "@param TypeName   The type name of the component that found entities must have\n"
    "@returns The array of entities that have a component of the desired type.\n",
    "table", "(string TypeName)"
};

int EntityT::FindByComponent(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntityT> Ent = Binder.GetCheckedObjectParam< IntrusivePtrT<EntityT> >(1);

    ArrayT< IntrusivePtrT<EntityT> > Result;
    Ent->FindByComponent(luaL_checkstring(LuaState, 2), Result);

    lua_newtable(LuaState);

    for (unsigned long EntNr = 0; EntNr < Result.Size(); EntNr++)
    {
        Binder.Push(Result[EntNr]);
        lua_rawseti(LuaState, -2, EntNr + 1);
    }

    return 1;
}


static const cf::TypeSys::MethsDocT META_GetBasics =
{
    "GetBasics",
    "This method returns the \"Basics\" component of this entity.",
    "ComponentBasicsT", "()"
};

int EntityT::GetBasics(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntityT> Ent = Binder.GetCheckedObjectParam< IntrusivePtrT<EntityT> >(1);

    Binder.Push(Ent->GetBasics());
    return 1;
}


static const cf::TypeSys::MethsDocT META_GetTransform =
{
    "GetTransform",
    "This method returns the \"Transform\" component of this entity.",
    "ComponentTransformT", "()"
};

int EntityT::GetTransform(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntityT> Ent = Binder.GetCheckedObjectParam< IntrusivePtrT<EntityT> >(1);

    Binder.Push(Ent->GetTransform());
    return 1;
}


static const cf::TypeSys::MethsDocT META_AddComponent =
{
    "AddComponent",
    "This method adds a component to this entity.",
    "", "(ComponentBaseT component)"
};

int EntityT::AddComponent(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntityT> Ent = Binder.GetCheckedObjectParam< IntrusivePtrT<EntityT> >(1);

    for (int i = 2; i <= lua_gettop(LuaState); i++)
    {
        IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(i);

        if (Comp->GetEntity())
            return luaL_argerror(LuaState, i, "the component is already a part of an entity");

        assert(Ent->m_Components.Find(Comp) < 0);

        Ent->m_Components.PushBack(Comp);
    }

    // Now that the whole set of components has been added,
    // have the components re-resolve their dependencies among themselves.
    Ent->UpdateAllDependencies();

    return 0;
}


static const cf::TypeSys::MethsDocT META_RemoveComponent =
{
    "RemoveComponent",
    "This method removes a component from this entity.",
    "", "(ComponentBaseT component)"
};

int EntityT::RmvComponent(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntityT>        Ent  = Binder.GetCheckedObjectParam< IntrusivePtrT<EntityT> >(1);
    IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(2);

    const int Index = Ent->m_Components.Find(Comp);

    if (Index < 0)
        return luaL_argerror(LuaState, 2, "component not found in this entity");

    Ent->DeleteComponent(Index);
    return 0;
}


static const cf::TypeSys::MethsDocT META_GetComponents =
{
    "GetComponents",
    "This method returns an array of the components of this entity.",
    "table", "()"
};

int EntityT::GetComponents(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntityT> Ent = Binder.GetCheckedObjectParam< IntrusivePtrT<EntityT> >(1);

    lua_newtable(LuaState);

    for (unsigned long CompNr = 0; CompNr < Ent->m_Components.Size(); CompNr++)
    {
        Binder.Push(Ent->m_Components[CompNr]);
        lua_rawseti(LuaState, -2, CompNr+1);
    }

    return 1;
}


static const cf::TypeSys::MethsDocT META_GetComponent =
{
    "GetComponent",
    "This method returns the (n-th) component of the given (type) name.\n"
    "Covers the \"custom\" components as well as the application components, \"Basics\" and \"Transform\".\n"
    "That is, `GetComponent(\"Basics\") == GetBasics()` and `GetComponent(\"Transform\") == GetTransform()`.\n"
    "@param type_name   The (type) name of the component to get, e.g. \"Image\".\n"
    "@param n           This parameter is optional, it defaults to 0 if not given.",
    "ComponentBaseT", "(string type_name, number n)"
};

int EntityT::GetComponent(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntityT>        Ent  = Binder.GetCheckedObjectParam< IntrusivePtrT<EntityT> >(1);
    IntrusivePtrT<ComponentBaseT> Comp = Ent->GetComponent(luaL_checkstring(LuaState, 2), lua_tointeger(LuaState, 3));

    if (Comp == NULL) lua_pushnil(LuaState);
                 else Binder.Push(Comp);

    return 1;
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int EntityT::toString(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<EntityT> Ent = Binder.GetCheckedObjectParam< IntrusivePtrT<EntityT> >(1);

    lua_pushfstring(LuaState, "A game entity with name \"%s\".", Ent->GetBasics()->GetEntityName().c_str());
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* EntityT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new EntityT(*static_cast<const cf::GameSys::EntityCreateParamsT*>(&Params));
}

const luaL_Reg EntityT::MethodsList[]=
{
    { "GetID",           GetID },
    { "AddChild",        AddChild },
    { "RemoveChild",     RemoveChild },
    { "GetParent",       GetParent },
    { "GetRoot",         GetRoot },
    { "GetChildren",     GetChildren },
    { "FindByID",        FindByID },
    { "FindByName",      FindByName },
    { "FindByComponent", FindByComponent },
    { "GetBasics",       GetBasics },
    { "GetTransform",    GetTransform },
    { "AddComponent",    AddComponent },
    { "RemoveComponent", RmvComponent },
    { "GetComponents",   GetComponents },
    { "GetComponent",    GetComponent },
    { "__tostring",      toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT EntityT::DocMethods[] =
{
    META_GetID,
    META_AddChild,
    META_RemoveChild,
    META_GetParent,
    META_GetRoot,
    META_GetChildren,
    META_FindByID,
    META_FindByName,
    META_FindByComponent,
    META_GetBasics,
    META_GetTransform,
    META_AddComponent,
    META_RemoveComponent,
    META_GetComponents,
    META_GetComponent,
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::MethsDocT EntityT::DocCallbacks[] =
{
    { "OnInit",
      "This method is called for each entity when a new world is loaded.\n"
      "Note that these OnInit() methods are automatically written by the Cafu Map Editor\n"
      "into a world's `.cent` files, you normally don't write them yourself.\n"
      "Use the ComponentBaseT::OnInit() methods instead for custom dynamic initialization.",
      "", "()" },
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT EntityT::TypeInfo(
    GetGameSysEntityTIM(),
    "GameSys::EntityT",
    NULL /*No base class.*/,
    EntityT::CreateInstance,
    MethodsList,
    DocClass,
    DocMethods,
    DocCallbacks);
