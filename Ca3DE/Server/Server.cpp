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

#include "Server.hpp"
#include "ServerWorld.hpp"
#include "ClientInfo.hpp"
#include "../NetConst.hpp"
#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/ConsoleStringBuffer.hpp"
#include "ConsoleCommands/ConsoleInterpreter.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "ConsoleCommands/ConFunc.hpp"
#include "../../Games/PlayerCommand.hpp"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <errno.h>
    #define WSAECONNRESET  ECONNRESET
    #define WSAEMSGSIZE    EMSGSIZE
    #define WSAEWOULDBLOCK EWOULDBLOCK
#endif


extern WinSockT* g_WinSock;
extern ConVarT Options_ServerPortNr;


static unsigned long GlobalClientNr;
static ServerT*      ServerPtr=NULL;


static ConVarT ServerRCPassword("sv_rc_password", "", ConVarT::FLAG_MAIN_EXE, "The password the server requires for remote console access.");


int ServerT::ConFunc_changeLevel_Callback(lua_State* LuaState)
{
    if (!ServerPtr) return luaL_error(LuaState, "The local server is not available.");

    std::string NewWorldName=(lua_gettop(LuaState)<1) ? "" : luaL_checkstring(LuaState, 1);
    std::string PathName    ="Games/"+ServerPtr->GameName+"/Worlds/"+NewWorldName+".cw";

    if (NewWorldName=="")
    {
        // changeLevel() or changeLevel("") was called, so change into the (implicit) "no map loaded" state.

        // First disconnect from / drop all the clients.
        // Note that this is a bit different from calling DropClient() for everyone though.
        for (unsigned long ClNr=0; ClNr<ServerPtr->ClientInfos.Size(); ClNr++)
        {
            ClientInfoT* ClInfo=ServerPtr->ClientInfos[ClNr];

            if (ClInfo->ClientState==ClientInfoT::Zombie) continue;

            NetDataT NewReliableMsg;

            NewReliableMsg.WriteByte  (SC1_DropClient);
            NewReliableMsg.WriteLong  (ClInfo->EntityID);
            NewReliableMsg.WriteString("Server has been stopped (map unloaded).");

            ClInfo->ReliableDatas.PushBack(NewReliableMsg.Data);

            // The client was disconnected, but it goes into a zombie state for a few seconds to make sure
            // any final reliable message gets resent if necessary.
            // The time-out will finally remove the zombie entirely when its zombie-time is over.
            ClInfo->ClientState         =ClientInfoT::Zombie;
            ClInfo->TimeSinceLastMessage=0.0;
        }

        delete ServerPtr->World;
        ServerPtr->World    =NULL;
        ServerPtr->WorldName="";
        ServerPtr->GuiCallback.OnServerStateChanged("idle");
        return 0;
    }

    // changeLevel("mapname") was called, so perform a proper map change.
    CaServerWorldT* NewWorld=NULL;

    try
    {
        NewWorld=new CaServerWorldT(PathName.c_str(), ServerPtr->m_ModelMan);
    }
    catch (const WorldT::LoadErrorT& E) { return luaL_error(LuaState, E.Msg); }

    delete ServerPtr->World;
    ServerPtr->World    =NewWorld;
    ServerPtr->WorldName=NewWorldName;

    // Stati der verbundenen Clients auf MapTransition setzen.
    for (unsigned long ClientNr=0; ClientNr<ServerPtr->ClientInfos.Size(); ClientNr++)
    {
        ClientInfoT* ClInfo=ServerPtr->ClientInfos[ClientNr];

        if (ClInfo->ClientState==ClientInfoT::Zombie) continue;

        const unsigned long ClEntityID=ServerPtr->World->InsertHumanPlayerEntityForNextFrame(ClInfo->PlayerName.c_str(), ClInfo->ModelName.c_str(), ClientNr);

        if (ClEntityID==0xFFFFFFFF)
        {
            ServerPtr->DropClient(ClientNr, "Inserting client entity failed.");
            continue;
        }

        ClInfo->InitForNewWorld(ClEntityID);

        NetDataT NewReliableMsg;
        NewReliableMsg.WriteByte  (SC1_WorldInfo);
        NewReliableMsg.WriteString(ServerPtr->GameName);
        NewReliableMsg.WriteString(ServerPtr->WorldName);
        NewReliableMsg.WriteLong  (ClInfo->EntityID);

        ClInfo->ReliableDatas.PushBack(NewReliableMsg.Data);
    }

    Console->Print("Level changed on server.\n");
    ServerPtr->GuiCallback.OnServerStateChanged("maploaded");
    return 0;
}

static ConFuncT ConFunc_changeLevel("changeLevel", ServerT::ConFunc_changeLevel_Callback, ConFuncT::FLAG_MAIN_EXE, "Makes the server load a new level.");


ServerT::ServerT(const std::string& GameName_, const GuiCallbackI& GuiCallback_, ModelManagerT& ModelMan)
    : ServerSocket(g_WinSock->GetUDPSocket(Options_ServerPortNr.GetValueInt())),
      GameName(GameName_),
      WorldName(""),
      World(NULL),
      GuiCallback(GuiCallback_),
      m_ModelMan(ModelMan)
{
    if (ServerSocket==INVALID_SOCKET) throw InitErrorT(cf::va("Unable to obtain UDP socket on port %i.", Options_ServerPortNr.GetValueInt()));

    assert(ServerPtr==NULL);
    ServerPtr=this;     // TODO: Turn the ServerT class into a singleton?
}


ServerT::~ServerT()
{
    for (unsigned long ClientNr=0; ClientNr<ClientInfos.Size(); ClientNr++)
        delete ClientInfos[ClientNr];

    delete World;
    World=NULL;

    assert(ServerPtr==this);
    ServerPtr=NULL;

    assert(ServerSocket!=INVALID_SOCKET);
    closesocket(ServerSocket);
    ServerSocket=INVALID_SOCKET;
}


void ServerT::MainLoop()
{
    // Von hier bis zu World->Think() führen wir ein gefährliches Leben:
    // - Arbeite auf dem AKTUELLEN Frame, über das die Clients aber schon informiert worden sind!
    //   D.h. insbesondere, daß wir nachträglich nicht noch einen HumanPlayer-Entity einfügen können.
    // - Führe diesen Teil u.U. viel öfter (mit viel höherer Framerate) aus als einen 'Server-Tic'.

    // Bestimme die FrameTime des letzten Frames
    float FrameTime=float(Timer.GetSecondsSinceLastCall());

    if (FrameTime<0.0001f) FrameTime=0.0001f;
    if (FrameTime>  10.0f) FrameTime=  10.0f;


    // Check Client Time-Outs
    for (unsigned long ClientNr=0; ClientNr<ClientInfos.Size(); ClientNr++)
    {
        ClientInfos[ClientNr]->TimeSinceLastMessage+=FrameTime;

        if (ClientInfos[ClientNr]->ClientState==ClientInfoT::Zombie)
        {
            if (ClientInfos[ClientNr]->TimeSinceLastMessage>6.0)
            {
                delete ClientInfos[ClientNr];
                ClientInfos[ClientNr]=ClientInfos[ClientInfos.Size()-1];
                ClientInfos.DeleteBack();
                ClientNr--;
                continue;
            }
        }
        else  // Online oder Wait4MapInfoACK
        {
            if (ClientInfos[ClientNr]->TimeSinceLastMessage>120.0)
            {
                DropClient(ClientNr, "Timed out!");
                ClientInfos[ClientNr]->TimeSinceLastMessage=300.0;  // Don't bother with zombie state
            }
        }
    }


    // Warte, bis wir neue Packets bekommen oder das Frame zu Ende geht
    fd_set  ReadabilitySocketSet;
    timeval TimeOut;

    TimeOut.tv_sec =0;
    TimeOut.tv_usec=10*1000;

    #if defined(_WIN32) && defined(__WATCOMC__)
    #pragma warning 555 9;
    #endif
    FD_ZERO(&ReadabilitySocketSet);
    FD_SET(ServerSocket, &ReadabilitySocketSet);
    #if defined(_WIN32) && defined(__WATCOMC__)
    #pragma warning 555 4;
    #endif

    // select() bricht mit der Anzahl der lesbar gewordenen Sockets ab, 0 bei TimeOut oder SOCKET_ERROR im Fehlerfall.
    const int Result=select(int(ServerSocket+1), &ReadabilitySocketSet, NULL, NULL, &TimeOut);

    if (Result==SOCKET_ERROR)
    {
        Console->Print(cf::va("ERROR: select() returned WSA fail code %u\n", WSAGetLastError()));
    }
    else if (Result!=0)
    {
        // In dieser Schleife werden solange die Packets der Clients abgeholt, bis keine mehr da sind (would block).
        unsigned long MaxPacketsCount=20;

        while (MaxPacketsCount--)
        {
            try
            {
                NetDataT    InData;
                NetAddressT SenderAddress=InData.Receive(ServerSocket);

                if (GameProtocol1T::IsIncomingMessageOutOfBand(InData))
                {
                    ProcessConnectionLessPacket(InData, SenderAddress);
                }
                else
                {
                    // ClientNr des Absenders heraussuchen
                    unsigned long ClientNr;

                    for (ClientNr=0; ClientNr<ClientInfos.Size(); ClientNr++)
                        if (SenderAddress==ClientInfos[ClientNr]->ClientAddress) break;

                    // Prüfe ClientNr
                    if (ClientNr>=ClientInfos.Size())
                    {
                        Console->Warning(cf::va("Client %s is not connected! Packet ignored!\n", SenderAddress.ToString()));
                        // Hier evtl. ein bad / disconnect / kick packet schicken?
                        continue;
                    }

                    // Was Zombies senden, interessiert uns nicht mehr
                    if (ClientInfos[ClientNr]->ClientState==ClientInfoT::Zombie) continue;

                    // Es ist eine gültige Nachricht (eines Online- oder Wait4MapInfoACK-Client) - bearbeite sie!
                    GlobalClientNr=ClientNr;
                    assert(ServerPtr==this);
                    ClientInfos[ClientNr]->TimeSinceLastMessage=0.0;    // Do not time-out
                    ClientInfos[ClientNr]->GameProtocol.ProcessIncomingMessage(InData, ProcessInGamePacketHelper);
                }
            }
            catch (const NetDataT::WinSockAPIError& E)
            {
                if (E.Error==WSAEWOULDBLOCK)
                {
                    // Break the while-loop, there is no point in trying to receive further messages right now.
                    // (This is also the reason why we cannot test E.Error in a switch-case block here.)
                    break;
                }

                // Deal with other errors that don't have to break the while-loop above.
                switch (E.Error)
                {
                    case WSAECONNRESET:
                    {
                        // Receiving this error indicates that a previous send operation resulted in an ICMP "Port Unreachable" message.
                        // This in turn occurs whenever a client disconnected without us having received the proper disconnect message.
                        // If we did nothing here, this message would be received until we stopped sending more messages to the client,
                        // that is, until we got rid of the client due to time-out.

                        // Try to find the offending client, and drop it.
                        bool FoundOffender=false;

                        for (unsigned long ClientNr=0; ClientNr<ClientInfos.Size(); ClientNr++)
                            if (E.Address==ClientInfos[ClientNr]->ClientAddress)
                            {
                                if (ClientInfos[ClientNr]->ClientState!=ClientInfoT::Zombie)
                                    DropClient(ClientNr, "ICMP message: client port unreachable!");

                                // Don't bother with the zombie state, rather remove this client entirely right now.
                                delete ClientInfos[ClientNr];
                                ClientInfos[ClientNr]=ClientInfos[ClientInfos.Size()-1];
                                ClientInfos.DeleteBack();
                                FoundOffender=true;
                                break;
                            }

                        if (!FoundOffender)
                        {
                            // If at all, this should happen only occassionally, e.g. because we sent something to a client
                            // that was removed in the time-out check above.
                            Console->Warning(cf::va("Indata.Receive() returned ECONNRESET from unknown address %s.\n", E.Address.ToString()));
                        }
                        break;
                    }

                    case WSAEMSGSIZE:
                    {
                        Console->Warning(cf::va("Sv: InData.Receive() returned WSA fail code %u (WSAEMSGSIZE). Sender %s. Packet ignored.\n", E.Error, E.Address.ToString()));
                        break;
                    }

                    default:
                    {
                        Console->Warning(cf::va("Sv: InData.Receive() returned WSA fail code %u. Sender %s. Packet ignored.\n", E.Error, E.Address.ToString()));
                        break;
                    }
                }
            }
        }
    }


    // Prüfe, ob das ServerTicInterval schon um ist, und führe ggf. einen "ServerTic" aus.
    /* TimeSinceLastServerTic+=FrameTime;

    if (TimeSinceLastServerTic<ServerTicInterval-0.001)
    {
        // Das ServerTicInterval ist noch nicht rum!

        // Falls wir nicht alleine (dedicated) laufen, mache sofort mit dem Client weiter.
        if (!ServerOnly) return true;

        // Andernfalls (dedicated), verschlafe den Rest des ServerTicIntervals oder bis ein Netzwerk-Paket eingeht.
        fd_set      ReadabilitySocketSet;
        timeval     TimeOut;
        const float RemainingSeconds=ServerTicInterval-TimeSinceLastServerTic;

        TimeOut.tv_sec =RemainingSeconds;   // Nachkomma-Anteil wird abgeschnitten
        TimeOut.tv_usec=(RemainingSeconds-float(TimeOut.tv_sec))*1000000.0;

        #pragma warning 555 9;
        FD_ZERO(&ReadabilitySocketSet);
        FD_SET(ServerSocket, &ReadabilitySocketSet);
        #pragma warning 555 4;

        // select() bricht mit der Anzahl der lesbar gewordenen Sockets ab, 0 bei TimeOut oder SOCKET_ERROR im Fehlerfall.
        // Wie auch immer - wir ignorieren den Rückgabewert.
        select(ServerSocket+1, &ReadabilitySocketSet, NULL, NULL, &TimeOut);
        return true;
    } */


    if (World)
    {
        // Überführe die ServerWorld über die Zeit 'FrameTime' in den nächsten Zustand.
        // Beachte 1: Der "Überführungsprozess" beginnt eigentlich schon oben (Einfügen von HumanPlayer-Entities ins nächste Frame).
        // Beachte 2: Es werden u.a. alle bis hierhin eingegangenen PlayerCommands verarbeitet.
        // Das ist wichtig für die Prediction, weil wir unten beim Senden mit den Sequence-Nummern des Game-Protocols
        // ja bestätigen, daß wir alles bis zur bestätigten Sequence-Nummer gesehen UND VERARBEITET haben!
        // Insbesondere muß dieser Aufruf daher zwischen dem Empfangen der PlayerCommand-Packets und dem Senden der
        // nächsten Delta-Update-Messages liegen.
        World->Think(FrameTime /*TimeSinceLastServerTic*/);
        // TimeSinceLastServerTic=0;


        // Hier wäre der richtige Ort zum "externen" Einfügen/Entfernen von HumanPlayer-Entities ins AKTUELLE Frame.


        // BaseLines neu erzeugter Entities zum reliable Senden vormerken (in die ReliableData-Buffer eintragen):
        // Sei 'ClientInfos[ClientNr]->BaseLineFrameNr' diejenige ServerFrameNr, bis zu der (einschließlich) ein Client alle BaseLines kennt.
        // Sende dann die BaseLines aller Entities, deren BaseLineFrameNr größer als diese Nr ist (d.h. diejenigen BaseLines, die der Client noch nicht kennt)!
        // Beachte die Reihenfolge: Darf nach diesem Teil keine neuen Entities mehr im AKTUELLEN Frame erzeugen, sondern erst im NÄCHSTEN Frame wieder!
        for (unsigned long ClientNr=0; ClientNr<ClientInfos.Size(); ClientNr++)
            ClientInfos[ClientNr]->BaseLineFrameNr=World->WriteClientNewBaseLines(ClientInfos[ClientNr]->BaseLineFrameNr, ClientInfos[ClientNr]->ReliableDatas);
    }


    // Sende die aufgestauten ReliableData und die hier "dynamisch" erzeugten UnreliableData
    // (bestehend aus den Delta-Update-Messages (FrameInfo+EntityUpdates)) an die für sie bestimmten Empfänger.
    for (unsigned long ClientNr=0; ClientNr<ClientInfos.Size(); ClientNr++)
    {
        NetDataT UnreliableData;

        if (World && ClientInfos[ClientNr]->ClientState!=ClientInfoT::Zombie)
        {
            World->WriteClientDeltaUpdateMessages(ClientInfos[ClientNr]->EntityID,
                                                  ClientInfos[ClientNr]->LastKnownFrameReceived,
                                                  ClientInfos[ClientNr]->OldStatesPVSEntityIDs,
                                                  ClientInfos[ClientNr]->CurrentStateIndex, UnreliableData);
        }

        ClientInfos[ClientNr]->TimeSinceLastUpdate+=FrameTime;

        // Delta-Komprimierte Entity-Daten senden
        // TODO: Diese Lösung ist natürlich nicht so toll:
        // Will / Muß jedes Frame World->WriteClientDeltaUpdateMessages(...) aufrufen (PRÜFEN!),
        // will aber außerdem nur alle 0.1 sec etwas senden!!!
        if (ClientInfos[ClientNr]->TimeSinceLastUpdate<0.05) continue;

        try
        {
            // Sende bewußt auch an Clients im Wait4MapInfoACK-Zustand! (Aber warum?? Eine Vermutung: Andernfalls werden RELIABLE Data auch nicht transportiert!)
            // TODO: Für Zombies statt rel.+unrel. Data nur leere Buffer übergeben! VORHER aber die DropMsg in ServerT::DropClient SENDEN!
            ClientInfos[ClientNr]->GameProtocol.GetTransmitData(ClientInfos[ClientNr]->ReliableDatas, UnreliableData.Data).Send(ServerSocket, ClientInfos[ClientNr]->ClientAddress);
        }
        catch (const GameProtocol1T::MaxMsgSizeExceeded& /*E*/) { Console->Warning(cf::va("(ClientNr==%u, EntityID==%u) caught a GameProtocol1T::MaxMsgSizeExceeded exception!", ClientNr, ClientInfos[ClientNr]->EntityID)); }
        catch (const NetDataT::WinSockAPIError&            E  ) { Console->Warning(cf::va("caught a NetDataT::WinSockAPIError exception (error %u)!", E.Error)); }
        catch (const NetDataT::MessageLength&              E  ) { Console->Warning(cf::va("caught a NetDataT::MessageLength exception (wanted %u, actual %u)!", E.Wanted, E.Actual)); }

        ClientInfos[ClientNr]->ReliableDatas.Clear();
        ClientInfos[ClientNr]->TimeSinceLastUpdate=0;
    }
}


void ServerT::DropClient(unsigned long ClientNr, const char* Reason)
{
    // Calling code should not try to drop a client what has been dropped before and thus is in zombie state already.
    assert(ClientInfos[GlobalClientNr]->ClientState!=ClientInfoT::Zombie);
    assert(World);

    // 1. Broadcast reliable DropClient message to everyone, including the client to be dropped.
    //    This messages includes the HumanPlayerEntityID and the reason for the drop.
    //    The other connected clients may or may not take the opportunity to remove the entity of
    //    the dropped client from their local world representations.
    for (unsigned long ClNr=0; ClNr<ClientInfos.Size(); ClNr++)
    {
        NetDataT NewReliableMsg;

        NewReliableMsg.WriteByte  (SC1_DropClient);
        NewReliableMsg.WriteLong  (ClientInfos[ClientNr]->EntityID);
        NewReliableMsg.WriteString(Reason);

        ClientInfos[ClNr]->ReliableDatas.PushBack(NewReliableMsg.Data);

        // TODO: HIER DIE MESSAGE AUCH ABSENDEN!
        // KANN DANN UNTEN IN DER HAUPTSCHLEIFE FÜR ZOMBIES DIE REL+UNREL. DATA KOMPLETT UNTERDRUECKEN!
    }
    Console->Print(cf::va("Dropped client %u (EntityID %u, PlayerName '%s'). Reason: %s\n", ClientNr, ClientInfos[ClientNr]->EntityID, ClientInfos[ClientNr]->PlayerName.c_str(), Reason));

    // 2. Remove the entity of the dropped client from our world.
    World->RemoveHumanPlayerEntity(ClientInfos[ClientNr]->EntityID);

    // 3. The client was disconnected, but it goes into a zombie state for a few seconds to make sure
    //    any final reliable message gets resent if necessary.
    //    The time-out will finally remove the zombie entirely when its zombie-time is over.
    ClientInfos[ClientNr]->ClientState         =ClientInfoT::Zombie;
    ClientInfos[ClientNr]->TimeSinceLastMessage=0.0;
}


void ServerT::ProcessConnectionLessPacket(NetDataT& InData, const NetAddressT& SenderAddress)
{
    unsigned long PacketID=InData.ReadLong();

    // Die Antwort auf ein "connection-less" Packet beginnt mit 0xFFFFFFFF, gefolgt von der PacketID,
    // sodaß das Rückpacket unten nur noch individuell beendet werden muß.
    NetDataT OutData;

    OutData.WriteLong(0xFFFFFFFF);
    OutData.WriteLong(PacketID);

    // Der Typ dieser Nachricht entscheidet, wie es weitergeht
    try
    {
        switch (InData.ReadByte())
        {
            case CS0_NoOperation:
                // Packet is a "no operation" request, thus we won't accomplish anything
                Console->Print("PCLP: Packet evaluated to NOP.\n");
                break;

            case CS0_Ping:
                Console->Print("PCLP: Acknowledging inbound ping.\n");
                OutData.WriteByte(SC0_ACK);
                OutData.Send(ServerSocket, SenderAddress);
                break;

            case CS0_Connect:
            {
                // Connection request
                Console->Print(cf::va("PCLP: Got a connection request from %s\n", SenderAddress.ToString()));

                if (World==NULL)
                {
                    Console->Print("No world loaded.\n");
                    OutData.WriteByte(SC0_NACK);
                    OutData.WriteString("No world loaded on server.");
                    OutData.Send(ServerSocket, SenderAddress);
                    break;
                }

                // Prüfe, ob wir die IP-Adr. und Port-Nr. des Clients schon haben, d.h. ob der Client schon connected ist.
                // (Dann hat z.B. der Client schonmal einen connection request geschickt, aber unser Acknowledge niemals erhalten,
                //  oder wir haben seinen Disconnected-Request nicht erhalten und er versucht nun vor dem TimeOut einen neuen Connect.)
                unsigned long ClientNr;

                for (ClientNr=0; ClientNr<ClientInfos.Size(); ClientNr++)
                    if (SenderAddress==ClientInfos[ClientNr]->ClientAddress) break;

                if (ClientNr<ClientInfos.Size())
                {
                    Console->Print("Already listed.\n");
                    OutData.WriteByte(SC0_NACK);
                    OutData.WriteString("Already listed. Wait for timeout and try again.");
                    OutData.Send(ServerSocket, SenderAddress);
                    break;
                }

                if (ClientInfos.Size()>=32)
                {
                    Console->Print("Server is full.\n");
                    OutData.WriteByte(SC0_NACK);
                    OutData.WriteString("Server is full.");
                    OutData.Send(ServerSocket, SenderAddress);
                    break;
                }

                const char* PlayerName=InData.ReadString();
                const char* ModelName =InData.ReadString();

                if (PlayerName==NULL || ModelName==NULL || InData.ReadOfl)
                {
                    Console->Print("Bad player or model name.\n");
                    OutData.WriteByte(SC0_NACK);
                    OutData.WriteString("Bad player or model name.");
                    OutData.Send(ServerSocket, SenderAddress);
                    break;
                }

                unsigned long ClientEntityID=World->InsertHumanPlayerEntityForNextFrame(PlayerName, ModelName, ClientInfos.Size());

                if (ClientEntityID==0xFFFFFFFF)
                {
                    Console->Print("Inserting human player entity failed.\n");
                    OutData.WriteByte(SC0_NACK);
                    OutData.WriteString("Inserting human player entity failed.");
                    OutData.Send(ServerSocket, SenderAddress);
                    break;
                }

                ClientNr=ClientInfos.Size();
                ClientInfos.PushBack(new ClientInfoT(SenderAddress, PlayerName, ModelName));
                ClientInfos[ClientNr]->InitForNewWorld(ClientEntityID);

                // Dem Client bestätigen, daß er im Spiel ist und ab sofort in-game Packets zu erwarten hat.
                OutData.WriteByte(SC0_ACK);
                OutData.WriteString(GameName);
                OutData.Send(ServerSocket, SenderAddress);
                Console->Print(ClientInfos[ClientNr]->PlayerName+" joined.\n");

                // Dem Client einen 'world-change' mitteilen.
                NetDataT NewReliableMsg;

                NewReliableMsg.WriteByte  (SC1_WorldInfo);
                NewReliableMsg.WriteString(GameName);
                NewReliableMsg.WriteString(WorldName);
                NewReliableMsg.WriteLong  (ClientInfos[ClientNr]->EntityID);

                ClientInfos[ClientNr]->ReliableDatas.PushBack(NewReliableMsg.Data);
                break;
            }

            case CS0_Info:
                // Dem Client Server-Infos schicken
                Console->Print(cf::va("PCLP: Got an information request from %s\n", SenderAddress.ToString()));
                OutData.WriteByte(SC0_NACK);
                OutData.WriteString("Information is not yet available!");
                OutData.Send(ServerSocket, SenderAddress);
                break;

            case CS0_RemoteConsoleCommand:
            {
                // See http://svn.icculus.org/quake3/trunk/code/server/sv_main.c?rev=2&view=markup
                // (function SVC_RemoteCommand()) for how they did this in Quake3.
                const char* Password=InData.ReadString(); if (!Password) break;
                const char* Command =InData.ReadString(); if (!Command ) break;

                // Make sure that at least 500ms pass between two successive remote commands.
                ;

                Console->Print(cf::va("Remote console command received from %s: %s\n", SenderAddress.ToString(), Command));

                // Make sure that the sv_rc_password is set (must be done in the config.lua file).
                if (ServerRCPassword.GetValueString()=="")
                {
                    Console->Print("Server rcon password not set.\n");
                    OutData.WriteByte(SC0_RccReply);
                    OutData.WriteString("Server rcon password not set.\n");
                    OutData.Send(ServerSocket, SenderAddress);
                    break;
                }

                // Make sure that the received password is valid.
                if (Password!=ServerRCPassword.GetValueString())
                {
                    Console->Print(cf::va("Invalid password \"%s\".\n", Password));
                    OutData.WriteByte(SC0_RccReply);
                    OutData.WriteString("Invalid password.\n");
                    OutData.Send(ServerSocket, SenderAddress);
                    break;
                }

                // Begin to temporarily redirect all console output.
                cf::ConsoleStringBufferT RedirCon;
                cf::ConsoleI*            OldCon=Console;
                Console=&RedirCon;

                // Ok, it has been a valid remote console command - process it.
                ConsoleInterpreter->RunCommand(Command);

                // End of temporarily redirected output.
                std::string Output=RedirCon.GetBuffer();
                Console=OldCon;
                Console->Print(Output);

                // Send result to sender (wrt. max. network packet size, limit the string length to 1024-16 though).
                if (Output.length()>1024-16) Output=std::string(Output, 0, 1024-16-4)+"...\n";

                OutData.WriteByte(SC0_RccReply);
                OutData.WriteString(Output.c_str());
                OutData.Send(ServerSocket, SenderAddress);
                break;
            }

            default:
                Console->Print(cf::va("PCLP: WARNING: Unknown packet type received! Sender: %s\n", SenderAddress.ToString()));
        }
    }
    catch (const NetDataT::WinSockAPIError& E) { Console->Warning(cf::va("PCLP: Answer failed (WSA error %u)!\n", E.Error)); }
    catch (const NetDataT::MessageLength&   E) { Console->Warning(cf::va("PCLP: Answer too long (wanted %u, actual %u)!\n", E.Wanted, E.Actual)); }
}


void ServerT::ProcessInGamePacketHelper(NetDataT& InData, unsigned long LastIncomingSequenceNr)
{
    ServerPtr->ProcessInGamePacket(InData, LastIncomingSequenceNr);
}


void ServerT::ProcessInGamePacket(NetDataT& InData, unsigned long LastIncomingSequenceNr)
{
    // We can only get here when the client who sent the message is not listed as a zombie,
    // and therefore (consequently) the World pointer must be valid, too.
    assert(ClientInfos[GlobalClientNr]->ClientState!=ClientInfoT::Zombie);
    assert(World);

    // TODO: Wasn't it much easier to simply catch Array-index-out-of-bounds exceptions here?
    while (!InData.ReadOfl && !(InData.ReadPos>=InData.Data.Size()))
    {
        char MessageType=InData.ReadByte();

        switch (MessageType)
        {
            case CS1_PlayerCommand:
            {
                PlayerCommandT PlayerCommand;

                PlayerCommand.FrameTime   =InData.ReadFloat();
                PlayerCommand.Keys        =InData.ReadLong();
                PlayerCommand.DeltaHeading=InData.ReadWord();
                PlayerCommand.DeltaPitch  =InData.ReadWord();
             // PlayerCommand.DeltaBank   =InData.ReadWord();
                PlayerCommand.Nr          =LastIncomingSequenceNr;

                if (InData.ReadOfl) return;     // Ignore the rest of the message!

                // PlayerCommand-Messages eines Clients, der im Wait4MapInfoACK-State ist, gehören idR zur vorher gespielten World.
                // Akzeptiere PlayerCommand-Messages daher erst (wieder), wenn der Client vollständig online ist.
                if (ClientInfos[GlobalClientNr]->ClientState!=ClientInfoT::Online) break;

                World->NotifyHumanPlayerEntityOfClientCommand(ClientInfos[GlobalClientNr]->EntityID, PlayerCommand);
                // Console->Print("    %s sent PlComm (req for Upd), ClientKeys=0x%X, Heading=%u\n", ClientInfos[GlobalClientNr].PlayerName, World->LifeForms[ClLFNr].CurrentState.ClientKeys, World->LifeForms[ClLFNr].CurrentState.Heading);
                break;
            }

            case CS1_Disconnect:
            {
                DropClient(GlobalClientNr, "He/She left the game.");
                return;     // Ignore the rest of the message!
            }

            case CS1_SayToAll:
            {
                const char* InMsg=InData.ReadString();

                // Console messages seem to be a popular choice for attackers.
                // Thus guard against problems.
                if (InMsg==NULL || InData.ReadOfl)
                {
                    Console->Print(cf::va("Bad say message from: %s\n", ClientInfos[GlobalClientNr]->PlayerName.c_str()));
                    break;
                }

                std::string OutMessage=ClientInfos[GlobalClientNr]->PlayerName+": "+InMsg;

                // Limit the length of the message wrt. max. network packet size.
                if (OutMessage.length()>1024-16) OutMessage=std::string(OutMessage, 0, 1024-16-4)+"...\n";

                for (unsigned long ClientNr=0; ClientNr<ClientInfos.Size(); ClientNr++)
                {
                    // TODO: Statt hier ClientState!=Zombie abzufragen, lieber in DropClient schon die Drop-Message SENDEN,
                    // und beim Senden in der Hauptschleife für Zombies leere Buffer für rel. + unrel. Data übergeben!
                    if (ClientInfos[ClientNr]->ClientState!=ClientInfoT::Zombie)
                    {
                        NetDataT NewReliableMsg;

                        NewReliableMsg.WriteByte  (SC1_ChatMsg);
                        NewReliableMsg.WriteString(OutMessage);

                        ClientInfos[ClientNr]->ReliableDatas.PushBack(NewReliableMsg.Data);
                    }
                }
                break;
            }

            case CS1_WorldInfoACK:
            {
                const char* ClientWorldName=InData.ReadString();

                if (InData.ReadOfl || ClientWorldName==NULL || strlen(ClientWorldName)==0)
                {
                    // UploadMap();
                    // Im Moment bei einem Fehler beim Client-WorldChange den Client einfach rausschmeißen!
                    DropClient(GlobalClientNr, "Failure on world change!");
                    return;     // Ignore the rest of the message!
                }

                if (ClientWorldName==WorldName) ClientInfos[GlobalClientNr]->ClientState=ClientInfoT::Online;
                break;
            }

            case CS1_FrameInfoACK:
            {
                const unsigned long LKFR=InData.ReadLong();

                if (InData.ReadOfl) return;     // Ignore the rest of the message!

                ClientInfos[GlobalClientNr]->LastKnownFrameReceived=LKFR;
                break;
            }

            default:
                Console->Print(cf::va("WARNING: Unknown in-game message type '%3u' received!\n", MessageType));
                Console->Print(cf::va("         Sender: %s\n", ClientInfos[GlobalClientNr]->PlayerName.c_str()));
                return;     // Ignore the rest of the message!
        }
    }
}
