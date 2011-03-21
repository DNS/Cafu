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

/*******************************/
/*** Look-up Tables (Header) ***/
/*******************************/

#ifndef _LOOKUP_TABLES_HPP_
#define _LOOKUP_TABLES_HPP_


namespace LookupTables
{
    extern float Angle16ToSin[1 << 16];
    extern float Angle16ToCos[1 << 16];

    extern unsigned short RandomUShort[1 << 12];    //   0 <= x <= 0xFFFF
    extern float          RandomFloat [1 << 12];    // 0.0 <= x <= 1.0

    // Initialisiert die Look-up Tables. Sollte aus DllMain() (bei DLL_PROCESS_ATTACH) aufgerufen werden!
    void Initialize();
};

#endif
