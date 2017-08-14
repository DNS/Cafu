/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_PLAYERCOMMAND_HPP_INCLUDED
#define CAFU_PLAYERCOMMAND_HPP_INCLUDED

#if defined(_WIN32) && _MSC_VER<1600
#include "pstdint.h"            // Paul Hsieh's portable implementation of the stdint.h header.
#else
#include <stdint.h>
#endif


// Player command key flags
const uint32_t PCK_MoveForward  = 0x00000001;
const uint32_t PCK_MoveBackward = 0x00000002;
const uint32_t PCK_TurnLeft     = 0x00000004;
const uint32_t PCK_TurnRight    = 0x00000008;
const uint32_t PCK_StrafeLeft   = 0x00000010;
const uint32_t PCK_StrafeRight  = 0x00000020;
const uint32_t PCK_LookUp       = 0x00000040;
const uint32_t PCK_LookDown     = 0x00000080;
const uint32_t PCK_CenterView   = 0x00000100;
const uint32_t PCK_Jump         = 0x00000200;
const uint32_t PCK_Duck         = 0x00000400;
const uint32_t PCK_Walk         = 0x00000800;    // Walk (slower than normal)
const uint32_t PCK_Fire1        = 0x00001000;    // Primary Fire (also used e.g. for "Respawn")
const uint32_t PCK_Fire2        = 0x00002000;    // Secondary Fire
const uint32_t PCK_Use          = 0x00004000;    // for "using" or "activating" things
// Bits 28-31 are reserved for storing a number, e.g. for selecting the weapon slot.
// Only one such number can be active at any time.


/// This struct represents per-frame player inputs for controlling human player entities.
/// Player commands are acquired on the clients and sent to the server.
struct PlayerCommandT
{
    PlayerCommandT(uint32_t Keys_=0)
        : FrameTime(0.0f),
          Keys(Keys_),
          DeltaHeading(0),
          DeltaPitch(0),
          DeltaBank(0)
    {
    }

    bool IsDown(uint32_t Key) const
    {
        return (Keys & Key) != 0;
    }

    void Set(uint32_t Key, bool set)
    {
        if (set)
        {
            Keys |= Key;
        }
        else
        {
            Keys &= ~Key;
        }
    }

    void SetNumber(uint32_t n)
    {
        Keys &= 0x0FFFFFFF;
        Keys |= (n << 28);
    }

    float    FrameTime;
    uint32_t Keys;
    uint16_t DeltaHeading;
    uint16_t DeltaPitch;
    uint16_t DeltaBank;
};

#endif
