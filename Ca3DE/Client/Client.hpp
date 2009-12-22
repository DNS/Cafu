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

#ifndef _CA3DE_CLIENT_HPP_
#define _CA3DE_CLIENT_HPP_

#include "Network/Network.hpp"


namespace cf { namespace GuiSys { class GuiI; } }
struct CaKeyboardEventT;
struct CaMouseEventT;
class  ClientStateT;
struct lua_State;


class ClientT
{
    public:

    ClientT();
    ~ClientT();

    void SetMainMenuGui(cf::GuiSys::GuiI* MainMenuGui_);

    // These methods are driven (called) by the GUI window that "owns" this client.
    bool ProcessInputEvent(const CaKeyboardEventT& KE);
    bool ProcessInputEvent(const CaMouseEventT&    ME);
    void Render(float FrameTime);
    void MainLoop(float FrameTime);


    static int ConFunc_rcon_Callback(lua_State* LuaState);
    static int ConFunc_connect_Callback(lua_State* LuaState);
    static int ConFunc_disconnect_Callback(lua_State* LuaState);


    private:

    friend class ClientStateIdleT;
    friend class ClientStateConnectingT;
    friend class ClientStateInGameT;

    enum StateIDT
    {
        IDLE=0,         ///< Client is in idle state, sitting around, doing nothing. This state is explicitly required (and cannot be implied by a NULL ClientT instance in user code) because the client may want to enter it on own discretion, especially after errors (e.g. unable to connect, disconnect from server, etc.).
        CONNECTING,     ///< Client is connecting to server.
     // W4WORLD,        ///< Client is waiting for "SC1_WorldInfo" message.
        INGAME,         ///< Client is fully joined into a game.
     // DISCONNECTING   ///< Client is disconnecting from server (not really used now, but with TCP or SCTP connections we might want to implement a graceful shutdown).
    };


    StateIDT GetStateID() const;        ///< Returns the ID of CurrentState.
    void UpdateCurrentState();          ///< Implements the transition to a new state.

    ClientT(const ClientT&);            ///< Use of the Copy    Constructor is not allowed.
    void operator = (const ClientT&);   ///< Use of the Assignment Operator is not allowed.


    ClientStateT*     CurrentState;     ///< The state that the client is currently in.
    StateIDT          NextState;        ///< The (ID of the) state that will become the current state before the next call to any of the methods of CurrentState.

    // Private data that is used in all (or at least multiple) states.
    SOCKET            Socket;           ///< The socket that we're using for the connection to the server. This is a native socket that is managed by this class.
    NetAddressT       ServerAddress;    ///< The server address we're using for the connection. Copied from the related ConVar whenever a new connection is established.
    unsigned long     PacketIDConnLess; ///< The ever increasing (and thus unique) PacketID for outgoing connection-less packets (e.g. connection requests, rcon commands, etc.).
    cf::GuiSys::GuiI* MainMenuGui;      ///< We inform the MainMenuGui whenever we enter a new state (a mini-implementation of the MVC pattern).
};

#endif
