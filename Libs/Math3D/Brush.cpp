/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Brush.hpp"


template<class T> Brush3T<T>::Brush3T(const BoundingBox3T<T>& BB, const Vector3T<T>& Pos)
{
    Planes.PushBack(Plane3T<T>(Vector3T<T>( 1,  0,  0),  BB.Max.x + Pos.x));
    Planes.PushBack(Plane3T<T>(Vector3T<T>(-1,  0,  0), -BB.Min.x - Pos.x));
    Planes.PushBack(Plane3T<T>(Vector3T<T>( 0,  1,  0),  BB.Max.y + Pos.y));
    Planes.PushBack(Plane3T<T>(Vector3T<T>( 0, -1,  0), -BB.Min.y - Pos.y));
    Planes.PushBack(Plane3T<T>(Vector3T<T>( 0,  0,  1),  BB.Max.z + Pos.z));
    Planes.PushBack(Plane3T<T>(Vector3T<T>( 0,  0, -1), -BB.Min.z - Pos.z));
}


template<class T> Brush3T<T>::Brush3T(const Vector3T<T>& A, const Vector3T<T>& B, const Vector3T<T>& C, const T Epsilon, bool IncludeBevelPlanes)
{
    Planes.PushBack(Plane3T<T>(A, B, C, Epsilon));
    Planes.PushBack(Planes[0].GetMirror());
    Planes.PushBack(Plane3T<T>(A, B, B + Planes[0].Normal, Epsilon));
    Planes.PushBack(Plane3T<T>(B, C, C + Planes[0].Normal, Epsilon));
    Planes.PushBack(Plane3T<T>(C, A, A + Planes[0].Normal, Epsilon));

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
    const T Epsilon = T(0.0625);

    // Bloat-Distance-Offsets für alle Planes dieses Brushs bestimmen.
    while (BloatDistanceOffsets.Size() < Planes.Size())
        BloatDistanceOffsets.PushBack(0.0);

    for (unsigned long PlaneNr = 0; PlaneNr < Planes.Size(); PlaneNr++)
    {
        const Plane3T<T>& P = Planes[PlaneNr];

        BloatDistanceOffsets[PlaneNr] = dot(P.Normal, Vector3T<T>(
            P.Normal.x < 0 ? BB.Max.x : BB.Min.x,
            P.Normal.y < 0 ? BB.Max.y : BB.Min.y,
            P.Normal.z < 0 ? BB.Max.z : BB.Min.z));
    }

    // Wenn 'Origin' im Inneren des (soliden) Brushs liegt, sitzen wir fest.
    unsigned long PlaneNr;
    const Plane3T<T>* StuckOnSurface = NULL;

    for (PlaneNr = 0; PlaneNr < Planes.Size(); PlaneNr++)
    {
        const T Dist = Planes[PlaneNr].GetDistance(Origin) + BloatDistanceOffsets[PlaneNr];

        if (Dist >= 0)
        {
            // Definitively not stuck inside.
            break;
        }

        if (Dist > -Epsilon)
        {
            // We are slightly inside the brush (probably due to rounding errors).
            const T Nenner = dot(Planes[PlaneNr].Normal, Dir);    // Computed exactly as below.

            if (Nenner >= 0)
            {
                // Slightly inside, but moving out. As moving out makes matters better,
                // not worse, let's consider this case as "not stuck inside" as well.
                // Note that we must be able to reproduce the same conclusion below, so here
                // and there, the computation of Nenner must be exactly the same!
                break;
            }
            else
            {
                // Slightly inside, and moving further in. As we *would* get out if Dir was
                // in another direction, flag this case explicitly, so that the result is a
                // Trace.Fraction of 0, but Trace.StartSolid is false.
                // Note that another plane might still clear us entirely.
                StuckOnSurface = &Planes[PlaneNr];
            }
        }
    }

    if (PlaneNr == Planes.Size())
    {
        Trace.StartSolid = (StuckOnSurface == NULL);
        Trace.Fraction   = 0;
        if (StuckOnSurface) Trace.ImpactNormal = StuckOnSurface->Normal;
        return;
    }

    for (PlaneNr = 0; PlaneNr < Planes.Size(); PlaneNr++)
    {
        // Bestimmen, bei welchem Bruchteil (Fraction F) von Dir wir die Plane schneiden.
        const T Nenner = dot(Planes[PlaneNr].Normal, Dir);    // Computed exactly as above.

        // Dir muß dem Normalenvektor der Ebene wirklich entgegenzeigen! Ist der Nenner==0, so ist Dir parallel zur Plane,
        // und mit dieser Plane ex. kein Schnittpunkt. Ist der Nenner>0, nutzen wir die Konvexität des Brushs aus:
        // Es gibt damit nur genau einen Schnittpunkt mit dem Brush (Eintrittsstelle) und die Plane behindert nicht
        // eine Bewegung von ihr weg (Feststecken wenn Dist==0 (s.u.)).
        if (Nenner >= 0) continue;

        const T Dist = Planes[PlaneNr].GetDistance(Origin) + BloatDistanceOffsets[PlaneNr];
        T       F    = -Dist / Nenner;

        // Der Schnitt macht nur Sinn, wenn F im gewünschten Intervall liegt
        if (F < 0 || F > Trace.Fraction) continue;

        // Prüfen, ob Schnittpunkt wirklich auf dem Brush liegt
        const Vector3T<T> HitPos = Origin + Dir * F;
        unsigned long PNr;

        for (PNr = 0; PNr < Planes.Size(); PNr++)
        {
            const T d = Planes[PNr].GetDistance(HitPos) + BloatDistanceOffsets[PNr];

            // The (PNr != PlaneNr) part is for anticipating rounding errors.
            // In the (d > 0) part, there is no need to use Epsilon in place of the 0,
            // because HitPos is supposed to be at 0 distance of Planes[PlaneNr] anyway.
            if (PNr != PlaneNr /*Rundungsfehlern zuvorkommen!*/ && d > 0)
                break;
        }

        if (PNr < Planes.Size()) continue;

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
        F = -(Dist - Epsilon) / Nenner;                 // Vgl. Berechnung von F oben!

        if (F < 0             ) F = 0;
        if (F > Trace.Fraction) F = Trace.Fraction;     // Pro forma.

        Trace.Fraction     = F;
        Trace.ImpactNormal = Planes[PlaneNr].Normal;
        break;
    }
}


template class Brush3T<float>;
template class Brush3T<double>;
