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

#ifndef _CF_CLIPSYS_TRACE_RESULT_HPP_
#define _CF_CLIPSYS_TRACE_RESULT_HPP_

#include "Math3D/Vector3.hpp"


class MaterialT;


namespace cf
{
    namespace ClipSys
    {
        class ClipModelT;


        /// This class describes the result of tracing an object (a ray or a bounding box) through a collision shape, model or world.
        struct TraceResultT
        {
            /// The constructor.
            TraceResultT(double Fraction_=1.0) : Fraction(Fraction_), StartSolid(false), Material(NULL) { }

            double     Fraction;        ///< Fraction/percentage of movement completed, 0.0 means that the trace begun already stuck in solid, 1.0 means that the entire movement was possible without hitting anything.
            bool       StartSolid;      ///< Did the movement start inside the object, "in solid"?
            Vector3dT  ImpactNormal;    ///< On impact, this is the normal vector of the hit surface.
            MaterialT* Material;        ///< The material at the point of impact. Can be NULL when an edge (i.e. a bevel plane) was hit.
        };


        struct ContactsResultT
        {
            const static unsigned long MAX_CONTACTS=16;

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
