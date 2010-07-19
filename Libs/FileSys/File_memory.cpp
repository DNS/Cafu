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

#include "File_memory.hpp"
#include <cstring>

using namespace cf::FileSys;


MemoryInFileT::MemoryInFileT(const std::string& FileName_)
    : FileName(FileName_),
      ReadPos(0)
{
}


bool MemoryInFileT::IsOpen() const
{
    return true;
}


const std::string& MemoryInFileT::GetBaseName() const
{
    return FileName;
}


const std::string& MemoryInFileT::GetFullName() const
{
    return FileName;
}


size_t MemoryInFileT::GetPos() const
{
    return ReadPos;
}


bool MemoryInFileT::Seek(int Offset, SeekFromT SeekFrom)
{
    switch (SeekFrom)
    {
        case FROM_BEGINNING:
        {
            if (Offset<0) return false;
            ReadPos=Offset;
            break;
        }

        case FROM_CURRENT_POS:
        {
            const int NewPos=ReadPos+Offset;
            if (NewPos<0) return false;
            ReadPos=NewPos;
            break;
        }

        case FROM_END:
        {
            const int NewPos=Buffer.Size()+Offset;
            if (NewPos<0) return false;
            ReadPos=NewPos;
            break;
        }
    }

    return true;
}


size_t MemoryInFileT::Read(char* ToBuffer, size_t Size)
{
    if (ReadPos>=Buffer.Size()) return 0;

    const unsigned long BytesLeft=Buffer.Size()-ReadPos;

    if (Size>BytesLeft) Size=BytesLeft;

    memcpy(ToBuffer, &Buffer[ReadPos], Size);
    ReadPos+=(unsigned long)Size;

    return Size;
}


size_t MemoryInFileT::GetSize() const
{
    return Buffer.Size();
}
