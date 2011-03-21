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

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "../Ca3DE/NetConst.hpp"


FILE* InFile=NULL;

char ReadByte()
{
    char Data1=0xFF;

    if (fread(&Data1, 1, 1, InFile)==0) { }
    return Data1;
}


unsigned short ReadWord(bool n2h=true)
{
    unsigned short Data2=0xFFFF;

    if (fread(&Data2, 2, 1, InFile)==0) { }
    return n2h ? ntohs(Data2) : Data2;
}


unsigned long ReadLong(bool n2h=true)
{
    unsigned long Data4=0xFFFFFFFF;

    if (fread(&Data4, 4, 1, InFile)==0) { }
    return n2h ? ntohl(Data4) : Data4;
}


float ReadFloat()
{
    unsigned long l=ReadLong();

    return *(float*)&l;
}


void CloseInFile()
{
    if (InFile) fclose(InFile);
    InFile=NULL;
}


int main(int ArgC, const char* ArgV[])
{
    printf("ReadDump: Reads network dumps in libpcap format and\n");
    printf("          decodes them with Cafu network protocol\n");
    printf("\n");

    if (ArgC!=2)
    {
        printf("USAGE: ReadDump <Path/FileName>\n");
        return 1;
    }


    InFile=fopen(ArgV[1], "rb");

    if (InFile==NULL)
    {
        printf("ERROR: Unable to open '%s'\n", ArgV[1]);
        return 1;
    }

    atexit(CloseInFile);


    // Header
    unsigned long MagicNumber=ReadLong(false);
    printf("Magic number             0x%lX\n", MagicNumber);
    printf("Version                  %u.%u\n", ReadWord(false), ReadWord(false));
    printf("TimeZone                 %li\n", ReadLong(false));
    printf("Accuracy of TimeStamps   %lu\n", ReadLong(false));
    printf("Max length of packets    %lu\n", ReadLong(false));

    unsigned long DataLink=ReadLong(false);
    printf("Data link type           %lu\n", DataLink);
    printf("\n");

    if (MagicNumber!=0xA1B2C3D4)
    {
        printf("ERROR: Bad magic number 0x%lX, expected 0xA1B2C3D4\n", MagicNumber);
        return 1;
    }

    if (DataLink!=1)
    {
        printf("ERROR: Bad data link type %lu, expected 1 (Ethernet 10Mb)\n", DataLink);
        return 1;
    }

    while (!feof(InFile))
    {
        // 1. Skip any remaining data from the last packet!
        static unsigned long BytesRemaining=0;

        while (BytesRemaining)
        {
            ReadByte();
            BytesRemaining--;
        }


        // 2. Normal processing of next packet
        static unsigned long PacketNumber   =1;
        static double        TimeStampOffset=0;

        unsigned long TimeStampSeconds    =ReadLong(false);
        unsigned long TimeStampMicroSec   =ReadLong(false);
        unsigned long PacketLengthCaptured=ReadLong(false);      // Number of bytes saved in file
        unsigned long PacketLength        =ReadLong(false);      // Number of bytes of actual packet

        BytesRemaining=PacketLengthCaptured;

        if (PacketNumber==1) TimeStampOffset=double(TimeStampSeconds)+double(TimeStampMicroSec)/1000000.0;
        printf("%5lu", PacketNumber);
        // printf("%5u", PacketLength);
        printf("%11.6f", double(TimeStampSeconds)+double(TimeStampMicroSec)/1000000.0-TimeStampOffset);
        PacketNumber++;

        if (PacketLength!=PacketLengthCaptured)
        {
            printf("\nBAD: PacketLength!=PacketLengthCaptured (%lu!=%lu)\n", PacketLength, PacketLengthCaptured);
            printf("     *** Make sure that packets are not saved truncated after capturing! ***\n");
            continue;
        }

/*      if (BytesRemaining<xxxx)
        {
            printf("\nBAD: packet too short! Expected at least xxx, found only xxx\n");
            continue;
        }*/

        char EthernetDest  [6];
        char EthernetSource[6];
        char c;

        for (c=0; c<6; c++) { EthernetDest  [c]=ReadByte(); BytesRemaining--; }
        for (c=0; c<6; c++) { EthernetSource[c]=ReadByte(); BytesRemaining--; }
        // printf(" %02X:%02X:%02X:%02X:%02X:%02X", EthernetSource[0], EthernetSource[1], EthernetSource[2], EthernetSource[3], EthernetSource[4], EthernetSource[5]);
        // printf(" %02X:%02X:%02X:%02X:%02X:%02X", EthernetDest  [0], EthernetDest  [1], EthernetDest  [2], EthernetDest  [3], EthernetDest  [4], EthernetDest  [5]);

        unsigned short EthernetNextProtocol=ReadWord(); BytesRemaining-=2;

        if (EthernetNextProtocol!=0x0800)
        {
            printf("\nBAD: Network layer protocol is not IP, expected 0x0800, found 0x%04X\n", EthernetNextProtocol);
            continue;
        }

        char IPVerOpts=ReadByte(); BytesRemaining--;

        if (IPVerOpts!=0x45)
        {
            printf("\nBAD: IP version or options field, expected 0x45, got 0x%02X\n", IPVerOpts);
            continue;
        }

        char TypeOfService=ReadByte(); BytesRemaining--;

        if (TypeOfService)
        {
            printf("\nBAD: Unknown type of service 0x%02X\n", TypeOfService);
            continue;
        }

        unsigned short IPTotalLength=ReadWord(); BytesRemaining-=2;
        printf("%5u", IPTotalLength);

        ReadByte(); BytesRemaining--;   // 16-bit Identification
        ReadByte(); BytesRemaining--;
        ReadByte(); BytesRemaining--;   // Flags
        ReadByte(); BytesRemaining--;   // Fragmentation offset

        char TimeToLive=ReadByte(); BytesRemaining--;
        printf(" TTL%4u", TimeToLive);

        char TransportLayerProtocol=ReadByte(); BytesRemaining--;

        if (TransportLayerProtocol!=17)
        {
            printf("\nBAD: Transport layer protocol is not UDP, expected 17, got %u\n", TransportLayerProtocol);
            continue;
        }

        unsigned short IPHeaderChecksum=ReadWord(); BytesRemaining-=2;
        printf(" IP Checksum %u\n", IPHeaderChecksum);

        char IPSource[4];
        char IPDest  [4];

        for (c=0; c<4; c++) { IPSource[c]=ReadByte(); BytesRemaining--; }
        for (c=0; c<4; c++) { IPDest  [c]=ReadByte(); BytesRemaining--; }
        // printf(" %3u.%3u.%3u.%3u", IPSource[0], IPSource[1], IPSource[2], IPSource[3]);
        // printf(" %3u.%3u.%3u.%3u", IPDest  [0], IPDest  [1], IPDest  [2], IPDest  [3]);

        unsigned short SourcePort =ReadWord(); BytesRemaining-=2;
        unsigned short DestPort   =ReadWord(); BytesRemaining-=2;
        unsigned short UDPLength  =ReadWord(); BytesRemaining-=2;
        unsigned short UDPChecksum=ReadWord(); BytesRemaining-=2;

        unsigned short DataLength=UDPLength-8;  // UDP Header is 8 Bytes large
        printf("UDP Checksum %u  DataLen %4u", UDPChecksum, DataLength);

        if (SourcePort!=30000 && DestPort!=30000)
        {
            printf("\nBAD: Neither source port nor destination port is Cafu default server port 30000\n");
            continue;
        }

        unsigned long SequNr1=ReadLong(); BytesRemaining-=4;
        unsigned long SequNr2=ReadLong(); BytesRemaining-=4;

        if (SourcePort==30000)
        {
            printf(" Sv->Cl");

            if (SequNr1==0xFFFFFFFF) printf(" CONNLESS");
                                else printf(" %08lX", SequNr1);
            printf(" %08lX", SequNr2);

            if (SequNr1==0xFFFFFFFF)
            {
                // decode connection-less packets from server to client
                switch (ReadByte())
                {
                    case SC0_ACK:  printf(" ACK "); break;
                    case SC0_NACK: printf(" NACK");
                                   while (true)
                                   {
                                       char c=ReadByte();

                                       BytesRemaining--;
                                       DataLength--;
                                       if (!c) break;
                                       printf("%c", c);
                                   }
                                   break;
                    default:       printf(" UNKN");
                }
                BytesRemaining--;
                DataLength--;
            }
            else
            {
                // decode connected packets from server to client
                while (true)
                {
                    // printf(" BytesRemaining %u   DataLength %u\n", BytesRemaining, DataLength);
                    if (BytesRemaining==0 || DataLength<=8) { printf("###\n"); break; }

                    char NextCommand=ReadByte(); BytesRemaining--; DataLength--;

                    printf(" | ");

                    switch (NextCommand)
                    {
                        case SC1_WorldInfo:
                        {
                            printf("WorldInfo ");

                            // GameName
                            while (true)
                            {
                                char c=ReadByte();

                                BytesRemaining--;
                                DataLength--;
                                if (!c) break;
                                printf("%c", c);
                            }
                            printf(" ");

                            // WorldName
                            while (true)
                            {
                                char c=ReadByte();

                                BytesRemaining--;
                                DataLength--;
                                if (!c) break;
                                printf("%c", c);
                            }
                            printf(" ");

                            printf("OurEntID %3lu", ReadLong());
                            BytesRemaining-=4;
                            DataLength-=4;
                            break;
                        }

                        case SC1_EntityBaseLine:
                        {
                            printf("EntBaseLine ");

                            unsigned long EntityID =ReadLong(); BytesRemaining-=4; DataLength-=4;
                            unsigned long TypeID   =ReadByte(); BytesRemaining-=1; DataLength-=1;
                            unsigned long MapFileID=ReadLong(); BytesRemaining-=4; DataLength-=4;

                            printf("EntID %3lu, Type%3lu, MapFileID %3lu", EntityID, TypeID, MapFileID);

                            unsigned long  FieldMaskBaseLine1=ReadLong(); BytesRemaining-=4; DataLength-=4;
                            unsigned short FieldMaskBaseLine2=0; if (FieldMaskBaseLine1 & 0x02000000) { FieldMaskBaseLine2=ReadWord(); BytesRemaining-=2; DataLength-=2; }
                            unsigned long  FieldMaskBaseLine3=0; if (FieldMaskBaseLine1 & 0x04000000) { FieldMaskBaseLine3=ReadLong(); BytesRemaining-=4; DataLength-=4; }

                            if (FieldMaskBaseLine1 & 0x00000001) { ReadFloat(); BytesRemaining-=4; DataLength-=4; }
                            if (FieldMaskBaseLine1 & 0x00000002) { ReadFloat(); BytesRemaining-=4; DataLength-=4; }
                            if (FieldMaskBaseLine1 & 0x00000004) { ReadFloat(); BytesRemaining-=4; DataLength-=4; }
                            if (FieldMaskBaseLine1 & 0x00000008) { ReadFloat(); BytesRemaining-=4; DataLength-=4; }
                            if (FieldMaskBaseLine1 & 0x00000010) { ReadFloat(); BytesRemaining-=4; DataLength-=4; }
                            if (FieldMaskBaseLine1 & 0x00000020) { ReadFloat(); BytesRemaining-=4; DataLength-=4; }
                            if (FieldMaskBaseLine1 & 0x00000040) { ReadFloat(); BytesRemaining-=4; DataLength-=4; }
                            if (FieldMaskBaseLine1 & 0x00000080) { ReadFloat(); BytesRemaining-=4; DataLength-=4; }
                            if (FieldMaskBaseLine1 & 0x00000100) { ReadWord (); BytesRemaining-=2; DataLength-=2; }
                            if (FieldMaskBaseLine1 & 0x00000200) { ReadWord (); BytesRemaining-=2; DataLength-=2; }
                            if (FieldMaskBaseLine1 & 0x00000400) { ReadWord (); BytesRemaining-=2; DataLength-=2; }
                            if (FieldMaskBaseLine1 & 0x00000800) { ReadByte (); BytesRemaining-=1; DataLength-=1; }
                            if (FieldMaskBaseLine1 & 0x00001000) { ReadByte (); BytesRemaining-=1; DataLength-=1; }
                            if (FieldMaskBaseLine1 & 0x00002000) { printf(" "); while (true) { char c=ReadByte(); BytesRemaining--; DataLength--; printf("%c", c); if (!c) break; } }
                            if (FieldMaskBaseLine1 & 0x00004000) { ReadByte (); BytesRemaining-=1; DataLength-=1; }
                            if (FieldMaskBaseLine1 & 0x00008000) { ReadByte (); BytesRemaining-=1; DataLength-=1; }
                            if (FieldMaskBaseLine1 & 0x00010000) { ReadFloat(); BytesRemaining-=4; DataLength-=4; }
                            if (FieldMaskBaseLine1 & 0x00020000) { ReadByte (); BytesRemaining-=1; DataLength-=1; }
                            if (FieldMaskBaseLine1 & 0x00040000) { ReadByte (); BytesRemaining-=1; DataLength-=1; }
                            if (FieldMaskBaseLine1 & 0x00080000) { ReadLong (); BytesRemaining-=4; DataLength-=4; }
                            if (FieldMaskBaseLine1 & 0x00100000) { ReadLong (); BytesRemaining-=4; DataLength-=4; }
                            if (FieldMaskBaseLine1 & 0x00200000) { ReadByte (); BytesRemaining-=1; DataLength-=1; }
                            if (FieldMaskBaseLine1 & 0x00400000) { ReadByte (); BytesRemaining-=1; DataLength-=1; }
                            if (FieldMaskBaseLine1 & 0x00800000) { ReadFloat(); BytesRemaining-=4; DataLength-=4; }
                            if (FieldMaskBaseLine1 & 0x01000000) { ReadLong (); BytesRemaining-=4; DataLength-=4; }

                            char Nr;

                            for (Nr=0; Nr<16; Nr++) if (FieldMaskBaseLine2 & (1L << Nr)) { ReadWord (); BytesRemaining-=2; DataLength-=2; }
                            for (Nr=0; Nr<32; Nr++) if (FieldMaskBaseLine3 & (1L << Nr)) { ReadByte (); BytesRemaining-=1; DataLength-=1; }
                            break;
                        }

                        case SC1_FrameInfo:
                            printf("FrameInfo%6lu", ReadLong());
                            printf("%6lu", ReadLong());
                            BytesRemaining-=8;
                            DataLength-=8;
                            break;

                        case SC1_EntityUpdate:
                        {
                            printf("SC1_EntityUpdate ");

                            unsigned long EntID     =ReadLong(); BytesRemaining-=4; DataLength-=4; printf("NewEntID  %6lu", EntID     );
                            unsigned long FieldMask1=ReadLong(); BytesRemaining-=4; DataLength-=4; // printf("FieldMask %6u", FieldMask1);

                            unsigned short FieldMask2=0; if (FieldMask1 & 0x02000000) { FieldMask2=ReadWord(); BytesRemaining-=2; DataLength-=2; }
                            unsigned long  FieldMask3=0; if (FieldMask1 & 0x04000000) { FieldMask3=ReadLong(); BytesRemaining-=4; DataLength-=4; }

                            if (FieldMask1 & 0x00000001) { ReadFloat(); BytesRemaining-=4; DataLength-=4; }
                            if (FieldMask1 & 0x00000002) { ReadFloat(); BytesRemaining-=4; DataLength-=4; }
                            if (FieldMask1 & 0x00000004) { ReadFloat(); BytesRemaining-=4; DataLength-=4; }
                            if (FieldMask1 & 0x00000008) { ReadFloat(); BytesRemaining-=4; DataLength-=4; }
                            if (FieldMask1 & 0x00000010) { ReadFloat(); BytesRemaining-=4; DataLength-=4; }
                            if (FieldMask1 & 0x00000020) { ReadFloat(); BytesRemaining-=4; DataLength-=4; }
                            if (FieldMask1 & 0x00000040) { ReadFloat(); BytesRemaining-=4; DataLength-=4; }
                            if (FieldMask1 & 0x00000080) { ReadFloat(); BytesRemaining-=4; DataLength-=4; }
                            if (FieldMask1 & 0x00000100) { ReadWord (); BytesRemaining-=2; DataLength-=2; }
                            if (FieldMask1 & 0x00000200) { ReadWord (); BytesRemaining-=2; DataLength-=2; }
                            if (FieldMask1 & 0x00000400) { ReadWord (); BytesRemaining-=2; DataLength-=2; }
                            if (FieldMask1 & 0x00000800) { ReadByte (); BytesRemaining-=1; DataLength-=1; }
                            if (FieldMask1 & 0x00001000) { ReadByte (); BytesRemaining-=1; DataLength-=1; }
                            if (FieldMask1 & 0x00002000) { printf(" "); while (true) { char c=ReadByte(); BytesRemaining--; DataLength--; printf("%c", c); if (!c) break; } }
                            if (FieldMask1 & 0x00004000) { ReadByte (); BytesRemaining-=1; DataLength-=1; }
                            if (FieldMask1 & 0x00008000) { ReadByte (); BytesRemaining-=1; DataLength-=1; }
                            if (FieldMask1 & 0x00010000) { ReadFloat(); BytesRemaining-=4; DataLength-=4; }
                            if (FieldMask1 & 0x00020000) { ReadByte (); BytesRemaining-=1; DataLength-=1; }
                            if (FieldMask1 & 0x00040000) { ReadByte (); BytesRemaining-=1; DataLength-=1; }
                            if (FieldMask1 & 0x00080000) { ReadLong (); BytesRemaining-=4; DataLength-=4; }
                            if (FieldMask1 & 0x00100000) { ReadLong (); BytesRemaining-=4; DataLength-=4; }
                            if (FieldMask1 & 0x00200000) { ReadByte (); BytesRemaining-=1; DataLength-=1; }
                            if (FieldMask1 & 0x00400000) { ReadByte (); BytesRemaining-=1; DataLength-=1; }
                            if (FieldMask1 & 0x00800000) { ReadFloat(); BytesRemaining-=4; DataLength-=4; }
                            if (FieldMask1 & 0x01000000) { ReadLong (); BytesRemaining-=4; DataLength-=4; }

                            char Nr;

                            for (Nr=0; Nr<16; Nr++) if (FieldMask2 & (1L << Nr)) { ReadWord (); BytesRemaining-=2; DataLength-=2; }
                            for (Nr=0; Nr<32; Nr++) if (FieldMask3 & (1L << Nr)) { ReadByte (); BytesRemaining-=1; DataLength-=1; }
                            break;
                        }

                        case SC1_DropClient:
                            printf("SC1_DropClient %3lu Reason: ", ReadLong()); BytesRemaining-=4; DataLength-=4;

                            // Reason.
                            while (true)
                            {
                                char c=ReadByte();

                                BytesRemaining--;
                                DataLength--;
                                if (!c) break;
                                printf("%c", c);
                            }
                            printf(" ");
                            break;

                        case SC1_ChatMsg:
                            printf("ChatMsg ");
                            while (true)
                            {
                                char c=ReadByte();

                                BytesRemaining--;
                                DataLength--;
                                if (!c) break;
                                printf("%c", c);
                            }
                            break;

                        default:
                            printf("UNKNOWN 0x%X", NextCommand);
                    }
                }
            }
        }
        else  // DestPort==30000
        {
            printf(" Cl->Sv");

            if (SequNr1==0xFFFFFFFF) printf(" CONNLESS");
                                else printf(" %08lX", SequNr1);
            printf(" %08lX", SequNr2);

            if (SequNr1==0xFFFFFFFF)
            {
                // decode connection-less packets from client to server
                switch (ReadByte())
                {
                    case CS0_NoOperation: printf(" NOP "); break;
                    case CS0_Ping:        printf(" PING"); break;
                    case CS0_Connect:     printf(" CONN"); break;
                    case CS0_Info:        printf(" INFO"); break;
                    default:              printf(" UNKN");
                }
                BytesRemaining--;
                DataLength--;
            }
            else
            {
                // decode connected packets from client to server
                /*  char ClientID=ReadByte(); // OBSOLETE

                BytesRemaining--;
                DataLength--;

                printf(" ClID%2u", ClientID); */

                while (true)
                {
                    char NextCommand=ReadByte();

                    BytesRemaining--;
                    DataLength--;

                    if (DataLength==0) break;
                    if (NextCommand==0xFF)
                    {
                        printf(" ###");
                        break;
                    }

                    printf(" | ");

                    switch (NextCommand)
                    {
                        /* case CS1_PlayerCmd:  // VERALTET! PRÜFEN!
                        {
                            printf("PlayerCmd Keys 0x%08X", ReadLong());
                            printf(" Hdg %5u", ReadWord());

                            unsigned long FrameTime=ReadLong());
                            printf(  " FrameTime%5.2f", *(float*)&FrameTime);
                            BytesRemaining-=10;
                            DataLength-=10;
                            break;
                        } */

                        case CS1_Disconnect:
                            printf("Disconnect");
                            break;

                        case CS1_SayToAll:
                            printf("SayToAll ");
                            while (true)
                            {
                                char c=ReadByte();

                                BytesRemaining--;
                                DataLength--;
                                if (!c) break;
                                printf("%c", c);
                            }
                            break;

                        case CS1_WorldInfoACK:
                            printf("WorldInfoACK ");
                            while (true)
                            {
                                char c=ReadByte();

                                BytesRemaining--;
                                DataLength--;
                                if (!c) break;
                                printf("%c", c);
                            }
                            break;

                        /* case CS1_FrameInfo: // VERALTET! PRÜFEN!
                            printf("FrameInfo%6u", ReadLong());
                            BytesRemaining-=4;
                            DataLength-=4;
                            break; */

                        default:
                            printf("UNKNOWN 0x%X", NextCommand);
                    }
                }
            }
        }

        printf("\n");
    }

    return 0;
}
