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

#include "EntityClassVar.hpp"
#include "EntProperty.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}


struct TypeMapT
{
    EntClassVarT::TypeT Type;   // The type.
    const char*         Name;   // The name for this type.
};


static TypeMapT TypeMap[] =
{
    { EntClassVarT::TYPE_CHOICE,     "choice"     },
    { EntClassVarT::TYPE_COLOR,      "color"      },
    { EntClassVarT::TYPE_FILE,       "file"       },
    { EntClassVarT::TYPE_FILE_MODEL, "file_model" },
    { EntClassVarT::TYPE_FLAGS,      "flags"      },
    { EntClassVarT::TYPE_FLOAT,      "float"      },
    { EntClassVarT::TYPE_INTEGER,    "integer"    },
    { EntClassVarT::TYPE_STRING,     "string"     },
};


EntClassVarT::EntClassVarT(lua_State* LuaState)
    : m_Name(),
      m_LongName(),
      m_Description(),
      m_Type(TYPE_INVALID),
      m_DefaultValue(),
      m_ShowUser(true),
      m_ReadOnly(false),
      m_Unique(false)
{
    // We expect the variable name to be at stack index -2,
    // and the table that defines the variable properties at index -1.
    if (lua_type(LuaState, -2)!=LUA_TSTRING || lua_tostring(LuaState, -2)==NULL) throw InitErrorT(/*"Name of entity class variable is not a string."*/);

    m_Name    =lua_tostring(LuaState, -2);
    m_LongName=m_Name;


    // Traverse the table with the properties of the variable.
    if (lua_type(LuaState, -1)!=LUA_TTABLE) throw InitErrorT(/*"No table for entity class variable \""+m_Name+"\"."*/);

    // The initial key for the traversal.
    lua_pushnil(LuaState);

    while (lua_next(LuaState, -2)!=0)
    {
        // The key is now at stack index -2, the value is at index -1.
        // Note that in general, the warning from the Lua reference documentation applies:
        // "While traversing a table, do not call lua_tolstring() directly on a key, unless you know that the key is actually a string."
        if (lua_type(LuaState, -2)!=LUA_TSTRING || lua_tostring(LuaState, -2)==NULL)
        {
            // Just skip table entries whose keys are not of type string.
            wxMessageBox("Entity class variable \""+m_Name+"\" has non-string property.");
            lua_pop(LuaState, 1);
            continue;
        }

        const wxString Key=lua_tostring(LuaState, -2);

        if (Key=="type")
        {
            const char* TypeName_=lua_tostring(LuaState, -1);

            if (TypeName_!=NULL)
            {
                const wxString TypeName=TypeName_;

                for (unsigned long i=0; i<sizeof(TypeMap)/sizeof(TypeMap[0]); i++)
                {
                    // Note that if no one matches, there is a check at the very end of this ctor for m_Type==TYPE_INVALID.
                    if (TypeName==TypeMap[i].Name)
                    {
                        m_Type=TypeMap[i].Type;
                        break;
                    }
                }
            }
        }
        else if (Key=="description")
        {
            const char* Descr=lua_tostring(LuaState, -1);

            if (Descr!=NULL) m_Description=Descr;
        }
        else if (Key=="value")
        {
            const char* Value=lua_tostring(LuaState, -1);

            if (Value!=NULL) m_DefaultValue=Value;
        }
        else if (Key=="choices")
        {
            const int NumFlags=lua_objlen(LuaState, -1);

            for (int i=1; i<=NumFlags; i++)
            {
                lua_rawgeti(LuaState, -1, i);
                const char* Label=lua_tostring(LuaState, -1);
                m_AuxInfo.PushBack(Label!=NULL ? Label : "NULL");
                lua_pop(LuaState, 1);
            }
        }
        else if (Key=="flags")
        {
            unsigned long DefaultValue=0;
            const int     NumFlags    =lua_objlen(LuaState, -1);

            for (int i=1; i<=NumFlags; i+=2)
            {
                lua_rawgeti(LuaState, -1, i);
                const char* Label=lua_tostring(LuaState, -1);
                m_AuxInfo.PushBack(Label!=NULL ? Label : "NULL");
                lua_pop(LuaState, 1);

                lua_rawgeti(LuaState, -1, i+1);
                DefaultValue|=lua_toboolean(LuaState, -1) << ((i-1)/2);
                lua_pop(LuaState, 1);
            }

            m_DefaultValue=wxString::Format("%lu", DefaultValue);
        }
        else if (Key=="uniqueValue")
        {
            m_Unique=lua_toboolean(LuaState, -1)!=0;
        }
        else
        {
            // Unknown variable property.
            wxMessageBox("Unknown property \""+Key+"\" for entity class variable \""+m_Name+"\" ignored.");
        }

        // Remove the value, keep the key for the next iteration.
        lua_pop(LuaState, 1);
    }

    if (m_Type==TYPE_INVALID) throw InitErrorT();
}


// Returns EntPropertyT instance of this class variable using default value.
EntPropertyT EntClassVarT::GetInstance() const
{
    return EntPropertyT(m_Name, m_DefaultValue);
}
