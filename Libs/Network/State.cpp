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

#undef min    // See http://stackoverflow.com/questions/5004858/stdmin-gives-error
#undef max

using namespace cf::Network;


namespace
{
    // We use a one byte header N, where N is interpreted as follows:
    //   N in [ 0,  63]: copy the next N+1 bytes verbatim.
    //   N in [64, 255]: repeat the next byte N-61 times.
    const unsigned int MAX_VERBATIM = 64;
    const unsigned int MAX_RUN      = 255-MAX_VERBATIM+3;

    void WriteVerbatimBlocks(ArrayT<uint8_t>& Dest, const uint8_t* Source, unsigned int StartVerbatim, unsigned int i)
    {
        while (StartVerbatim < i)
        {
            const uint8_t NumVerbatim = std::min(MAX_VERBATIM, i-StartVerbatim);

            Dest.PushBack(NumVerbatim-1);

            for (uint8_t j = 0; j < NumVerbatim; j++)
            {
                Dest.PushBack(Source[StartVerbatim]);
                StartVerbatim++;
            }
        }
    }

    void PackBits(ArrayT<uint8_t>& Dest, const uint8_t* Source, const unsigned int SourceLen)
    {
        unsigned int StartVerbatim = 0;

        for (unsigned int i = 0; i < SourceLen; i++)
        {
            bool StartRun = (i+2 < SourceLen) && (Source[i] == Source[i+1]) && (Source[i] == Source[i+2]);

            if (StartRun)
            {
                // 1. Write all verbatim blocks so far.
                WriteVerbatimBlocks(Dest, Source, StartVerbatim, i);
                StartVerbatim = i;

                // 2. Write a single run.
                while ((i < SourceLen) && (Source[i] == Source[StartVerbatim]) && (i-StartVerbatim < MAX_RUN))
                    i++;

                assert(i-StartVerbatim >= 3);

                Dest.PushBack(i-StartVerbatim-3+MAX_VERBATIM);
                Dest.PushBack(Source[StartVerbatim]);

                StartVerbatim = i;
            }
        }

        // Write the remaining verbatim blocks.
        WriteVerbatimBlocks(Dest, Source, StartVerbatim, SourceLen);
    }

    void UnpackBits(ArrayT<uint8_t>& Dest, const uint8_t* Source, const unsigned int SourceLen)
    {
        unsigned int i = 0;

        while (i < SourceLen)
        {
            const unsigned int N = Source[i];

            i++;

            if (N < MAX_VERBATIM)
            {
                for (unsigned int Stop = i+N+1; i < Stop; i++)
                    Dest.PushBack(Source[i]);
            }
            else
            {
                for (unsigned int j = 0; j < N-MAX_VERBATIM+3; j++)
                    Dest.PushBack(Source[i]);

                i++;
            }
        }
    }
}


StateT::StateT(const StateT& Other, const ArrayT<uint8_t>& DeltaMessage)
{
    if (DeltaMessage[0] == 1)
    {
        // Run the RLE-decompression.
        UnpackBits(m_Data, &DeltaMessage[1], DeltaMessage.Size()-1);
    }
    else
    {
        m_Data.PushBackEmptyExact(DeltaMessage.Size()-1);

        for (unsigned int i = 0; i < m_Data.Size(); i++)
            m_Data[i] = DeltaMessage[i+1];
    }

    // Run the delta-decompression.
    for (unsigned int i = 0; i < m_Data.Size(); i++)
        m_Data[i] ^= i < Other.m_Data.Size() ? Other.m_Data[i] : 0;
}


ArrayT<uint8_t> StateT::GetDeltaMessage(const StateT& Other, bool Compress) const
{
    // Delta-compress the data.
    static ArrayT<uint8_t> DeltaData;

    DeltaData.Overwrite();

    for (unsigned int i = 0; i < m_Data.Size(); i++)
        DeltaData.PushBack(m_Data[i] ^ (i < Other.m_Data.Size() ? Other.m_Data[i] : 0));

    // Optionally RLE-compress the data, then write the delta message.
    ArrayT<uint8_t> DeltaMessage;

    if (Compress)
    {
        DeltaMessage.PushBack(1);
        PackBits(DeltaMessage, &DeltaData[0], DeltaData.Size());
    }
    else
    {
        DeltaMessage.PushBack(0);
        DeltaMessage.PushBack(DeltaData);
    }

#if 0
    static std::ofstream Log("compress_log.txt");

    Log << "\n" << DeltaData.Size() << " bytes in original delta message\n";

    {
        uLongf          DestLen=compressBound(DeltaData.Size());
        ArrayT<uint8_t> Dest;

        Dest.PushBackEmptyExact(DestLen);

        const int Result=compress2(&Dest[0], &DestLen, &DeltaData[0], DeltaData.Size(), 9);

        Log << DestLen << " bytes in deflate-compressed message, ";
        Log << "compression result is " << Result << " (" << (Result == Z_OK ? "Z_OK" : "error") << ")\n";
    }

    {
        ArrayT<uint8_t> DestRLE;
        PackBits(DestRLE, &DeltaData[0], DeltaData.Size());

        Log << DestRLE.Size() << " bytes in RLE-compressed message.\n";

        ArrayT<uint8_t> DestRLE_CHECK;
        UnpackBits(DestRLE_CHECK, &DestRLE[0], DestRLE.Size());

        assert(DestRLE_CHECK == DeltaData);
    }
#endif

    return DeltaMessage;
}


/*static*/ bool StateT::IsDeltaMessageEmpty(const ArrayT<uint8_t>& DeltaMessage)
{
    if (DeltaMessage[0] == 1)
    {
        // Check the RLE-compressed delta message.
        unsigned int i = 1;

        while (i < DeltaMessage.Size())
        {
            const unsigned int N = DeltaMessage[i];

            i++;

            if (N < MAX_VERBATIM)
            {
                for (unsigned int Stop = i+N+1; i < Stop; i++)
                    if (DeltaMessage[i]) return false;
            }
            else
            {
                if (DeltaMessage[i]) return false;
                i++;
            }
        }
    }
    else
    {
        // Check the uncompressed delta message.
        for (unsigned int i = 1; i < DeltaMessage.Size(); i++)
            if (DeltaMessage[i])
                return false;
    }

    return true;
}
