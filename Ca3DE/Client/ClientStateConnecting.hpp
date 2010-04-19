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

#ifndef _CAFU_CLIENT_STATE_CONNECTING_HPP_
#define _CAFU_CLIENT_STATE_CONNECTING_HPP_

#include "ClientState.hpp"
#include "Fonts/Font.hpp"


class ClientT;
class NetAddressT;
class NetDataT;


/// This class implements the "connecting-to-server" state of the client.
class ClientStateConnectingT : public ClientStateT
{
    public:

    ClientStateConnectingT(ClientT& Client_);

    int GetID() const;

    // Implementation of the ClientStateT interface.
    bool ProcessInputEvent(const CaKeyboardEventT& KE);
    bool ProcessInputEvent(const CaMouseEventT&    ME);
    void Render(float FrameTime);
    void MainLoop(float FrameTime);


    private:

    void ProcessConnectionLessPacket(NetDataT& InData, const NetAddressT& SenderAddress);

    ClientT&            Client;     ///< The context this state is a part of.
    const unsigned long PacketID;   ///< The packet ID with which we sent off our connection request.
    float               TimeLeft;   ///< Time left after connection request to requeck ACK until timeout.
};

#endif
