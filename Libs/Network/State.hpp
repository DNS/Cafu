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

#ifndef CAFU_NETWORK_STATE_HPP_INCLUDED
#define CAFU_NETWORK_STATE_HPP_INCLUDED

#include "Templates/Array.hpp"

#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#undef GetProp
#else
#include <netinet/in.h>
#endif

#if defined(_WIN32) && _MSC_VER<1600
#include "pstdint.h"            // Paul Hsieh's portable implementation of the stdint.h header.
#else
#include <stdint.h>
#endif


namespace cf {
namespace Network {


/// This class holds the serialized state of another object (typically a game entity).
/// It is used in combination with the OutStreamT and InStreamT classes, which handle
/// the serialization and deserialization.
/// The delta messages created and taken by this class are intended for network transfer,
/// disk storage, etc.
/// Internally, the encapsulated state is kept as an array of raw bytes, and as such it
/// is "opaque" -- we don't have any idea of the meaning of the contained data.
class StateT
{
    public:

    /// Constructor for creating an empty StateT instance.
    /// The constructed instance is typically used with an OutStreamT,
    /// filled indirectly in a call to e.g. BaseEntityT::Serialize(OutStreamT& Stream).
    StateT();

    /// Constructor for creating a state from another StateT instance and a delta message.
    /// @param Other          The other state to create this state from.
    /// @param DeltaMessage   The delta message that expresses how this state is different from Other.
    StateT(const StateT& Other, const ArrayT<uint8_t>& DeltaMessage);

    /// Creates a delta message from this state.
    /// The delta message expresses how this state is different from another.
    /// @param Other      The other state that the generated delta message is relative to.
    /// @param Compress   Whether the delta message should be RLE-compressed.
    ArrayT<uint8_t> GetDeltaMessage(const StateT& Other, bool Compress=true) const;


    private:

    friend class InStreamT;
    friend class OutStreamT;

    ArrayT<uint8_t> m_Data;
};


/// This class is used for writing data into a StateT instance (serialization).
/// TODO: Optimize handling of strings and bools.
class OutStreamT
{
    public:

    /// Creates a stream for writing data into the given state.
    OutStreamT(StateT& State, bool Overwrite=true)
        : m_Data(State.m_Data)
    {
        if (Overwrite)
            m_Data.Overwrite();
    }

    //@{
    /// Writes a data item of the given type into the stream.
    OutStreamT& operator << (char b)
    {
        m_Data.PushBack(b);

        return *this;
    }

    OutStreamT& operator << (unsigned short w)
    {
        m_Data.PushBackEmpty(2);
        *(unsigned short*)&m_Data[m_Data.Size()-2]=htons(w);

        return *this;
    }

    OutStreamT& operator << (uint32_t ui)
    {
        m_Data.PushBackEmpty(4);
        *(uint32_t*)&m_Data[m_Data.Size()-4]=htonl(ui);

        return *this;
    }

    OutStreamT& operator << (float f)
    {
        *this << *(uint32_t*)&f;

        return *this;
    }

    OutStreamT& operator << (const char* s)
    {
        if (!s) s="NULL";

        const unsigned long Start =m_Data.Size();
        const unsigned long Length=(unsigned long)strlen(s)+1;

        m_Data.PushBackEmpty(Length);
        memcpy(&m_Data[Start], s, Length);

        return *this;
    }

    OutStreamT& operator << (const std::string& str)
    {
        *this << str.c_str();

        return *this;
    }

    template <class T> OutStreamT& operator << (const ArrayT<T>& A)
    {
        *this << uint32_t(A.Size());

        for (unsigned long i=0; i<A.Size(); i++)
            *this << A[i];

        return *this;
    }
    //@}


    private:

    ArrayT<uint8_t>& m_Data;    ///< The buffer that this stream is writing to.
};


/// This class is used for reading data from a StateT instance (deserialization).
/// TODO: Optimize handling of strings and bools.
class InStreamT
{
    public:

    /// Creates a stream for reading data from the given state.
    InStreamT(const StateT& State)
        : m_Data(State.m_Data),
          m_ReadPos(0),
          m_ReadOfl(false)
    {
    }

    /// Re-initializes reading from the stream.
    void Restart()
    {
        m_ReadPos=0;
        m_ReadOfl=false;
    }

    //@{
    /// Reads a data item of the given type from the stream.
    InStreamT& operator >> (char& b)
    {
        if (m_ReadPos+1 > m_Data.Size()) { m_ReadOfl=true; return *this; }

        b = m_Data[m_ReadPos];

        m_ReadPos+=1;
        return *this;
    }

    InStreamT& operator >> (unsigned short& w)
    {
        if (m_ReadPos+2 > m_Data.Size()) { m_ReadOfl=true; return *this; }

        w = ntohs(*(unsigned short*)&m_Data[m_ReadPos]);

        m_ReadPos+=2;
        return *this;
    }

    InStreamT& operator >> (uint32_t& ui)
    {
        if (m_ReadPos+4>m_Data.Size()) { m_ReadOfl=true; return *this; }

        ui = ntohl(*(uint32_t*)&m_Data[m_ReadPos]);

        m_ReadPos+=4;
        return *this;
    }

    InStreamT& operator >> (float& f)
    {
        uint32_t ui=0;

        *this >> ui;
        f = *(float*)&ui;

        return *this;
    }

    InStreamT& operator >> (char* s)
    {
        const unsigned long Start=m_ReadPos;

        while (m_ReadPos < m_Data.Size() && m_Data[m_ReadPos] != 0)
            m_ReadPos++;

        // Also account for the trailing 0 character.
        m_ReadPos++;

        if (m_ReadPos > m_Data.Size()) { m_ReadOfl=true; return *this; }

        memcpy(s, &m_Data[Start], m_ReadPos-Start);

        return *this;
    }

    InStreamT& operator >> (std::string& s)
    {
        const unsigned long Start=m_ReadPos;

        while (m_ReadPos < m_Data.Size() && m_Data[m_ReadPos] != 0)
            m_ReadPos++;

        // Also account for the trailing 0 character.
        m_ReadPos++;

        if (m_ReadPos > m_Data.Size()) { m_ReadOfl=true; return *this; }

        s=std::string((char*)&m_Data[Start], (m_ReadPos-1)-Start);

        return *this;
    }

    template <class T> InStreamT& operator >> (ArrayT<T>& A)
    {
        uint32_t Size=0;

        *this >> Size;
        A.Overwrite();
        A.PushBackEmptyExact(Size);

        for (uint32_t i=0; i<Size; i++)
            *this >> A[i];

        return *this;
    }
    //@}


    private:

    const ArrayT<uint8_t>& m_Data;      ///< The buffer that this stream is reading from.
    unsigned long          m_ReadPos;   ///< Current read position in the data.
    bool                   m_ReadOfl;   ///< Whether an attempt was made to read over the data buffer boundaries.
};

}   // namespace Network
}   // namespace cf

#endif
