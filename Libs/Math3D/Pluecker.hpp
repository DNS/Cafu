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

#ifndef _CF_MATH_PLUECKER_HPP_
#define _CF_MATH_PLUECKER_HPP_

#include "Vector3.hpp"
#include <cassert>


namespace cf
{
    namespace math
    {
        /// This class represents Pluecker coordinates.
        /// Good introductions about Pluecker coordinates are found in publications by Seth Teller
        /// (e.g. "Visibility Computations in Densely Occluded Polyhedral Environments" (Ph.D. dissertation, Berkeley, 1992)),
        /// at Wikipedia (http://en.wikipedia.org/wiki/Pl%C3%BCcker_co-ordinates), and many other internet sites.
        template<class T> class PlueckerT
        {
            public:

            /// The default constructor.
            PlueckerT()
            {
                p[0]=p[1]=p[2]=p[3]=p[4]=p[5]=0;
            }

            /// Constructor for creating a Pluecker coordinate from individual components.
            PlueckerT(const T p0, const T p1, const T p2, const T p3, const T p4, const T p5)
            {
                p[0]=p0;
                p[1]=p1;
                p[2]=p2;
                p[3]=p3;
                p[4]=p4;
                p[5]=p5;
            }

            /// Creates a Pluecker coordinate from the line (segment) that starts at point A and ends at point B.
            static PlueckerT CreateFromLine(const Vector3T<T>& A, const Vector3T<T>& B)
            {
                return PlueckerT(A.x*B.y - B.x*A.y,
                                 A.x*B.z - B.x*A.z,
                                 A.x     - B.x,
                                 A.y*B.z - B.y*A.z,
                                 A.z     - B.z,
                                 B.y     - A.y);
            }

            /// Creates a Pluecker coordinate from the ray that starts at (or "passes through") point A into direction Dir.
            static PlueckerT CreateFromRay(const Vector3T<T>& A, const Vector3T<T>& Dir)
            {
                return PlueckerT(A.x*Dir.y - Dir.x*A.y,
                                 A.x*Dir.z - Dir.x*A.z,
                                -Dir.x,
                                 A.y*Dir.z - Dir.y*A.z,
                                -Dir.z,
                                 Dir.y);
            }


            /// Returns the i-th component of this Pluecker coordinate.
            T& operator [] (unsigned long i) { assert(i<6); return p[i]; }

            /// Returns the i-th component of this Pluecker coordinate.
            const T& operator [] (unsigned long i) const { assert(i<6); return p[i]; }

            /// This operator computes the permuted inner product (as described in Tellers dissertation) of the two Pluecker coordinates.
            T operator * (const PlueckerT& Other) const
            {
                return p[0]*Other.p[4] + p[1]*Other.p[5] + p[2]*Other.p[3] + p[4]*Other.p[0] + p[5]*Other.p[1] + p[3]*Other.p[2];
            }


            // GetLine() const;
            // GetRay() const;
            // GetDir() const;


            /// The six components of the Pluecker coordinate.
            T p[6];
        };


        /// Typedef for an PlueckerT of floats.
        typedef PlueckerT<float> PlueckerfT;

        /// Typedef for an Pluecker of doubles.
        typedef PlueckerT<double> PlueckerdT;
    }
}

#endif
