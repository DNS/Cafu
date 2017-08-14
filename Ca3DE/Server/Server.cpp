/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Server.hpp"
#include "ServerWorld.hpp"
#include "ClientInfo.hpp"
#include "../GameInfo.hpp"
#include "../NetConst.hpp"
#include "../PlayerCommand.hpp"
#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/ConsoleStringBuffer.hpp"
#include "ConsoleCommands/ConsoleInterpreter.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "ConsoleCommands/ConFunc.hpp"

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
    std::string PathName    ="Games/" + ServerPtr->m_GameInfo.GetName() + "/Worlds/" + NewWorldName + ".cw";

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
        NewWorld=new CaServerWorldT(PathName.c_str(), ServerPtr->m_ModelMan, ServerPtr->m_GuiRes);
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

        // A player entity will be assigned and the SC1_WorldInfo message be sent in the main loop.
        ClInfo->InitForNewWorld(0);
    }

    Console->Print("Level changed on server.\n");
    ServerPtr->GuiCallback.OnServerStateChanged("maploaded");
    return 0;
}

static ConFuncT ConFunc_changeLevel("changeLevel", ServerT::ConFunc_changeLevel_Callback, ConFuncT::FLAG_MAIN_EXE, "Makes the server load a new level.");


// This console function is called at any time (e.g. when we're NOT thinking)...
/*static*/ int ServerT::ConFunc_runMapCmd_Callback(lua_State* LuaState)
{
    if (!ServerPtr) return luaL_error(LuaState, "The local server is not available.");
    if (!ServerPtr->World) return luaL_error(LuaState, "There is no world loaded in the local server.");

    // ServerPtr->World->GetScriptState_OLD().DoString(luaL_checkstring(LuaState, 1));
    // return 0;
    return luaL_error(LuaState, "Sorry, this function is not implemented at this time.");
}

static ConFuncT ConFunc_runMapCmd("runMapCmd", ServerT::ConFunc_runMapCmd_Callback, ConFuncT::FLAG_MAIN_EXE,
    "Runs the given command string in the context of the current map/entity script.");


ServerT::ServerT(const GameInfoT& GameInfo, const GuiCallbackI& GuiCallback_, ModelManagerT& ModelMan, cf::GuiSys::GuiResourcesT& GuiRes)
    : ServerSocket(g_WinSock->GetUDPSocket(Options_ServerPortNr.GetValueInt())),
      m_GameInfo(GameInfo),
      WorldName(""),
      World(NULL),
      GuiCallback(GuiCallback_),
      m_ModelMan(ModelMan),
      m_GuiRes(GuiRes)
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
        // Beachte: Es werden u.a. alle bis hierhin eingegangenen PlayerCommands verarbeitet.
        // Das ist wichtig für die Prediction, weil wir unten beim Senden mit den Sequence-Nummern des Game-Protocols
        // bestätigen, daß wir alles bis zur bestätigten Sequence-Nummer gesehen UND VERARBEITET haben!
        // Insbesondere muß dieser Aufruf daher zwischen dem Empfangen der PlayerCommand-Packets und dem Senden der
        // nächsten Delta-Update-Messages liegen (WriteClientDeltaUpdateMessages()).
        World->Think(FrameTime /*TimeSinceLastServerTic*/, ClientInfos);
        // TimeSinceLastServerTic=0;

        // All pending player commands have been processed, now clean them up for the next frame.
        for (unsigned int ClientNr = 0; ClientNr < ClientInfos.Size(); ClientNr++)
        {
            ClientInfoT*            CI   = ClientInfos[ClientNr];
            ArrayT<PlayerCommandT>& PPCs = CI->PendingPlayerCommands;

            if (PPCs.Size() == 0) continue;

            CI->PreviousPlayerCommand = PPCs[PPCs.Size() - 1];
            PPCs.Overwrite();
        }

        // If a client has newly joined the game:
        //   - create a human player entity for it,
        //   - send it the related world info message.
        for (unsigned int ClientNr = 0; ClientNr < ClientInfos.Size(); ClientNr++)
        {
            ClientInfoT* CI = ClientInfos[ClientNr];

            if (CI->ClientState == ClientInfoT::Wait4MapInfoACK && CI->EntityID == 0)
            {
                CI->EntityID = World->InsertHumanPlayerEntity(CI->PlayerName, CI->ModelName, ClientNr);

                if (CI->EntityID == 0)
                {
                    DropClient(ClientNr, "Inserting the player entity failed.");
                    continue;
                }

                // The client remains in Wait4MapInfoACK state until it has ack'ed this message.
                NetDataT NewReliableMsg;
                NewReliableMsg.WriteByte  (SC1_WorldInfo);
                NewReliableMsg.WriteString(m_GameInfo.GetName());
                NewReliableMsg.WriteString(WorldName);
                NewReliableMsg.WriteLong  (CI->EntityID);

                CI->ReliableDatas.PushBack(NewReliableMsg.Data);
            }
        }

        // Update the connected clients according to the new (now current) world state.
        for (unsigned long ClientNr = 0; ClientNr < ClientInfos.Size(); ClientNr++)
        {
            ClientInfoT* CI = ClientInfos[ClientNr];

            // Prepare reliable BaseLine messages for newly created entities that the client
            // does not yet know about. CI->BaseLineFrameNr is the server frame number up to
            // which the client has received and acknowledged all BaseLines. That is, BaseLine
            // messages are created for entities whose BaseLineFrameNr is larger than that.
            //
            // Note that after this, no further entities can be added to the world in the
            // *current* frame, but only in the next!
            CI->BaseLineFrameNr = World->WriteClientNewBaseLines(CI->BaseLineFrameNr, CI->ReliableDatas);

            // Update the client's frame info corresponding to the the current server frame.
            World->UpdateFrameInfo(*CI);
        }
    }


    // Sende die aufgestauten ReliableData und die hier "dynamisch" erzeugten UnreliableData
    // (bestehend aus den Delta-Update-Messages (FrameInfo+EntityUpdates)) an die für sie bestimmten Empfänger.
    for (unsigned long ClientNr = 0; ClientNr < ClientInfos.Size(); ClientNr++)
    {
        ClientInfoT* CI = ClientInfos[ClientNr];

        ClientInfos[ClientNr]->TimeSinceLastUpdate+=FrameTime;

        // Only send something after a minimum interval.
        if (CI->TimeSinceLastUpdate < 0.05) continue;

        NetDataT UnreliableData;

        if (World && CI->ClientState != ClientInfoT::Zombie)
        {
            World->WriteClientDeltaUpdateMessages(*CI, UnreliableData);
        }

        try
        {
            // Note that we intentionally also send to clients in Wait4MapInfoACK and Zombie
            // states, in order to keep the handling (with possible re-transfers) of reliable
            // data going.
            // TODO: Für Zombies statt rel.+unrel. Data nur leere Buffer übergeben! VORHER aber die DropMsg in ServerT::DropClient SENDEN!
            CI->GameProtocol.GetTransmitData(CI->ReliableDatas, UnreliableData.Data).Send(ServerSocket, CI->ClientAddress);
        }
        catch (const GameProtocol1T::MaxMsgSizeExceeded& /*E*/) { Console->Warning(cf::va("(ClientNr==%u, EntityID==%u) caught a GameProtocol1T::MaxMsgSizeExceeded exception!", ClientNr, CI->EntityID)); }
        catch (const NetDataT::WinSockAPIError&            E  ) { Console->Warning(cf::va("caught a NetDataT::WinSockAPIError exception (error %u)!", E.Error)); }
        catch (const NetDataT::MessageLength&              E  ) { Console->Warning(cf::va("caught a NetDataT::MessageLength exception (wanted %u, actual %u)!", E.Wanted, E.Actual)); }

        CI->ReliableDatas.Clear();
        CI->TimeSinceLastUpdate = 0;
    }
}


void ServerT::DropClient(unsigned long ClientNr, const char* Reason)
{
    // Calling code should not try to drop a client what has been dropped before and thus is in zombie state already.
    assert(ClientInfos[GlobalClientNr]->ClientState!=ClientInfoT::Zombie);

    // Broadcast reliable DropClient message to everyone, including the client to be dropped.
    // This messages includes the entity ID and the reason for the drop.
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

    // The server world will clean-up the client's now abandoned human player entity at
    // ClientInfos[ClientNr]->EntityID automatically, as no (non-zombie) client is referring to it any longer.
    // From the view of the remaining clients this is the same as with any other entity that has been deleted.

    // The client was disconnected, but it goes into a zombie state for a few seconds to make sure
    // any final reliable message gets resent if necessary.
    // The time-out will finally remove the zombie entirely when its zombie-time is over.
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

                ClientInfoT* CI = new ClientInfoT(SenderAddress, PlayerName, ModelName);
                ClientInfos.PushBack(CI);

                // A player entity will be assigned and the SC1_WorldInfo message be sent in the main loop.
                // Dem Client bestätigen, daß er im Spiel ist und ab sofort in-game Packets zu erwarten hat.
                OutData.WriteByte(SC0_ACK);
                OutData.WriteString(m_GameInfo.GetName());
                OutData.Send(ServerSocket, SenderAddress);
                Console->Print(CI->PlayerName + " joined.\n");
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
    // The LastIncomingSequenceNr is unused now.
    ServerPtr->ProcessInGamePacket(InData);
}


void ServerT::ProcessInGamePacket(NetDataT& InData)
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

                const unsigned int PlCmdNr = InData.ReadLong();
                PlayerCommand.FrameTime    = InData.ReadFloat();
                PlayerCommand.Keys         = InData.ReadLong();
                PlayerCommand.DeltaHeading = InData.ReadWord();
                PlayerCommand.DeltaPitch   = InData.ReadWord();
             // PlayerCommand.DeltaBank    = InData.ReadWord();

                if (InData.ReadOfl) return;     // Ignore the rest of the message!

                // PlayerCommand-Messages eines Clients, der im Wait4MapInfoACK-State ist, gehören idR zur vorher gespielten World.
                // Akzeptiere PlayerCommand-Messages daher erst (wieder), wenn der Client vollständig online ist.
                if (ClientInfos[GlobalClientNr]->ClientState!=ClientInfoT::Online) break;

                ClientInfos[GlobalClientNr]->PendingPlayerCommands.PushBack(PlayerCommand);

                assert(ClientInfos[GlobalClientNr]->LastPlayerCommandNr < PlCmdNr);
                ClientInfos[GlobalClientNr]->LastPlayerCommandNr = PlCmdNr;
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
