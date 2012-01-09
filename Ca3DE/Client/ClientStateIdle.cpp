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

#include "ClientStateIdle.hpp"
#include "Client.hpp"


ClientStateIdleT::ClientStateIdleT(ClientT& Client_)
    : Client(Client_)
{
    if (Client.Socket!=INVALID_SOCKET)
    {
        closesocket(Client.Socket);
        Client.Socket=INVALID_SOCKET;
    }

    Client.ServerAddress=NetAddressT(0, 0, 0, 0, 0);
}


int ClientStateIdleT::GetID() const
{
    return ClientT::IDLE;
}


bool ClientStateIdleT::ProcessInputEvent(const CaKeyboardEventT& KE)
{
    // No, we did not process this event.
    return false;
}


bool ClientStateIdleT::ProcessInputEvent(const CaMouseEventT& ME)
{
    // No, we did not process this event.
    return false;
}


void ClientStateIdleT::Render(float FrameTime)
{
    // This was mostly for debugging, and is now occluded by full-screen main menu background images anyway.
    // If you want to output the current client state in the GUI, just use something like   self:setText(ci.GetValue("clState"));
    // Font.Print(130, 400, 0x00808080, "Idle...");
}


void ClientStateIdleT::MainLoop(float FrameTime)
{
}
