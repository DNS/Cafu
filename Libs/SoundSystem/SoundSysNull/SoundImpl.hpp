/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SOUND_IMPL_HPP_INCLUDED
#define CAFU_SOUND_IMPL_HPP_INCLUDED

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
