/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CLIENTINFO_HPP_INCLUDED
#define CAFU_CLIENTINFO_HPP_INCLUDED

#include "Network/Network.hpp"
#include "../PlayerCommand.hpp"


struct ClientInfoT
{
    ClientInfoT(const NetAddressT& ClientAddress_, const std::string& PlayerName_, const std::string& ModelName_);

    void InitForNewWorld(unsigned long ClientEntityID);


    enum ClientStateT { Online, Wait4MapInfoACK, Zombie };

    // Connection-related data
    NetAddressT                     ClientAddress;          ///< IP+Port of the client.
    GameProtocol1T                  GameProtocol;           ///< The network protocol instance we use for communication with the client.
    ArrayT< ArrayT<char> >          ReliableDatas;          ///< Puffer für wichtige, zu bestätigende Messages an den Client.
    float                           TimeSinceLastMessage;   ///< Time that passed since the last message arrived (for time-outs and zombies).
    float                           TimeSinceLastUpdate;

    // Client-related data
    ClientStateT                    ClientState;            ///< Online, Wait4MapInfoACK, Zombie, Download, ...
    std::string                     PlayerName;             ///< Player name (e.g. Thunderbird, Firefox, Mordred, ...).
    std::string                     ModelName;              ///< Model  name (e.g. Trinity, T801, James, ...).

    // World-related data
    unsigned long                   EntityID;               ///< ID of our HumanPlayer entity.
    PlayerCommandT                  PreviousPlayerCommand;  ///< The last player command that we have received from the client in the *previous* server frame.
    ArrayT<PlayerCommandT>          PendingPlayerCommands;  ///< Player commands that we have received in the current server frame but have not yet been processed in the game world.
    unsigned int                    LastPlayerCommandNr;    ///< The number of the last player command that we have received from the client.
    unsigned long                   LastKnownFrameReceived; ///< Für Delta-Kompression: Letztes Frame, von dem wir wissen, das der Cl es empf. hat.
    unsigned long                   BaseLineFrameNr;
    ArrayT< ArrayT<unsigned long> > OldStatesPVSEntityIDs;  ///< TODO: Replace type with `ArrayT<FrameInfoT>` ?
};

#endif
