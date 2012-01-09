/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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


/// Determines if the given string is a valid Lua identifier.
///
/// @param id   The identifier name to check.
/// @return Whether \c id is a valid Lua identifier.
bool IsLuaIdentifier(const wxString& id);


/// Determines if the given string is a valid Lua identifier.
/// If valid, \c id is returned unchanged.
/// Otherwise, for the return value a variant of \c id is created that is valid.
///
/// @param id   The identifier name to check.
/// @return A valid Lua identifier derived from \c id.
wxString CheckLuaIdentifier(const wxString& id);

#endif
