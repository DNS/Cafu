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

#ifndef _CLIENTINFO_HPP_
#define _CLIENTINFO_HPP_

#include "Network/Network.hpp"


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
    unsigned long                   LastKnownFrameReceived; ///< Für Delta-Kompression: Letztes Frame, von dem wir wissen, das der Cl es empf. hat.
    unsigned long                   BaseLineFrameNr;
    ArrayT< ArrayT<unsigned long> > OldStatesPVSEntityIDs;
    unsigned long                   CurrentStateIndex;
};

#endif
