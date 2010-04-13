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

#ifndef _CA3DE_CLIENT_STATE_INGAME_HPP_
#define _CA3DE_CLIENT_STATE_INGAME_HPP_

#include "ClientState.hpp"
#include "Graphs.hpp"
#include "ScrlInfo.hpp"
#include "Fonts/Font.hpp"
#include "Network/Network.hpp"
#include "../../Games/PlayerCommand.hpp"


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

    int GetID() const;

    // These methods are driven (called) by the GUI window that "owns" this client.
    bool ProcessInputEvent(const CaKeyboardEventT& KE);
    bool ProcessInputEvent(const CaMouseEventT&    ME);
    void Render(float FrameTime);
    void MainLoop(float FrameTime);


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
    bool                   IsLoadingWorld;      ///< True while the world is loaded, false at all other times. This is relevant only because cf::GuiSys::GuiMan->Yield() is called while loading, which in turn calls our Render() method.
    bool                   WasLMBOnceUp;        ///< The left mouse button must be in the released (non-pressed, up) state after the world has been loaded. This variable is false until this has been the case!

    ScrollInfoT            ChatScrollInfo;
    ScrollInfoT            SystemScrollInfo;
    GraphsT                Graphs;
    unsigned long          ClientFrameNr;
    PlayerCommandT         PlayerCommand;       ///< The player command structure that collects the input until the next call to MainLoop().
    PathRecorderT*         m_PathRecorder;      ///< Records the path of this client in a pointfile that can be loaded into CaWE.
};

#endif
