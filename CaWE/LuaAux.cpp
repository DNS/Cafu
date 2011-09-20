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

#include "LuaAux.hpp"

#include "wx/string.h"
#include "wx/regex.h"


namespace
{
    const wxString Keywords[]=
    {
        "and", "break", "do", "else", "elseif",
        "end", "false", "for", "function", "if",
        "in", "local", "nil", "not", "or", "repeat",
        "return", "then", "true", "until", "while"
    };

    const unsigned int NrOfKeywords=sizeof(Keywords)/sizeof(*Keywords);
}


bool IsLuaIdentifier(const wxString& id)
{
    // Does id fail to meet the Lua identifier rules?
    const wxRegEx LuaIdRegEx("^[A-Za-z_][\\w]+$", wxRE_ADVANCED);
    wxASSERT(LuaIdRegEx.IsValid());

    if (!LuaIdRegEx.Matches(id)) return false;

    // Is id a global reserved Lua identifier?
    const wxRegEx LuaGlobalRegEx("^[_][A-Z]+$", wxRE_ADVANCED);
    wxASSERT(LuaGlobalRegEx.IsValid());

    if (LuaGlobalRegEx.Matches(id)) return false;

    // Is id a reserved Lua keyword?
    for (unsigned int kwNr=0; kwNr<NrOfKeywords; kwNr++)
        if (id==Keywords[kwNr]) return false;

    // All tests passed: id is a valid Lua identifier!
    return true;
}


wxString CheckLuaIdentifier(const wxString& id)
{
    // If id is valid, return it unchanged.
    if (IsLuaIdentifier(id)) return id;

    // If id is empty, just come up with some valid identifier.
    if (id=="") return "id";

    // Replace all non-alphanumeric characters with an underscore.
    wxString NewId=id;

    const wxRegEx NotAlphaNumRegEx("\\W", wxRE_ADVANCED);
    wxASSERT(NotAlphaNumRegEx.IsValid());

    NotAlphaNumRegEx.Replace(&NewId, "_");

    if (IsLuaIdentifier(NewId)) return NewId;

    // Still not good? Add a prefix.
    NewId="id_"+NewId;

    wxASSERT(IsLuaIdentifier(NewId));
    return NewId;
}
