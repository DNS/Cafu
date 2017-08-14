/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CLIENT_STATE_INGAME_HPP_INCLUDED
#define CAFU_CLIENT_STATE_INGAME_HPP_INCLUDED

#include "ClientState.hpp"
#include "Graphs.hpp"
#include "ScrlInfo.hpp"
#include "../PlayerCommand.hpp"
#include "Fonts/Font.hpp"
#include "Network/Network.hpp"


struct lua_State;
class  CaClientWorldT;
struct CaKeyboardEventT;
struct CaMouseEventT;
class  ClientT;
class  PathRecorderT;


/// This class implements the state of the client when it is fully connected and thus "in-game".
class ClientStateInGameT : public ClientStateT
{
    public:

    ClientStateInGameT(ClientT& Client_);
    ~ClientStateInGameT();

    // Implement the ClientStateT interface.
    int GetID() const override;
    bool ProcessInputEvent(const CaKeyboardEventT& KE) override;
    bool ProcessInputEvent(const CaMouseEventT& ME) override;
    void Render(float FrameTime) override;
    void MainLoop(float FrameTime) override;


    static int ConFunc_say_Callback(lua_State* LuaState);
    static int ConFunc_chatPrint_Callback(lua_State* LuaState);
    static int ConFunc_showPath_Callback(lua_State* LuaState);
    static int ConFunc_recordPath_Callback(lua_State* LuaState);


    private:

    ClientStateInGameT(const ClientStateInGameT&);      ///< Use of the Copy    Constructor is not allowed.
    void operator = (const ClientStateInGameT&);        ///< Use of the Assignment Operator is not allowed.

    void        ProcessConnectionLessPacket(NetDataT& InData, const NetAddressT& SenderAddress);
    void        ParseServerPacket      (NetDataT& InData);
    static void ParseServerPacketHelper(NetDataT& InData, unsigned long /*LastIncomingSequenceNr*/);


    ClientT&               Client;

    GameProtocol1T         GameProtocol;
    ArrayT< ArrayT<char> > ReliableDatas;
    NetDataT               UnreliableData;

    FontT                  Font_v;
    FontT                  Font_f;
    CaClientWorldT*        World;
    bool                   IsLoadingWorld;        ///< True while the world is loaded, false at all other times. This is relevant only because cf::GuiSys::GuiMan->Yield() is called while loading, which in turn calls our Render() method.

    ScrollInfoT            ChatScrollInfo;
    ScrollInfoT            SystemScrollInfo;
    GraphsT                Graphs;
    unsigned long          ClientFrameNr;         ///< Counts the calls to Render(). Only used with Graphs; should be integrated there.
    PlayerCommandT         m_PlayerCommand;       ///< The player command structure that collects the input until the next call to MainLoop().
    unsigned int           m_PlayerCommandCount;  ///< The unique number of the next player command that is sent to the server (and locally processed for prediction).
    PathRecorderT*         m_PathRecorder;        ///< Records the path of this client in a pointfile that can be loaded into CaWE.
};

#endif
