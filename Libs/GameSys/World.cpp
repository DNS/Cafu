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

#include "World.hpp"
#include "AllComponents.hpp"
#include "Entity.hpp"
#include "EntityCreateParams.hpp"

#include "ClipSys/ClipWorld.hpp"
#include "ClipSys/TraceResult.hpp"
#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/Console_Lua.hpp"
#include "ConsoleCommands/ConsoleInterpreter.hpp"
#include "MaterialSystem/Material.hpp"


using namespace cf::GameSys;


// Note that we cannot simply replace this method with a global TypeInfoManT instance,
// because it is called during global static initialization time. The TIM instance being
// embedded in the function guarantees that it is properly initialized before first use.
cf::TypeSys::TypeInfoManT& cf::GameSys::GetWorldTIM()
{
    static cf::TypeSys::TypeInfoManT TIM;

    return TIM;
}


const char* WorldT::DocClass =
    "This class holds the hierarchy of game entities that populate a game world.\n"
    "The root of the hierarchy is the map entity, all other entities are direct or indirect children of it.";


WorldT::InitErrorT::InitErrorT(const std::string& Message)
    : std::runtime_error(Message)
{
}


/*static*/ void WorldT::InitScriptState(cf::UniScriptStateT& ScriptState)
{
    lua_State* LuaState = ScriptState.GetLuaState();

    // Load the console library. (Adds a global table with name "Console" to the LuaState with the functions of the ConsoleI interface.)
    cf::Console_RegisterLua(LuaState);

    // Load the "ci" (console interpreter) library. (Adds a global table with name "ci" to the LuaState with (some of) the functions of the ConsoleInterpreterI interface.)
    ConsoleInterpreterI::RegisterLua(LuaState);

    // For each class that the TypeInfoManTs know about, add a (meta-)table to the registry of the LuaState.
    // The (meta-)table holds the Lua methods that the respective class implements in C++,
    // and is to be used as metatable for instances of this class.
    ScriptBinderT Binder(LuaState);

    Binder.Init(GetWorldTIM());
    Binder.Init(GetGameSysEntityTIM());
    Binder.Init(GetComponentTIM());
}


/*static*/ void WorldT::LoadScript(IntrusivePtrT<WorldT> World, const std::string& ScriptName, int Flags)
{
    const std::string PrintScriptName((Flags & InitFlag_InlineCode) ? "<inline code>" : ScriptName);
    lua_State*        LuaState = World->GetScriptState().GetLuaState();
    ScriptBinderT     Binder(LuaState);

    // Add a global variable with name "world" to the Lua state.
    Binder.Push(World);
    lua_setglobal(LuaState, "world");


    // Load the user script!
    const int LoadResult = (Flags & InitFlag_InlineCode) ? luaL_loadstring(LuaState, ScriptName.c_str())
                                                         : luaL_loadfile  (LuaState, ScriptName.c_str());

    if (LoadResult != 0 || lua_pcall(LuaState, 0, 0, 0) != 0)
    {
        const std::string Msg = "Could not load \"" + PrintScriptName + "\":\n" + lua_tostring(LuaState, -1);

        Console->Warning(Msg + "\n");

        // The LuaState will be closed by the m_ScriptState.
        throw InitErrorT(Msg);
    }

    if (World->GetRootEntity() == NULL)
    {
        const std::string Msg = "No root entity set in world \"" + PrintScriptName + "\".";

        Console->Warning(Msg + "\n");

        // The LuaState will be closed by the m_ScriptState.
        throw InitErrorT(Msg);
    }

    // Make sure that everyone dealt properly with the Lua stack so far.
    assert(lua_gettop(LuaState) == 0);


    // Finally call the Lua OnInit() and OnInit2() methods of each entity.
    ArrayT< IntrusivePtrT<EntityT> > AllEnts;
    World->GetRootEntity()->GetAll(AllEnts);

    World->Init();  // The script has the option to call this itself (via world:Init()) at an earlier time.

    for (unsigned long EntNr = 0; EntNr < AllEnts.Size(); EntNr++)
    {
        // Script entities that are directly loaded from `.cent` files (as opposed to those that are created
        // dynamically throughout the game) are normally associated to static entity data from the related
        // `.cw` world file.
        // For such entities, the Cafu Engine co-uses the entity ID as an index into the array of static entity data.
        // Especially when the Cafu game client receives a new entity ID over the network, it uses the ID to lookup
        // the static entity data with a statement like `cw_World.m_StaticEntityData[EntityID]`
        // (if `EntityID < World.m_StaticEntityData.Size()`).
        // As by convention we "align" entities from `.cent` files to their counterparts in `.cw` files by traversing
        // them in depth-first order, the co-use of entity IDs implies that the IDs must match the entity's ordinal
        // number in a depth first traversal. Make sure here that this condition is actually met.
        // Note that the main motivation here is catching program bugs, like an assert() statement that is also checked
        // in release builds. Although not impossible, it's very unlikely that a user action will ever trigger this.
        if (AllEnts[EntNr]->GetID() != EntNr)
        {
            const std::string Msg = "ID of entity \"" + AllEnts[EntNr]->GetBasics()->GetEntityName() +
                cf::va("\" is %u, expected %lu.\n", AllEnts[EntNr]->GetID(), EntNr) +
                "(Are the entities in \"" + PrintScriptName + "\" created in depth-first order?)";

            Console->Warning(Msg + "\n");

            // The LuaState will be closed by the m_ScriptState.
            throw InitErrorT(Msg);
        }

        // The OnInit2() methods contain custom, hand-written code by the user (*_main.cgui files).
        AllEnts[EntNr]->CallLuaMethod("OnInit2", 0);

        // Let each component know that the "static" part of initialization is now complete.
        const ArrayT< IntrusivePtrT<ComponentBaseT> >& Components = AllEnts[EntNr]->GetComponents();

        for (unsigned int CompNr = 0; CompNr < Components.Size(); CompNr++)
            Components[CompNr]->OnPostLoad((Flags & InitFlag_InMapEditor) != 0);
    }


    // Make sure that everyone dealt properly with the Lua stack so far.
    assert(lua_gettop(LuaState) == 0);
}


WorldT::WorldT(cf::UniScriptStateT& ScriptState, ModelManagerT& ModelMan, cf::GuiSys::GuiResourcesT& GuiRes,
               cf::ClipSys::CollModelManI& CollModelMan, cf::ClipSys::ClipWorldT* ClipWorld)
    : m_ScriptState(ScriptState),
      m_RootEntity(NULL),
      m_IsInited(false),
      m_NextEntID(0),
      m_ModelMan(ModelMan),
      m_GuiResources(GuiRes),
      m_CollModelMan(CollModelMan),
      m_ClipWorld(ClipWorld)
{
}


void WorldT::Init()
{
    if (m_IsInited) return;

    ArrayT< IntrusivePtrT<EntityT> > AllChildren;

    AllChildren.PushBack(m_RootEntity);
    m_RootEntity->GetChildren(AllChildren, true);

    for (unsigned long ChildNr = 0; ChildNr < AllChildren.Size(); ChildNr++)
    {
        // The OnInit() methods are automatically written by the Cafu Map Editor (*_init.cgui files).
        AllChildren[ChildNr]->CallLuaMethod("OnInit", 0);
    }

    m_IsInited = true;
}


unsigned int WorldT::GetNextEntityID(unsigned int ForcedID)
{
    if (ForcedID != UINT_MAX)
    {
        if (ForcedID >= m_NextEntID)
            m_NextEntID = std::max(m_NextEntID, ForcedID) + 1;

        // Make sure that an entity with ID `ForcedID` does not yet exist.
        assert(m_RootEntity.IsNull() || m_RootEntity->FindID(ForcedID).IsNull());

        return ForcedID;
    }

    return m_NextEntID++;
}


void WorldT::Render() const
{
    // Well... is WorldT::Render() really a useful method?
    // Contrary to the windows in a GUI, it seems like our entities are organized for special-purposes in
    // "draw worlds", "physics worlds", "sound worlds", "editor worlds", etc., so that having a Render()
    // method here seems inappropriate. (We might consider this WorldT as a "script world".)

    // m_RootEntity->Render();
}


bool WorldT::ProcessDeviceEvent(const CaKeyboardEventT& KE)
{
    return false;
}


bool WorldT::ProcessDeviceEvent(const CaMouseEventT& ME)
{
    return false;
}


/**
 * At the time of this writing, this method is only called from the Map Editor's
 * ViewWindow3DT::OnPaint() method, because in the Cafu Engine, the `Ca3DEWorldT`s
 * keep explicit lists of entities and call the OnClientFrame() method on them directly
 * (in CaClientWorldT::PostDrawEntities()).
 *
 * As a result, m_ScriptState.RunPendingCoroutines() is currently *not* called for
 * client worlds in the Cafu Engine!
 *
 * Note that on the server side, all this is accounted for by CaServerWorldT::Think()
 * and EngineEntityT::Think() already.
 */
void WorldT::OnClientFrame(float t)
{
    ArrayT< IntrusivePtrT<EntityT> > AllChildren;

    AllChildren.PushBack(m_RootEntity);
    m_RootEntity->GetChildren(AllChildren, true);

    for (unsigned long ChildNr = 0; ChildNr < AllChildren.Size(); ChildNr++)
        AllChildren[ChildNr]->OnClientFrame(t);

    // Run the pending coroutines.
    m_ScriptState.RunPendingCoroutines(t);
}


/***********************************************/
/*** Implementation of Lua binding functions ***/
/***********************************************/

static const cf::TypeSys::MethsDocT META_SetRootEntity =
{
    "SetRootEntity",
    "Sets the root entity for this world.",
    "", "(EntityT ent)"
};

int WorldT::SetRootEntity(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<WorldT> World = Binder.GetCheckedObjectParam< IntrusivePtrT<WorldT> >(1);

    World->m_RootEntity = Binder.GetCheckedObjectParam< IntrusivePtrT<EntityT> >(2);
    return 0;
}


static const cf::TypeSys::MethsDocT META_CreateNew =
{
    "CreateNew",
    "Creates and returns a new entity or component.",
    "object", "(string ClassName)"
};

int WorldT::CreateNew(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<WorldT> World    = Binder.GetCheckedObjectParam< IntrusivePtrT<WorldT> >(1);
    const char*           TypeName = luaL_checkstring(LuaState, 2);
    const char*           ObjName  = lua_tostring(LuaState, 3);    // Passing an object name is optional.

    const cf::TypeSys::TypeInfoT* TI = GetGameSysEntityTIM().FindTypeInfoByName(TypeName);

    if (TI)
    {
        IntrusivePtrT<EntityT> Entity(static_cast<EntityT*>(TI->CreateInstance(EntityCreateParamsT(*World))));

        // Console->DevPrint(cf::va("Creating entity %p.\n", Entity));
        assert(Entity->GetType() == TI);
        assert(strcmp(TI->ClassName, TypeName) == 0);

        if (ObjName) Entity->GetBasics()->SetEntityName(ObjName);

        Binder.Push(Entity);
        return 1;
    }

    TI = GetComponentTIM().FindTypeInfoByName(TypeName);

    if (TI)
    {
        IntrusivePtrT<ComponentBaseT> Comp(static_cast<ComponentBaseT*>(TI->CreateInstance(cf::TypeSys::CreateParamsT())));

        Binder.Push(Comp);
        return 1;
    }

    return luaL_argerror(LuaState, 2, (std::string("unknown class name \"") + TypeName + "\"").c_str());
}


static const cf::TypeSys::MethsDocT META_Init =
{
    "Init",
    "Calls the OnInit() script methods of all entities.",
    "", "()"
};

int WorldT::Init(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<WorldT> World = Binder.GetCheckedObjectParam< IntrusivePtrT<WorldT> >(1);

    World->Init();
    return 0;
}


static const cf::TypeSys::MethsDocT META_TraceRay =
{
    "TraceRay",
    "Employs m_ClipWorld->TraceRay() to trace a ray through the (clip) world.",
    "table", "(table Start, table Ray)"
};

int WorldT::TraceRay(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<WorldT> World = Binder.GetCheckedObjectParam< IntrusivePtrT<WorldT> >(1);

    if (!World->m_ClipWorld)
        luaL_error(LuaState, "There is no clip world in this world.");

    Vector3dT Start;
    Vector3dT Ray;

    // TODO:
    //   This code was written when the EntEagleT::Think() code was ported from C++ to Lua,
    //   and thus supports only the Start and Ray parameters.
    //   Can we augment it to support more parameters of the C++ TraceRay() in Lua's TraceRay()??
    lua_rawgeti(LuaState, 2, 1); Start.x = luaL_checknumber(LuaState, -1); lua_pop(LuaState, 1);
    lua_rawgeti(LuaState, 2, 2); Start.y = luaL_checknumber(LuaState, -1); lua_pop(LuaState, 1);
    lua_rawgeti(LuaState, 2, 3); Start.z = luaL_checknumber(LuaState, -1); lua_pop(LuaState, 1);

    lua_rawgeti(LuaState, 3, 1); Ray.x = luaL_checknumber(LuaState, -1); lua_pop(LuaState, 1);
    lua_rawgeti(LuaState, 3, 2); Ray.y = luaL_checknumber(LuaState, -1); lua_pop(LuaState, 1);
    lua_rawgeti(LuaState, 3, 3); Ray.z = luaL_checknumber(LuaState, -1); lua_pop(LuaState, 1);

    // Delegate the work to the World->m_ClipWorld.
    cf::ClipSys::TraceResultT Result(1.0);

    World->m_ClipWorld->TraceRay(Start, Ray, MaterialT::Clip_Players, NULL /*Ignore*/, Result);

    // Return the results.
    lua_newtable(LuaState);

    lua_pushstring(LuaState, "Fraction");   lua_pushnumber (LuaState, Result.Fraction);   lua_rawset(LuaState, -3);
    lua_pushstring(LuaState, "StartSolid"); lua_pushboolean(LuaState, Result.StartSolid); lua_rawset(LuaState, -3);

    return 1;
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
    "Returns a short string description of this world.",
    "string", "()"
};

int WorldT::toString(lua_State* LuaState)
{
    lua_pushfstring(LuaState, "A world. It holds the hierarchy of game entities.");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* WorldT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    // At this time, WorldT instances cannot be created "anonymously" via the TypeSys' CreateInstance() method,
    // as it would require us to derive from cf::TypeSys::CreateParamsT and deal with that.
    // That's not a problem though, because there is no class hierarchy deriving from WorldT, so any code that
    // instantiates WorldTs can do so by using the normal constructor -- no "virtual constructor" is needed.

    // return new WorldT(*static_cast<const cf::GameSys::WorldCreateParamsT*>(&Params));
    return NULL;
}

const luaL_Reg WorldT::MethodsList[] =
{
    { "SetRootEntity", WorldT::SetRootEntity },
    { "new",           WorldT::CreateNew },
    { "Init",          WorldT::Init },
    { "TraceRay",      WorldT::TraceRay },
    { "__tostring",    WorldT::toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT WorldT::DocMethods[] =
{
    META_SetRootEntity,
    META_CreateNew,
    META_Init,
    META_TraceRay,
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT WorldT::TypeInfo(GetWorldTIM(), "WorldT", NULL /*No base class.*/, CreateInstance, MethodsList, DocClass, DocMethods);
