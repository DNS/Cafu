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

/**********************/
/*** Network (Code) ***/
/**********************/

#include "Network.hpp"

#include <stdio.h>
#ifndef _WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#endif


/***************/
/*** WinSock ***/
/***************/

WinSockT::WinSockT(unsigned short RequestedVersion) /*throw (InitFailure, BadVersion)*/
{
#ifdef _WIN32
    WSAData WSA_Data;
    int     Result=WSAStartup(RequestedVersion, &WSA_Data);

    if (Result) throw InitFailure(Result);
    if (WSA_Data.wVersion!=RequestedVersion) throw BadVersion(RequestedVersion);
#endif
}

WinSockT::~WinSockT()
{
#ifdef _WIN32
    WSACleanup();   // Ignoriere das R�ckgabeergebnis (und das m�gliche Scheitern) von WSACleanup().
#endif
}

SOCKET WinSockT::GetTCPServerSocket(unsigned short PortNr) const
{
    SOCKET      ServerSocket=socket(AF_INET, SOCK_STREAM, 0);           // Neues Socket erzeugen
    sockaddr_in Adresse;

    if (ServerSocket==INVALID_SOCKET) return INVALID_SOCKET;

    Adresse.sin_family     =AF_INET;                                    // Address-Family: Internet
    Adresse.sin_port       =htons(PortNr);                              // Diesem Socket Port PortNr zuordnen,
    Adresse.sin_addr.s_addr=htonl(INADDR_ANY);                          // aber keine spezielle Zieladresse.

    // ServerSocket an Adresse binden, zum Listener-Socket machen und als non-blocking markieren.
    if (bind       (ServerSocket, (sockaddr*)&Adresse, sizeof(Adresse))==SOCKET_ERROR) return INVALID_SOCKET;
    if (listen     (ServerSocket, 8                                   )==SOCKET_ERROR) return INVALID_SOCKET;
#ifdef _WIN32
    unsigned long DummyTrue=1;
    if (ioctlsocket(ServerSocket, FIONBIO, &DummyTrue                 )==SOCKET_ERROR) return INVALID_SOCKET;
#else
    if (fcntl      (ServerSocket, F_SETFL, O_NONBLOCK                 )==SOCKET_ERROR) return INVALID_SOCKET;
#endif

    return ServerSocket;
}

SOCKET WinSockT::GetTCPClientSocket(const char* ServerAddress, unsigned short ServerPortNr) const
{
    SOCKET      ClientSocket=socket(AF_INET, SOCK_STREAM, 0);           // Neues Socket erzeugen
    sockaddr_in Adresse;

    if (ClientSocket==INVALID_SOCKET) return INVALID_SOCKET;

    Adresse.sin_family     =AF_INET;                                    // Address-Family: Internet
    Adresse.sin_port       =htons(ServerPortNr);                        // Diesem Socket den Port und die
    Adresse.sin_addr.s_addr=inet_addr(ServerAddress);                   // IP-Adresse des Server zuordnen.

    // Verbindung herstellen und Socket als non-blocking markieren.
    if (connect    (ClientSocket, (sockaddr*)&Adresse, sizeof(Adresse))==SOCKET_ERROR) return INVALID_SOCKET;
#ifdef _WIN32
    unsigned long DummyTrue=1;
    if (ioctlsocket(ClientSocket, FIONBIO, &DummyTrue                 )==SOCKET_ERROR) return INVALID_SOCKET;
#else
    if (fcntl      (ClientSocket, F_SETFL, O_NONBLOCK                 )==SOCKET_ERROR) return INVALID_SOCKET;
#endif

    return ClientSocket;
}

SOCKET WinSockT::GetUDPSocket(unsigned short PortNr) const
{
    SOCKET      UDPSocket=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);     // Neues Socket erzeugen
    sockaddr_in Adresse;

    if (UDPSocket==INVALID_SOCKET) return INVALID_SOCKET;

    Adresse.sin_family     =AF_INET;                                    // Address-Family: Internet
    Adresse.sin_port       =htons(PortNr);                              // Diesem Socket Port PortNr zuordnen,
    Adresse.sin_addr.s_addr=htonl(INADDR_ANY);                          // aber keine spezielle Zieladresse.

    // UDPSocket an PortNr binden und als non-blocking markieren.
    if (bind       (UDPSocket, (sockaddr*)&Adresse, sizeof(Adresse))==SOCKET_ERROR) return INVALID_SOCKET;
#ifdef _WIN32
    unsigned long DummyTrue=1;
    if (ioctlsocket(UDPSocket, FIONBIO, &DummyTrue                 )==SOCKET_ERROR) return INVALID_SOCKET;
#else
    if (fcntl      (UDPSocket, F_SETFL, O_NONBLOCK                 )==SOCKET_ERROR) return INVALID_SOCKET;
#endif

    return UDPSocket;
}


/***********************/
/*** Network Address ***/
/***********************/

NetAddressT::NetAddressT(char IP0, char IP1, char IP2, char IP3, unsigned short Port_)
{
    IP[0]=IP0;
    IP[1]=IP1;
    IP[2]=IP2;
    IP[3]=IP3;
    Port =Port_;
}

NetAddressT::NetAddressT(const char* Name, unsigned short Port_) /*throw (BadHostName)*/
{
    // Versuche 'Name' als einen String der Form 'a.b.c.d' zu interpretieren.
    unsigned long INetAddr=inet_addr(Name);

    if (INetAddr==INADDR_NONE)
    {
        // Hat nicht geklappt, versuche 'Name' als Name-String zu interpretieren (z.B. 'ThuBi-Bir' oder 'localhost').
        const hostent* HostEnt=gethostbyname(Name);

        if (!HostEnt) throw BadHostName();
        INetAddr=*(unsigned long*)(HostEnt->h_addr_list[0]);
    }

    IP[0]=char((INetAddr >>  0) & 0xFF);
    IP[1]=char((INetAddr >>  8) & 0xFF);
    IP[2]=char((INetAddr >> 16) & 0xFF);
    IP[3]=char((INetAddr >> 24) & 0xFF);
    Port =Port_;
}

NetAddressT::NetAddressT(const std::string& Name, unsigned short Port_) /*throw (BadHostName)*/
{
    // Versuche 'Name' als einen String der Form 'a.b.c.d' zu interpretieren.
    unsigned long INetAddr=inet_addr(Name.c_str());

    if (INetAddr==INADDR_NONE)
    {
        // Hat nicht geklappt, versuche 'Name' als Name-String zu interpretieren (z.B. 'ThuBi-Bir' oder 'localhost').
        const hostent* HostEnt=gethostbyname(Name.c_str());

        if (!HostEnt) throw BadHostName();
        INetAddr=*(unsigned long*)(HostEnt->h_addr_list[0]);
    }

    IP[0]=char((INetAddr >>  0) & 0xFF);
    IP[1]=char((INetAddr >>  8) & 0xFF);
    IP[2]=char((INetAddr >> 16) & 0xFF);
    IP[3]=char((INetAddr >> 24) & 0xFF);
    Port =Port_;
}

NetAddressT::NetAddressT(const sockaddr_in& SockAddrIn)
{
    IP[0]=char((SockAddrIn.sin_addr.s_addr >>  0) & 0xFF);
    IP[1]=char((SockAddrIn.sin_addr.s_addr >>  8) & 0xFF);
    IP[2]=char((SockAddrIn.sin_addr.s_addr >> 16) & 0xFF);
    IP[3]=char((SockAddrIn.sin_addr.s_addr >> 24) & 0xFF);
    Port =ntohs(SockAddrIn.sin_port);
}

const char* NetAddressT::ToString() const
{
    static char String[32];     // Max. L�nge ist 22 (23 mit abschlie�ender Null)

    sprintf(String, "%u.%u.%u.%u:%u", IP[0], IP[1], IP[2], IP[3], Port);
    return String;
}

sockaddr_in NetAddressT::ToSockAddrIn() const
{
    unsigned long IP0=IP[0];
    unsigned long IP1=IP[1];
    unsigned long IP2=IP[2];
    unsigned long IP3=IP[3];

    sockaddr_in Address;

    Address.sin_family     =AF_INET;
    Address.sin_addr.s_addr=(IP0 << 0) + (IP1 << 8) + (IP2 << 16) + (IP3 << 24);
    Address.sin_port       =htons(Port);
    memset(&Address.sin_zero, 0, sizeof(Address.sin_zero));

    return Address;
}

bool NetAddressT::operator == (const NetAddressT& Address) const
{
    return IP[0]==Address.IP[0] &&
           IP[1]==Address.IP[1] &&
           IP[2]==Address.IP[2] &&
           IP[3]==Address.IP[3] &&
           Port ==Address.Port;
}

bool NetAddressT::operator != (const NetAddressT& Address) const
{
    return IP[0]!=Address.IP[0] ||
           IP[1]!=Address.IP[1] ||
           IP[2]!=Address.IP[2] ||
           IP[3]!=Address.IP[3] ||
           Port !=Address.Port;
}


/**********************/
/*** Network Socket ***/
/**********************/

NetSocketT::NetSocketT(SOCKET Socket_)
    : Socket(Socket_)
{
}

NetSocketT::~NetSocketT()
{
    if (Socket!=INVALID_SOCKET) closesocket(Socket);
}

/*SOCKET*/ NetSocketT::operator SOCKET () const
{
    return Socket;
}


/********************/
/*** Network Data ***/
/********************/

void NetDataT::Send(SOCKET Socket, const NetAddressT& ReceiverAddress) const /*throw (WinSockAPIError, MessageLength)*/
{
    /*if (ReceiverAdress.IsLoopBack)
    {
        return;
    }*/

    sockaddr_in RA    =ReceiverAddress.ToSockAddrIn();
    int         Result=sendto(Socket, &Data[0], Data.Size(), 0, (sockaddr*)&RA, sizeof(RA));

    if (Result==SOCKET_ERROR) throw (WinSockAPIError(WSAGetLastError()));
    if ((unsigned long)(Result)<Data.Size()) throw (MessageLength(Data.Size(), Result));
}

NetAddressT NetDataT::Receive(SOCKET Socket) /*throw (WinSockAPIError)*/
{
    /*if (f�r Socket gibt es Daten am LoopBack)
    {
        return faked loopback-adress;
    }*/

    static char ReceiveBuffer[16384*4];
    sockaddr_in SenderAddr;
    socklen_t   SenderAddrLength=sizeof(SenderAddr);
    int         Result          =recvfrom(Socket, ReceiveBuffer, sizeof(ReceiveBuffer), 0, (sockaddr*)&SenderAddr, &SenderAddrLength);

    if (Result==SOCKET_ERROR) throw (WinSockAPIError(WSAGetLastError(), NetAddressT(SenderAddr)));
    ReadPos=0;
    ReadOfl=false;
    Data.Clear();
    Data.PushBackEmpty(Result);
    memcpy(&Data[0], ReceiveBuffer, Result);

    return NetAddressT(SenderAddr);
}


/*******************************/
/*** Game Network Protocol 1 ***/
/*******************************/

const unsigned long GameProtocol1T::ACK_FLAG    =1 << 31;       // 0x80000000
const unsigned long GameProtocol1T::ACK_MASK    =~ACK_FLAG;     // 0x7FFFFFFF
const unsigned long GameProtocol1T::MAX_MSG_SIZE=1400;          // IPv6 compliant

GameProtocol1T::GameProtocol1T()
{
    NextOutgoingSequenceNr=1;
    LastIncomingSequenceNr=0;
    LastReliableSequenceNr=0;

    ExpectedIncomingAckBit=0;
    ExpectedOutgoingAckBit=0;

    ResendReliableDataInFlight=false;

    // ReliableDataInFlight
    // ReliableDataQueue
}

const NetDataT& GameProtocol1T::GetTransmitData(const ArrayT< ArrayT<char> >& ReliableDatas, const ArrayT<char>& UnreliableData) /*throw (MaxMsgSizeExceeded)*/
{
    // Der Auftrag dieser Methode lautet, ein NetDataT Paket zu erstellen, um die 'ReliableDatas' und 'UnreliableData' an die Gegenstelle zu �bertragen.
    // Die 'UnreliableData' spielen dabei kaum eine Rolle, da wir sie bei Problemen einfach unter den Tisch fallen lassen k�nnen (s.u.).
    // Betrachten wir die 'ReliableDatas':
    // Weil immer nur eine "reliable Message" unterwegs ("in flight") sein kann, k�nnen wir die 'ReliableDatas' u.U. nicht sofort senden.
    // Au�erdem k�nnte der Gesamtinhalt von 'ReliableDatas' zu gro� sein f�r ein einzelnes Netzwerk-Paket.
    // Deshalb reihen wir alle 'ReliableDatas[i]' zun�chst einfach mal in die 'ReliableDatasQueue' ein (durch anh�ngen).
    // Vorher beachte aber, da� die kleinste Einheit, mit der wir uns besch�ftigen und die wir sp�ter auch versenden, ein 'ReliableDatas[i]' ist.
    // Thus make sure that for all i the condition 'ReliableDatas[i].Size()<=MAX_MSG_SIZE' holds.
    unsigned long Nr;

    for (Nr=0; Nr<ReliableDatas.Size(); Nr++)
        if (ReliableDatas[Nr].Size()>MAX_MSG_SIZE)
            throw MaxMsgSizeExceeded();

    // Add all 'ReliableDatas[i]' to the end of the 'ReliableDatasQueue'.
    for (Nr=0; Nr<ReliableDatas.Size(); Nr++)
        ReliableDatasQueue.PushBack(ReliableDatas[Nr]);

    // Falls im Moment keine reliable Daten unterwegs sind, aber in der Queue welche darauf warten,
    // abgeschickt zu werden, schicke soviele Daten wie m�glich vom Anfang der Queue als neue reliable Message auf den Weg.
    if (ReliableDataInFlight.Size()==0 && ReliableDatasQueue.Size()>0)
    {
        for (Nr=0; Nr<ReliableDatasQueue.Size(); Nr++)
        {
            if (ReliableDataInFlight.Size()+ReliableDatasQueue[Nr].Size()>MAX_MSG_SIZE)
            {
                // if (Nr==0) throw MaxMsgSizeExceeded();   // This would save the above test!
                break;
            }

            // ReliableDataInFlight.Append(ReliableDatasQueue[Nr].Data);
            for (unsigned long c=0; c<ReliableDatasQueue[Nr].Size(); c++)
                ReliableDataInFlight.PushBack(ReliableDatasQueue[Nr][c]);
        }

        // Delete elements 0...Nr-1 from the queue.
        unsigned long Nr2;

        for (Nr2=Nr; Nr2<ReliableDatasQueue.Size(); Nr2++) ReliableDatasQueue[Nr2-Nr]=ReliableDatasQueue[Nr2];
        for (Nr2= 0; Nr2<Nr; Nr2++) ReliableDatasQueue.DeleteBack();

        ResendReliableDataInFlight=true;
        ExpectedIncomingAckBit^=1;
    }

    // Ein Paket unseres Protokolls besteht aus einem Header (zwei LongWords 'a' und 'b') und der Payload 'c':
    // a1) Das oberste Bit (Bit 31) zeigt an, ob diese Nachricht best�tigt werden soll (die Nachricht enth�lt eine 'reliable message' als payload).
    // a2) Die n�chsten 31 Bits (Bits 30 bis 0) sind die gegenw�rtige SequenceNr des Pakets ("Hey, ich sende Dir eine Nachricht mit Nummer ...!").
    // b1) Das oberste Bit (Bit 31) des n�chsten LongWords enth�lt als Empfangsbest�tigung auf eine reliable Message ein umschlagendes Odd/Even-AckFlag.
    //     Das hei�t, wann immer eine zu best�tigende Nachricht empfangen wird, wird dieses Bit umgedreht. (Es ist immer nur eine rel. Msg unterwegs!)
    // b2) Restliche 31 Bits: SequenceNr des zuletzt von der Gegenstelle gesehenen Pakets.
    // c)  Die 'PayLoad'.
    static NetDataT OutData;
    OutData=NetDataT();

    // Packet-Header voranstellen.
    OutData.WriteLong(NextOutgoingSequenceNr | (ResendReliableDataInFlight ? ACK_FLAG : 0));
    OutData.WriteLong(LastIncomingSequenceNr | (ExpectedOutgoingAckBit << 31));

    // Die ReliableDataInFlight ggf. als erstes im Paket einf�gen, immer vor den UnreliableData
    if (ResendReliableDataInFlight)
    {
        OutData.WriteArrayOfBytes(ReliableDataInFlight);
        LastReliableSequenceNr=NextOutgoingSequenceNr;
        ResendReliableDataInFlight=false;
    }

    // Falls noch genug Platz �brig ist, h�nge die UnreliableData an. Der Empf�nger kann sp�ter nicht mehr zwischen dem
    // 'reliable' und 'unreliable' Teil der Nachricht unterscheiden, er bearbeitet sie einfach als eine einzige, gro�e Nachricht.
    if (OutData.Data.Size()+UnreliableData.Size()<=MAX_MSG_SIZE) OutData.WriteArrayOfBytes(UnreliableData);

    NextOutgoingSequenceNr++;
    return OutData;
}

unsigned long GameProtocol1T::ProcessIncomingMessage(NetDataT& InData, void (*ProcessPayload)(NetDataT& /*Payload*/, unsigned long /*LastIncomingSequenceNr*/))
{
    unsigned long RemoteThisOutgoingSequenceNr=InData.ReadLong();                   // Die Packet-SequenceNr der Gegenstelle
    unsigned long RemoteLastIncomingSequenceNr=InData.ReadLong();                   // Die letzte SequenceNr, die die Gegenstelle von uns gesehen hat
    unsigned long RemoteThisOutgoingAckBit    =RemoteThisOutgoingSequenceNr >> 31;  // Soll dieses Packet von uns best�tigt werden?
    unsigned long RemoteLastIncomingAckBit    =RemoteLastIncomingSequenceNr >> 31;  // Odd/Even Flag (Gegenstelle schl�gt um, wenn sie von uns ein reliable Packet empfangen hat)

    RemoteThisOutgoingSequenceNr&=ACK_MASK;                                         // Bereinige die RemoteThisOutgoingSequenceNr vom Flag
    RemoteLastIncomingSequenceNr&=ACK_MASK;                                         // Bereinige die RemoteLastIncomingSequenceNr vom Flag

    // Werte RemoteThisOutgoingSequenceNr aus (veraltete Packets werden vollst�ndig ignoriert)
 // if (RemoteThisOutgoingSequenceNr> LastIncomingSequenceNr+1) printf("Packet loss detected! (%u packet(s) lost)\n", ClientSequenceNr-LastClientSequenceNr-1);
 // if (RemoteThisOutgoingSequenceNr< LastIncomingSequenceNr  ) printf("Got an out-of-order packet! (I throw it away.)\n");
 // if (RemoteThisOutgoingSequenceNr==LastIncomingSequenceNr  ) printf("Got a  duplicate    packet! (I throw it away.)\n");
    if (RemoteThisOutgoingSequenceNr<=LastIncomingSequenceNr  ) return 0;

    // Zuletzt haben wir von der Gegenstelle dieses Packet gesehen, mit der Nummer 'RemoteThisOutgoingSequenceNr'
    LastIncomingSequenceNr=RemoteThisOutgoingSequenceNr;

    // Falls die Gegenstelle von uns eine Empfangsbest�tigung haben m�chte, drehe das ExpectedOutgoingAckBit um!
    if (RemoteThisOutgoingAckBit) ExpectedOutgoingAckBit^=1;

    // Die Gegenstelle selbst merkt nicht, ob ihr eine reliable Message durch die Lappen gegangen ist.
    // Wir merken das nur hier auf 'unserer' Seite, wenn wir eine 'RemoteLastIncomingSequenceNr' bekommen,
    // die gr��er oder gleich der zuletzt von uns gesendeten 'LastReliableSequenceNr' ist,
    // und zugleich das falsche Ack-Bit gesetzt ist (wurde nicht 'umgeschlagen').
    // Hat die Gegenstelle schon unser letztes reliable Packet gesehen (und evtl. sogar schon weitere 'j�ngere')?
    if (RemoteLastIncomingSequenceNr>=LastReliableSequenceNr)
    {
        // Falls die Gegenstelle ihr Ack-Bit umgestellt hat, hat sie das aufgrund unserer reliable Message getan,
        // und wir k�nnen die ReliableDataInFlight l�schen, um Platz f�r die n�chsten Daten (in ReliableDataQueue) zu machen.
        // Andernfalls ging die reliable Nachricht verloren, und wir m�ssen die alten Daten erneut senden!
        // Wichtig: Ein erneutes Senden (retransmit) wird immer nur hier ausgel��t, d.h. immer nur dann, wenn wir merken,
        // da� die letzte reliable Nachricht eigentlich schon h�tte angekommen sein m�ssen, das aber nicht der Fall war
        // (RemoteLastIncomingSequenceNr>=LastReliableSequenceNr, aber falsches AckBit).
        if (ExpectedIncomingAckBit==RemoteLastIncomingAckBit) ReliableDataInFlight.Clear();
                                                         else ResendReliableDataInFlight=true;
    }

    if (ProcessPayload) ProcessPayload(InData, LastIncomingSequenceNr);

    return RemoteLastIncomingSequenceNr;
}

const NetDataT& GameProtocol1T::GetTransmitOutOfBandData(const ArrayT<char>& UnreliableData)
{
    static NetDataT OutData;
    OutData=NetDataT();

    OutData.WriteLong(0xFFFFFFFF);
    if (OutData.Data.Size()+UnreliableData.Size()<=MAX_MSG_SIZE) OutData.WriteArrayOfBytes(UnreliableData);

    return OutData;
}

bool GameProtocol1T::IsIncomingMessageOutOfBand(NetDataT& InData)
{
    if (InData.ReadLong()==0xFFFFFFFF) return true;

    InData.ReadBegin();
    return false;
}
