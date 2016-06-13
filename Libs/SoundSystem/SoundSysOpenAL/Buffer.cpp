/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
