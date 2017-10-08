/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Client.hpp"
#include "ClientStateConnecting.hpp"
#include "ClientStateIdle.hpp"
#include "ClientStateInGame.hpp"
#include "../NetConst.hpp"

#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/ConFunc.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "GuiSys/GuiImpl.hpp"
#include "UniScriptState.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}


static const char* StateNames[]={ "idle", "connecting", "ingame" };
static ClientT*    ClientPtr   =NULL;


ClientT::ClientT(MainWindowT& MainWin, const GameInfoT& GameInfo, ModelManagerT& ModelMan, cf::GuiSys::GuiResourcesT& GuiRes)
    : CurrentState(NULL),
      NextState(IDLE),
      m_MainWin(MainWin),
      m_GameInfo(GameInfo),
      Socket(INVALID_SOCKET),
      ServerAddress(0, 0, 0, 0, 0),
      PacketIDConnLess(0),
      MainMenuGui(NULL),
      m_ModelMan(ModelMan),
      m_GuiRes(GuiRes)
{
    // Cannot do this directly in the initializer list above, because then the
    // ClientStateIdleT ctor would try to access the not yet initialized client ("this").
    CurrentState=new ClientStateIdleT(*this);

    assert(ClientPtr==NULL);
    ClientPtr=this;
}


ClientT::~ClientT()
{
    if (Socket!=INVALID_SOCKET) closesocket(Socket);

    delete CurrentState;

    assert(ClientPtr==this);
    ClientPtr=NULL;
}


void ClientT::SetMainMenuGui(IntrusivePtrT<cf::GuiSys::GuiImplT> MainMenuGui_)
{
    MainMenuGui=MainMenuGui_;

    MainMenuGui->GetScriptState().Call("OnClientStateChanged", "s", StateNames[GetStateID()]);
}


bool ClientT::ProcessInputEvent(const CaKeyboardEventT& KE)
{
    UpdateCurrentState();
    return CurrentState->ProcessInputEvent(KE);
}


bool ClientT::ProcessInputEvent(const CaMouseEventT& ME)
{
    UpdateCurrentState();
    return CurrentState->ProcessInputEvent(ME);
}


void ClientT::Render(float FrameTime)
{
    UpdateCurrentState();
    CurrentState->Render(FrameTime);
}


void ClientT::MainLoop(float FrameTime)
{
    UpdateCurrentState();
    CurrentState->MainLoop(FrameTime);
}


ClientT::StateIDT ClientT::GetStateID() const
{
    assert(CurrentState!=NULL);

    return (StateIDT)CurrentState->GetID();
}


// Some notes about changing the state (CurrentState) of the client:
// The state is usually changed from within another state by using a statement like: Client.NextState=IDLE;
// The client will then make a state instance with ID NextState the CurrentState before the next call to any method of CurrentState.
// The advantage of handling state changes like this is that the ctors and dtors of the states are properly run, that is,
// the state that is left is properly destructed by its dtor and then the state that is entered is properly constructed by its ctor,
// in this order. This is obviously good for the reliable management of (shared) resources.
// Moreover, the callers don't need to really "know" the other state classes, i.e. no need for all those #include's everywhere.
// Before that, I kept all the states as member objects as private members here, and had CurrentState point to one of them.
// That was nice and simple, but required *very* ugly OnEnter() and OnLeave() methods for the resource management,
// so that I rather changed to this solution that is slightly more complex but properly employs the ctors and dtors.
void ClientT::UpdateCurrentState()
{
    if (GetStateID()!=NextState)
    {
        // Delete the CurrentState first, then allocate the new state.
        // This makes sure that the dtor of the current state is run before the ctor of the next.
        delete CurrentState;

        switch (NextState)
        {
            case IDLE:       CurrentState=new ClientStateIdleT(*this); break;
            case CONNECTING: CurrentState=new ClientStateConnectingT(*this); break;
            case INGAME:     CurrentState=new ClientStateInGameT(*this); break;
        }

        MainMenuGui->GetScriptState().Call("OnClientStateChanged", "s", StateNames[NextState]);
    }

    assert(GetStateID()==NextState);
}


static ConVarT ClientRCPassword("cl_rc_password", "", ConVarT::FLAG_MAIN_EXE, "The password with which the client sends remote console commands to the server.");


// IMPORTANT NOTE:
// This function is actually UNRELATED to the game client, because it's more generic:
// It can also be used when the client is disconnected (idle).
int ClientT::ConFunc_rcon_Callback(lua_State* LuaState)
{
    if (!ClientPtr) return 0;

    // Die Antwort auf ein "connection-less" Packet beginnt mit 0xFFFFFFFF, gefolgt von der PacketID,
    // sodaß das Rückpacket unten nur noch individuell beendet werden muß.
    NetDataT OutData;

    OutData.WriteLong(0xFFFFFFFF);
    OutData.WriteLong(ClientPtr->PacketIDConnLess++);
    OutData.WriteByte(CS0_RemoteConsoleCommand);
    OutData.WriteString(ClientRCPassword.GetValueString());
    OutData.WriteString(luaL_checkstring(LuaState, 1));

    try
    {
        OutData.Send(ClientPtr->Socket, ClientPtr->ServerAddress);
    }
    catch (const NetDataT::WinSockAPIError& E) { Console->Print(cf::va("Send failed (WSA error %u)!\n", E.Error)); }
    catch (const NetDataT::MessageLength&   E) { Console->Print(cf::va("Message too long (wanted %u, actual %u)!\n", E.Wanted, E.Actual)); }

    return 0;
}

static ConFuncT ConFunc_rcon("rcon", ClientT::ConFunc_rcon_Callback, ConFuncT::FLAG_MAIN_EXE, "Sends a command to the remote console, e.g.   rcon(\"changeLevel('<MapName>')\")");


int ClientT::ConFunc_connect_Callback(lua_State* LuaState)
{
    if (!ClientPtr) return luaL_error(LuaState, "The client instance is not available.\n");

    switch (ClientPtr->GetStateID())
    {
        case IDLE:
        {
            const char* ServerName=luaL_checkstring(LuaState, 1);
            const int   ServerPort=luaL_checkint(LuaState, 2);

            try
            {
                // Must set the ServerAddress member before changing into connecting state.
                ClientPtr->ServerAddress=NetAddressT(ServerName, ServerPort);
            }
            catch (const NetAddressT::BadHostName& /*E*/)
            {
                return luaL_error(LuaState, "Unable to resolve server name \"%s\".\n", ServerName);
            }

            // Ok, connect to the server.
            ClientPtr->NextState=CONNECTING;
            return 0;
        }

        case CONNECTING: return luaL_error(LuaState, "The client is already connecting...\n");
        case INGAME:     return luaL_error(LuaState, "The client is already connected.\n");
    }

    return luaL_error(LuaState, "The client is in an unknown state.\n");
}

static ConFuncT ConFunc_connect("connect", ClientT::ConFunc_connect_Callback, ConFuncT::FLAG_MAIN_EXE, "Connects the client to the server.");


int ClientT::ConFunc_disconnect_Callback(lua_State* LuaState)
{
    if (!ClientPtr) return luaL_error(LuaState, "The client instance is not available.\n");

    switch (ClientPtr->GetStateID())
    {
        case IDLE:       return luaL_error(LuaState, "The client is not connected.\n");
        case CONNECTING: ClientPtr->NextState=IDLE; return 0;     // Abort connection attempt (premature timeout).
        case INGAME:     ClientPtr->NextState=IDLE; return 0;     // Ok, disconnect from server.
    }

    return luaL_error(LuaState, "The client is in an unknown state.\n");
}

static ConFuncT ConFunc_disconnect("disconnect", ClientT::ConFunc_disconnect_Callback, ConFuncT::FLAG_MAIN_EXE, "Disconnects the client from the server.");
