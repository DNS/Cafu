/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "BezierPatch.hpp"


using namespace cf::math;


#if defined(_WIN32) && defined(_MSC_VER) && (_MSC_VER<1300)

template class cf::math::BezierPatchT<float>;       // VC++ 6 requires the cf::math:: here,
template class cf::math::BezierPatchT<double>;      // even though "using" is used above.


template<float> void BezierPatchT<float>::VertexT::Average(const VertexT& A, const VertexT& B)
{
    const float onehalf=0.5f;

    Coord   =(A.Coord   +B.Coord   )*onehalf;
    TexCoord=(A.TexCoord+B.TexCoord)*onehalf;
    Normal  =(A.Normal  +B.Normal  )*onehalf;
    TangentS=(A.TangentS+B.TangentS)*onehalf;
    TangentT=(A.TangentT+B.TangentT)*onehalf;
}


template<double> void BezierPatchT<double>::VertexT::Average(const VertexT& A, const VertexT& B)
{
    const double onehalf=0.5;

    Coord   =(A.Coord   +B.Coord   )*onehalf;
    TexCoord=(A.TexCoord+B.TexCoord)*onehalf;
    Normal  =(A.Normal  +B.Normal  )*onehalf;
    TangentS=(A.TangentS+B.TangentS)*onehalf;
    TangentT=(A.TangentT+B.TangentT)*onehalf;
}

#endif


/// Returns the unit vector of A if the length of A is greater than Epsilon, the null vector otherwise.
template<class T> static Vector3T<T> myNormalize(const Vector3T<T>& A, const T Epsilon)
{
    const T Length=length(A);

    return (Length>Epsilon) ? A/Length : Vector3T<T>();
}


template<class T> void BezierPatchT<T>::VertexT::Average(const VertexT& A, const VertexT& B)
{
    const T onehalf=T(0.5);

    Coord   =(A.Coord   +B.Coord   )*onehalf;
    TexCoord=(A.TexCoord+B.TexCoord)*onehalf;
    Normal  =(A.Normal  +B.Normal  )*onehalf;
    TangentS=(A.TangentS+B.TangentS)*onehalf;
    TangentT=(A.TangentT+B.TangentT)*onehalf;
}


template<class T> BezierPatchT<T>::BezierPatchT()
    : Width(0),
      Height(0)
{
}


template<class T> BezierPatchT<T>::BezierPatchT(unsigned long Width_, unsigned long Height_, const ArrayT< Vector3T<T> >& Coords_)
    : Width(Width_),
      Height(Height_)
{
    Mesh.PushBackEmpty(Width*Height);

    for (unsigned long VertexNr=0; VertexNr<Mesh.Size(); VertexNr++)
    {
        Mesh[VertexNr].Coord=Coords_[VertexNr];
    }
}


template<class T> BezierPatchT<T>::BezierPatchT(unsigned long Width_, unsigned long Height_, const ArrayT< Vector3T<T> >& Coords_, const ArrayT< Vector3T<T> >& TexCoords_)
    : Width(Width_),
      Height(Height_)
{
    Mesh.PushBackEmpty(Width*Height);

    for (unsigned long VertexNr=0; VertexNr<Mesh.Size(); VertexNr++)
    {
        Mesh[VertexNr].Coord   =Coords_   [VertexNr];
        Mesh[VertexNr].TexCoord=TexCoords_[VertexNr];
    }
}


template<class T> void BezierPatchT<T>::ComputeTangentSpace()
{
    assert(Width >=3);
    assert(Height>=3);
    assert((Width  % 2)==1);
    assert((Height % 2)==1);

    // Make sure that we start with 0-vectors everywhere.
    for (unsigned long VertexNr=0; VertexNr<Mesh.Size(); VertexNr++)
    {
        Mesh[VertexNr].Normal  =Vector3T<T>();
        Mesh[VertexNr].TangentS=Vector3T<T>();
        Mesh[VertexNr].TangentT=Vector3T<T>();
    }

    // Loop over all 3x3 sub-patches.
    // Note that whereever two 3x3 sub-patches share common vertices, we have to average their tangent-space axes!
    for (unsigned long sp_j=0; sp_j+2<Height; sp_j+=2)
        for (unsigned long sp_i=0; sp_i+2<Width; sp_i+=2)
        {
            Vector3T<T> AverageOfGood[3];
            bool        IsGood[3][3];

            for (unsigned long j=0; j<3; j++)
                for (unsigned long i=0; i<3; i++)
                {
                    VertexT&    v=GetVertex(sp_i+i, sp_j+j);
                    Vector3T<T> Axes[3];

                    IsGood[i][j]=ComputeTangentSpaceInSubPatch(sp_i, sp_j, T(i)/2, T(j)/2, Axes);
                    if (!IsGood[i][j]) continue;

                    v.Normal  +=Axes[0];
                    v.TangentS+=Axes[1];
                    v.TangentT+=Axes[2];

                    AverageOfGood[0]+=Axes[0];
                    AverageOfGood[1]+=Axes[1];
                    AverageOfGood[2]+=Axes[2];
                }

            AverageOfGood[0]=myNormalize(AverageOfGood[0], T(0.0));
            AverageOfGood[1]=myNormalize(AverageOfGood[1], T(0.0));
            AverageOfGood[2]=myNormalize(AverageOfGood[2], T(0.0));

            // Use the average of the non-degenerate axes whereever the axes were degenerate.
            for (unsigned long j=0; j<3; j++)
                for (unsigned long i=0; i<3; i++)
                    if (!IsGood[i][j])
                    {
                        VertexT& v=GetVertex(sp_i+i, sp_j+j);

                        v.Normal  +=AverageOfGood[0];
                        v.TangentS+=AverageOfGood[1];
                        v.TangentT+=AverageOfGood[2];
                    }
        }

    if (WrapsHorz())
    {
        Vector3T<T> Temp;

        for (unsigned long j=0; j<Height; j++)
        {
            Temp=GetVertex(0, j).Normal  +GetVertex(Width-1, j).Normal;   GetVertex(0, j).Normal  =Temp; GetVertex(Width-1, j).Normal  =Temp;
            Temp=GetVertex(0, j).TangentS+GetVertex(Width-1, j).TangentS; GetVertex(0, j).TangentS=Temp; GetVertex(Width-1, j).TangentS=Temp;
            Temp=GetVertex(0, j).TangentT+GetVertex(Width-1, j).TangentT; GetVertex(0, j).TangentT=Temp; GetVertex(Width-1, j).TangentT=Temp;
        }
    }

    if (WrapsVert())
    {
        Vector3T<T> Temp;

        for (unsigned long i=0; i<Width; i++)
        {
            Temp=GetVertex(i, 0).Normal  +GetVertex(i, Height-1).Normal;   GetVertex(i, 0).Normal  =Temp; GetVertex(i, Height-1).Normal  =Temp;
            Temp=GetVertex(i, 0).TangentS+GetVertex(i, Height-1).TangentS; GetVertex(i, 0).TangentS=Temp; GetVertex(i, Height-1).TangentS=Temp;
            Temp=GetVertex(i, 0).TangentT+GetVertex(i, Height-1).TangentT; GetVertex(i, 0).TangentT=Temp; GetVertex(i, Height-1).TangentT=Temp;
        }
    }

    // Renormalize all the interpolated tangent space axes.
    for (unsigned long VertexNr=0; VertexNr<Mesh.Size(); VertexNr++)
    {
        Mesh[VertexNr].Normal  =myNormalize(Mesh[VertexNr].Normal,   T(0.0));
        Mesh[VertexNr].TangentS=myNormalize(Mesh[VertexNr].TangentS, T(0.0));
        Mesh[VertexNr].TangentT=myNormalize(Mesh[VertexNr].TangentT, T(0.0));
    }
}


template<class T> void BezierPatchT<T>::ComputeTangentSpace_Obsolete()
{
    // NOTES:
    // 1) This method does not care whether the mesh has already been subdivided or not - it works either way.
    // 2) Special cases like wrapping and degeneracies are properly taken into account.

#if 0
    // I disabled this section of code because it's not well possible to generalize it to also deal with the tangents.
    // Moreover, I find it a lot nicer to not treat coplanar patches as a special case anyway.

    // If all points are coplanar (they all lie in a common plane), set all normals to that plane.
    {
        const Vector3T<T> Extent_Horz=GetVertex(Width-1,        0).Coord - GetVertex(0, 0).Coord;
        const Vector3T<T> Extent_Diag=GetVertex(Width-1, Height-1).Coord - GetVertex(0, 0).Coord;
        const Vector3T<T> Extent_Vert=GetVertex(      0, Height-1).Coord - GetVertex(0, 0).Coord;

        Vector3T<T> Normal=cross(Extent_Diag, Extent_Horz);

        if (Normal.GetLengthSqr()==0.0)
        {
            Normal=cross(Extent_Vert, Extent_Horz);

            if (Normal.GetLengthSqr()==0.0)
            {
                Normal=cross(Extent_Vert, Extent_Diag);
                // Note that if the patch is wrapped, the Normal may still be not valid here...
            }
        }

        const T Length=length(Normal);

        if (Length!=0.0)
        {
            Normal/=Length;

            // Distance0 is the distance of vertex (0, 0) to the plane through the origin with normal vector Normal.
            const T       Distance0=dot(Mesh[0].Coord, Normal);
            unsigned long VertexNr;

            for (VertexNr=1; VertexNr<Mesh.Size(); VertexNr++)
            {
                const T Distance=dot(Mesh[VertexNr].Coord, Normal);

                if (fabs(Distance-Distance0)>T(1.0) /*COPLANAR_EPSILON*/) break;
            }

            if (VertexNr==Mesh.Size())
            {
                // All mesh vertices are coplanar (lie in a common plane).
                for (unsigned long VertexNr=0; VertexNr<Mesh.Size(); VertexNr++)
                    Mesh[VertexNr].Normal=Normal;

                return;
            }
        }
    }
#endif


    // First check whether the patch mesh "wraps" in any direction.
    // This is done by checking of the left and right / top and bottom borders are identical.
    // If so, the tangent space should be averaged ("smoothed") across the wrap seam.
    const bool wrapWidth =WrapsHorz();
    const bool wrapHeight=WrapsVert();

    const int iWidth =int(Width );
    const int iHeight=int(Height);

    static const int Neighbours[8][2]={ {0,1}, {1,1}, {1,0}, {1,-1}, {0,-1}, {-1,-1}, {-1,0}, {-1,1} };

    for (int i=0; i<iWidth; i++)
    {
        for (int j=0; j<iHeight; j++)
        {
            Vector3T<T> Around_Coords[8];
            Vector3T<T> Around_TexCoords[8];
            bool        good[8];

            for (unsigned long k=0; k<8; k++)
            {
                Around_Coords   [k]=Vector3T<T>(0, 0, 0);
                Around_TexCoords[k]=Vector3T<T>(0, 0, 0);
                good            [k]=false;

                for (int dist=1; dist<=3; dist++)
                {
                    int x=i+Neighbours[k][0]*dist;
                    int y=j+Neighbours[k][1]*dist;

                    if (wrapWidth)
                    {
                             if (x<      0) x=iWidth-1+x;
                        else if (x>=iWidth) x=1+x-iWidth;
                    }

                    if (wrapHeight)
                    {
                             if (y<       0) y=iHeight-1+y;
                        else if (y>=iHeight) y=1+y-iHeight;
                    }

                    if (x<0 || x>=iWidth || y<0 || y>=iHeight) break;   // edge of patch


                    Around_Coords   [k]=GetVertex(x, y).Coord   -GetVertex(i, j).Coord;
                    Around_TexCoords[k]=GetVertex(x, y).TexCoord-GetVertex(i, j).TexCoord;

                    const T acLen=length(Around_Coords[k]);

                    if (acLen>T(0.2))
                    {
                        // This is a "good" edge.
                        Around_Coords   [k]/=acLen;
                        Around_TexCoords[k]=myNormalize(Around_TexCoords[k], T(0.0));
                        good            [k]=true;
                        break;
                    }

                    // Else the edge is degenerate, continue to get more dist.
                }
            }


            // Finally compute the averages of the neighbourhood around the current vertex.
            GetVertex(i, j).Normal  =Vector3T<T>(0, 0, 0);
            GetVertex(i, j).TangentS=Vector3T<T>(0, 0, 0);
            GetVertex(i, j).TangentT=Vector3T<T>(0, 0, 0);

            for (unsigned long k=0; k<8; k++)
            {
                if (!good[ k       ]) continue;
                if (!good[(k+1) & 7]) continue;

                const Vector3T<T>& Edge01=Around_Coords[(k+1) & 7];
                const Vector3T<T>& Edge02=Around_Coords[ k       ];

                const Vector3T<T> Normal =cross(Edge02, Edge01);
                const T           NormalL=length(Normal);

                if (NormalL==0.0) continue;

                GetVertex(i, j).Normal+=Normal/NormalL;

                // Understanding what's going on here is easy. The key statement is
                // "The tangent vector is parallel to the direction of increasing S on a parametric surface."
                // First, there is a short explanation in "The Cg Tutorial", chapter 8.
                // Second, I have drawn a simple figure that leads to a simple 2x2 system of Gaussian equations, see my TechArchive.
                const Vector3T<T>& uv01=Around_TexCoords[(k+1) & 7];
                const Vector3T<T>& uv02=Around_TexCoords[ k       ];
                const T            f   =uv01.x*uv02.y-uv01.y*uv02.x>0.0 ? T(1.0f) : T(-1.0f);

                GetVertex(i, j).TangentS+=myNormalize(Edge02.GetScaled(-uv01.y*f) + Edge01.GetScaled(uv02.y*f), T(0.0));
                GetVertex(i, j).TangentT+=myNormalize(Edge02.GetScaled( uv01.x*f) - Edge01.GetScaled(uv02.x*f), T(0.0));
            }

            GetVertex(i, j).Normal  =myNormalize(GetVertex(i, j).Normal,   T(0.0));
            GetVertex(i, j).TangentS=myNormalize(GetVertex(i, j).TangentS, T(0.0));
            GetVertex(i, j).TangentT=myNormalize(GetVertex(i, j).TangentT, T(0.0));
        }
    }
}


// "Automatic" subdivision.
template<class T> void BezierPatchT<T>::Subdivide(T MaxError, T MaxLength, bool OptimizeFlat)
{
    assert(Width >=3);
    assert(Height>=3);
    assert((Width  % 2)==1);
    assert((Height % 2)==1);

    const T maxHorizontalErrorSqr=MaxError*MaxError;
    const T maxVerticalErrorSqr  =MaxError*MaxError;
    const T maxLengthSqr         =MaxLength*MaxLength;


    // horizontal subdivisions
    for (unsigned long j=0; j+2<Width; j+=2)
    {
        unsigned long i;

        // check subdivided midpoints against control points
        for (i=0; i<Height; i++)
        {
            // Consider a point triple A, B, C, where A and B are the curve endpoints and C is the control point.
            // Let L=(A+C)/2, R=(B+C)/2, N=(L+R)/2, H=(A+B)/2. Then HN==NC==HC/2==(2C-A-B)/4.
            // Thus, take HN (the "error" in world units) as a measure for tesselation error,
            // as is cares both for the world size of the patch (ie. is the diameter of a pipe very big or very small),
            // as well as the severity of the deformation (almost flat curves get less triangles than very tight ones).
            const Vector3T<T>& A=GetVertex(j  , i).Coord;
            const Vector3T<T>& B=GetVertex(j+2, i).Coord;
            const Vector3T<T>& C=GetVertex(j+1, i).Coord;

            const Vector3T<T> AC=C-A;
            const Vector3T<T> CB=B-C;
            const Vector3T<T> N =(A+B+C*T(2.0))*T(0.25);

            // if the span length is too long, force a subdivision
            if (MaxLength>0 && (AC.GetLengthSqr()>maxLengthSqr || CB.GetLengthSqr()>maxLengthSqr)) break;

            // const T lenHN = length((C * T(2.0) - A - B) * T(0.25));
            // const T lenAB = length(B - A);

            // see if this midpoint is off far enough to subdivide
            if ((C-N).GetLengthSqr()>maxHorizontalErrorSqr) break;

            // This is an alternative to the above `if` test that seems to work very well,
            // but is prone to infinite looping if A, B and C are very close to each other, but not quite the same
            // (e.g. `lenAB > T(0.0001)` in the first half of the test freezes CaWE when loading the TechDemo map).
            // Maybe we should use something conservative like `lenAB > T(0.1) && lenAC > T(0.1) && lenBC > T(0.1)`?
            // if (lenAB > T(0.001) &&
            //     lenHN > T(0.2) * lenAB) break;
        }

        if (i==Height) continue;   // didn't need subdivision

        SetMeshSize(Width+2, Height);

        // Insert two columns and replace the peak a la deCasteljau.
        for (i=0; i<Height; i++)
        {
            VertexT Left, Right, Center;

            Left  .Average(GetVertex(j,   i), GetVertex(j+1, i));
            Right .Average(GetVertex(j+1, i), GetVertex(j+2, i));
            Center.Average(Left, Right);

            for (unsigned long k=Width-1; k>j+3; k--)
                GetVertex(k, i)=GetVertex(k-2, i);

            GetVertex(j+1, i)=Left;
            GetVertex(j+2, i)=Center;
            GetVertex(j+3, i)=Right;
        }

        // back up and recheck this set again, it may need more subdivision
        j-=2;
    }

    // vertical subdivisions
    for (unsigned long j=0; j+2<Height; j+=2)
    {
        unsigned long i;

        // check subdivided midpoints against control points
        for (i=0; i<Width; i++)
        {
            const Vector3T<T>& A=GetVertex(i, j  ).Coord;
            const Vector3T<T>& B=GetVertex(i, j+2).Coord;
            const Vector3T<T>& C=GetVertex(i, j+1).Coord;

            const Vector3T<T> AC=C-A;
            const Vector3T<T> CB=B-C;
            const Vector3T<T> N =(A+B+C*T(2.0))*T(0.25);

            // if the span length is too long, force a subdivision
            if (MaxLength>0 && (AC.GetLengthSqr()>maxLengthSqr || CB.GetLengthSqr()>maxLengthSqr)) break;

            // const T lenHN = length((C * T(2.0) - A - B) * T(0.25));
            // const T lenAB = length(B - A);

            // see if this midpoint is off far enough to subdivide
            if ((C-N).GetLengthSqr()>maxVerticalErrorSqr) break;

            // This is an alternative to the above `if` test that seems to work very well,
            // but is prone to infinite looping if A, B and C are very close to each other, but not quite the same
            // (e.g. `lenAB > T(0.0001)` in the first half of the test freezes CaWE when loading the TechDemo map).
            // Maybe we should use something conservative like `lenAB > T(0.1) && lenAC > T(0.1) && lenBC > T(0.1)`?
            // if (lenAB > T(0.001) &&
            //     lenHN > T(0.2) * lenAB) break;
        }

        if (i==Width) continue;     // didn't need subdivision

        SetMeshSize(Width, Height+2);

        // insert two columns and replace the peak
        for (i=0; i<Width; i++)
        {
            VertexT Left, Right, Center;

            Left  .Average(GetVertex(i, j  ), GetVertex(i, j+1));
            Right .Average(GetVertex(i, j+1), GetVertex(i, j+2));
            Center.Average(Left, Right);

            for (unsigned long k=Height-1; k>j+3; k--)
                GetVertex(i, k)=GetVertex(i, k-2);

            GetVertex(i, j+1)=Left;
            GetVertex(i, j+2)=Center;
            GetVertex(i, j+3)=Right;
        }

        // back up and recheck this set again, it may need more subdivision
        j-=2;
    }


    // Move all the "approximation" control vertices onto the true shape of the curve.
    // Note that the other vertices (the "interpolation" control vertices) already and always are on the curve per definition.
    for (unsigned long i=0; i<Width; i++)
    {
        for (unsigned long j=1; j<Height; j+=2)
        {
            VertexT Left, Right;

            Left .Average(GetVertex(i, j), GetVertex(i, j+1));
            Right.Average(GetVertex(i, j), GetVertex(i, j-1));
            GetVertex(i, j).Average(Left, Right);
        }
    }

    for (unsigned long j=0; j<Height; j++)
    {
        for (unsigned long i=1; i<Width; i+=2)
        {
            VertexT Left, Right;

            Left .Average(GetVertex(i, j), GetVertex(i+1, j));
            Right.Average(GetVertex(i, j), GetVertex(i-1, j));
            GetVertex(i, j).Average(Left, Right);
        }
    }


    if (OptimizeFlat) OptimizeFlatRowAndColumnStrips();

    // Normalize all the lerped tangent space axes.
    for (unsigned long VertexNr=0; VertexNr<Mesh.Size(); VertexNr++)
    {
        Mesh[VertexNr].Normal  =myNormalize(Mesh[VertexNr].Normal,   T(0.0));
        Mesh[VertexNr].TangentS=myNormalize(Mesh[VertexNr].TangentS, T(0.0));
        Mesh[VertexNr].TangentT=myNormalize(Mesh[VertexNr].TangentT, T(0.0));
    }

    // GenerateIndexes();
}


// "Explicit" subdivision.
template<class T> void BezierPatchT<T>::Subdivide(unsigned long SubDivsHorz, unsigned long SubDivsVert, bool OptimizeFlat)
{
    assert(Width >=3);
    assert(Height>=3);
    assert((Width  % 2)==1);
    assert((Height % 2)==1);

    const unsigned long TargetWidth =((Width -1)/2 * SubDivsHorz)+1;
    const unsigned long TargetHeight=((Height-1)/2 * SubDivsVert)+1;
    ArrayT<VertexT>     TargetMesh;

    TargetMesh.PushBackEmpty(TargetWidth*TargetHeight);

    unsigned long baseCol=0;

    for (unsigned long i=0; i+2<Width; i+=2)
    {
        unsigned long baseRow=0;

        for (unsigned long j=0; j+2<Height; j+=2)
        {
            VertexT SubPatch[3][3];

            for (unsigned long k=0; k<3; k++)
                for (unsigned long l=0; l<3; l++)
                    SubPatch[k][l]=GetVertex(i+k, j+l);

            SampleSinglePatch(SubPatch, baseCol, baseRow, TargetWidth, SubDivsHorz, SubDivsVert, TargetMesh);
            baseRow+=SubDivsVert;
        }

        baseCol+=SubDivsHorz;
    }

    // Copy the target mesh back into our mesh.
    Width =TargetWidth;
    Height=TargetHeight;
    Mesh  =TargetMesh;

    if (OptimizeFlat) OptimizeFlatRowAndColumnStrips();

    // Normalize all the lerped tangent space axes.
    for (unsigned long VertexNr=0; VertexNr<Mesh.Size(); VertexNr++)
    {
        Mesh[VertexNr].Normal  =myNormalize(Mesh[VertexNr].Normal,   T(0.0));
        Mesh[VertexNr].TangentS=myNormalize(Mesh[VertexNr].TangentS, T(0.0));
        Mesh[VertexNr].TangentT=myNormalize(Mesh[VertexNr].TangentT, T(0.0));
    }

    // GenerateIndexes();
}


template<class T> void BezierPatchT<T>::ForceLinearMaxLength(T MaxLength)
{
    const T MaxLengthSqr=MaxLength*MaxLength;

    for (unsigned long i=0; i+1<Width; i++)
    {
        unsigned long j;

        for (j=0; j<Height; j++)
            if ((GetVertex(i, j).Coord-GetVertex(i+1, j).Coord).GetLengthSqr() > MaxLengthSqr)
                break;

        // If the edges were all short enough, no need to subdivide - just consider the next column.
        if (j==Height) continue;

        // Make room for another column between i and i+1.
        SetMeshSize(Width+1, Height);

        for (j=0; j<Height; j++)
        {
            for (unsigned long k=Width-1; k>i+1; k--)
                GetVertex(k, j)=GetVertex(k-1, j);

            // Compute the contents of the new column.
            VertexT& NewV=GetVertex(i+1, j);

            NewV.Average(GetVertex(i, j), GetVertex(i+2, j));

            NewV.Normal  =myNormalize(NewV.Normal,   T(0.0));
            NewV.TangentS=myNormalize(NewV.TangentS, T(0.0));
            NewV.TangentT=myNormalize(NewV.TangentT, T(0.0));
        }

        i--;
    }

    for (unsigned long j=0; j+1<Height; j++)
    {
        unsigned long i;

        for (i=0; i<Width; i++)
            if ((GetVertex(i, j).Coord-GetVertex(i, j+1).Coord).GetLengthSqr() > MaxLengthSqr)
                break;

        // If the edges were all short enough, no need to subdivide - just consider the next column.
        if (i==Width) continue;

        // Make room for another row between j and j+1.
        SetMeshSize(Width, Height+1);

        for (i=0; i<Width; i++)
        {
            for (unsigned long k=Height-1; k>j+1; k--)
                GetVertex(i, k)=GetVertex(i, k-1);

            // Compute the contents of the new column.
            VertexT& NewV=GetVertex(i, j+1);

            NewV.Average(GetVertex(i, j), GetVertex(i, j+2));

            NewV.Normal  =myNormalize(NewV.Normal,   T(0.0));
            NewV.TangentS=myNormalize(NewV.TangentS, T(0.0));
            NewV.TangentT=myNormalize(NewV.TangentT, T(0.0));
        }

        j--;
    }

    // GenerateIndexes();
}


template<class T> T BezierPatchT<T>::GetSurfaceAreaAtVertex(unsigned long i, unsigned long j) const
{
    static const int Neighbours[8][2]={ {0,1}, {1,1}, {1,0}, {1,-1}, {0,-1}, {-1,-1}, {-1,0}, {-1,1} };

    const VertexT& Center=GetVertex(i, j);
    T              Area  =0;

    for (unsigned long NeighbourNr=0; NeighbourNr<8; NeighbourNr++)
    {
        const int Edge1_i=int(i)+Neighbours[NeighbourNr][0];
        const int Edge1_j=int(j)+Neighbours[NeighbourNr][1];

        const int Edge2_i=int(i)+Neighbours[(NeighbourNr+1) & 7][0];
        const int Edge2_j=int(j)+Neighbours[(NeighbourNr+1) & 7][1];

        if (Edge1_i<0 || Edge1_i>=int(Width )) continue;
        if (Edge1_j<0 || Edge1_j>=int(Height)) continue;

        if (Edge2_i<0 || Edge2_i>=int(Width )) continue;
        if (Edge2_j<0 || Edge2_j>=int(Height)) continue;

        // Note that we only take half of the edges into account - the other half "belongs" to other vertices!
        Vector3T<T> Edge1=(GetVertex(Edge1_i, Edge1_j).Coord-Center.Coord)*0.5;
        Vector3T<T> Edge2=(GetVertex(Edge2_i, Edge2_j).Coord-Center.Coord)*0.5;

        Area+=length(cross(Edge1, Edge2))/2.0f;
     // Area+=dot(Center.Normal, cross(Edge1, Edge2))/2.0f;     // Vermutung: Dies ergibt die auf die Ebene (durch Center.Normal definiert) projezierte Fläche! Beweis??
    }

    return Area;
}


/// Changes the size of the mesh to NewWidth times NewHeight.
template<class T> void BezierPatchT<T>::SetMeshSize(unsigned long NewWidth, unsigned long NewHeight)
{
    // Well, the implementation is really simple, stable, and covers all cases
    // (mesh gets broader/narrower and/or heigher/lower), but it's slow, too...
    ArrayT<VertexT> NewMesh;

    NewMesh.PushBackEmpty(NewWidth*NewHeight);

    for (unsigned long j=0; j<NewHeight && j<Height; j++)
        for (unsigned long i=0; i<NewWidth && i<Width; i++)
            NewMesh[j*NewWidth+i]=Mesh[j*Width+i];

    Mesh  =NewMesh;
    Width =NewWidth;
    Height=NewHeight;
}


template<class T> typename BezierPatchT<T>::VertexT BezierPatchT<T>::SampleSinglePatchPoint(const VertexT SubPatch[3][3], const T u, const T v) const
{
    VertexT SubColumnAtU[3];

    const T u_0=(1-u)*(1-u);
    const T u_1=2*u*(1-u);
    const T u_2=u*u;

    // Find the control points in u direction for the v coordinate.
    for (unsigned long RowNr=0; RowNr<3; RowNr++)
    {
        SubColumnAtU[RowNr].Coord   =SubPatch[0][RowNr].Coord   *u_0 + SubPatch[1][RowNr].Coord   *u_1 + SubPatch[2][RowNr].Coord   *u_2;
        SubColumnAtU[RowNr].TexCoord=SubPatch[0][RowNr].TexCoord*u_0 + SubPatch[1][RowNr].TexCoord*u_1 + SubPatch[2][RowNr].TexCoord*u_2;
        SubColumnAtU[RowNr].Normal  =SubPatch[0][RowNr].Normal  *u_0 + SubPatch[1][RowNr].Normal  *u_1 + SubPatch[2][RowNr].Normal  *u_2;
        SubColumnAtU[RowNr].TangentS=SubPatch[0][RowNr].TangentS*u_0 + SubPatch[1][RowNr].TangentS*u_1 + SubPatch[2][RowNr].TangentS*u_2;
        SubColumnAtU[RowNr].TangentT=SubPatch[0][RowNr].TangentT*u_0 + SubPatch[1][RowNr].TangentT*u_1 + SubPatch[2][RowNr].TangentT*u_2;
    }

    VertexT Result;

    const T v_0=(1-v)*(1-v);
    const T v_1=2*v*(1-v);
    const T v_2=v*v;

    Result.Coord   =SubColumnAtU[0].Coord   *v_0 + SubColumnAtU[1].Coord   *v_1 + SubColumnAtU[2].Coord   *v_2;
    Result.TexCoord=SubColumnAtU[0].TexCoord*v_0 + SubColumnAtU[1].TexCoord*v_1 + SubColumnAtU[2].TexCoord*v_2;
    Result.Normal  =SubColumnAtU[0].Normal  *v_0 + SubColumnAtU[1].Normal  *v_1 + SubColumnAtU[2].Normal  *v_2;
    Result.TangentS=SubColumnAtU[0].TangentS*v_0 + SubColumnAtU[1].TangentS*v_1 + SubColumnAtU[2].TangentS*v_2;
    Result.TangentT=SubColumnAtU[0].TangentT*v_0 + SubColumnAtU[1].TangentT*v_1 + SubColumnAtU[2].TangentT*v_2;

    return Result;
}


template<class T> void BezierPatchT<T>::SampleSinglePatch(const VertexT SubPatch[3][3], unsigned long baseCol, unsigned long baseRow, unsigned long TargetWidth, unsigned long SubDivsHorz, unsigned long SubDivsVert, ArrayT<VertexT>& TargetMesh) const
{
    SubDivsHorz+=1;
    SubDivsVert+=1;

    for (unsigned long i=0; i<SubDivsHorz; i++)
    {
        for (unsigned long j=0; j<SubDivsVert; j++)
        {
            const T u=T(i)/(SubDivsHorz-1);
            const T v=T(j)/(SubDivsVert-1);

            TargetMesh[(baseRow+j)*TargetWidth + baseCol+i]=SampleSinglePatchPoint(SubPatch, u, v);
        }
    }
}


/// Returns the result of Point projected onto the vector from Start to End.
template<class T> Vector3T<T> BezierPatchT<T>::ProjectPointOntoVector(const Vector3T<T>& Point, const Vector3T<T>& Start, const Vector3T<T>& End) const
{
    Vector3T<T> Vec=End-Start;
    const T     Len=length(Vec);

    if (Len!=0) Vec/=Len;

    return Start+Vec*dot(Point-Start, Vec);
}


/// This method removes unnecessary vertices of "flat" sub-strips from the mesh.
/// That is, if a vertex is in the line through its left and right neighbour vertices,
/// and the same is true for all vertices in the same column (or row) of that vertex,
/// the entire column (or row) can be removed, because the left and right vertices alone
/// are enough to define that flat column (or row).
template<class T> void BezierPatchT<T>::OptimizeFlatRowAndColumnStrips()
{
    for (unsigned long j=1; j<Width-1; j++)
    {
        T MaxSqrDist=0;

        for (unsigned long i=0; i<Height; i++)
        {
            const Vector3T<T> Offset =GetVertex(j, i).Coord-ProjectPointOntoVector(GetVertex(j, i).Coord, GetVertex(j-1, i).Coord, GetVertex(j+1, i).Coord);
            const T           DistSqr=Offset.GetLengthSqr();

            if (DistSqr>MaxSqrDist) MaxSqrDist=DistSqr;
        }

        if (MaxSqrDist<=T(0.2*0.2))
        {
            for (unsigned long i=0; i<Height; i++)
                for (unsigned long k=j; k+1<Width; k++)
                    GetVertex(k, i)=GetVertex(k+1, i);

            SetMeshSize(Width-1, Height);
            j--;
        }
    }

    for (unsigned long j=1; j<Height-1; j++)
    {
        T MaxSqrDist=0;

        for (unsigned long i=0; i<Width; i++)
        {
            const Vector3T<T> Offset =GetVertex(i, j).Coord-ProjectPointOntoVector(GetVertex(i, j).Coord, GetVertex(i, j-1).Coord, GetVertex(i, j+1).Coord);
            const T           DistSqr=Offset.GetLengthSqr();

            if (DistSqr>MaxSqrDist) MaxSqrDist=DistSqr;
        }

        if (MaxSqrDist<T(0.2*0.2))
        {
            for (unsigned long i=0; i<Width; i++)
                for (unsigned long k=j; k+1<Height; k++)
                    GetVertex(i, k)=GetVertex(i, k+1);

            SetMeshSize(Width, Height-1);
            j--;
        }
    }
}


/// Computes the three tangent-space axes (the normal and the two tangent vectors)
/// of the 3x3 sub-patch whose upper left corner is at (sp_i, sp_j) at (s, t), where s and t are values between 0 and 1.
/// The method returns true on success, false on failure (the normal vector (Axes[0]) could not be computed because it is degenerate).
template<class T> bool BezierPatchT<T>::ComputeTangentSpaceInSubPatch(unsigned long sp_i, unsigned long sp_j, const T s, const T t, Vector3T<T> Axes[3]) const
{
    const T B_s [3]={ (1.0f-s)*(1.0f-s), 2.0f*s*(1.0f-s), s*s };
    const T B_t [3]={ (1.0f-t)*(1.0f-t), 2.0f*t*(1.0f-t), t*t };

    const T B_s_[3]={ 2.0f*(s-1.0f), 2.0f-4.0f*s, 2.0f*s };     // Derivation along s.
    const T B_t_[3]={ 2.0f*(t-1.0f), 2.0f-4.0f*t, 2.0f*t };     // Derivation along t.

    Vector3T<T> TangentS;     // Tangents of the spatial patch surface.
    Vector3T<T> TangentT;

    Vector3T<T> TexTangentS;  // Tangents of the texture image in 2D texture space.
    Vector3T<T> TexTangentT;

    for (unsigned long i=0; i<3; i++)
        for (unsigned long j=0; j<3; j++)
        {
            const VertexT& v=GetVertex(sp_i+i, sp_j+j);

         // Origin     =Origin     +scale(v.Coord,    B_s [i]*B_t [j]);
         // TexCoord   =TexCoord   +scale(v.TexCoord, B_s [i]*B_t [j]);

            TangentS   =TangentS   +scale(v.Coord,    B_s_[i]*B_t [j]);
            TangentT   =TangentT   +scale(v.Coord,    B_s [i]*B_t_[j]);

            TexTangentS=TexTangentS+scale(v.TexCoord, B_s_[i]*B_t [j]);
            TexTangentT=TexTangentT+scale(v.TexCoord, B_s [i]*B_t_[j]);
        }


    // This is documented in my Tech Archive, see "Computing the tangent space basis vectors".
    // Note that only the *sign* of d is really important (less its magnitude).
    const T d=TexTangentS.x*TexTangentT.y-TexTangentS.y*TexTangentT.x;

    Axes[0]=cross(TangentT, TangentS);                                          // Normal
    Axes[1]=scale(TangentT, -TexTangentS.y) + scale(TangentS, TexTangentT.y);   // Tangent
    Axes[2]=scale(TangentT,  TexTangentS.x) - scale(TangentS, TexTangentT.x);   // BiTangent

    if (d<0)
    {
        Axes[1]=-Axes[1];
        Axes[2]=-Axes[2];
    }

    const T a0=length(Axes[0]);

    if (a0<0.000001f) return false;

    Axes[0]/=a0;
    myNormalize(Axes[1], T(0.0));
    myNormalize(Axes[2], T(0.0));

    return true;
}


/// Returns whether the patch mesh wraps "in the width", i.e. if the left and right borders are identical.
template<class T> bool BezierPatchT<T>::WrapsHorz() const
{
    for (unsigned long j=0; j<Height; j++)
        if ((GetVertex(0, j).Coord-GetVertex(Width-1, j).Coord).GetLengthSqr() > T(1.0*1.0))
            return false;

    return true;
}


/// Returns whether the patch mesh wraps "in the height", i.e. if the top and bottom borders are identical.
template<class T> bool BezierPatchT<T>::WrapsVert() const
{
    for (unsigned long i=0; i<Width; i++)
        if ((GetVertex(i, 0).Coord-GetVertex(i, Height-1).Coord).GetLengthSqr() > T(1.0*1.0))
            return false;

    return true;
}


template class BezierPatchT<float>;
template class BezierPatchT<double>;
