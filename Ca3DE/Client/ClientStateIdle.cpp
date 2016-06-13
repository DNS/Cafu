/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
