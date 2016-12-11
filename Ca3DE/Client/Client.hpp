/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CLIENT_HPP_INCLUDED
#define CAFU_CLIENT_HPP_INCLUDED

#include "Network/Network.hpp"
#include "Templates/Pointer.hpp"


namespace cf { namespace GuiSys { class GuiImplT; } }
namespace cf { namespace GuiSys { class GuiResourcesT; } }
struct CaKeyboardEventT;
struct CaMouseEventT;
class  ClientStateT;
class  GameInfoT;
class  MainWindowT;
class  ModelManagerT;
struct lua_State;


class ClientT
{
    public:

    ClientT(MainWindowT& MainWin, const GameInfoT& GameInfo, ModelManagerT& ModelMan, cf::GuiSys::GuiResourcesT& GuiRes);
    ~ClientT();

    void SetMainMenuGui(IntrusivePtrT<cf::GuiSys::GuiImplT> MainMenuGui_);

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
    MainWindowT&                        m_MainWin;          ///< The client's main window.
    const GameInfoT&                    m_GameInfo;         ///< The info for the game that we're running.
    SOCKET                              Socket;             ///< The socket that we're using for the connection to the server. This is a native socket that is managed by this class.
    NetAddressT                         ServerAddress;      ///< The server address we're using for the connection. Copied from the related ConVar whenever a new connection is established.
    unsigned long                       PacketIDConnLess;   ///< The ever increasing (and thus unique) PacketID for outgoing connection-less packets (e.g. connection requests, rcon commands, etc.).
    IntrusivePtrT<cf::GuiSys::GuiImplT> MainMenuGui;        ///< We inform the MainMenuGui whenever we enter a new state (a mini-implementation of the MVC pattern).
    ModelManagerT&                      m_ModelMan;         ///< The model manager that our client worlds load their models from.
    cf::GuiSys::GuiResourcesT&          m_GuiRes;           ///< The GUI resources that are commonly used in our client worlds.
};

#endif
