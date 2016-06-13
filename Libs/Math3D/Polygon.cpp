/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/***************/
/*** Polygon ***/
/***************/

#include "Polygon.hpp"


template<class T> bool Polygon3T<T>::IsValid(const double RoundEpsilon, const double MinVertexDist) const
{
    // Punkt 4: Es muß mindestens 3 Vertices geben.
    if (Vertices.Size()<3) return false;


    // Punkt 4: Die Plane muß halbwegs wohldefiniert sein.
    if (!Plane.IsValid()) return false;


    // Ohne expliziten Punkt: Alle Vertices müssen *in* der Ebene liegen.
    for (unsigned long VertexNr=0; VertexNr<Vertices.Size(); VertexNr++)
    {
        const double Dist=Plane.GetDistance(Vertices[VertexNr]);

        if (Dist> RoundEpsilon) return false;
        if (Dist<-RoundEpsilon) return false;
    }


    // Punkt 5: No two vertices must be coincident.
    // Normally I'd only run the check for all (V[i], V[i+1]) pairs, but even with the additional check for
    // convexity below, that would not catch rhombuses (Rauten) whose top and bottom vertices are near identical,
    // that is, closer than MinVertexDist.
    for (unsigned long VertexNr=0; VertexNr+1<Vertices.Size(); VertexNr++)
        for (unsigned long VNr=VertexNr+1; VNr<Vertices.Size(); VNr++)
            if (length(Vertices[VertexNr]-Vertices[VNr])<MinVertexDist) return false;


    // Punkt 1 und Punkt 3: Stelle Konvexität und Orientierung sicher.
    for (unsigned long VertexNr=0; VertexNr<Vertices.Size(); VertexNr++)
    {
        // Intentionally don't cache any DivisionByZero exceptions here.
        // If one occurs, the situation is much worse than a "return false;" would express
        // (the EdgePlane ctor *must* succeed after all the assertions above).
        unsigned long NextVertexNr=VertexNr+1<Vertices.Size() ? VertexNr+1 : 0;
        Plane3T<T> EdgePlane(Vertices[VertexNr],
                             Vertices[NextVertexNr],
                             Vertices[VertexNr]-Plane.Normal, T(0.00001));   // (Assume that MinVertexDist is well larger than 1.)

        // Alle Vertices des Polys außer VertexNr und NextVertexNr müssen mindestens RoundEpsilon über der EdgePlane liegen!
        for (unsigned long VNr=0; VNr<Vertices.Size(); VNr++)
            if (VNr!=VertexNr && VNr!=NextVertexNr)
                if (EdgePlane.GetDistance(Vertices[VNr])<RoundEpsilon) return false;
    }


    // Punkt 2: Das Polygon befindet sich (bzgl. Orientierung) auf der *Vorderseite* der Ebene
    // Konstruiere dazu einen Vektor, der auf den Vektoren der Edges 0-1 und 1-2 senkrecht steht,
    // und projeziere diesen Vektor auf den Einheitsnormalenvektor der Poly.Plane.
    const Vector3T<T> N_=cross(Vertices[1]-Vertices[0], Vertices[2]-Vertices[1]);
    const T           l_=length(N_);

    // Wenn l_==0.0 tatsächlich vorkäme, wäre das ein schwerer Fehler!
    // l_ darf nämlich sehr klein sein, aber nach obigen Tests keinesfalls 0.
    if (l_==0.0) return false;

    const T Proj=-dot(scale(N_, T(1.0)/l_), Plane.Normal);

    if (Proj>1.0+RoundEpsilon) return false;
    if (Proj<1.0-RoundEpsilon) return false;


    // Alle Tests bestanden!
    return true;
}


template<class T> Polygon3T<T> Polygon3T<T>::GetMirror() const
{
    Polygon3T<T> Poly;

    Poly.Plane=Plane.GetMirror();

    for (unsigned long VertexNr=0; VertexNr<Vertices.Size(); VertexNr++)
        Poly.Vertices.PushBack(Vertices[Vertices.Size()-VertexNr-1]);

    return Poly;
}


template<class T> T Polygon3T<T>::GetArea() const
{
    if (Vertices.Size()<3) return 0;

    Vector3T<T> A;

    for (unsigned long VertexNr=0; VertexNr+1<Vertices.Size(); VertexNr++)
        A+=cross(Vertices[VertexNr], Vertices[VertexNr+1]);

    A+=cross(Vertices[Vertices.Size()-1], Vertices[0]);

    return fabs(dot(Plane.Normal, A)/T(2));
}


template<class T> bool Polygon3T<T>::HasVertex(const Vector3T<T>& A, const T Epsilon) const
{
    for (unsigned long VertexNr=0; VertexNr<Vertices.Size(); VertexNr++)
        if (Vertices[VertexNr].IsEqual(A, Epsilon)) return true;

    return false;
}


template<class T> typename Polygon3T<T>::SideT Polygon3T<T>::WhatSide(const Plane3T<T>& P, const T HalfPlaneThickness) const
{
    int SideFlags=Empty;


    for (unsigned long VertexNr=0; VertexNr<Vertices.Size(); VertexNr++)
    {
        const double Dist=P.GetDistance(Vertices[VertexNr]);

             if (Dist> HalfPlaneThickness) SideFlags|=Front;    // Setze Flag 2 - Es gibt Vertices über  der Ebene.
        else if (Dist<-HalfPlaneThickness) SideFlags|=Back;     // Setze Flag 1 - Es gibt Vertices unter der Ebene.
        else                               SideFlags|=On;       // Setze Flag 0 - Es gibt Vertices in    der Ebene.
    }


    if (SideFlags==On) return dot(Plane.Normal, P.Normal)>0 ? InIdentical : InMirrored;     // Das Polygon liegt in der Ebene.

    return (SideT)SideFlags;
}


template<class T> typename Polygon3T<T>::SideT Polygon3T<T>::WhatSideSimple(const Plane3T<T>& P, const T HalfPlaneThickness) const
{
    SideT Side=WhatSide(P, HalfPlaneThickness);

    switch (Side)
    {
        case FrontAndOn: return Front;
        case BackAndOn:  return Back;
        case BothAndOn:  return Both;
        default: break;
    }

    return Side;
}


template<class T> bool Polygon3T<T>::Overlaps(const Polygon3T<T>& OtherPoly, bool ReportTouchesAsOverlaps, const T EdgeThickness) const
{
    // See if this polygon and OtherPoly are in the same or mirrored plane.
    SideT Side=OtherPoly.WhatSide(this->Plane, EdgeThickness);
    if (Side!=InIdentical && Side!=InMirrored) return false;


    for (unsigned long VertexNr=0; VertexNr<this->Vertices.Size(); VertexNr++)
        switch (OtherPoly.WhatSide(this->GetEdgePlane(VertexNr, EdgeThickness), EdgeThickness))
        {
            case Back:      return false; break;
            case BackAndOn: if (!ReportTouchesAsOverlaps) return false; break;
            default:        break;
        }

    for (unsigned long VertexNr=0; VertexNr<OtherPoly.Vertices.Size(); VertexNr++)
        switch (this->WhatSide(OtherPoly.GetEdgePlane(VertexNr, EdgeThickness), EdgeThickness))
        {
            case Back:      return false; break;
            case BackAndOn: if (!ReportTouchesAsOverlaps) return false; break;
            default:        break;
        }

    return true;
}


template<class T> bool Polygon3T<T>::Encloses(const Polygon3T<T>& OtherPoly, bool MayTouchEdges, const T EdgeThickness) const
{
    // See if this polygon and OtherPoly are in the same or mirrored plane.
    SideT Side=OtherPoly.WhatSide(Plane, EdgeThickness);
    if (Side!=InIdentical && Side!=InMirrored) return false;


    for (unsigned long VertexNr=0; VertexNr<Vertices.Size(); VertexNr++)
        switch (OtherPoly.WhatSide(GetEdgePlane(VertexNr, EdgeThickness), EdgeThickness))
        {
            case Front:      break;
            case FrontAndOn: if (!MayTouchEdges) return false; break;
            default:         return false;
        }

    return true;
}


template<class T> ArrayT< Polygon3T<T> > Polygon3T<T>::GetSplits(const Plane3T<T>& SplitPlane, const T HalfPlaneThickness) const
{
    const unsigned long FRONT=0;
    const unsigned long BACK =1;

    ArrayT< Polygon3T<T> > Result;

    Result.PushBackEmpty(2);

    Result[FRONT].Plane=this->Plane;
    Result[BACK ].Plane=this->Plane;

    if (!Vertices.Size()) return Result;

    Vector3T<T> LastVertex=Vertices[Vertices.Size()-1];
    double      LastDist  =SplitPlane.GetDistance(LastVertex);

    for (unsigned long VertexNr=0; VertexNr<Vertices.Size(); VertexNr++)
    {
        const Vector3T<T>& ThisVertex=Vertices[VertexNr];
        const double       ThisDist  =SplitPlane.GetDistance(ThisVertex);

        if (ThisDist>HalfPlaneThickness)
        {
            if (LastDist<-HalfPlaneThickness)
            {
                Vector3T<T> Intersection=SplitPlane.GetIntersection(LastVertex, ThisVertex, 0);

                Result[BACK ].Vertices.PushBack(Intersection);
                Result[FRONT].Vertices.PushBack(Intersection);
            }
            else if (LastDist<=HalfPlaneThickness) Result[FRONT].Vertices.PushBack(LastVertex);

            Result[FRONT].Vertices.PushBack(ThisVertex);
        }
        else if (ThisDist<-HalfPlaneThickness)
        {
            if (LastDist>HalfPlaneThickness)
            {
                Vector3T<T> Intersection=SplitPlane.GetIntersection(LastVertex, ThisVertex, 0);

                Result[FRONT].Vertices.PushBack(Intersection);
                Result[BACK ].Vertices.PushBack(Intersection);
            }
            else if (LastDist>=-HalfPlaneThickness) Result[BACK].Vertices.PushBack(LastVertex);

            Result[BACK].Vertices.PushBack(ThisVertex);
        }
        else
        {
                 if (LastDist> HalfPlaneThickness) Result[FRONT].Vertices.PushBack(ThisVertex);
            else if (LastDist<-HalfPlaneThickness) Result[BACK ].Vertices.PushBack(ThisVertex);
        }

        LastVertex=ThisVertex;
        LastDist  =ThisDist;
    }

    return Result;
}


template<class T> void Polygon3T<T>::GetChoppedUpAlong(const Polygon3T<T>& SplittingPoly, const T EdgeThickness, ArrayT< Polygon3T<T> >& NewPolys) const
{
    Polygon3T<T> FragmentPoly=*this;

    NewPolys.Clear();

    for (unsigned long VertexNr=0; VertexNr<SplittingPoly.Vertices.Size(); VertexNr++)
    {
        const Plane3T<T> SplitPlane=SplittingPoly.GetEdgePlane(VertexNr, EdgeThickness);

        if (FragmentPoly.WhatSideSimple(SplitPlane, EdgeThickness)!=Both) continue;

        ArrayT< Polygon3T<T> > SplitResult=FragmentPoly.GetSplits(SplitPlane, EdgeThickness);

        FragmentPoly=SplitResult[0];
        NewPolys.PushBack(SplitResult[1]);
    }

    NewPolys.PushBack(FragmentPoly);
}


template<class T> void Polygon3T<T>::FillTJunctions(const Polygon3T<T>& Poly, const T EdgeThickness)
{
          Polygon3T& Poly1=*this;
    const Polygon3T& Poly2=Poly;

    // Betrachte jede Kante E1 von Poly1 und jede Kante E2 von Poly2.
    // Wir suchen nach einem Kanten-Paar (E1, E2), für das gilt:
    // 1. E2 liegt in der Geraden von E1.
    //    1a. Die Vertices von E2 liegen in Poly1.Plane.
    //    1b. Die Vertices von E2 liegen in PolygonEdgePlane(Poly1, Vertex1Nr).
    // 2. Mindestens einer der beiden Vertices von E2 liegt zwischen den Vertices von E1.
    for (unsigned long Vertex1Nr=0; Vertex1Nr<Poly1.Vertices.Size(); Vertex1Nr++)
    {
        unsigned long Vertex1NrNext=Vertex1Nr<Poly1.Vertices.Size()-1 ? Vertex1Nr+1 : 0;
        Plane3T<T>    Edge1Plane   =Poly1.GetEdgePlane(Vertex1Nr, EdgeThickness);

        for (unsigned long Vertex2Nr=0; Vertex2Nr<Poly2.Vertices.Size(); Vertex2Nr++)
        {
            unsigned long Vertex2NrNext=Vertex2Nr<Poly2.Vertices.Size()-1 ? Vertex2Nr+1 : 0;

            // Stelle sicher, daß E2 in der Geraden von E1 liegt (1a und 1b)
            if (fabs(Poly1.Plane.GetDistance(Poly2.Vertices[Vertex2Nr    ]))>EdgeThickness) continue;
            if (fabs(Poly1.Plane.GetDistance(Poly2.Vertices[Vertex2NrNext]))>EdgeThickness) continue;
            if (fabs(Edge1Plane .GetDistance(Poly2.Vertices[Vertex2Nr    ]))>EdgeThickness) continue;
            if (fabs(Edge1Plane .GetDistance(Poly2.Vertices[Vertex2NrNext]))>EdgeThickness) continue;

            double LengthE1=length(Poly1.Vertices[Vertex1NrNext]-Poly1.Vertices[Vertex1Nr]);

            // Sanity check
            if (LengthE1<=EdgeThickness) continue;

            // Vertex2NrNext muß immer rechts von Vertex2Nr liegen. Dies spielt vor allem eine Rolle, wenn sie BEIDE zwischen
            // den Vertices von E1 liegen. Wir könnten es explizit verifizieren, aber hier geht es auch sehr schön implizit:
            // Wenn Vertex2Nr schon zw. den Vertices von E1 liegt, wird E1 sowieso zur neuen Kante zwischen den Vertices
            // Vertex2Nr (wird in Poly1 eingefügt) und Vertex1Nr. Damit wird im zweiten Teil die Möglichkeit ausgeschlossen,
            // daß Vertex2NrNext zwischen den Vertices der ursprünglichen E1 und links von Vertex2Nr lag!
            // Mit diesem Hintergrund können wir viel leichter in allen 4 möglichen Fällen (Vertices von E2 zwischen oder
            // nicht zwischen den Vertices von E1) die Array-Operationen durchführen!

            // Liegt Poly2.Vertices[Vertex2Nr] zwischen den Vertices von E1?
            if (length(Poly2.Vertices[Vertex2Nr]-Poly1.Vertices[Vertex1Nr    ])<=LengthE1-EdgeThickness &&
                length(Poly2.Vertices[Vertex2Nr]-Poly1.Vertices[Vertex1NrNext])<=LengthE1-EdgeThickness)
            {
                // Füge Poly2.Vertices[Vertex2Nr] zwischen die Vertices von E1 ein.
                Poly1.Vertices.InsertAt(Vertex1Nr+1, Poly2.Vertices[Vertex2Nr]);

                Vertex1NrNext=Vertex1Nr+1;
                LengthE1=length(Poly1.Vertices[Vertex1NrNext]-Poly1.Vertices[Vertex1Nr]);
            }

            // Liegt Poly2.Vertices[Vertex2NrNext] zwischen den Vertices der neuen E1?
            if (length(Poly2.Vertices[Vertex2NrNext]-Poly1.Vertices[Vertex1Nr    ])<=LengthE1-EdgeThickness &&
                length(Poly2.Vertices[Vertex2NrNext]-Poly1.Vertices[Vertex1NrNext])<=LengthE1-EdgeThickness)
            {
                // Füge Poly2.Vertices[Vertex2NrNext] zwischen die Vertices der neuen E1 ein.
                Poly1.Vertices.InsertAt(Vertex1Nr+1, Poly2.Vertices[Vertex2NrNext]);

                Vertex1NrNext=Vertex1Nr+1;
                LengthE1=length(Poly1.Vertices[Vertex1NrNext]-Poly1.Vertices[Vertex1Nr]);
            }
        }
    }
}


// This is a static method.
template<class T> Polygon3T<T> Polygon3T<T>::Merge(const Polygon3T<T>& Poly1, const Polygon3T<T>& Poly2, const T EdgeThickness)
{
    if (Poly1.Vertices.Size()<3 || Poly2.Vertices.Size()<3 || Poly1.WhatSide(Poly2.Plane, EdgeThickness)!=InIdentical) throw InvalidOperationE();

    // Haben Poly1 und Poly2 eine gegenläufige Edge gemeinsam? Dazu für jedes Polygon eine "Edge-Schlange" aus
    // den Vertices AB, BC und CD bilden. Mit beiden Schlangen verschachtelt die Polygone ablaufen, dabei die
    // mittleren Edges (BC) betrachten und auf Gegenläufigkeit testen.
    unsigned long V1b=Poly1.Vertices.Size()-1; unsigned long V2b=Poly2.Vertices.Size()-1;   // V2 kann hier mitinitialisiert werden,
    unsigned long V1c=Poly1.Vertices.Size()-2; unsigned long V2c=Poly2.Vertices.Size()-2;   // da auch beim mehrfachen Durchlauf der inneren
    unsigned long V1d=Poly1.Vertices.Size()-3; unsigned long V2d=Poly2.Vertices.Size()-3;   // Schleife keine Neuinitialisierung notwendig ist!

    for (unsigned long V1a=0; V1a<Poly1.Vertices.Size(); V1a++)
    {
        for (unsigned long V2a=0; V2a<Poly2.Vertices.Size(); V2a++)
        {
            if (Poly1.Vertices[V1b].IsEqual(Poly2.Vertices[V2c], EdgeThickness) && Poly2.Vertices[V2b].IsEqual(Poly1.Vertices[V1c], EdgeThickness))
            {
                const double Dist1=Poly1.GetEdgePlane(V1b, EdgeThickness).GetDistance(Poly2.Vertices[V2d]);
                const double Dist2=Poly2.GetEdgePlane(V2b, EdgeThickness).GetDistance(Poly1.Vertices[V1d]);

                if (Dist1<-EdgeThickness || Dist2<-EdgeThickness) throw InvalidOperationE();

                // Endlich grünes Licht zum mergen!
                Polygon3T<T> PolyMerge=Poly1; PolyMerge.Vertices.Clear();

                for (unsigned long VNr=(Dist1>EdgeThickness) ? V1b : V1a; VNr!=V1c; VNr=(VNr+1) % Poly1.Vertices.Size()) PolyMerge.Vertices.PushBack(Poly1.Vertices[VNr]);
                for (unsigned long VNr=(Dist2>EdgeThickness) ? V2b : V2a; VNr!=V2c; VNr=(VNr+1) % Poly2.Vertices.Size()) PolyMerge.Vertices.PushBack(Poly2.Vertices[VNr]);

                return PolyMerge;
            }

            V2d=V2c;
            V2c=V2b;
            V2b=V2a;
        }

        V1d=V1c;
        V1c=V1b;
        V1b=V1a;
    }

    throw InvalidOperationE();
}


template<class T> void Polygon3T<T>::Complete(ArrayT< Polygon3T<T> >& Polys, const T HalfPlaneThickness, const Vector3T<T>& BoundingSphereCenter, const T BoundingSphereRadius)
{
#if 1
    for (unsigned long PolyNr=0; PolyNr<Polys.Size(); PolyNr++)
    {
        Polygon3T<T>& Poly=Polys[PolyNr];
        Vector3T<T>   Span1;
        Vector3T<T>   Span2;

        // Note that Span2 points "down", not "up" as we'd normally expect for an y-axis.
        Poly.Plane.GetSpanVectorsByRotation(Span1, Span2);

        const Vector3T<T> Origin   =BoundingSphereCenter-Poly.Plane.Normal*Poly.Plane.GetDistance(BoundingSphereCenter);
        const Vector3T<T> HugeSpan1=Span1*BoundingSphereRadius;
        const Vector3T<T> HugeSpan2=Span2*BoundingSphereRadius;

        Poly.Vertices.Clear();
        Poly.Vertices.PushBack(Origin-HugeSpan1+HugeSpan2);
        Poly.Vertices.PushBack(Origin-HugeSpan1-HugeSpan2);
        Poly.Vertices.PushBack(Origin+HugeSpan1-HugeSpan2);
        Poly.Vertices.PushBack(Origin+HugeSpan1+HugeSpan2);

        // The initial winding should never be invalid, but I've never tested this code with T=float.
        // (Even if the points were slightly off due to rounding error, would it matter here??)
        if (!Poly.IsValid(HalfPlaneThickness, 2.0*HalfPlaneThickness))
        {
            throw InvalidOperationE();

            // for (unsigned long VertexNr=0; VertexNr<Poly.Vertices.Size(); VertexNr++)
            //     Poly.Vertices[VertexNr]-=Poly.Plane.Normal*Poly.Plane.GetDistance(Poly.Vertices[VertexNr]);
        }

        for (unsigned long PlaneNr=0; PlaneNr<Polys.Size(); PlaneNr++)
        {
            if (PlaneNr==PolyNr) continue;

            const Plane3T<T>& Plane=Polys[PlaneNr].Plane;
            const SideT       Side =Poly.WhatSideSimple(Plane, HalfPlaneThickness);

            if (Side==Front) { Poly.Vertices.Clear(); break; }                          // Make Poly invalid.
            if (Side==Both ) { Poly=Poly.GetSplits(Plane, HalfPlaneThickness)[1]; }     // Split Poly and keep the backside.
            // There is nothing to do if Side is Back, InMirrored or InIdentical.

            // I'm not doing an IsValid() check here because in a few rare cases, calling Poly.GetSplits() yields a temporarily invalid
            // polygon, i.e. a polygon that has vertices that will be clipped in a later iteration but are closer than 2.0*HalfPlaneThickness.
            // For example, I've seen this with an initial huge winding in the XY plane whose four edges were not axis aligned, but rotated by
            // 45 degrees (because the spans happened to be this way, probably because the plane normal was not 100% exactly (0, 0, 1)).
            // Then, this winding is typically clipped against an axis-aligned plane that goes through two of its vertices.
            // However, with a minimal roundoff error present, the far away vertices are just not "in" the splitting plane, which yields
            // additional vertices that however violate the minimum vertex distance requirement.
            // if (!Poly.IsValid(HalfPlaneThickness, 2.0*HalfPlaneThickness)) break;
        }

        if (!Poly.IsValid(HalfPlaneThickness, 2.0*HalfPlaneThickness)) Poly.Vertices.Clear();
    }
#else
    for (unsigned long P1Nr=0; P1Nr<Polys.Size(); P1Nr++)
    {
        for (unsigned long P2Nr=0; P2Nr+1<Polys.Size(); P2Nr++)
            for (unsigned long P3Nr=P2Nr+1; P3Nr<Polys.Size(); P3Nr++)
            {
                try
                {
                    // Bilde den Schnittpunkt der Planes.
                    Vector3T<T> A=Plane3T<T>::GetIntersection(Polys[P1Nr].Plane, Polys[P2Nr].Plane, Polys[P3Nr].Plane);

                    // Ist A gültig bzw. liegt A im Brush?
                    unsigned long P4Nr;

                    for (P4Nr=0; P4Nr<Polys.Size(); P4Nr++)
                        if (Polys[P4Nr].Plane.GetDistance(A)>PlaneThickness) break;
                    if (P4Nr<Polys.Size()) continue;

                    // Liegt A auch wirklich in Polys[P1Nr].Plane?
                    // Dies ist bei extrem degenerierten, beinahe-parallelen Eingabe-Planes nach Rundungsfehlern denkbar!
                    if (Polys[P1Nr].Plane.GetDistance(A)<-PlaneThickness) continue;

                    // Wenn A nicht ohnehin schon im Poly vorkommt (Haus-des-Nikolaus-Effekt!), A ins Poly einfügen.
                    if (Polys[P1Nr].HasVertex(A, PlaneThickness)) continue;

                    // Aufgrund der Problemstellung (Ebenenschnitte) können kolineare Vertices normalerweise nicht
                    // vorkommen (mehr als 2 Vertices auf einer Geraden).

                    // A ist ein Punkt des Polygons.
                    Polys[P1Nr].Vertices.PushBack(A);
                }
                catch (const DivisionByZeroE&) { }
            }

        // Die Vertices im Uhrzeigersinn sortieren.
        for (unsigned long Vertex1Nr=0; Vertex1Nr+1<Polys[P1Nr].Vertices.Size(); Vertex1Nr++)
        {
            unsigned long Vertex2Nr;

            // Ausgehend vom Vertex1 denjenigen Vertex2 heraussuchen, mit dem sich eine Edge (im Uhrzeigersinn) bilden läßt.
            for (Vertex2Nr=Vertex1Nr+1; Vertex2Nr<Polys[P1Nr].Vertices.Size(); Vertex2Nr++)
            {
                const Vector3T<T>& N=Polys[P1Nr].Plane.Normal;
                const Vector3T<T>& A=Polys[P1Nr].Vertices[Vertex1Nr];
                const Vector3T<T>& B=Polys[P1Nr].Vertices[Vertex2Nr];
                const Vector3T<T>  C=A-N;   // Do *NOT* insert a '&' (reference) here - big bug!

                // According to their construction, A, B and C are guaranteed to be non-colinear, and so the plane constructor below should never fail.
                Plane3T<T> P(A, B, C, 0);

                unsigned long Vertex3Nr;

                for (Vertex3Nr=Vertex2Nr+1; Vertex3Nr<Polys[P1Nr].Vertices.Size(); Vertex3Nr++)
                    if (P.GetDistance(Polys[P1Nr].Vertices[Vertex3Nr])<-PlaneThickness) break;

                if (Vertex3Nr==Polys[P1Nr].Vertices.Size()) break;
            }

            // Wenn Vertex1 und Vertex2 schon in Folge vorliegen oder keine Edge gefunden werden
            // kann (sollte niemals vorkommen, schwerer Fehler!), einfach weitermachen.
            if (Vertex1Nr+1==Vertex2Nr || Vertex2Nr==Polys[P1Nr].Vertices.Size()) continue;

            // Andernfalls Vertex2 als Nachfolger für Vertex1 zutauschen,
            // damit es im nächsten Schleifendurchlauf damit weitergeht.
            Vector3T<T> Temp=Polys[P1Nr].Vertices[Vertex1Nr+1];
            Polys[P1Nr].Vertices[Vertex1Nr+1]=Polys[P1Nr].Vertices[Vertex2Nr];
            Polys[P1Nr].Vertices[Vertex2Nr]=Temp;
        }
    }
#endif
}


template class Polygon3T<float>;
template class Polygon3T<double>;
