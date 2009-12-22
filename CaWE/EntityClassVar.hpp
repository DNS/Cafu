/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#ifndef _ENTITY_CLASS_VAR_HPP_
#define _ENTITY_CLASS_VAR_HPP_

#include "Templates/Array.hpp"
#include "wx/wx.h"


struct lua_State;
class EntPropertyT;


/// This class represents a variable of an entity class.
class EntClassVarT
{
    public:

    class InitErrorT { };

    enum TypeT
    {
        // TODO: Add anything we have special pickers for: entity (names), materials, file_sound (specify wildcard in type string??), angles, vectors, ...
        TYPE_INVALID=-1,
        TYPE_CHOICE,
        TYPE_COLOR,
        TYPE_FILE,
        TYPE_FILE_MODEL,
        TYPE_FLAGS,
        TYPE_FLOAT,
        TYPE_INTEGER,
        TYPE_STRING,
    };


    /// Creates an EntClassVarT instance from the stack contents of the given LuaState.
    /// @param LuaState   The LuaState whose stack contents the EntClassVarT instance is to be created from.
    EntClassVarT(lua_State* LuaState);

    wxString GetName() const        { return m_Name;         }
    wxString GetLongName() const    { return m_LongName;     }
    wxString GetDescription() const { return m_Description;  }
    TypeT    GetType() const        { return m_Type;         }
    wxString GetDefault() const     { return m_DefaultValue; }
    bool     IsReportable() const   { return m_ShowUser;     }   ///< Returns whether or not this variable should be displayed in the Entity Report dialog.
    bool     IsReadOnly() const     { return m_ReadOnly;     }   ///< Returns whether or not this variable is read only. Read only variables cannot be edited in the Entity Properties dialog.
    bool     IsUnique() const       { return m_Unique;       }   ///< Returns whether or not the value of this variable must be unique among all entities in the world.

    const ArrayT<wxString>& GetAuxInfo() const { return m_AuxInfo; }

    EntPropertyT GetInstance() const; ///< Get a property instance of this class key using default value


    private:

    wxString         m_Name;
    wxString         m_LongName;
    wxString         m_Description;
    TypeT            m_Type;
    wxString         m_DefaultValue;    ///< This is the default value for initializing concrete instances of this variable.
    bool             m_ShowUser;
    bool             m_ReadOnly;
    bool             m_Unique;
    ArrayT<wxString> m_AuxInfo;         ///< Auxiliary information. E.g. for "flags" and "choice", this is the list of labels.
};

#endif
