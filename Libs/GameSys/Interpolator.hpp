/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2014 Carsten Fuchs Software.

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

#ifndef CAFU_GAMESYS_INTERPOLATOR_HPP_INCLUDED
#define CAFU_GAMESYS_INTERPOLATOR_HPP_INCLUDED

#include "Math3D/Quaternion.hpp"
#include "Variables.hpp"


namespace cf
{
    namespace GameSys
    {
        /// A common base class for "approximators" (interpolators and extrapolators),
        /// so that approximators of different types can easily be managed together.
        ///
        /// ApproxBaseT%s are used in ComponentBaseT in order to advance values over
        /// client frames in order to bridge the larger intervals between server frames,
        /// especially the origin and the orientation of movable NPC entities.
        class ApproxBaseT
        {
            public:

            virtual ~ApproxBaseT() { }

            virtual cf::TypeSys::VarBaseT* GetVar() const = 0;
            virtual void ReInit() = 0;
            virtual void NotifyOverwriteUpdate() = 0;
            virtual void Interpolate(float Time) = 0;
        };


        /// This visitor is used to obtain an ApproxBaseT instance for a variable.
        class VarVisitorGetApproxT : public cf::TypeSys::VisitorT
        {
            public:

            VarVisitorGetApproxT();
            ~VarVisitorGetApproxT();

            ApproxBaseT* TransferApprox();

            void visit(cf::TypeSys::VarT<float>& Var);
            void visit(cf::TypeSys::VarT<double>& Var);
            void visit(cf::TypeSys::VarT<int>& Var);
            void visit(cf::TypeSys::VarT<unsigned int>& Var);
            void visit(cf::TypeSys::VarT<uint16_t>& Var);
            void visit(cf::TypeSys::VarT<uint8_t>& Var);
            void visit(cf::TypeSys::VarT<bool>& Var);
            void visit(cf::TypeSys::VarT<std::string>& Var);
            void visit(cf::TypeSys::VarT<Vector2fT>& Var);
            void visit(cf::TypeSys::VarT<Vector3fT>& Var);
            void visit(cf::TypeSys::VarT<Vector3dT>& Var);
            void visit(cf::TypeSys::VarT<BoundingBox3dT>& Var);
            void visit(cf::TypeSys::VarArrayT<uint32_t>& Var);
            void visit(cf::TypeSys::VarArrayT<uint16_t>& Var);
            void visit(cf::TypeSys::VarArrayT<uint8_t>& Var);
            void visit(cf::TypeSys::VarArrayT<std::string>& Var);


            private:

            ApproxBaseT* m_Approx;
        };


        /// Linearly interpolates a value over a period of time.
        /// The course of the interpolation is adapted/updated whenever a new reference value is set.
        /// This is mostly intended for game entities on clients, where we interpolate from the "current" value
        /// towards the reference value of the most recent server update. If the subsequent server update happens
        /// to arrive later than the one before it, our interpolation may (briefly) turn into an extrapolation
        /// for bridging the gap.
        template<class T>
        class VarInterpolatorT : public ApproxBaseT
        {
            public:

            VarInterpolatorT(cf::TypeSys::VarT<T>& v)
                : m_Value(v),
                  m_LastValue(v.Get()),
                  m_Gradient(T()),  // If T is float, this initializes m_Gradient with 0.0f.
                  m_ExtTime(0.0f)
            {
            }

            cf::TypeSys::VarBaseT* GetVar() const
            {
                return &m_Value;
            }

            /// Used to re-initialize this interpolator at the current value.
            void ReInit()
            {
                m_LastValue = m_Value.Get();
                m_Gradient  = T();
                m_ExtTime   = 0.0f;
            }

            /// The user calls this method in order to let the interpolator know that the interpolated value was changed externally.
            void NotifyOverwriteUpdate()
            {
                const T NewRef = m_Value.Get();
                m_Value.Set(m_LastValue);

                UpdateRef(NewRef);
            }

            /// Advances the interpolation over the given time.
            void Interpolate(float Time)
            {
                m_Value.Set(m_Value.Get() + m_Gradient*Time);
                m_ExtTime += Time;

                m_LastValue = m_Value.Get();
            }


            private:

            bool CanContinue(const T& DeltaY) const;

            /// Sets a new reference value: the value that we should interpolate to.
            void UpdateRef(const T& Ref)
            {
                const T DeltaY = Ref - m_Value.Get();

                if (m_ExtTime < 0.0001f || !CanContinue(DeltaY))
                {
                    m_Value.Set(Ref);
                    m_LastValue = Ref;
                    m_Gradient  = T();
                }
                else
                {
                    m_Gradient = DeltaY/m_ExtTime;
                }

                m_ExtTime = 0.0f;
            }


            cf::TypeSys::VarT<T>& m_Value;      ///< The value that is interpolated.
            T                     m_LastValue;  ///< The last interpolated value. Normally m_LastValue == m_Value.
            T                     m_Gradient;   ///< The change in the value over time.
            float                 m_ExtTime;    ///< The time we've been interpolating since the reference value was last set.
        };


        template<class T> inline bool VarInterpolatorT<T>::CanContinue(const T& DeltaY) const
        {
            return DeltaY < 128;
        }

        template<> inline bool VarInterpolatorT<Vector2fT>::CanContinue(const Vector2fT& DeltaY) const
        {
            return length(DeltaY) < 128;
        }

        template<> inline bool VarInterpolatorT<Vector3fT>::CanContinue(const Vector3fT& DeltaY) const
        {
            return length(DeltaY) < 128;
        }

        template<> inline bool VarInterpolatorT<Vector3dT>::CanContinue(const Vector3dT& DeltaY) const
        {
            return length(DeltaY) < 128;
        }


        /// This class is like VarInterpolatorT, but for Vector3fT%s that represent quaternions.
        class VarSlerpT : public ApproxBaseT
        {
            public:

            VarSlerpT(cf::TypeSys::VarT<Vector3fT>& v)
                : m_Value(v),
                  m_A(cf::math::QuaternionfT::FromXYZ(v.Get())),
                  m_B(m_A),
                  m_Total(0.0f),
                  m_Time(0.0f)
            {
                assert(v.HasFlag("IsQuat"));
            }

            cf::TypeSys::VarBaseT* GetVar() const
            {
                return &m_Value;
            }

            /// Used to re-initialize this interpolator at the current value.
            void ReInit()
            {
                m_A     = cf::math::QuaternionfT::FromXYZ(m_Value.Get());
                m_B     = m_A;
                m_Total = 0.0f;
                m_Time  = 0.0f;
            }

            /// The user calls this method in order to let the interpolator know that the interpolated value was changed externally.
            void NotifyOverwriteUpdate()
            {
                m_A     = m_Total > 0.001f ? slerp(m_A, m_B, m_Time / m_Total) : m_B;
                m_B     = cf::math::QuaternionfT::FromXYZ(m_Value.Get());
                m_Total = m_Time;
                m_Time  = 0.0f;

                m_Value.Set(m_A.GetXYZ());
            }

            /// Advances the interpolation over the given time.
            void Interpolate(float Time)
            {
                m_Time += Time;
                m_Value.Set(m_Total > 0.001f ? slerp(m_A, m_B, m_Time / m_Total).GetXYZ() : m_B.GetXYZ());
            }


            private:

            cf::TypeSys::VarT<Vector3fT>& m_Value;  ///< The value that is interpolated.
            cf::math::QuaternionfT        m_A;      ///< The start value that we're interpolating from.
            cf::math::QuaternionfT        m_B;      ///< The target value that we're interpolating to.
            float                         m_Total;  ///< The total time that the interpolation is expected to take from A to B.
            float                         m_Time;   ///< The actual time that has passed since we started from A.
        };


#if 0
        /// Linearly interpolates a value over a period of time.
        /// The course of the interpolation is adapted/updated whenever a new reference value is set.
        /// This is mostly intended for game entities on clients, where we interpolate from the "current" value
        /// towards the reference value of the most recent server update. If the subsequent server update happens
        /// to arrive later than the one before it, our interpolation may (briefly) turn into an extrapolation
        /// for bridging the gap.
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

            /// Advances the interpolation over the given time.
            void Interpolate(float Time)
            {
                m_Value   += m_Gradient*Time;
                m_ExtTime += Time;

                m_LastValue = m_Value;
            }


            private:

            bool CanContinue(const T& DeltaY) const
            {
                return length(DeltaY) < 5000.0f;
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
        /// Unfortunately, extrapolation is less useful and worthwhile than it initially seems, because
        /// when it is applied to entity origins, all kinds of unwanted side effects can happen: items
        /// that fall to the floor penetrate the ground plane, then resurface; the wings of closing doors
        /// bump into each other, then retract; lifts overshoot their halting position; grenades that are
        /// deflected by walls briefly vanish into them; etc.
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

            /// Advances the extrapolation over the given time.
            void Interpolate(float Time)
            {
                m_Value   += m_Gradient*Time;
                m_ExtTime += Time;

                m_LastValue = m_Value;
            }


            private:

            bool CanContinue(const T& DeltaY) const
            {
                return length(DeltaY) < 5000.0f;
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


            T&     m_Value;     ///< The value that is extrapolated.
            T      m_LastValue; ///< The last extrapolated value. Normally m_LastValue == m_Value.
            T      m_Gradient;  ///< The change in the value over time.
            T      m_LastRef;   ///< The last reference value (*not* necessarily the value from which the extrapolation started).
            float  m_ExtTime;   ///< The time we've been extrapolating since the reference value was last set.
        };
#endif
    }
}

#endif
