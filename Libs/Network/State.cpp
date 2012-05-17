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

#include "State.hpp"

using namespace cf::Network;


StateT::StateT(const StateT& Other, const ArrayT<uint8_t>& DeltaMessage)
{
    // TODO: if (DeltaMessage[0] == 1) decompress...
    m_Data.PushBackEmptyExact(DeltaMessage.Size() - 1);

    for (unsigned long i=0; i<m_Data.Size(); i++)
    {
        m_Data[i] = (i < Other.m_Data.Size() ? Other.m_Data[i] : 0) ^ DeltaMessage[i+1];
    }
}


ArrayT<uint8_t> StateT::GetDeltaMessage(const StateT& Other, bool Compress) const
{
    ArrayT<uint8_t> DeltaMessage;

    DeltaMessage.PushBackEmptyExact(1 + m_Data.Size());
    DeltaMessage.Overwrite();

    // The first byte holds the flags.
    DeltaMessage.PushBack(Compress ? 1 : 0);

    // Write the delta message.
    for (unsigned long i=0; i<m_Data.Size(); i++)
    {
        DeltaMessage.PushBack(m_Data[i] ^ (i < Other.m_Data.Size() ? Other.m_Data[i] : 0));
    }

    // TODO: Implement RLE compression if Compress==true.
    return DeltaMessage;
}
