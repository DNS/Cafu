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

#ifndef _CA3DE_CLIENT_STATE_HPP_
#define _CA3DE_CLIENT_STATE_HPP_


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
