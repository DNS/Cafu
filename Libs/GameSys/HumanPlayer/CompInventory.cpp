/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "CompInventory.hpp"
#include "../AllComponents.hpp"

#include "Network/State.hpp"
#include "UniScriptState.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GameSys;


const char* ComponentInventoryT::DocClass =
    "This component keeps an inventory count for an arbitrary set of items.\n"
    "An item can be anything that can be described with a string.\n"
    "Contrary to other components, an inventory is flexible regarding the \"kind\" and number\n"
    "of items that it keeps the counts for. However, it is focused on being used by script code;\n"
    "it is not possible to inspect and edit the contained items in the Map Editor at this time.";


const cf::TypeSys::VarsDocT ComponentInventoryT::DocVars[] =
{
    { NULL, NULL }
};


ComponentInventoryT::ComponentInventoryT()
    : ComponentBaseT(),
      m_Items()
{
}


ComponentInventoryT::ComponentInventoryT(const ComponentInventoryT& Comp)
    : ComponentBaseT(Comp),
      m_Items(Comp.m_Items)
{
}


void ComponentInventoryT::DoSerialize(cf::Network::OutStreamT& Stream) const
{
    uint16_t NUM_ITEMS = uint16_t(m_Items.size());

    Stream << NUM_ITEMS;

    for (std::map<std::string, uint16_t>::const_iterator It = m_Items.begin(); It != m_Items.end(); It++)
    {
        Stream << It->first;
        Stream << It->second;

        assert(NUM_ITEMS > 0);
        NUM_ITEMS--;
    }

    assert(NUM_ITEMS == 0);
}


void ComponentInventoryT::DoDeserialize(cf::Network::InStreamT& Stream, bool IsIniting)
{
    uint16_t NUM_ITEMS = 0;

    Stream >> NUM_ITEMS;

    // In order to avoid the frequent and expensive reallocation of memory, clear
    // m_Items by manually resetting all values to 0 rather than calling `m_Items.clear()`.
    for (std::map<std::string, uint16_t>::iterator It = m_Items.begin(); It != m_Items.end(); It++)
        It->second = 0;

    for (unsigned int i = 0; i < NUM_ITEMS; i++)
    {
        std::string ItemName;

        Stream >> ItemName;
        Stream >> m_Items[ItemName];
    }
}


ComponentInventoryT* ComponentInventoryT::Clone() const
{
    return new ComponentInventoryT(*this);
}


static const cf::TypeSys::MethsDocT META_Get =
{
    "get",
    "Returns the inventory count for the specified item.\n"
    "@param item_name   The name of the item to return the count for.",
    "any", "(string item_name)"
};

int ComponentInventoryT::Get(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentInventoryT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentInventoryT> >(1);
    const char* ItemName = luaL_checkstring(LuaState, 2);
    std::map<std::string, uint16_t>::const_iterator It = Comp->m_Items.find(ItemName);

    if (It == Comp->m_Items.end())
        return 0;

    lua_pushinteger(LuaState, It->second);
    return 1;
}


static const cf::TypeSys::MethsDocT META_Set =
{
    "set",
    "Sets the inventory count for the specified item.\n"
    "@param item_name    The name of the item to set the count for.\n"
    "@param item_count   The new inventory count to set.",
    "", "(string item_name, number item_count)"
};

int ComponentInventoryT::Set(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentInventoryT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentInventoryT> >(1);

    const char* ItemName  = luaL_checkstring(LuaState, 2);
    const int   ItemCount = luaL_checkint(LuaState, 3);

    Comp->m_Items[ItemName] = ItemCount;
    return 0;
}


static const cf::TypeSys::MethsDocT META_Add =
{
    "Add",
    "Changes the inventory count of the specified item by the given amount.\n"
    "The amount can be positive to increase the inventory count or negative to decrease it.\n"
    "The resulting inventory count is clamped to the maximum for the item, if such a maximum is set.\n"
    "It is also clamped to 0 (for negative amounts).\n"
    "The function returns `true` if the resulting inventory count was clamped on either boundary,\n"
    "or `false` if no clamping was applied.\n"
    "@param item_name   The name of the item whose count is to be changed.\n"
    "@param amount      The amount by which the inventory count is changed.",
    "", "(string item_name, number amount)"
};

int ComponentInventoryT::Add(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentInventoryT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentInventoryT> >(1);

    const char* ItemName  = luaL_checkstring(LuaState, 2);
    const int   ItemCount = Comp->m_Items[ItemName] + luaL_checkint(LuaState, 3);

    if (ItemCount < 0)
    {
        Comp->m_Items[ItemName] = 0;
        lua_pushboolean(LuaState, 1);   // clamped to 0
        return 1;
    }

    std::map<std::string, uint16_t>::const_iterator MaxIt = Comp->m_Items.find(std::string("Max") + ItemName);

    if (MaxIt == Comp->m_Items.end())
    {
        Comp->m_Items[ItemName] = ItemCount;
        lua_pushboolean(LuaState, 0);   // no maximum defined, not clamped
        return 1;
    }

    if (ItemCount > MaxIt->second)
    {
        Comp->m_Items[ItemName] = MaxIt->second;
        lua_pushboolean(LuaState, 1);   // clamped to MaxIt->second
        return 1;
    }

    Comp->m_Items[ItemName] = ItemCount;
    lua_pushboolean(LuaState, 0);       // no clamping applicable, not clamped
    return 1;
}


static const cf::TypeSys::MethsDocT META_CheckMax =
{
    "CheckMax",
    "Checks if the inventory count of the specified item is at the item's maximum.\n"
    "Returns `true` if the inventory count of the specified item is equal to (or even exceeds) its defined maximum.\n"
    "Returns `false` if no maximum is defined or if the inventory count is below the defined value."
    "@param item_name   The name of the item to check the count for.",
    "", "(string item_name)"
};

int ComponentInventoryT::CheckMax(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentInventoryT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentInventoryT> >(1);

    const char* ItemName = luaL_checkstring(LuaState, 2);
    std::map<std::string, uint16_t>::const_iterator It    = Comp->m_Items.find(ItemName);
    std::map<std::string, uint16_t>::const_iterator MaxIt = Comp->m_Items.find(std::string("Max") + ItemName);

    lua_pushboolean(LuaState,
        It != Comp->m_Items.end() &&
        MaxIt != Comp->m_Items.end() &&
        It->second >= MaxIt->second);

    return 1;
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int ComponentInventoryT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "inventory component");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentInventoryT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentInventoryT();
}

const luaL_Reg ComponentInventoryT::MethodsList[] =
{
    { "get",        Get },
    { "set",        Set },
    { "Add",        Add },
    { "CheckMax",   CheckMax },
    { "__tostring", toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentInventoryT::DocMethods[] =
{
    META_Get,
    META_Set,
    META_Add,
    META_CheckMax,
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentInventoryT::TypeInfo(GetComponentTIM(), "GameSys::ComponentInventoryT", "GameSys::ComponentBaseT", ComponentInventoryT::CreateInstance, MethodsList, DocClass, DocMethods, NULL, DocVars);
