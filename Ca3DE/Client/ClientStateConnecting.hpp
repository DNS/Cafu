/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CLIENT_STATE_CONNECTING_HPP_INCLUDED
#define CAFU_CLIENT_STATE_CONNECTING_HPP_INCLUDED

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

    // Implement the ClientStateT interface.
    int GetID() const override;
    bool ProcessInputEvent(const CaKeyboardEventT& KE) override;
    bool ProcessInputEvent(const CaMouseEventT&    ME) override;
    void Render(float FrameTime) override;
    void MainLoop(float FrameTime) override;


    private:

    void ProcessConnectionLessPacket(NetDataT& InData, const NetAddressT& SenderAddress);

    ClientT&            Client;     ///< The context this state is a part of.
    const unsigned long PacketID;   ///< The packet ID with which we sent off our connection request.
    float               TimeLeft;   ///< Time left after connection request to requeck ACK until timeout.
};

#endif
