/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#ifndef _SOUND_IMPL_HPP_
#define _SOUND_IMPL_HPP_

#include "../Sound.hpp"


/// NULL implementation of the sound object.
class SoundT : public SoundI
{
    public:

    /// Constructor.
    SoundT() {}

    // Implement the SoundI interface.
    bool         Play() { return true; }
    void         Stop() { }
    void         Pause() { }
    bool         Resume() { return true; }
    bool         IsPlaying() const { return false; }
    bool         Is3D() const { return true; }
    void         ResetProperties() { }
    void         SetPosition(const Vector3dT& Position_) {}
    void         SetVelocity(const Vector3dT& Velocity_) {}
    void         SetDirection(const Vector3dT& Direction_) {}
    void         SetPriority(unsigned int Priority) { }
    void         SetInnerVolume(float InnerVolume_) { }
    void         SetMinDistance(float MinDist_) { }
    void         SetMaxDistance(float MaxDist_) { }
    void         SetInnerConeAngle(float InnerConeAngle_) { }
    void         SetOuterConeAngle(float OuterConeAngle_) { }
    void         SetOuterVolume(float OuterVolume_) { }
    unsigned int GetPriority() const { return 0; }
    float        GetInnerVolume() const { return 1.0; }
    float        GetMinDistance() const { return 0.0; }
    float        GetMaxDistance() const { return 10000000.0; }
    float        GetInnerConeAngle() const { return 360.0; }
    float        GetOuterConeAngle() const { return 0.0; }
    float        GetOuterVolume() const { return 0.0; }
};

#endif
