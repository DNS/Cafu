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

#include "Buffer.hpp"


BufferT::BufferT(const std::string& ResName, bool ForceMono)
    : References(0),
      m_ResName(ResName),
      m_ForceMono(ForceMono)
{
}


unsigned int BufferT::ConvertToMono(unsigned char* Buffer, unsigned int Size)
{
    short* BufferShort=(short*)Buffer;

    for (unsigned int i=0; i<Size/4; i++)
    {
        BufferShort[i]=(short)(((int)BufferShort[i*2]+(int)BufferShort[i*2+1])/2);
    }

    return Size/2;
}
