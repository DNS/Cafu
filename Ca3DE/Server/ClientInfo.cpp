/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2014 Carsten Fuchs Software.

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

#include "ClientInfo.hpp"


ClientInfoT::ClientInfoT(const NetAddressT& ClientAddress_, const std::string& PlayerName_, const std::string& ModelName_)
    : ClientAddress(ClientAddress_),
      GameProtocol(),
      ReliableDatas(),
      TimeSinceLastMessage(0.0),
      TimeSinceLastUpdate(0.0),
      ClientState(Wait4MapInfoACK),
      PlayerName(PlayerName_),
      ModelName(ModelName_),
      EntityID(0),
      LastPlayerCommandNr(0),
      LastKnownFrameReceived(0),
      BaseLineFrameNr(1),
      OldStatesPVSEntityIDs(),
      CurrentStateIndex(0)
{
}


void ClientInfoT::InitForNewWorld(unsigned long ClientEntityID)
{
    ClientState            = Wait4MapInfoACK;
    EntityID               = ClientEntityID;

    // The client restarts the player command numbering at 1 whenever a new world is entered.
    // A value of 0 means that no player command has been received yet (in the current world).
    LastPlayerCommandNr    = 0;

    // From Ca3DEWorldT init (ctor), the client knows the baselines ("create states") for frame 1,
    // so the server does not have to sent `SC1_EntityBaseLine` messages for the entities in the map file.
    // However, the first `SC1_FrameInfo` message can *not* delta from frame 1, but must delta from the
    // baselines, because the client's PVS info for frame 1 is not setup. And even if it was, the server
    // frame is probably much much larger than 1, so that limited PVS information buffering would require
    // sending from the baseline anyway. Thus LastKnownFrameReceived = 0.
    LastKnownFrameReceived = 0;
    BaseLineFrameNr        = 1;

    OldStatesPVSEntityIDs.Clear();
    CurrentStateIndex      = 0;
}
