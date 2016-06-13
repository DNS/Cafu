/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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


uint64_t MemoryInFileT::GetPos() const
{
    return ReadPos;
}


bool MemoryInFileT::Seek(int32_t Offset, SeekFromT SeekFrom)
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
            const long int NewPos=ReadPos+Offset;
            if (NewPos<0) return false;
            ReadPos=NewPos;
            break;
        }

        case FROM_END:
        {
            const long int NewPos=Buffer.Size()+Offset;
            if (NewPos<0) return false;
            ReadPos=NewPos;
            break;
        }
    }

    return true;
}


uint32_t MemoryInFileT::Read(char* ToBuffer, uint32_t Size)
{
    if (ReadPos>=Buffer.Size()) return 0;

    const unsigned long BytesLeft=Buffer.Size()-ReadPos;

    if (Size>BytesLeft) Size=BytesLeft;

    memcpy(ToBuffer, &Buffer[ReadPos], Size);
    ReadPos+=Size;

    return Size;
}


uint64_t MemoryInFileT::GetSize() const
{
    return Buffer.Size();
}
