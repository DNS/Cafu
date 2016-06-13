/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_NETWORK_HPP_INCLUDED
#define CAFU_NETWORK_HPP_INCLUDED

#include <string>

// TODO: Können wir diesen #if... Teil in das Network.cpp File verlagern?
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#define socklen_t int
#undef GetProp
#else
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#define WSAGetLastError() errno
#endif

#include "Templates/Array.hpp"

#if defined(_WIN32) && _MSC_VER<1600
#include "pstdint.h"            // Paul Hsieh's portable implementation of the stdint.h header.
#else
#include <stdint.h>
#endif


/// Network error (all other exceptions of this library derive from this one).
struct NetworkError { };


/// This class simplifies the usage of the WinSock API.
class WinSockT
{
    private:

    WinSockT(const WinSockT&);          // Use of the Copy Constructor    is not allowed
    void operator = (const WinSockT&);  // Use of the Assignment Operator is not allowed


    public:

    /// WinSock initialization error.
    struct InitFailure : public NetworkError
    {
        int Error; ///< Error code.

        /// Constructor.
        /// @param Error_ Error code.
        InitFailure(unsigned long Error_) : Error(Error_) { }
    };

    /// Bad version error.
    struct BadVersion : public NetworkError
    {
        unsigned short Version; ///< The bad version number.

        /// Constructor.
        /// @param Version_ Bad version number.
        BadVersion(unsigned short Version_) : Version(Version_) { }
    };

    /// Constructor: Initializes WinSock.
    /// @param RequestedVersion WinSock version to request.
    /// @throws InitFailure on failure
    /// @throws BadVersion if the requested version is not supported.
    WinSockT(unsigned short RequestedVersion=0x0002);

    /// Shuts down WinSock properly by calling WSACleanup().
    ~WinSockT();

    /// Creates a non-blocking TCP/IP listener socket, that is bound to port PortNr.
    /// @param PortNr The port number number to bind this listener socket to.
    SOCKET GetTCPServerSocket(unsigned short PortNr) const;

    /// Creates a non-blocking TCP/IP socket that is connected to ServerPortNr at ServerAddr.
    /// @param ServerAddress The server address to connect to.
    /// @param ServerPortNr The port number number to connect to.
    SOCKET GetTCPClientSocket(const char* ServerAddress, unsigned short ServerPortNr) const;

    /// Creates a non-blocking UDP socket that is bound to port PortNr.
    /// @param PortNr The port number this UDP socket is bound to.
    SOCKET GetUDPSocket(unsigned short PortNr) const;
};


/// Network address consisting of an IP4 address and port number.
class NetAddressT
{
    public:

    /// Exception that is thrown on name look-up failure
    struct BadHostName : public NetworkError { };

    /// Constructor.
    /// @param IP0 The first byte of the IP address.
    /// @param IP1 The second byte of the IP address.
    /// @param IP2 The third byte of the IP address.
    /// @param IP3 The fourth byte of the IP address.
    /// @param Port_ The port of this network adress
    NetAddressT(char IP0, char IP1, char IP2, char IP3, unsigned short Port_);

    /// Constructor.
    /// @param Name Hostname from which the network address should be created.
    /// @param Port_ The port of this network address.
    /// @throw BadHostName
    NetAddressT(const char* Name, unsigned short Port_);

    /// Constructor.
    /// @param Name Hostname from which the network address should be created.
    /// @param Port_ The port of this network address.
    /// @throw BadHostName
    NetAddressT(const std::string& Name, unsigned short Port_);

    /// Constructor.
    /// @param SockAddrIn sockaddr_in stuct from which this network address is created.
    NetAddressT(const sockaddr_in& SockAddrIn);

    /// Converts the network address into a string.
    /// @return The network address in string format.
    const char* ToString() const;

    /// Converts the network address into a sockaddr_in struct.
    /// @return The network address as sockaddr_in.
    sockaddr_in ToSockAddrIn() const;

    /// Compares two network addresses.
    /// @param Address The address to compare to.
    /// @return 'true' if the addresses are the same.
    bool operator == (const NetAddressT& Address) const;

    /// Compares two network addresses.
    /// @param Address The address to compare to.
    /// @return 'true' if the addresses are different.
    bool operator != (const NetAddressT& Address) const;


    char           IP[4];   ///< The IP number of the network address.
    unsigned short Port;    ///< The port number of the network address.
};


/// A socket that closes itself at the end of its livetime (scope).
class NetSocketT
{
    public:

    /// Constructor.
    /// Create a NetSockeT from a socket handle.
    /// @param Socket_ Socket handle to create from.
    NetSocketT(SOCKET Socket_);

    /// Desctructor.
    /// Releases the socket handle it was created with.
    ~NetSocketT();

    /// Get the socket handle.
    /*SOCKET*/ operator SOCKET () const;


    private:

    NetSocketT(const NetSocketT&);          ///< Use of the Copy Constructor    is not allowed.
    void operator = (const NetSocketT&);    ///< Use of the Assignment Operator is not allowed.

    SOCKET Socket;
};


/// Class that allows easy and portable handling, sending and receiving of data over a network.
class NetDataT
{
    public:

    /// WinSock error.
    struct WinSockAPIError : public NetworkError
    {
        /// Constructor.
        /// @param Error_ Error code.
        WinSockAPIError(unsigned long Error_) : Error(Error_), Address(0, 0, 0, 0, 0) { }

        /// Constructor.
        /// @param Error_ Error code.
        /// @param Address_ Remote address of this error.
        WinSockAPIError(unsigned long Error_, const NetAddressT& Address_) : Error(Error_), Address(Address_) { }

        unsigned long Error;   ///< Error code.
        NetAddressT   Address; ///< Remote address on which this error occured.
    };

    /// Message length error.
    /// Could not read as many bytes as requested.
    struct MessageLength : public NetworkError
    {
        unsigned long Wanted; ///< Bytes wanted to read.
        unsigned long Actual; ///< Actually read bytes.

        /// Constructor.
        /// @param Wanted_ Bytes wanted to read.
        /// @param Actual_ Actually read bytes.
        MessageLength(unsigned long Wanted_, unsigned long Actual_) : Wanted(Wanted_), Actual(Actual_) { }
    };


    ArrayT<char>  Data;         ///< Data buffer contents.
    unsigned long ReadPos;      ///< Reading position in data.
    bool          ReadOfl;      ///< Whether the attempt was made to read over the data buffer boundaries.


    /// Constructor.
    NetDataT() : ReadPos(0), ReadOfl(false)
    {
    }

    /// Initializes reading of the data buffer.
    void ReadBegin()
    {
        ReadPos=0;
        ReadOfl=false;
    }

    /// Reads one Byte (8 Bit) from the data buffer.
    /// @return Byte read.
    char ReadByte()
    {
        if (ReadPos+1>Data.Size()) { ReadOfl=true; return 0; }

        char b=Data[ReadPos];

        ReadPos+=1;
        return b;
    }

    /// Read one Word (16 Bit) from the data buffer.
    /// @return Word read.
    unsigned short ReadWord()
    {
        if (ReadPos+2>Data.Size()) { ReadOfl=true; return 0; }

        unsigned short w=*(unsigned short*)&Data[ReadPos];

        ReadPos+=2;
        return ntohs(w);
    }

    /// Reads one uint32_t from the data buffer.
    /// @return The read unsigned integer.
    uint32_t ReadLong()
    {
        if (ReadPos+4>Data.Size()) { ReadOfl=true; return 0; }

        const uint32_t ui=*(uint32_t*)&Data[ReadPos];

        ReadPos+=4;
        return ntohl(ui);
    }

    /// Reads one Float (32 Bit) from the data buffer.
    /// @return Float value read.
    float ReadFloat()
    {
        const uint32_t ui=ReadLong();

        return *(float*)&ui;
    }

    /// Reads a String from the data buffer.
    /// @return String read.
    const char* ReadString()
    {
        const unsigned long StringStart=ReadPos;

        // Figure out if the terminating 0 character is present.
        // This is really important in order to guard against attacks.
        while (true)
        {
            if (ReadPos>=Data.Size()) { ReadOfl=true; return NULL; }
            if (Data[ReadPos]==0) break;
            ReadPos++;
        }

        ReadPos++;
        return &Data[StringStart];
    }

    /// Reads a delta message for cf::Network::StateT from the data buffer.
    /// @return Delta message read.
    ArrayT<uint8_t> ReadDMsg()
    {
        ArrayT<uint8_t> DMsg;

        DMsg.PushBackEmptyExact(ReadLong());
        for (unsigned long i=0; i<DMsg.Size(); i++) DMsg[i]=ReadByte();

        return DMsg;
    }

    /// Writes one Byte (8 Bit) into the data buffer.
    /// @param b Byte to write.
    void WriteByte(char b)
    {
        Data.PushBack(b);
    }

    /// Writes one Word (16 Bit) into the data buffer.
    /// @param w Word to write.
    void WriteWord(unsigned short w)
    {
        Data.PushBackEmpty(2);
        *(unsigned short*)&Data[Data.Size()-2]=htons(w);
    }

    /// Writes one uint32_t into the data buffer.
    /// @param ui   The 32-bit unsigned integer to write.
    void WriteLong(uint32_t ui)
    {
        Data.PushBackEmpty(4);
        *(uint32_t*)&Data[Data.Size()-4]=htonl(ui);
    }

    /// Writes one float (32 Bit) into the data buffer.
    /// @param f Float to write.
    void WriteFloat(float f)
    {
        WriteLong(*(uint32_t*)&f);
    }

    /// Writes a String into the data buffer.
    /// @param String String to write.
    void WriteString(const char* String)
    {
        if (!String) return;

        const unsigned long Start =Data.Size();
        const unsigned long Length=(unsigned long)strlen(String)+1;

        Data.PushBackEmpty(Length);
        memcpy(&Data[Start], String, Length);
    }

    /// Writes a String into the data buffer.
    /// @param String String to write.
    void WriteString(const std::string& String)
    {
        WriteString(String.c_str());
    }

    /// Writes an ArrayT<char> into the data buffer.
    /// @param AoB Byte array to write.
    void WriteArrayOfBytes(const ArrayT<char>& AoB)
    {
        for (unsigned long i=0; i<AoB.Size(); i++) Data.PushBack(AoB[i]);
    }

    /// Writes a delta message as created by cf::Network::StateT into the data buffer.
    /// @param DMsg Delta message to write.
    void WriteDMsg(const ArrayT<uint8_t>& DMsg)
    {
        WriteLong(DMsg.Size());
        for (unsigned long i=0; i<DMsg.Size(); i++) Data.PushBack(DMsg[i]);
    }


    /// Send the content of NetDataT through 'Socket' (non-blocking UDP socket) to 'ReceiverAddress'.
    /// @param Socket The socket to send the data through.
    /// @param ReceiverAddress The address of the receiver.
    /// @throw WinSockAPIError if theres a WinSock API error
    /// @throw MessageLength when the message was not completely sent.
    void Send(SOCKET Socket, const NetAddressT& ReceiverAddress) const;

    /// Receives one packet from 'Socket' (non-blocking UDP socket), overwrites the content of this NetDataT and
    /// returns the NetAddressT of the sender.
    /// @param Socket The socket to receive the data from.
    /// @throw WinSockAPIError, when receicing failes.
    NetAddressT Receive(SOCKET Socket);
};


/// This class implements a mixture of a reliable and unreliable, bi-directional network protocol for Cafu.
/// Focus is on delivering unreliable messages fast and reliable messages reliable (excatly once and in order).
class GameProtocol1T
{
    private:

    /// Flag for reliable message acknowledgements
    static const unsigned long ACK_FLAG;

    /// Bit mask (bit-wise inverse of the ACK_FLAG)
    static const unsigned long ACK_MASK;

    /// Maximum message size that can be sent over the network (1400 bytes for IPv6 compliance)
    static const unsigned long MAX_MSG_SIZE;

    /// Number of the next packet to be sent to the remote party: >> Hey, I am sending you a packet with sequence number 'NextOutgoingSequenceNr'! <<
    unsigned long NextOutgoingSequenceNr;

    /// Number of the last packet we got from the remote party: >> The last packet I have seen from you had sequence number 'LastIncomingSequenceNr'! <<
    unsigned long LastIncomingSequenceNr;

    /// Number of the last packet we sent that carried a reliable message payload (only one such packet can be in-flight at a time)
    unsigned long LastReliableSequenceNr;

    /// A single bit only: The alternating bit we expect to receive for reliable message acknowledgement
    unsigned long ExpectedIncomingAckBit;

    /// A single bit only: The alternating bit we are expected to send for reliable message acknowledgement
    unsigned long ExpectedOutgoingAckBit;

    /// The flag that indicates if the reliable data is to be resent (either when the data is new or packet loss was detected)
    bool ResendReliableDataInFlight;

    /// The data that is currently being transmitted reliable
    ArrayT<char> ReliableDataInFlight;

    /// The data that is waiting to be transmitted reliable, once the old 'in-flight' data was received by the remote party
    ArrayT< ArrayT<char> > ReliableDatasQueue;


    public:

    /// The exception that might be thrown on failure of the GetTransmitData() method
    struct MaxMsgSizeExceeded : public NetworkError { };

    /// Create a connection over the GameProtocol1T.
    GameProtocol1T();

    /// This function takes 'ReliableDatas' and 'UnreliableData' and returns a 'NetDataT' object created from them that can be sent
    /// to the address of the protocol remote client.
    /// Note: 'ReliableDatas' is an array of arrays by intention. Its elements, e.g. 'ReliableDatas[i]', should consists of small self contained
    /// "reliable messages". This is necessary so the function has a clue where to cut the complete message. Examples:
    /// a) The sum of the data in 'ReliableDatas' is to great for a single network packet, but each component ('ReliableDatas[i]') is small enough.
    /// b) Because of network problems more and more "reliable messages" are jammed.
    /// In both cases this function takes as many data from the queue as possible at the borders of whole 'ReliableDatas[i]'.
    /// The ONLY failure case (exception 'MaxMsgSizeExceeded') occurs if any 'ReliableDatas[i]' exceeds the maximal possible network packet size.
    /// This should NEVER happen.
    /// @param ReliableDatas Reliable data parts from which the NetDataT is build.
    /// @param UnreliableData Unreliable data from which the NetDataT is build.
    /// @return NetDataT object ready to send over the network.
    const NetDataT& GetTransmitData(const ArrayT< ArrayT<char> >& ReliableDatas, const ArrayT<char>& UnreliableData) /*throw (MaxMsgSizeExceeded)*/;

    /// Passes the data ('Payload') that has been received from our protocol remote client to ProcessPayload() for evaluation.
    /// If an error occurs (e.g. InData not usable because obsolete) 0 is returned.
    /// Otherwise the return value is the last SequenceNr that the remote client has seen from us (RemoteLastIncomingSequenceNr).
    /// @param InData The data received from a remote client.
    /// @param ProcessPayload The function used to process the payload.
    /// @return The last SequenceNr that the remote client has seen from us (RemoteLastIncomingSequenceNr)
    unsigned long ProcessIncomingMessage(NetDataT& InData, void (*ProcessPayload)(NetDataT& /*Payload*/, unsigned long /*LastIncomingSequenceNr*/));

    /// Returns the sequence number of the next packet sent to the remote client that is not-out-of-band.
    /// @return The sequence number of the next packet sent to the remote client that is not-out-of-band.
    unsigned long GetNextOutgoingSequenceNr() { return NextOutgoingSequenceNr; }

    /// Returns NetDataT with 'UnreliableData' as 'out-of-band' message. NetData can then be sent to the protocol remote client.
    /// @param UnreliableData The unrelilable data to create a NetDataT object from.
    /// @return The NetDataT object created from UnreliableData.
    static const NetDataT& GetTransmitOutOfBandData(const ArrayT<char>& UnreliableData);

    /// Returns true if 'InData' is a 'out-of-band' message, 'false' otherwise.
    /// If 'true', 'InData' is changed in a way that after this call the payload can be read directly.
    /// @param InData NetDataT to check.
    /// @return True if InData is a 'out-of-band' message, false otherwise.
    static bool IsIncomingMessageOutOfBand(NetDataT& InData);
};

#endif
