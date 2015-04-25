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

#ifndef CAFU_CLIPSYS_TRACE_RESULT_HPP_INCLUDED
#define CAFU_CLIPSYS_TRACE_RESULT_HPP_INCLUDED

#include "Math3D/Vector3.hpp"


class MaterialT;


namespace cf
{
    namespace ClipSys
    {
        class ClipModelT;


        /// This class describes the result of tracing an object (a ray, a bounding-box,
        /// or a convex solid) through a collision model, a clip model, or a clip world.
        ///
        ///   - If `StartSolid` is `true`, the trace started in solid. In this case,
        ///     `Fraction` is accordingly set to 0.0.
        ///
        ///   - If `Fraction` is smaller than the value that the trace was started with,
        ///     something was hit along the trace. `Fraction`, `ImpactNormal` and `Material`
        ///     provide details about the point of impact.
        ///
        ///   - If `Fraction` did not change, the entire trace succeeded without hitting
        ///     anything.
        ///
        struct TraceResultT
        {
            /// The constructor.
            TraceResultT(double Fraction_ = 1.0) : Fraction(Fraction_), StartSolid(false), Material(NULL) { }

            double     Fraction;        ///< How much of the trace could be completed before a hit occurred (if any).
            bool       StartSolid;      ///< Did the trace start in a solid part of the collision or clip model?
            Vector3dT  ImpactNormal;    ///< On impact, this is the normal vector of the hit surface.
            MaterialT* Material;        ///< The material at the point of impact. Can be NULL when an edge (i.e. a bevel plane) was hit.
        };


        struct ContactsResultT
        {
            const static unsigned long MAX_CONTACTS = 16;

            /// The constructor.
            ContactsResultT() : NrOfRepContacts(0), NrOfAllContacts(0) { }

            TraceResultT  TraceResults[MAX_CONTACTS];
            ClipModelT*   ClipModels  [MAX_CONTACTS];
            unsigned long NrOfRepContacts;      ///< The number of contacts actually reported in the TraceResults and ClipModels pair of array (at most MAX_CONTACTS many).
            unsigned long NrOfAllContacts;      ///< The number of contacts that would have been reported if NrOfRepContacts was not capped by MAX_CONTACTS.
        };
    }
}

#endif
