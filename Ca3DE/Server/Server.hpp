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

#ifndef CAFU_SERVER_HPP_INCLUDED
#define CAFU_SERVER_HPP_INCLUDED

#include "Network/Network.hpp"
#include "Util/Util.hpp"

#include <stdexcept>
#include <string>


struct lua_State;
struct ClientInfoT;
class  CaServerWorldT;
class  ModelManagerT;
namespace cf { namespace GameSys { class GameI; } }


/// The server, like the client, is a state machine.
/// It doesn't present its state explicitly however, but only as two implicit states: map loaded ("normal, running") and map unloaded ("idle").
/// As with the client, having a true "idle" state (rather than expressing it with a NULL ServerT instance pointer) has several advantages:
/// a) We can gracefully terminate pending network connections (e.g. resend reliable data in the clients zombie state), and
/// b) the server can receive and process conn-less network packets, and thus is available for administration via rcon commands.
class ServerT
{
    public:

    class InitErrorT;

    /// A class that the server uses in order to let a GUI know in which state the server currently is.
    /// The GUI uses it to decide which buttons it should enable/disable (i.e. which confuncs it makes sense to call).
    /// (This is the C++ equivalent to a traditional C call-back function.)
    class GuiCallbackI
    {
        public:

        virtual void OnServerStateChanged(const char* NewState) const=0;
        virtual ~GuiCallbackI() { }
    };


    /// The constructor.
    /// @throws InitErrorT if the server could not be initialized (e.g. a socket for the desired port could not be aquired).
    ServerT(cf::GameSys::GameI* Game, const std::string& GameName_, const GuiCallbackI& GuiCallback_, ModelManagerT& ModelMan);

    ~ServerT();

    void MainLoop();   // Server main loop. To be called once per frame.


    static int ConFunc_changeLevel_Callback(lua_State* LuaState);


    private:

    ServerT(const ServerT&);                    ///< Use of the Copy    Constructor is not allowed.
    void operator = (const ServerT&);           ///< Use of the Assignment Operator is not allowed.

    void        DropClient(unsigned long ClientNr, const char* Reason);
    void        ProcessConnectionLessPacket(NetDataT& InData, const NetAddressT& SenderAddress);
    void        ProcessInGamePacket      (NetDataT& InData, unsigned long LastIncomingSequenceNr);
    static void ProcessInGamePacketHelper(NetDataT& InData, unsigned long LastIncomingSequenceNr);


    TimerT               Timer;
    SOCKET               ServerSocket;
    ArrayT<ClientInfoT*> ClientInfos;
    cf::GameSys::GameI*  m_Game;
    std::string          GameName;
    std::string          WorldName;
    CaServerWorldT*      World;
    const GuiCallbackI&  GuiCallback;
    ModelManagerT&       m_ModelMan;
};


/// A class that is thrown on server initialization errors.
class ServerT::InitErrorT : public std::runtime_error
{
    public:

    InitErrorT(const std::string& Message) : std::runtime_error(Message) { }
};

#endif
