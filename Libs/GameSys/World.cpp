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


WorldT::InitErrorT::InitErrorT(const std::string& Message)
    : std::runtime_error(Message)
{
}


WorldT::WorldT(cf::UniScriptStateT& ScriptState, const std::string& ScriptName, ModelManagerT& ModelMan, cf::GuiSys::GuiResourcesT& GuiRes,
               cf::ClipSys::CollModelManI& CollModelMan, cf::ClipSys::ClipWorldT* ClipWorld, int Flags)
    : m_ScriptName((Flags & InitFlag_InlineCode) ? "" : ScriptName),
      m_ScriptState(ScriptState),
      m_RootEntity(NULL),
      m_IsInited(false),
      m_NextEntID(0),
      m_ModelMan(ModelMan),
      m_GuiResources(GuiRes),
      m_CollModelMan(CollModelMan),
      m_ClipWorld(ClipWorld)
{
    lua_State* LuaState = m_ScriptState.GetLuaState();

    // Load the console library. (Adds a global table with name "Console" to the LuaState with the functions of the ConsoleI interface.)
    cf::Console_RegisterLua(LuaState);

    // Load the "ci" (console interpreter) library. (Adds a global table with name "ci" to the LuaState with (some of) the functions of the ConsoleInterpreterI interface.)
    ConsoleInterpreterI::RegisterLua(LuaState);

    // Adds a global (meta-)table with methods for cf::GameSys::WorldTs to the LuaState, to be used as metatable for userdata of type cf::GameSys::WorldT.
    WorldT::RegisterLua(LuaState);

    // For each class that the TypeInfoManTs know about, add a (meta-)table to the registry of the LuaState.
    // The (meta-)table holds the Lua methods that the respective class implements in C++,
    // and is to be used as metatable for instances of this class.
    ScriptBinderT Binder(LuaState);

    Binder.Init(GetGameSysEntityTIM());
    Binder.Init(GetComponentTIM());


    // Add a global variable with name "world" to the Lua state. "world" is a table that scripts can use to call methods of this class.
    {
        assert(lua_gettop(LuaState) == 0);

        // Stack indices of the table and userdata that we create.
        const int USERDATA_INDEX = 2;
        const int TABLE_INDEX    = 1;

        // Create a new table T, which is pushed on the stack and thus at stack index TABLE_INDEX.
        lua_newtable(LuaState);

        // Create a new user datum UD, which is pushed on the stack and thus at stack index USERDATA_INDEX.
        WorldT** UserData = (WorldT**)lua_newuserdata(LuaState, sizeof(WorldT*));

        // Initialize the memory allocated by the lua_newuserdata() function.
        *UserData = this;

        // T["__userdata_cf"] = UD
        lua_pushvalue(LuaState, USERDATA_INDEX);    // Duplicate the userdata on top of the stack (as the argument for lua_setfield()).
        lua_setfield(LuaState, TABLE_INDEX, "__userdata_cf");

        // Get the table with name (key) "cf::GameSys::WorldT" from the registry,
        // and set it as metatable of the newly created table.
        luaL_getmetatable(LuaState, "cf::GameSys::WorldT");
        lua_setmetatable(LuaState, TABLE_INDEX);

        // Get the table with name (key) "cf::GameSys::WorldT" from the registry,
        // and set it as metatable of the newly created user data (for user data type safety, see PiL2, chapter 28.2).
        luaL_getmetatable(LuaState, "cf::GameSys::WorldT");
        lua_setmetatable(LuaState, USERDATA_INDEX);

        // Remove UD from the stack, so that only the new table T is left on top of the stack.
        // Then add it as a global variable whose name is "world".
        // As lua_setglobal() pops the table from the stack, the stack is left empty.
        lua_pop(LuaState, 1);
        lua_setglobal(LuaState, "world");
        // Could instead do   lua_setfield(LuaState, LUA_REGISTRYINDEX, "world");   as well,
        // so that script methods like "new()" and "thread()" could also be called without the "world:" prefix.
    }

    // Make sure that everyone dealt properly with the Lua stack so far.
    assert(lua_gettop(LuaState) == 0);


    // Load the user script!
    const int LoadResult = (Flags & InitFlag_InlineCode) ? luaL_loadstring(LuaState, ScriptName.c_str())
                                                         : luaL_loadfile  (LuaState, ScriptName.c_str());

    if (LoadResult != 0 || lua_pcall(LuaState, 0, 0, 0) != 0)
    {
        const std::string Msg = "Could not load \"" + m_ScriptName + "\":\n" + lua_tostring(LuaState, -1);

        Console->Warning(Msg + "\n");

        // The LuaState will be closed by the m_ScriptState.
        throw InitErrorT(Msg);
    }

    if (m_RootEntity == NULL)
    {
        const std::string Msg = "No root entity set in world \"" + m_ScriptName + "\".";

        Console->Warning(Msg + "\n");

        // The LuaState will be closed by the m_ScriptState.
        throw InitErrorT(Msg);
    }

    // Make sure that everyone dealt properly with the Lua stack so far.
    assert(lua_gettop(LuaState) == 0);


    // Finally call the Lua OnInit() and OnInit2() methods of each entity.
    ArrayT< IntrusivePtrT<EntityT> > AllEnts;
    m_RootEntity->GetAll(AllEnts);

    Init();     // The script has the option to call this itself (via world:Init()) at an earlier time.

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
        // them in depth-firsth order, the co-use of entity IDs implies that the IDs must match the entity's ordinal
        // number in a depth first traversal. Make sure here that this condition is actually met.
        // Note that the main motivation here is catching program bugs, like an assert() statement that is also checked
        // in release builds. Although not impossible, it's very unlikely that a user action will ever trigger this.
        if (AllEnts[EntNr]->GetID() != EntNr)
        {
            const std::string Msg = "ID of entity \"" + AllEnts[EntNr]->GetBasics()->GetEntityName() +
                cf::va("\" is %u, expected %lu.\n", AllEnts[EntNr]->GetID(), EntNr) +
                "(Are the entities in \"" + m_ScriptName + "\" created in depth-first order?)";

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


WorldT::~WorldT()
{
    // Manually "destruct" these references before the Lua state (m_ScriptState).
    // (This is redundant: the normal member destruction sequence achieves the same.)
    m_RootEntity = NULL;
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

static WorldT* CheckParams(lua_State* LuaState)
{
    luaL_argcheck(LuaState, lua_istable(LuaState, 1), 1, "Expected a table that represents a game world.");
    lua_getfield(LuaState, 1, "__userdata_cf");

    WorldT** UserData = (WorldT**)luaL_checkudata(LuaState, -1, "cf::GameSys::WorldT"); if (!UserData) luaL_error(LuaState, "NULL userdata in world table.");
    WorldT*  World    = (*UserData);

    // Pop the userdata from the stack again. Not necessary though as it doesn't hurt there.
    // lua_pop(LuaState, 1);
    return World;
}


int WorldT::SetRootEntity(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    WorldT*       World = CheckParams(LuaState);

    World->m_RootEntity = Binder.GetCheckedObjectParam< IntrusivePtrT<EntityT> >(2);
    return 0;
}


int WorldT::CreateNew(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    WorldT*       World    = CheckParams(LuaState);
    const char*   TypeName = luaL_checkstring(LuaState, 2);
    const char*   ObjName  = lua_tostring(LuaState, 3);    // Passing an object name is optional.

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


int WorldT::Init(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    WorldT*       World = CheckParams(LuaState);

    World->Init();
    return 0;
}


int WorldT::TraceRay(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    WorldT*       World = CheckParams(LuaState);

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


int WorldT::toString(lua_State* LuaState)
{
    WorldT* World = CheckParams(LuaState);

    lua_pushfstring(LuaState, "A world loaded from script \"%s\".", World->m_ScriptName.c_str());
    return 1;
}


void WorldT::RegisterLua(lua_State* LuaState)
{
    // Create a new table T and add it into the registry table with "cf::GameSys::WorldT" as the key and T as the value.
    // This also leaves T on top of the stack. See PiL2 chapter 28.2 for more details.
    luaL_newmetatable(LuaState, "cf::GameSys::WorldT");

    // See PiL2 chapter 28.3 for a great explanation on what is going on here.
    // Essentially, we set T.__index = T (the luaL_newmetatable() function left T on the top of the stack).
    lua_pushvalue(LuaState, -1);                // Pushes/duplicates the new table T on the stack.
    lua_setfield(LuaState, -2, "__index");      // T.__index = T;

    static const luaL_Reg WorldMethods[] =
    {
        { "SetRootEntity", SetRootEntity },
        { "new",           CreateNew },
        { "Init",          Init },
        { "TraceRay",      TraceRay },
        { "__tostring",    toString },
        { NULL, NULL }
    };

    // Now insert the functions listed in WorldMethods into T (the table on top of the stack).
    luaL_setfuncs(LuaState, WorldMethods, 0);

    // Clear the stack.
    lua_settop(LuaState, 0);
}
