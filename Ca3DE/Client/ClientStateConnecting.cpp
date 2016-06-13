/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ClientStateConnecting.hpp"
#include "Client.hpp"
#include "../GameInfo.hpp"
#include "../NetConst.hpp"
#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "Network/Network.hpp"

#ifdef _WIN32
    // #define WIN32_LEAN_AND_MEAN
    // #include <windows.h>
#else
    #include <errno.h>
    #define WSAECONNRESET  ECONNRESET
    #define WSAEMSGSIZE    EMSGSIZE
    #define WSAEWOULDBLOCK EWOULDBLOCK
#endif


const int SERVER_RUNMODE=2;

extern WinSockT* g_WinSock;
extern ConVarT Options_PlayerName;
extern ConVarT Options_PlayerModelName;
extern ConVarT Options_ClientPortNr;
extern ConVarT Options_ClientRemoteName;
extern ConVarT Options_ClientRemotePortNr;
extern ConVarT Options_ServerPortNr;


ClientStateConnectingT::ClientStateConnectingT(ClientT& Client_)
    : Client(Client_),
      PacketID(Client.PacketIDConnLess++),
      TimeLeft(8.0f)
{
    // Aquire a new client socket.
    Client.Socket=g_WinSock->GetUDPSocket(Options_ClientPortNr.GetValueInt());

    if (Client.Socket==INVALID_SOCKET)
    {
        Console->Print("Could not obtain a client socket.\n");
        Client.NextState=ClientT::IDLE;
        return;
    }


    // The Client.ServerAddress has already been set in the connect() confunc.
    // Client.ServerAddress=...;


    // Send connection request to server.
    NetDataT OutData;

    OutData.WriteLong(0xFFFFFFFF);
    OutData.WriteLong(PacketID);
    OutData.WriteByte(CS0_Connect);
    OutData.WriteString(Options_PlayerName.GetValueString());
    OutData.WriteString(Options_PlayerModelName.GetValueString());

    try
    {
        OutData.Send(Client.Socket, Client.ServerAddress);
    }
    catch (const NetworkError& /*E*/)
    {
        Console->Print("Couldn't send connection request.\n");
        Client.NextState=ClientT::IDLE;
        return;
    }

    Console->Print(cf::va("OK\n\nConnecting to %s\n(timeout 8.0 seconds)...\n\n", Client.ServerAddress.ToString()));
}


int ClientStateConnectingT::GetID() const
{
    return ClientT::CONNECTING;
}


bool ClientStateConnectingT::ProcessInputEvent(const CaKeyboardEventT& KE)
{
    // No, we did not process this event.
    return false;
}


bool ClientStateConnectingT::ProcessInputEvent(const CaMouseEventT& ME)
{
    // No, we did not process this event.
    return false;
}


void ClientStateConnectingT::Render(float FrameTime)
{
    // Intentionally make no attempt to render anything (e.g. a progress bar).
    // Whatever GUI is monitoring our connection progress is likely to implement some related graphical output as it desires.

    // This was mostly for debugging, and is now occluded by full-screen main menu background images anyway.
    // If you want to output the current client state in the GUI, just use something like   self:setText(ci.GetValue("clState"));
    // Font.Print(130, 400, 0x00FFFF00, "Connecting...");
}


void ClientStateConnectingT::ProcessConnectionLessPacket(NetDataT& InData, const NetAddressT& SenderAddress)
{
    if (SenderAddress!=Client.ServerAddress)
    {
        Console->Warning(std::string("Received a packet from ")+SenderAddress.ToString()+" while connecting to "+Client.ServerAddress.ToString()+"\n");
        return;
    }

    const unsigned long IncomingPacketID=InData.ReadLong();

    if (IncomingPacketID!=PacketID)
    {
        Console->Warning(cf::va("Expected incoming packet ID %lu, got %lu.\n", PacketID, IncomingPacketID));
        return;
    }

    switch (InData.ReadByte())
    {
        case SC0_ACK:
        {
            const std::string SvGameName = InData.ReadString();   // SC0_ACK is followed by the game name.

            if (SvGameName != Client.m_GameInfo.GetName())
            {
                Console->Warning("Client is running game '" + Client.m_GameInfo.GetName() + "', server is running game '" + SvGameName + "' -- connection ignored.\n");
                break;
            }

            // Ok, the connection request was acknowledged.
            // Now change immediately to the "in-game" state.
            Client.NextState=ClientT::INGAME;
            break;
        }

        case SC0_NACK:
        {
            Console->Warning(cf::va("Connection denied. Reason: %s\n", InData.ReadString()));
            Client.NextState=ClientT::IDLE;     // Go back to idle state...
            break;
        }

        case SC0_RccReply:
        default:
        {
            Console->Warning("Received unexpected message type while connecting.");
            break;
        }
    }
}


void ClientStateConnectingT::MainLoop(float FrameTime)
{
    // Handle incoming server packets (hopefully the desired connection acknowledgement).
    // Prüfe auf Server-Antwort(en) und verarbeite diese. Wir holen in einer Schleife die Packets ab, bis keine mehr da sind.
    // Dies ist insbesondere wichtig, wenn wir auf einem langsamen Computer schneller Server-Packets erhalten als wir Frames generieren können!
    // (Würde pro Frame nur ein Packet bearbeitet werden, gäbe es in einem solchen Fall Buffer-Overflows im OS und folglich packet-loss!)
    unsigned long MaxPacketsCount=20;

    while (MaxPacketsCount--)
    {
        try
        {
            NetDataT    InData;
            NetAddressT SenderAddress=InData.Receive(Client.Socket);

            if (GameProtocol1T::IsIncomingMessageOutOfBand(InData))
            {
                // It's well possible that we receive connection-less messages,
                // e.g. in reply to the connection-less messages that we have sent.
                ProcessConnectionLessPacket(InData, SenderAddress);

                // If the clients state was changed above due to an incoming package,
                // quit early here (code below might erroneously change the state again!).
                if (Client.NextState!=GetID()) return;
            }
            else
            {
                // "In-band" (=="in-game", connection established) messages are utterly unexpected here.
                Console->Warning("Ignoring in-game network message in \"connecting\" state!\n");
            }
        }
        catch (const NetDataT::WinSockAPIError& E)
        {
            if (E.Error==WSAEWOULDBLOCK) break;

            const char* ErrorString="";

            switch (E.Error)
            {
             // case WSAEWOULDBLOCK: ErrorString=" (WSAEWOULDBLOCK)"; break;   // WSAEWOULDBLOCK is silently handled above
                case WSAEMSGSIZE   : ErrorString=" (WSAEMSGSIZE)"   ; break;
                case WSAECONNRESET : ErrorString=" (WSAECONNRESET)" ; break;
            }

            Console->Warning(cf::va("Connecting: InData.Receive() returned WSA fail code %u%s. Packet ignored.\n", E.Error, ErrorString));
        }
    }


    // Decrease time left until timeout.
    TimeLeft-=FrameTime;

    // Handle timout.
    if (TimeLeft<=0)
    {
        Console->Print("No server response. Please try again later.\n");

        TimeLeft=0;
        Client.NextState=ClientT::IDLE;
    }
}
