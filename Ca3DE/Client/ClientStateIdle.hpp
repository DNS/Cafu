/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CLIENT_STATE_IDLE_HPP_INCLUDED
#define CAFU_CLIENT_STATE_IDLE_HPP_INCLUDED

#include "ClientState.hpp"
#include "Fonts/Font.hpp"


class ClientT;


/// This class implements the idle state of the client.
class ClientStateIdleT : public ClientStateT
{
    public:

    ClientStateIdleT(ClientT& Client_);

    // Implement the ClientStateT interface.
    int GetID() const override;
    bool ProcessInputEvent(const CaKeyboardEventT& KE) override;
    bool ProcessInputEvent(const CaMouseEventT& ME) override;
    void Render(float FrameTime) override;
    void MainLoop(float FrameTime) override;


    private:

    ClientT& Client;    ///< The context this state is a part of.
};

#endif
