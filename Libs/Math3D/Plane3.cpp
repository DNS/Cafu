/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/*************/
/*** Plane ***/
/*************/

#include "Plane3.hpp"


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
