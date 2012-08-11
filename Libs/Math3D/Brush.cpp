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
/*** Brush ***/
/*************/

#include "Brush.hpp"

#if defined(_WIN32) && defined(_MSC_VER)
    #if (_MSC_VER<1300)
        #define for if (false) ; else for
    #endif
#endif


template<class T> Brush3T<T>::Brush3T(const BoundingBox3T<T>& BB, const Vector3T<T>& Pos)
{
    Planes.PushBack(Plane3T<T>(Vector3T<T>( 1,  0,  0),  BB.Max.x+Pos.x));
    Planes.PushBack(Plane3T<T>(Vector3T<T>(-1,  0,  0), -BB.Min.x-Pos.x));
    Planes.PushBack(Plane3T<T>(Vector3T<T>( 0,  1,  0),  BB.Max.y+Pos.y));
    Planes.PushBack(Plane3T<T>(Vector3T<T>( 0, -1,  0), -BB.Min.y-Pos.y));
    Planes.PushBack(Plane3T<T>(Vector3T<T>( 0,  0,  1),  BB.Max.z+Pos.z));
    Planes.PushBack(Plane3T<T>(Vector3T<T>( 0,  0, -1), -BB.Min.z-Pos.z));
}


template<class T> Brush3T<T>::Brush3T(const Vector3T<T>& A, const Vector3T<T>& B, const Vector3T<T>& C, const T Epsilon, bool IncludeBevelPlanes)
{
    Planes.PushBack(Plane3T<T>(A, B, C, Epsilon));
    Planes.PushBack(Planes[0].GetMirror());
    Planes.PushBack(Plane3T<T>(A, B, B+Planes[0].Normal, Epsilon));
    Planes.PushBack(Plane3T<T>(B, C, C+Planes[0].Normal, Epsilon));
    Planes.PushBack(Plane3T<T>(C, A, A+Planes[0].Normal, Epsilon));

    if (!IncludeBevelPlanes) return;

    BoundingBox3T<T> BB(A, B);
    BB.Insert(C);

    Planes.PushBack(Plane3T<T>(Vector3T<T>(-1.0,  0.0,  0.0), -BB.Min.x));     // Left   plane.
    Planes.PushBack(Plane3T<T>(Vector3T<T>( 1.0,  0.0,  0.0),  BB.Max.x));     // Right  plane.
    Planes.PushBack(Plane3T<T>(Vector3T<T>( 0.0, -1.0,  0.0), -BB.Min.y));     // Near   plane.
    Planes.PushBack(Plane3T<T>(Vector3T<T>( 0.0,  1.0,  0.0),  BB.Max.y));     // Far    plane.
    Planes.PushBack(Plane3T<T>(Vector3T<T>( 0.0,  0.0, -1.0), -BB.Min.z));     // Bottom plane.
    Planes.PushBack(Plane3T<T>(Vector3T<T>( 0.0,  0.0,  1.0),  BB.Max.z));     // Top    plane.
}


template<class T> void Brush3T<T>::TraceBoundingBox(const BoundingBox3T<T>& BB, const Vector3T<T>& Origin, const Vector3T<T>& Dir, VB_Trace3T<T>& Trace) const
{
    static ArrayT<T> BloatDistanceOffsets;

    // Bloat-Distance-Offsets für alle Planes dieses Brushs bestimmen.
    while (BloatDistanceOffsets.Size()<Planes.Size()) BloatDistanceOffsets.PushBack(0.0);

    for (unsigned long PlaneNr=0; PlaneNr<Planes.Size(); PlaneNr++)
    {
        const Plane3T<T>& P=Planes[PlaneNr];

        BloatDistanceOffsets[PlaneNr]=dot(P.Normal, Vector3T<T>(P.Normal.x<0 ? BB.Max.x : BB.Min.x,
                                                                P.Normal.y<0 ? BB.Max.y : BB.Min.y,
                                                                P.Normal.z<0 ? BB.Max.z : BB.Min.z));
    }

    // Wenn 'Origin' im Inneren des (soliden) Brushs liegt, sitzen wir fest.
    unsigned long PlaneNr;

    for (PlaneNr=0; PlaneNr<Planes.Size(); PlaneNr++)
        if (Planes[PlaneNr].GetDistance(Origin)+BloatDistanceOffsets[PlaneNr]>=0) break;

    if (PlaneNr==Planes.Size())
    {
        Trace.StartSolid=true;
        Trace.Fraction  =0;
        return;
    }

    for (PlaneNr=0; PlaneNr<Planes.Size(); PlaneNr++)
    {
        // Bestimmen, bei welchem Bruchteil (Fraction F) von Dir wir die Plane schneiden.
        T Nenner=dot(Planes[PlaneNr].Normal, Dir);

        // Dir muß dem Normalenvektor der Ebene wirklich entgegenzeigen! Ist der Nenner==0, so ist Dir parallel zur Plane,
        // und mit dieser Plane ex. kein Schnittpunkt. Ist der Nenner>0, nutzen wir die Konvexität des Brushs aus:
        // Es gibt damit nur genau einen Schnittpunkt mit dem Brush (Eintrittsstelle) und die Plane behindert nicht
        // eine Bewegung von ihr weg (Feststecken wenn Dist==0 (s.u.)).
        if (Nenner>=0) continue;

        T Dist= Planes[PlaneNr].GetDistance(Origin)+BloatDistanceOffsets[PlaneNr];
        T F   =-Dist/Nenner;

        // Der Schnitt macht nur Sinn, wenn F im gewünschten Intervall liegt
        if (F<0 || F>Trace.Fraction) continue;

        // Prüfen, ob Schnittpunkt wirklich auf dem Brush liegt
        Vector3T<T> HitPos=Origin + Dir*F;
        unsigned long PNr;

        for (PNr=0; PNr<Planes.Size(); PNr++)
            if (PNr!=PlaneNr /*Rundungsfehlern zuvorkommen!*/ && Planes[PNr].GetDistance(HitPos)+BloatDistanceOffsets[PNr]>0.01) break;

        if (PNr<Planes.Size()) continue;

        // Wir haben die einzige Eintrittsstelle gefunden!
        // Eigentlich ist das errechete F einwandfrei. Wir wollen es jedoch nochmal etwas verringern, sodaß der sich
        // ergebende Schnittpunkt (HitPos) in einem Abstand von 0.03125 ÜBER der Ebene liegt! Bildhaft wird dazu die
        // Schnittebene um 0.03125 in Richtung ihres Normalenvektors geschoben und F wie oben neu errechnet.
        // Dies erspart uns ansonsten ernste Probleme:
        // - Wird diese Funktion nochmals mit dem Trace-Ergebnis (HitPos) als Origin-Vektor aufgerufen,
        //   kann dieser neue Origin-Vektor nicht wegen Rundungsfehlern in den Brush geraten (Solid!).
        // - Wird unser Trace-Ergebnis (HitPos) als Origin-Vektor dieser Funktion, aber mit einem anderen
        //   Brush benutzt, der gegenüber diesem Brush nur in der Schnittebene verschoben ist (z.B. eine lange
        //   Wand, die aus mehreren "Backsteinen" besteht), kommt es auch hier nicht zum (falschen)
        //   "Hängenbleiben" an einer gemeinsamen Brush-Kante.
        // Aber: Die HitPos kann natürlich trotzdem näher als 0.03125 an der Ebene liegen, nämlich genau dann, wenn
        // es nicht zu einem Schnitt kam und Dir zufällig dort endet. Wir ignorieren diese Möglichkeit: Kommt es doch
        // noch zu einem Schnitt, wird eben F==0. Deshalb können wir uns auch in diesem Fall nicht durch Rundungsfehler
        // ins Innere des Brushs schaukeln.
        F=-(Dist-(T)0.03125)/Nenner;                   // Vgl. Berechnung von F oben!

        if (F<0             ) F=0;
        if (F>Trace.Fraction) F=Trace.Fraction;     // pro forma

        Trace.Fraction    =F;
        Trace.ImpactNormal=Planes[PlaneNr].Normal;
        break;
    }
}


template class Brush3T<float>;
template class Brush3T<double>;
