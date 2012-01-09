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

/*****************************/
/*** Look-up Tables (Code) ***/
/*****************************/

#include <math.h>
#include <stdlib.h>
#include "LookupTables.hpp"


float LookupTables::Angle16ToSin[1 << 16];
float LookupTables::Angle16ToCos[1 << 16];

unsigned short LookupTables::RandomUShort[1 << 12];
float          LookupTables::RandomFloat [1 << 12];


void LookupTables::Initialize()
{
    const double Pi=3.14159265358979323846;

    for (unsigned long Angle16=0; Angle16<(1 << 16); Angle16++)
    {
        Angle16ToSin[Angle16]=float(sin(double(Angle16)/32768.0*Pi));
        Angle16ToCos[Angle16]=float(cos(double(Angle16)/32768.0*Pi));
    }


    srand(0);

    for (unsigned long RandomNum=0; RandomNum<(1 << 12); RandomNum++)
    {
        RandomUShort[RandomNum]=(unsigned short)((((unsigned long)rand()) << 1) + (rand() & 0x1));  // assume 'RAND_MAX==32767'
        RandomFloat [RandomNum]=float(double(rand())/double(RAND_MAX));
    }
}
