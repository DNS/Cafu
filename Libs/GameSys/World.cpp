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

#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/Console_Lua.hpp"
#include "ConsoleCommands/ConsoleInterpreter.hpp"


using namespace cf::GameSys;


WorldT::InitErrorT::InitErrorT(const std::string& Message)
    : std::runtime_error(Message)
{
}


WorldT::WorldT(const std::string& ScriptName, ModelManagerT& ModelMan, int Flags)
    : m_ScriptName((Flags & InitFlag_InlineCode) ? "" : ScriptName),
      m_ScriptState(),
      m_RootEntity(NULL),
      m_IsInited(false),
      m_ModelMan(ModelMan)
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
    ArrayT< IntrusivePtrT<EntityT> > AllChildren;

    AllChildren.PushBack(m_RootEntity);
    m_RootEntity->GetChildren(AllChildren, true);

    Init();     // The script has the option to call this itself (via world:Init()) at an earlier time.

    for (unsigned long ChildNr = 0; ChildNr < AllChildren.Size(); ChildNr++)
    {
        // The OnInit2() methods contain custom, hand-written code by the user (*_main.cgui files).
        AllChildren[ChildNr]->CallLuaMethod("OnInit2");

        // Let each component know that the "static" part of initialization is now complete.
        const ArrayT< IntrusivePtrT<ComponentBaseT> >& Components = AllChildren[ChildNr]->GetComponents();

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
        AllChildren[ChildNr]->CallLuaMethod("OnInit");
    }

    m_IsInited = true;
}


void WorldT::Render() const
{
    m_RootEntity->Render();
}


bool WorldT::ProcessDeviceEvent(const CaKeyboardEventT& KE)
{
    return false;
}


bool WorldT::ProcessDeviceEvent(const CaMouseEventT& ME)
{
    return false;
}


void WorldT::DistributeClockTickEvents(float t)
{
    ArrayT< IntrusivePtrT<EntityT> > AllChildren;

    AllChildren.PushBack(m_RootEntity);
    m_RootEntity->GetChildren(AllChildren, true);

    for (unsigned long ChildNr = 0; ChildNr < AllChildren.Size(); ChildNr++)
    {
        AllChildren[ChildNr]->OnClockTickEvent(t);
        AllChildren[ChildNr]->CallLuaMethod("OnFrame");
    }

    // Run the pending coroutines.
    m_ScriptState.RunPendingCoroutines(t);
}


/**********************************************/
/*** Impementation of Lua binding functions ***/
/**********************************************/

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

    static const luaL_reg WorldMethods[] =
    {
        { "SetRootEntity", SetRootEntity },
        { "new",           CreateNew },
        { "Init",          Init },
        { "__tostring",    toString },
        { NULL, NULL }
    };

    // Now insert the functions listed in WorldMethods into T (the table on top of the stack).
    luaL_register(LuaState, NULL, WorldMethods);

    // Clear the stack.
    lua_settop(LuaState, 0);
}
