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


static const std::string Keywords_Lua[]={"and", "break", "do", "else", "elseif",
                                         "end", "false", "for", "function", "if",
                                         "in", "local", "nil", "not", "or", "repeat",
                                         "return", "then", "true", "until", "while"};

static const int NrOfKeywords=sizeof(Keywords_Lua)/sizeof(*Keywords_Lua);


bool CheckLuaVarCompat(const wxString& Varname)
{
    wxRegEx LuaVarName("^[A-Za-z_][\\w]+$", wxRE_ADVANCED);
    wxASSERT(LuaVarName.IsValid());

    if (LuaVarName.Matches(Varname))
    {
        // Check if variable name is a reserved LUA keyword.
        for (int KeywordNr=0; KeywordNr<NrOfKeywords; KeywordNr++)
            if (Varname.c_str()==Keywords_Lua[KeywordNr]) return false;

        // Check if variable name matches a LUA global variable.
        wxRegEx LuaGlobals("^[_][A-Z]+$", wxRE_ADVANCED);
        wxASSERT(LuaGlobals.IsValid());

        if (LuaGlobals.Matches(Varname)) return false;

        return true;
    }

    return false;
}


wxString MakeLuaVarName(const wxString& Varname)
{
    // Return the original name if it is already valid.
    if (CheckLuaVarCompat(Varname)) return Varname;

    // Return a default string if passed varname is empty so this functions never fails.
    if (Varname=="") return "Variable";

    wxRegEx LuaVarName("\\W", wxRE_ADVANCED);
    wxASSERT(LuaVarName.IsValid());

    wxString LuaVar=Varname;

    LuaVarName.Replace(&LuaVar, "_"); // Replace all non compatible chars with underscore.

    // If variable is now compatible, return it.
    if (CheckLuaVarCompat(LuaVar)) return LuaVar;

    // Add a leading underscore which will make the variable compatible in any case.
    LuaVar="_"+LuaVar;

    wxASSERT(CheckLuaVarCompat(LuaVar));

    return LuaVar;
}
