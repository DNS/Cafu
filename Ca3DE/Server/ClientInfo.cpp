/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
      PreviousPlayerCommand(),
      PendingPlayerCommands(),
      LastPlayerCommandNr(0),
      LastKnownFrameReceived(0),
      BaseLineFrameNr(1),
      OldStatesPVSEntityIDs()
{
    OldStatesPVSEntityIDs.PushBackEmpty(16);    // The size MUST be a power of 2.
}


void ClientInfoT::InitForNewWorld(unsigned long ClientEntityID)
{
    ClientState            = Wait4MapInfoACK;
    EntityID               = ClientEntityID;

    PreviousPlayerCommand = PlayerCommandT();
    PendingPlayerCommands.Overwrite();

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

    for (unsigned int i = 0; i < OldStatesPVSEntityIDs.Size(); i++)
        OldStatesPVSEntityIDs[i].Overwrite();
}
