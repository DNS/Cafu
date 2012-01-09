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

#ifndef _CF_DEBUG_LOG_HPP_
#define _CF_DEBUG_LOG_HPP_


namespace cf
{
    void LogDebugBasic(const char* Category, const char* FileName, const int LineNr, const char* FormatString, ...);

    inline void LogDebugNOP() { }
}


// Enable or disable debug logging here.
// Note that filters (which categories are included in or exempted from logging)
// are defined in the implementation of LogDebugBasic().
#if 0
    #define LogDebug(cat, fmt, ...)   LogDebugBasic(#cat, __FILE__, __LINE__, fmt, __VA_ARGS__)
#else
    #define LogDebug(cat, fmt, ...)   LogDebugNOP()
#endif

#endif
