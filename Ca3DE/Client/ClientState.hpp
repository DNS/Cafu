/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CLIENT_STATE_HPP_INCLUDED
#define CAFU_CLIENT_STATE_HPP_INCLUDED


struct CaKeyboardEventT;
struct CaMouseEventT;


/// This is the base class for the concrete classes that implement the states of the client.
/// It is part of the State pattern (see the GoF book) that we employ to manage the states of the client:
/// "It defines the interface for encapsulating the behaviour that is associated with a certain state of the client."
class ClientStateT
{
    public:

    /// The virtual destructor.
    virtual ~ClientStateT() { }

    /// Returns some client-specific, unique ID for this state.
    virtual int GetID() const=0;

    // These are the methods that implement the state-specific behaviour.
    // The client will forward all calls to its own methods to these methods of the current state.
    virtual bool ProcessInputEvent(const CaKeyboardEventT& KE)=0;
    virtual bool ProcessInputEvent(const CaMouseEventT&    ME)=0;
    virtual void Render(float FrameTime)=0;
    virtual void MainLoop(float FrameTime)=0;
};

#endif
