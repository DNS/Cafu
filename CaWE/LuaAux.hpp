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

#ifndef _LUA_AUX_HPP_
#define _LUA_AUX_HPP_

class wxString;


// MOVEMENT INFO
// CF: Achtung: Diese Konstanten zu entfernen und durch AngleT's pitch(), roll() und yaw() zu ersetzen *kann*
// problematisch sein, weil im Code ab und zu auch statt [PITCH] einfach [0] usw. geschrieben wird.
// Am besten einfach vorübergehend den [] operator mal privat machen, um alle Stellen zuverlässig zu finden...
// Es bleibt zu hoffen, dass nirgendwo direkt .x .y oder .z geschrieben wird...
enum
{
    PITCH=0,    // Nose up/down.
    YAW,        // Heading.
    ROLL        // Bank angle.
};


/// Checks if a string is a valid LUA variable name.
/// @param Varname The variable name to be checked.
/// @return Whether the string is valid or not.
bool CheckLuaVarCompat(const wxString& Varname);

/// Converts a string into a valid LUA variable name by replacing
/// all special invalid characters with underscores and/or adding
/// underscores to the begining of the name to make it valid.
/// @param Varname The variable name to convert.
/// @return The converted valid LUA variable name.
wxString MakeLuaVarName(const wxString& Varname);

#endif
