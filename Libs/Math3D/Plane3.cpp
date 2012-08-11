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

/*************/
/*** Plane ***/
/*************/

#include "Plane3.hpp"

#if defined(_WIN32) && defined(_MSC_VER)
    #if (_MSC_VER<1300)
        #define for if (false) ; else for
    #endif
#endif


template<class T> void Plane3T<T>::ConvexHull(const ArrayT< Vector3T<T> >& Points, ArrayT<Plane3T>& HullPlanes, ArrayT<unsigned long>* HullPlanesPIs, const T Epsilon)
{
    HullPlanes.Overwrite();
    if (HullPlanesPIs) HullPlanesPIs->Overwrite();

    // Bilde die konvexe Hülle über die Points, um neue Ebenen zu erhalten.
    // Uhhh, this is essentially in O(n^4)...
    for (unsigned long P1Nr=0; P1Nr+2<Points.Size(); P1Nr++)
        for (unsigned long P2Nr=P1Nr+1; P2Nr+1<Points.Size(); P2Nr++)
            for (unsigned long P3Nr=P2Nr+1; P3Nr<Points.Size(); P3Nr++)
            {
                // Ebenen sollen nicht doppelt eingefügt werden.
                // Prüfe daher zuerst, ob es bereits eine Ebene gibt, die die drei Vertices enthält.
                unsigned long PlaneNr;

                for (PlaneNr=0; PlaneNr<HullPlanes.Size(); PlaneNr++)
                    if (fabs(HullPlanes[PlaneNr].GetDistance(Points[P1Nr]))<Epsilon &&
                        fabs(HullPlanes[PlaneNr].GetDistance(Points[P2Nr]))<Epsilon &&
                        fabs(HullPlanes[PlaneNr].GetDistance(Points[P3Nr]))<Epsilon) break;

                if (PlaneNr<HullPlanes.Size()) continue;

                try
                {
                    // An epsilon of 0 should be okay here... the exception is caught below.
                    const Plane3T<T> HullPlane(Points[P1Nr], Points[P2Nr], Points[P3Nr], 0.0);

                    bool HavePointsAbove=false;
                    bool HavePointsBelow=false;

                    for (unsigned long P4Nr=0; P4Nr<Points.Size(); P4Nr++)
                    {
                        const T Dist=HullPlane.GetDistance(Points[P4Nr]);

                        if (Dist> Epsilon) HavePointsAbove=true;
                        if (Dist<-Epsilon) HavePointsBelow=true;
                    }

                    if (HavePointsAbove && HavePointsBelow) continue;   // Not a hull plane.

                    if (!HavePointsAbove)
                    {
                        HullPlanes.PushBack(HullPlane);

                        if (HullPlanesPIs)
                        {
                            HullPlanesPIs->PushBack(P1Nr);
                            HullPlanesPIs->PushBack(P2Nr);
                            HullPlanesPIs->PushBack(P3Nr);
                        }
                    }

                    if (!HavePointsBelow)
                    {
                        HullPlanes.PushBack(HullPlane.GetMirror());

                        if (HullPlanesPIs)
                        {
                            HullPlanesPIs->PushBack(P1Nr);
                            HullPlanesPIs->PushBack(P3Nr);  // Swap the last two points for mirrored plane.
                            HullPlanesPIs->PushBack(P2Nr);
                        }
                    }
                }
                catch (const DivisionByZeroE&) { continue; }
            }
}


template class Plane3T<float>;
template class Plane3T<double>;
