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

/**********************/
/*** Player Command ***/
/**********************/

#ifndef CAFU_PLAYERCOMMAND_HPP_INCLUDED
#define CAFU_PLAYERCOMMAND_HPP_INCLUDED


// Player command key flags
const unsigned long PCK_MoveForward =0x00000001;
const unsigned long PCK_MoveBackward=0x00000002;
const unsigned long PCK_TurnLeft    =0x00000004;
const unsigned long PCK_TurnRight   =0x00000008;
const unsigned long PCK_StrafeLeft  =0x00000010;
const unsigned long PCK_StrafeRight =0x00000020;
const unsigned long PCK_LookUp      =0x00000040;
const unsigned long PCK_LookDown    =0x00000080;
const unsigned long PCK_CenterView  =0x00000100;
const unsigned long PCK_Jump        =0x00000200;    // Jump
const unsigned long PCK_Duck        =0x00000400;    // Duck
const unsigned long PCK_Walk        =0x00000800;    // Walk (slower than normal)
const unsigned long PCK_Fire1       =0x00001000;    // Primary   Fire (also used e.g. for "Respawn")
const unsigned long PCK_Fire2       =0x00002000;    // Secondary Fire
const unsigned long PCK_Use         =0x00004000;    // Use
// Bits 28-31 are reserved for selecting the weapon slot!


// Commonly used data structure to store and pass around player commands.
// Shared among the game DLLs, the client and the server code.
struct PlayerCommandT
{
    // 'Nr' is the consecutive number of this 'PlayerCommandT'.
    // It can be thought of as the clients frame number, or the sequence number of the packet with which this
    // 'PlayerCommandT' is sent to the server.
    // The main property is that a human player entity never sees the same number twice ('Nr' is counted up).
    // (However, 'PlayerCommandT's arriving at the server from *different* human player entites can well contain the same
    //  numbers at a time. In such cases, even the enitre sequence of 'Nr's in consecutive 'PlayerCommandT's is identical,
    //  because they are all treated likewise. Fortunately, that's not a problem:
    //  Clients usually join the game at different times (yielding different starting numbers),
    //  they have different FPS (the sequence of 'PlayerCommandT's advances at a different pace),
    //  and finally different human player entities use the numbers entirely different.)
    // The main purpose is to give the human player entity in the game DLL a way to derive pseudo random numbers
    // (by using 'Nr' as table index) that also work in the presence of "client prediction".
    float          FrameTime;
    unsigned long  Keys;
    unsigned short DeltaHeading;
    unsigned short DeltaPitch;
    unsigned short DeltaBank;
    unsigned long  Nr;

    PlayerCommandT(float FrameTime_=0, unsigned long Keys_=0, unsigned short DeltaHeading_=0, unsigned short DeltaPitch_=0, unsigned short DeltaBank_=0, unsigned long Nr_=0)
        : FrameTime(FrameTime_), Keys(Keys_), DeltaHeading(DeltaHeading_), DeltaPitch(DeltaPitch_), DeltaBank(DeltaBank_), Nr(Nr_) {}
};

#endif
