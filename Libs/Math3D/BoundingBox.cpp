/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/********************/
/*** Bounding Box ***/
/********************/

#include "BoundingBox.hpp"


// This ctor is required for example in order to be able to put BoundingBox3Ts into ArrayTs etc.
template<class T> BoundingBox3T<T>::BoundingBox3T()
{
    const T MaxVal=std::numeric_limits<T>::max();

    Min=Vector3T<T>( MaxVal,  MaxVal,  MaxVal);
    Max=Vector3T<T>(-MaxVal, -MaxVal, -MaxVal);

    assert(!IsInited());
}


template<class T> BoundingBox3T<T>::BoundingBox3T(const Vector3T<T>& A)
    : Min(A),
      Max(A)
{
    assert(IsInited());
}


template<class T> BoundingBox3T<T>::BoundingBox3T(const Vector3T<T>& A, const Vector3T<T>& B)
    : Min(A),
      Max(A)
{
    Insert(B);

    assert(IsInited());
}


template<class T> BoundingBox3T<T>::BoundingBox3T(const ArrayT< Vector3T<T> >& A)
{
    const T MaxVal=std::numeric_limits<T>::max();

    Min=Vector3T<T>( MaxVal,  MaxVal,  MaxVal);
    Max=Vector3T<T>(-MaxVal, -MaxVal, -MaxVal);

    assert(!IsInited());

    for (unsigned long VertexNr=0; VertexNr<A.Size(); VertexNr++)
        Insert(A[VertexNr]);
}


template<class T> bool BoundingBox3T<T>::IsValid() const
{
    const T MaxVal=std::numeric_limits<T>::max();

    // Uninitialized bounding-boxes are valid.
    if (Min==Vector3T<T>( MaxVal,  MaxVal,  MaxVal) &&
        Max==Vector3T<T>(-MaxVal, -MaxVal, -MaxVal)) return true;

    // Properly initialized bounding-boxes must have a non-negative volume.
    return Min.x<=Max.x && Min.y<=Max.y && Min.z<=Max.z;
}


template<class T> bool BoundingBox3T<T>::IsInited() const
{
    assert(IsValid());

    return Min.x<=Max.x;
}


template<class T> void BoundingBox3T<T>::Insert(const Vector3T<T>& A)
{
    if (A.x<Min.x) Min.x=A.x;
    if (A.y<Min.y) Min.y=A.y;
    if (A.z<Min.z) Min.z=A.z;

    if (A.x>Max.x) Max.x=A.x;
    if (A.y>Max.y) Max.y=A.y;
    if (A.z>Max.z) Max.z=A.z;

    assert(IsInited());
}


template<class T> void BoundingBox3T<T>::Insert(const ArrayT< Vector3T<T> >& A)
{
    for (unsigned long VertexNr=0; VertexNr<A.Size(); VertexNr++)
        Insert(A[VertexNr]);
}


template<class T> void BoundingBox3T<T>::Insert(const BoundingBox3T<T>& BB)
{
    assert(BB.IsInited());

    Insert(BB.Min);
    Insert(BB.Max);

    assert(IsInited());
}


template<class T> void BoundingBox3T<T>::InsertValid(const BoundingBox3T<T>& BB)
{
    if (!BB.IsInited()) return;

    Insert(BB.Min);
    Insert(BB.Max);

    assert(IsInited());
}


template<class T> BoundingBox3T<T> BoundingBox3T<T>::GetOverallTranslationBox(const Vector3T<T>& Start, const Vector3T<T>& End) const
{
    assert(IsInited());

    BoundingBox3T<T> ResultBB(*this);

    // Move the ResultBB to the Start point.
    ResultBB.Min+=Start;
    ResultBB.Max+=Start;

    // Insert the corresponding points near End.
    ResultBB.Insert(Min+End);
    ResultBB.Insert(Max+End);

    return ResultBB;
}


template<class T> bool BoundingBox3T<T>::Contains(const Vector3T<T>& A) const
{
    assert(IsInited());

    return A.x>Min.x && A.x<Max.x &&
           A.y>Min.y && A.y<Max.y &&
           A.z>Min.z && A.z<Max.z;
}


template<class T> bool BoundingBox3T<T>::Intersects(const BoundingBox3T<T>& BB) const
{
    assert(IsInited());
    assert(BB.IsInited());

    // This expression can easily be considered and verified in 2D on a sheet of paper!
    return Min.x<BB.Max.x && Max.x>BB.Min.x &&
           Min.y<BB.Max.y && Max.y>BB.Min.y &&
           Min.z<BB.Max.z && Max.z>BB.Min.z;
}


template<class T> bool BoundingBox3T<T>::IntersectsOrTouches(const BoundingBox3T<T>& BB) const
{
    assert(IsInited());
    assert(BB.IsInited());

    // This expression can easily be considered and verified in 2D on a sheet of paper!
    return Min.x<=BB.Max.x && Max.x>=BB.Min.x &&
           Min.y<=BB.Max.y && Max.y>=BB.Min.y &&
           Min.z<=BB.Max.z && Max.z>=BB.Min.z;
}


template<class T> typename BoundingBox3T<T>::SideT BoundingBox3T<T>::WhatSide(const Plane3T<T>& P, const T Epsilon) const
{
    assert(IsInited());

    Vector3T<T> VertNear;
    Vector3T<T> VertFar;

    // We can determine the vertex that is nearest and the vertex that is farthest
    // from P by evaluating the sign of the components of the normal vector of P.
    if (P.Normal.x>0)
    {
        VertNear.x=Min.x;
        VertFar.x =Max.x;
    }
    else
    {
        VertNear.x=Max.x;
        VertFar.x =Min.x;
    }

    if (P.Normal.y>0)
    {
        VertNear.y=Min.y;
        VertFar.y =Max.y;
    }
    else
    {
        VertNear.y=Max.y;
        VertFar.y =Min.y;
    }

    if (P.Normal.z>0)
    {
        VertNear.z=Min.z;
        VertFar.z =Max.z;
    }
    else
    {
        VertNear.z=Max.z;
        VertFar.z =Min.z;
    }

    if (P.GetDistance(VertNear)>= Epsilon) return Front;
    if (P.GetDistance(VertFar )<=-Epsilon) return Back;

    return Both;
}


template<class T> typename BoundingBox3T<T>::SideT BoundingBox3T<T>::WhatSide_OLD(const Plane3T<T>& P, const T Epsilon) const
{
    assert(IsInited());

#if 1
    const Vector3T<T> Center=(Min+Max)*0.5f;
    const T           DistC =P.GetDistance(Center);
    const T           Dist2 =fabs((Max.x-Center.x)*P.Normal.x) +
                             fabs((Max.y-Center.y)*P.Normal.y) +
                             fabs((Max.z-Center.z)*P.Normal.z);

    // Dist2 bedeutet:
    // "Wie weit muß ich eine Ebene, die den Normalenvektor P.Normal hat und Center enthält, parallel verschieben
    //  (entlang ihres Normalvektors), um zu der in dieser Richtung liegenden äußersten Ecke der BB zu gelangen?"
    if (DistC-Dist2>= Epsilon) return Front;
    if (DistC+Dist2<=-Epsilon) return Back;

    return Both;
#else
    Vector3T<T> VertBB[8]={ Min,                              Vector3T<T>(Min.x, Min.y, Max.z),
                            Vector3T<T>(Min.x, Max.y, Min.z), Vector3T<T>(Min.x, Max.y, Max.z),
                            Vector3T<T>(Max.x, Min.y, Min.z), Vector3T<T>(Max.x, Min.y, Max.z),
                            Vector3T<T>(Max.x, Max.y, Min.z), Max };

    int VertFront=0;
    int VertBack =0;

    for (unsigned long Nr=0; Nr<8; Nr++)
    {
        if (P.GetDistance(VertBB[Nr])>0.0) VertFront= 1;
                                      else VertBack =-1;
    }

    return SideT(VertFront+VertBack);
#endif
}


template<class T> T BoundingBox3T<T>::GetDistance(const Plane3T<T>& P) const
{
    assert(IsInited());

#if 1
    Vector3T<T> VertNear;
    Vector3T<T> VertFar;

    // We can determine the vertex that is nearest and the vertex that is farthest
    // from P by evaluating the sign of the components of the normal vector of P.
    if (P.Normal.x>0)
    {
        VertNear.x=Min.x;
        VertFar.x =Max.x;
    }
    else
    {
        VertNear.x=Max.x;
        VertFar.x =Min.x;
    }

    if (P.Normal.y>0)
    {
        VertNear.y=Min.y;
        VertFar.y =Max.y;
    }
    else
    {
        VertNear.y=Max.y;
        VertFar.y =Min.y;
    }

    if (P.Normal.z>0)
    {
        VertNear.z=Min.z;
        VertFar.z =Max.z;
    }
    else
    {
        VertNear.z=Max.z;
        VertFar.z =Min.z;
    }

    T Dist;
    Dist=P.GetDistance(VertNear); if (Dist>0) return Dist;
    Dist=P.GetDistance(VertFar ); if (Dist<0) return Dist;

    return 0;
#else
    const Vector3T<T> Center=(Min+Max)*0.5f;
    const T           DistC =P.GetDistance(Center);
    const T           Dist2 =fabs((Max.x-Center.x)*P.Normal.x) +
                             fabs((Max.y-Center.y)*P.Normal.y) +
                             fabs((Max.z-Center.z)*P.Normal.z);

    // Dist2 bedeutet:
    // "Wie weit muß ich eine Ebene, die den Normalenvektor P.Normal hat und Center enthält, parallel verschieben
    //  (entlang ihres Normalvektors), um zu der in dieser Richtung liegenden äußersten Ecke der BB zu gelangen?"
    if (DistC-Dist2>0) return DistC-Dist2;
    if (DistC+Dist2<0) return DistC+Dist2;

    return 0;
#endif
}


#if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable: 4723)    // potential divide by 0
#endif

// Algorithm derived from Amy Williams et al., "An Efficient and Robust Ray-Box Intersection Algorithm", http://www.cs.utah.edu/~awilliam/box/.
template<class T> bool BoundingBox3T<T>::TraceRay(const Vector3T<T>& RayOrigin, const Vector3T<T>& RayDir, T& Fraction) const
{
    assert(IsInited());

    // The RayDir components can be 0, potentially causing a division by 0.
    // This is perfectly intentional here as described in the above paper.
    const Vector3T<T> OneDivRayDir(1/RayDir.x, 1/RayDir.y, 1/RayDir.z);
    const bool        RayDirSigns[3]={ RayDir.x<0, RayDir.y<0, RayDir.z<0 };

          T tmin =((RayDirSigns[0] ? Max : Min).x - RayOrigin.x) * OneDivRayDir.x;
          T tmax =((RayDirSigns[0] ? Min : Max).x - RayOrigin.x) * OneDivRayDir.x;
    const T tymin=((RayDirSigns[1] ? Max : Min).y - RayOrigin.y) * OneDivRayDir.y;
    const T tymax=((RayDirSigns[1] ? Min : Max).y - RayOrigin.y) * OneDivRayDir.y;

    if ((tmin>tymax) || (tymin>tmax)) return false;
    if (tymin>tmin) tmin=tymin;
    if (tymax<tmax) tmax=tymax;

    const T tzmin=((RayDirSigns[2] ? Max : Min).z - RayOrigin.z) * OneDivRayDir.z;
    const T tzmax=((RayDirSigns[2] ? Min : Max).z - RayOrigin.z) * OneDivRayDir.z;

    if ((tmin>tzmax) || (tzmin>tmax)) return false;
    if (tzmin>tmin) tmin=tzmin;
    if (tzmax<tmax) tmax=tzmax;

    if (tmin<0) return false;   // Ray starts in or already "out of" bounding box.
    Fraction=tmin;
    return true;
}

#if defined(_MSC_VER)
    #pragma warning(pop)
#endif


template<class T> void BoundingBox3T<T>::GetCornerVertices(Vector3T<T>* Vertices) const
{
    assert(IsInited());

    Vertices[0]=Min;
    Vertices[1]=Vector3T<T>(Min.x, Min.y, Max.z);
    Vertices[2]=Vector3T<T>(Min.x, Max.y, Min.z);
    Vertices[3]=Vector3T<T>(Min.x, Max.y, Max.z);
    Vertices[4]=Vector3T<T>(Max.x, Min.y, Min.z);
    Vertices[5]=Vector3T<T>(Max.x, Min.y, Max.z);
    Vertices[6]=Vector3T<T>(Max.x, Max.y, Min.z);
    Vertices[7]=Max;
}


template<class T> ArrayT< BoundingBox3T<T> > BoundingBox3T<T>::GetSplits(const Plane3T<T>& SplitPlane, const T PlaneThickness) const
{
    assert(IsInited());

    Vector3T<T> VertBB[8]={ Min,                              Vector3T<T>(Min.x, Min.y, Max.z),
                            Vector3T<T>(Min.x, Max.y, Min.z), Vector3T<T>(Min.x, Max.y, Max.z),
                            Vector3T<T>(Max.x, Min.y, Min.z), Vector3T<T>(Max.x, Min.y, Max.z),
                            Vector3T<T>(Max.x, Max.y, Min.z), Max };

    signed char           Side[8];
    ArrayT< Vector3T<T> > VertFront;
    ArrayT< Vector3T<T> > VertBack;
    ArrayT< Vector3T<T> > VertOn;

    const char Edge[12][2]={ {0,1}, {0,2}, {0,4}, {7,3}, {7,5}, {7,6}, {1,3}, {1,5}, {6,2}, {6,4}, {4,5}, {2,3} };

    for (unsigned long Nr=0; Nr<8; Nr++)
    {
        const double Dist=SplitPlane.GetDistance(VertBB[Nr]);

             if (Dist> PlaneThickness) { VertFront.PushBack(VertBB[Nr]); Side[Nr]= 1; }
        else if (Dist<-PlaneThickness) { VertBack .PushBack(VertBB[Nr]); Side[Nr]=-1; }
        else                           { VertOn   .PushBack(VertBB[Nr]); Side[Nr]= 0; }
    }

    for (unsigned long Nr=0; Nr<12; Nr++)
    {
        char v1=Edge[Nr][0];
        char v2=Edge[Nr][1];

        if (Side[v1] && Side[v1]==-Side[v2]) VertOn.PushBack(SplitPlane.GetIntersection(VertBB[v1], VertBB[v2], 0));
    }


    ArrayT< BoundingBox3T<T> > Result;

    Result.PushBackEmpty(2);

    if (VertFront.Size())
    {
        Result[0]=BoundingBox3T<T>(VertFront);
        Result[0].Insert(VertOn);
    }

    if (VertBack.Size())
    {
        Result[1]=BoundingBox3T<T>(VertBack);
        Result[1].Insert(VertOn);
    }

    return Result;
}


template class BoundingBox3T<float>;
template class BoundingBox3T<double>;
