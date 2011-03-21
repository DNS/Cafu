/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#include "EntityClass.hpp"
#include "EntityClassVar.hpp"
#include "GameConfig.hpp"
#include "Options.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}


EntityClassT::EntityClassT(const GameConfigT& GameConfig, lua_State* LuaState)
    : m_GameConfig(GameConfig),
      m_Name(),
      m_Description(),
      m_Color(Options.colors.Entity),
      m_BoundingBox(Vector3fT(-8, -8, -8), Vector3fT(8, 8, 8)),
      m_Solid(false),
      m_Point(false),
      m_Variables(),
      m_Helpers()
{
    // The key at stack index -2 is the name of the class.
    if (lua_type(LuaState, -2)!=LUA_TSTRING) throw InitErrorT();

    m_Name=lua_tostring(LuaState, -2);


    // The value at -1 must be a table that describes the actual class.
    if (lua_type(LuaState, -1)!=LUA_TTABLE) throw InitErrorT();

    lua_pushnil(LuaState);  // The initial key for the traversal.

    while (lua_next(LuaState, -2)!=0)
    {
        // The key is now at stack index -2, the value is at index -1.
        // Note that in general, the warning from the Lua reference documentation applies:
        // "While traversing a table, do not call lua_tolstring() directly on a key, unless you know that the key is actually a string."
        // Fortunately, we know that the key is of type string, and so the called function is free to inspect (but not to modify!)
        // the two topmost stack values.
        if (lua_type(LuaState, -2)!=LUA_TSTRING)
        {
            // Just skip table entries whose keys are not of type string.
            lua_pop(LuaState, 1);
            continue;
        }

        const wxString Key=lua_tostring(LuaState, -2);

        if (Key=="isPoint")
        {
            m_Point=lua_toboolean(LuaState, -1)!=0;
        }
        else if (Key=="isSolid")
        {
            m_Solid=lua_toboolean(LuaState, -1)!=0;
        }
        else if (Key=="CppClass")
        {
            // m_CppClass=lua_tostring(LuaState, -1);   // TODO: Check lua_tostring() for NULL?
        }
        else if (Key=="description")
        {
            const char* Descr=lua_tostring(LuaState, -1);

            if (Descr!=NULL) m_Description=Descr;
        }
        else if (Key=="size")
        {
            lua_rawgeti(LuaState, -1, 1);
            {
                lua_rawgeti(LuaState, -1, 1);
                m_BoundingBox.Min.x=lua_tonumber(LuaState, -1);
                lua_pop(LuaState, 1);

                lua_rawgeti(LuaState, -1, 2);
                m_BoundingBox.Min.y=lua_tonumber(LuaState, -1);
                lua_pop(LuaState, 1);

                lua_rawgeti(LuaState, -1, 3);
                m_BoundingBox.Min.z=lua_tonumber(LuaState, -1);
                lua_pop(LuaState, 1);
            }
            lua_pop(LuaState, 1);

            lua_rawgeti(LuaState, -1, 2);
            {
                lua_rawgeti(LuaState, -1, 1);
                m_BoundingBox.Max.x=lua_tonumber(LuaState, -1);
                lua_pop(LuaState, 1);

                lua_rawgeti(LuaState, -1, 2);
                m_BoundingBox.Max.y=lua_tonumber(LuaState, -1);
                lua_pop(LuaState, 1);

                lua_rawgeti(LuaState, -1, 3);
                m_BoundingBox.Max.z=lua_tonumber(LuaState, -1);
                lua_pop(LuaState, 1);
            }
            lua_pop(LuaState, 1);
        }
        else if (Key=="color")
        {
            lua_rawgeti(LuaState, -1, 1);
            const int r=lua_tointeger(LuaState, -1);
            lua_pop(LuaState, 1);

            lua_rawgeti(LuaState, -1, 2);
            const int g=lua_tointeger(LuaState, -1);
            lua_pop(LuaState, 1);

            lua_rawgeti(LuaState, -1, 3);
            const int b=lua_tointeger(LuaState, -1);
            lua_pop(LuaState, 1);

            m_Color=wxColour(r, g, b);
        }
        else if (Key=="helpers")
        {
            lua_pushnil(LuaState);  // The initial key for the traversal.

            while (lua_next(LuaState, -2)!=0)
            {
                if (lua_type(LuaState, -2)!=LUA_TSTRING)
                {
                    // Just skip table entries whose keys are not of type string.
                    lua_pop(LuaState, 1);
                    continue;
                }

                HelperInfoT* Helper=new HelperInfoT;
                Helper->Name=lua_tostring(LuaState, -2);

                if (lua_isstring(LuaState, -1))
                {
                    Helper->Parameters.PushBack(lua_tostring(LuaState, -1));
                }
                else if (lua_istable(LuaState, -1))
                {
                    lua_pushnil(LuaState);  // The initial key for the traversal.

                    while (lua_next(LuaState, -2)!=0)
                    {
                        const char* Param=lua_tostring(LuaState, -1);

                        if (Param!=NULL)
                            Helper->Parameters.PushBack(Param);

                        // Remove the value, keep the key for the next iteration.
                        lua_pop(LuaState, 1);
                    }
                }

                m_Helpers.PushBack(Helper);

                // Remove the value, keep the key for the next iteration.
                lua_pop(LuaState, 1);
            }
        }
        else
        {
            // It's a regular variable.
            try
            {
                m_Variables.PushBack(new EntClassVarT(LuaState));
            }
            catch (const EntClassVarT::InitErrorT&)
            {
                for (unsigned long VarNr=0; VarNr<m_Variables.Size(); VarNr++) delete m_Variables[VarNr];
                for (unsigned long HlpNr=0; HlpNr<m_Helpers  .Size(); HlpNr++) delete m_Helpers  [HlpNr];

                throw InitErrorT();
            }
        }

        // Remove the value, keep the key for the next iteration.
        lua_pop(LuaState, 1);
    }

    wxASSERT(m_Solid!=m_Point);
}


EntityClassT::EntityClassT(const GameConfigT& GameConfig, const wxString& Name, bool HasOrigin)
    : m_GameConfig(GameConfig),
      m_Name(Name),
      m_Description("An entity class that is unknown in this game configuration."),
      m_Color(Options.colors.Entity),
      m_BoundingBox(Vector3fT(-8, -8, -8), Vector3fT(8, 8, 8)),
      m_Solid(!HasOrigin),
      m_Point(HasOrigin),
      m_Variables(),
      m_Helpers()
{
}


EntityClassT::~EntityClassT()
{
    for (unsigned long VarNr=0; VarNr<m_Variables.Size(); VarNr++) delete m_Variables[VarNr];
    for (unsigned long HlpNr=0; HlpNr<m_Helpers  .Size(); HlpNr++) delete m_Helpers  [HlpNr];
}


bool EntityClassT::IsInGameConfig() const
{
    return m_GameConfig.GetEntityClasses().Find(this)!=-1;
}


const EntClassVarT* EntityClassT::FindVar(const wxString& Name, unsigned long* Index) const
{
    for (unsigned long VarNr=0; VarNr<m_Variables.Size(); VarNr++)
    {
        if (m_Variables[VarNr]->GetName()==Name)
        {
            if (Index) *Index=VarNr;
            return m_Variables[VarNr];
        }
    }

    return NULL;
}
