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

#ifndef CAFU_INTERPOLATOR_HPP_INCLUDED
#define CAFU_INTERPOLATOR_HPP_INCLUDED


/// A common base class for "approximators" (interpolators and extrapolators),
/// so that approximators of different types can easily be managed together.
class ApproxBaseT
{
    public:

    virtual void ReInit()=0;
    virtual void NotifyOverwriteUpdate()=0;
    virtual void Interpolate(float Time)=0;

    virtual ~ApproxBaseT() { }
};


/// Linearly interpolates a value over a period of time.
/// The course of the interpolation is adapted/updated whenever a new reference value is set.
/// This is mostly intended for game entities on clients, where we interpolate from the "current" value towards the
/// reference value of the most recent server update. If the subsequent server update happens to arrives later than
/// the one before it, our interpolation may (briefly) turn into an extrapolation for bridging the gap.
template<class T>
class InterpolatorT : public ApproxBaseT
{
    public:

    InterpolatorT(T& v)
        : m_Value(v),
          m_LastValue(v),
          m_Gradient(T()),  // If T is float, this initializes m_Gradient with 0.0f.
          m_ExtTime(0.0f)
    {
    }

    /// Used to re-initialize this interpolator at the current value.
    void ReInit()
    {
        m_LastValue = m_Value;
        m_Gradient  = T();
        m_ExtTime   = 0.0f;
    }

    /// The user calls this method in order to let the interpolator know that the interpolated value was changed externally.
    void NotifyOverwriteUpdate()
    {
        const T NewRef = m_Value;
        m_Value = m_LastValue;

        UpdateRef(NewRef);
    }

    /// Sets a new reference value: the value that we should interpolate to.
    void UpdateRef(const T& Ref)
    {
        const T DeltaY = Ref - m_Value;

        if (m_ExtTime < 0.0001f || !CanContinue(DeltaY))
        {
            m_Value     = Ref;
            m_LastValue = Ref;
            m_Gradient  = T();
        }
        else
        {
            m_Gradient = DeltaY/m_ExtTime;
        }

        m_ExtTime = 0.0f;
    }

    /// Advances the interpolation over the given time.
    void Interpolate(float Time)
    {
        m_Value   += m_Gradient*Time;
        m_ExtTime += Time;

        m_LastValue = m_Value;
    }


    private:

    bool CanContinue(const T& DeltaY) const { return length(DeltaY) < 5000.0f; }

    T&     m_Value;     ///< The value that is interpolated.
    T      m_LastValue; ///< The last interpolated value. Normally m_LastValue == m_Value.
    T      m_Gradient;  ///< The change in the value over time.
    float  m_ExtTime;   ///< The time we've been interpolating since the reference value was last set.
};


/// Linearly extrapolates a value over a period of time.
/// The course of the extrapolation is adapted/updated whenever a new reference value is set.
/// This is mostly intended for game entities on clients, where we try to extrapolate values
/// close to where we expect the next server update will bring them to.
///
/// Unfortunately, extrapolation is less useful and worthwhile than it initially seems, because when it is applied
/// to entity origins, all kinds of unwanted side effects can happen: items that fall to the floor penetrate the
/// ground plane, then resurface; the wings of closing doors bump into each other, then retract; lifts overshoot
/// their halting position; grenades that are deflected by walls briefly vanish into them; etc.
template<class T>
class ExtrapolatorT : public ApproxBaseT
{
    public:

    ExtrapolatorT(T& v)
        : m_Value(v),
          m_LastValue(v),
          m_Gradient(T()),  // If T is float, this initializes m_Gradient with 0.0f.
          m_LastRef(v),
          m_ExtTime(0.0f)
    {
    }

    /// Used to re-initialize this extrapolator at the current value.
    void ReInit()
    {
        m_LastValue = m_Value;
        m_Gradient  = T();
        m_LastRef   = m_Value;
        m_ExtTime   = 0.0f;
    }

    /// The user calls this method in order to let the extrapolator know that the extrapolated value was changed externally.
    void NotifyOverwriteUpdate()
    {
        const T NewRef = m_Value;
        m_Value = m_LastValue;

        UpdateRef(NewRef);
    }

    /// Sets a new reference value: the value that the extrapolated value currently "should have".
    void UpdateRef(const T& Ref)
    {
        /// m_Value was extrapolated starting from its own value at the time the reference value was last set,
        /// using the gradient m_Gradient that was applied over the the extrapolation time m_ExtTime.
        /// If the new reference value Ref now turns out to be different from m_Value, it used a gradient TrueG
        /// other than the expected gradient from the previous m_LastRef to m_LastRef.
        /// Considering that the value changed from m_LastRef to Ref, and assuming that this time TrueG will remain
        /// constant over the next extrapolation period, the new expected value is Ref + (Ref - m_LastRef), and the
        /// change that is required to get from m_Value to the new expected value is 2*Ref - m_LastRef - m_Value.
        const T DeltaY = Ref*2 - m_LastRef - m_Value;

        if (m_ExtTime < 0.0001f || !CanContinue(DeltaY))
        {
            m_Value     = Ref;
            m_LastValue = Ref;
            m_Gradient  = T();
        }
        else
        {
            m_Gradient = DeltaY/m_ExtTime;
        }

        m_LastRef = Ref;
        m_ExtTime = 0.0f;
    }

    /// Advances the extrapolation over the given time.
    void Interpolate(float Time)
    {
        m_Value   += m_Gradient*Time;
        m_ExtTime += Time;

        m_LastValue = m_Value;
    }


    private:

    bool CanContinue(const T& DeltaY) const { return length(DeltaY) < 5000.0f; }

    T&     m_Value;     ///< The value that is extrapolated.
    T      m_LastValue; ///< The last extrapolated value. Normally m_LastValue == m_Value.
    T      m_Gradient;  ///< The change in the value over time.
    T      m_LastRef;   ///< The last reference value (*not* necessarily the value from which the extrapolation started).
    float  m_ExtTime;   ///< The time we've been extrapolating since the reference value was last set.
};

#endif
