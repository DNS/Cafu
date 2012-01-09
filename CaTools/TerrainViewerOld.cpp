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

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#if defined _WIN32 && defined (_MSC_VER)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#if (_MSC_VER<1300)
        #define for if (false) ; else for
    #endif
#endif

#include <GL/gl.h>
#include <GL/glu.h>
#include "MaterialSystem/Common/OpenGLEx.hpp"   // for glext.h

#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/ConsoleStdout.hpp"
#include "FileSys/FileManImpl.hpp"
#include "Templates/Array.hpp"
#include "Math3D/Plane3.hpp"
#include "Bitmap/Bitmap.hpp"
#include "OpenGL/OpenGLWindow.hpp"
#include "Util/Util.hpp"
#include "Terrain/Terrain.hpp"


static cf::ConsoleStdoutT ConsoleStdout;
cf::ConsoleI* Console=&ConsoleStdout;

static cf::FileSys::FileManImplT FileManImpl;
cf::FileSys::FileManI* cf::FileSys::FileMan=&FileManImpl;


// ******************** common.h ************************************************

// Miscellaneous math macros.
#define DEG2RAD(x)      ((3.1415927f / 180.0f) * (x))
#define MAX(a, b)       ((a) > (b) ? (a) : (b))
#define SQR(x)          ((x) * (x))

// ******************** index.h ************************************************

#define LINEAR_INDEX(i, j, m)   ((i) + (j) + ((j) << (m)))
#define LINEAR_SPLIT(i, j, k)   (((j) + (k)) / 2)
#define LINEAR_CHILD_L(i, j, k) LINEAR_SPLIT(i, j, k), j, i
#define LINEAR_CHILD_R(i, j, k) LINEAR_SPLIT(i, j, k), i, k

// Standard row-major (linear) matrix layout.
#define I_SW(m)               LINEAR_INDEX(0 << (m), 0 << (m), m)
#define I_SE(m)               LINEAR_INDEX(1 << (m), 0 << (m), m)
#define I_NE(m)               LINEAR_INDEX(1 << (m), 1 << (m), m)
#define I_NW(m)               LINEAR_INDEX(0 << (m), 1 << (m), m)
#define I_C(m)                LINEAR_INDEX(1 << ((m) - 1), 1 << ((m) - 1), m)
#define ROOT_S(m)             I_C(m), I_SW(m), I_SE(m)
#define ROOT_E(m)             I_C(m), I_SE(m), I_NE(m)
#define ROOT_N(m)             I_C(m), I_NE(m), I_NW(m)
#define ROOT_W(m)             I_C(m), I_NW(m), I_SW(m)
#define TRIANGLE(i, j, k)     i, j, k
#define SPLIT(i, j, k)        LINEAR_SPLIT(i, j, k)
#define CHILD_L(i, j, k)      LINEAR_CHILD_L(i, j, k)
#define CHILD_R(i, j, k)      LINEAR_CHILD_R(i, j, k)


// ******************** terrain.h ************************************************

typedef unsigned long VERTEXid;

// TODO: struct VERTEX : public VectorT   ???
struct VERTEX
{
    VectorT p;     // vertex position
    float   e;     // error
    float   r;     // bounding sphere radius
};


// ******************** End Of Headers.h ************************************************


struct TerrainOldT
{
    ArrayT<VERTEX> Vertices;
    unsigned long  Size;        // Lateral dimensions of the terrain / heightmap, e.g. 257, 513, 1025, ...
    unsigned long  Levels;      // Size == 2^(Levels/2)+1


    unsigned long GetIdx(unsigned long i, unsigned long j)
    {
        return i+Size*j;
    }


    // i  - column index
    // j  - row index
    // di - non-negative col offset to bisected edge endpoint
    // dj - row offset to bisected edge endpoint
    // n  - one less array width/height (zero for leaves)
    void ComputeVertexLoD(unsigned long i, unsigned long j, int di, int dj, unsigned long n)
    {
        VERTEX& Vertex=Vertices[GetIdx(i, j)];

        // Compute actual error and initialize radius to zero. The error is simply the vertical difference between the vertex and the bisected edge,
        // i.e. the error between two consecutive levels of refinement. This object-space error can be replaced with any measure of error,
        // as long as the nesting step below is performed.
        Vertex.e=float(fabs(Vertex.p.z-0.5*(Vertices[GetIdx(i-di, j-dj)].p.z+Vertices[GetIdx(i+di, j+dj)].p.z)));
        Vertex.r=0.0f;

        // If the vertex is not a leaf node, ensure that the error and radius are nested using information from its four children.
        // Note that the offsets (+di, +dj) and (-di, -dj) from (i, j) initially get us to the two vertices of the bisected edge.
        // By "rotating" (di, dj) 45 degrees (in a topological sense), we arrive at one of the children of (i, j) (assuming we're not on a boundary).
        // Successive 90-degree rotations then allow us to visit all four children.
        if (n==0) return;

        dj=(di+dj)/2;   // dj' = (di+dj)/2
        di-=dj;         // di' = (di-dj)/2

        for (unsigned long k=0; k<4; k++)
        {
            // Test whether child vertex exists.
            if ((i!=0 || di>=0) && (i!=Size-1 || di<=0) &&
                (j!=0 || dj>=0) && (j!=Size-1 || dj<=0))
            {
                // Inflate error and radius as needed.
                VERTEX& Child=Vertices[GetIdx(i+di, j+dj)];

                Vertex.e=MAX(Vertex.e, Child.e);
                Vertex.r=MAX(Vertex.r, float(length(Vertex.p-Child.p))+Child.r);
            }

            // di' = -dj
            // dj' = +di
            dj += di;
            di -= dj;
            dj += di;
        }
    }


    TerrainOldT(const char* FileNameHeightMap) /*throw (BitmapT::LoadErrorT)*/
    {
        BitmapT HeightMap(FileNameHeightMap);

        // Compute Levels and Size, and thereby perform some sanity checks.
        if (HeightMap.SizeX!=HeightMap.SizeY) throw BitmapT::LoadErrorT();
        if (HeightMap.SizeX<3) throw BitmapT::LoadErrorT();
        if (HeightMap.SizeX>(1UL << 30/2)+1) throw BitmapT::LoadErrorT();

        Levels=1;
        while ((1UL << Levels)+1<HeightMap.SizeX) Levels++;
        Levels*=2;

        Size=(1UL << (Levels/2))+1;

        if (Size!=HeightMap.SizeX) throw BitmapT::LoadErrorT();


        // Copy the vertex coordinates from HeightMap into the p components of the Vertices.
        // Note that we pick the red channel of the RBG HeightMap.Data as the relevant channel.
        // Moreover, note that the y-axis of the HeightMap.Data points down in screen-space and thus towards us in world space.
        // Our world-space y-axis points opposite (away from us), though, and therefore we access the HeightMap.Data at (i, Size-j-1).
        Vertices.PushBackEmpty(Size*Size);

        for (unsigned long j=0; j<Size; j++)
            for (unsigned long i=0; i<Size; i++)
            {
                const float Resolution[3]={ 160.0, 160.0, 25.5*255.0 };

                Vertices[GetIdx(i, j)].p.x=Resolution[0]*(i-0.5*(Size-1));
                Vertices[GetIdx(i, j)].p.y=Resolution[1]*(j-0.5*(Size-1));
                Vertices[GetIdx(i, j)].p.z=Resolution[2]*((HeightMap.Data[GetIdx(i, Size-j-1)] & 0xFF)/255.0);

                // Note! The extra-brackets around  ( (HeightMap.Data[GetIdx(i, Size-j-1)] & 0xFF)/255.0 )  in the above line
                // are needed to be 100% (bitwise) rounding error compatible to the new terrain lib!
            }


        // Compute error and radius bottom-up, level-by-level.
        // Unfortunately, the paper explains this only rather briefly in section V.B. on page 15.
        const unsigned long n=Size-1;
        unsigned long       s;
        int                 a, b, c;

        for (a=c=1, b=2, s=0; (unsigned long)a!=n; a=c=b, b*=2, s=n)
        {
            unsigned long j;

            // Process level in black quadtree.
            for (j=a; j<n; j+=b)
                for (unsigned long i=0; i<=n; i+=b)
                {
                    ComputeVertexLoD(i, j, 0, a, s);
                    ComputeVertexLoD(j, i, a, 0, s);
                }

            // Process level in white quadtree.
            for (j=a; j<n; c=-c, j+=b)
                for (unsigned long i=a; i<n; c=-c, i+=b)
                    ComputeVertexLoD(i, j, a, c, n);
        }

        // Lock center and corner vertices.
        Vertices[GetIdx(0, 0)].e=Vertices[GetIdx(n, 0)].e=Vertices[GetIdx(0, n)].e=Vertices[GetIdx(n, n)].e=Vertices[GetIdx(n/2, n/2)].e=float(HUGE_VAL);
        Vertices[GetIdx(0, 0)].r=Vertices[GetIdx(n, 0)].r=Vertices[GetIdx(0, n)].r=Vertices[GetIdx(n, n)].r=Vertices[GetIdx(n/2, n/2)].r=float(HUGE_VAL);
    }


    // ComputeIndexStrip() const;
    // ComputeVectorStrip() const;
};


struct VDinfo
{
    bool    cull;               // perform view culling when set
    bool    morph;              // perform geomorphing when set
    float   nu;                 // inverse of error tolerance in radians
    float   nu_min;             // lower morph parameter
    float   nu_max;             // upper morph parameter
    VectorT viewpoint;          // viewpoint
    Plane3T<double>  viewplanes[5];      // view frustum planes (without the "far" plane)
};


const unsigned long SPHERE_MASK_UNDECIDED=0x20;     // 0010 0000   initial visibility mask
const unsigned long SPHERE_MASK_VISIBLE  =0x3F;     // 0011 1111   guaranteed visible


// vp   - vertex
// vdp  - view-dependent parameters
// mask - visibility mask
static unsigned long sphere_visible(const VERTEX& vp, const VDinfo& vdp, unsigned long mask)
{
    // Compare the radius of the vertex's bounding sphere against the signed distance to each plane of the view volume.
    // If the sphere is completely on the exterior side of a plane, it is invisible.
    // Otherwise, if it is entirely on the interior side, then a flag is set for this plane, and no further
    // tests are made between this plane and the vertex's descendants since the bounding spheres are nested.
    for (unsigned long i=0; i<5; i++)
        if (!(mask & (1u << i)))
        {
            const float d=float(vdp.viewplanes[i].GetDistance(vp.p));

            if (d> vp.r) return 0;          // completely outside view volume
            if (d<-vp.r) mask|=1u << i;     // completely on interior side of view plane
        }

    return mask;
}


// ******************************** BEGIN MORPH **************************************************
//@p-r-i-v-a-t-e---t-y-p-e-s-------------------------------------------------

ArrayT<VectorT> VectorStrip;
VERTEXid        VectorStripHead;    // ID of most recent vertex.
VERTEXid        VectorStripTail;    // ID of second most recent vertex.
bool            VectorStripParity;  // Parity of most recent vertex.


//@p-r-i-v-a-t-e---f-u-n-c-t-i-o-n-s-----------------------------------------

// vv - array of vertices
// v  - index of vertex to append
// p  - parity of vertex
// z  - elevation of morphed vertex
static void tstrip_vector_append(const VERTEX* vv, VERTEXid v, bool Parity, float z)
{
    if (v!=VectorStripTail && v!=VectorStripHead)
    {
        if (Parity==VectorStripParity)
        {
            VectorStrip.PushBack(VectorStrip[VectorStrip.Size()-2]);    // turn corner; duplicate vertex
        }
        else
        {
            VectorStripParity=Parity;
            VectorStripTail  =VectorStripHead;
        }

        VectorStripHead=v;

        // append new vertex
        VectorStrip.PushBack(VectorT(vv[v].p.x, vv[v].p.y, z));

        // TriangleCount++;                                         // Count triangles for statistics.
    }
}


// zmp - pointer to morphed elevation
// vp  - pointer to vertex
// vdp - view-dependent parameters
// zl  - elevation of left endpoint of split edge
// zr  - elevation of right endpoint of split edge
static bool vertex_morph(float* zmp, const VERTEX* vp, const VDinfo& vdp, float zl, float zr)
{
    // Compute the elevation of the morphed vertex.  The return value indicates whether the vertex is active or not.
    const VectorT View=vp->p-vdp.viewpoint;
    const float   d   =float(dot(View, View));    // d=length(vp->p-vdp.viewpoint)^2
    const float   dmax=SQR(vdp.nu_max * vp->e + vp->r);

    if (dmax>d)
    {
        const float dmin=SQR(vdp.nu_min * vp->e + vp->r);

        *zmp=dmin>d ? float(vp->p.z) : ((dmax-d) * float(vp->p.z) + (d-dmin) * 0.5f * (zl+zr)) / (dmax-dmin);
        return true;
    }
    else return false;
}


// vv  - vertex array
// vdp - view-dependent parameters
// l   - refinement level
// i   - triangle apex
// j   - supplemental vertex #1
// k   - supplemental vertex #2
// za  - elevation of apex
// zl  - elevation of left corner
// zr  - elevation of right corner
static void submesh_morph(const VERTEX* vv, const VDinfo& vdp, unsigned long l, TRIANGLE(VERTEXid i, VERTEXid j, VERTEXid k), float za, float zl, float zr)
{
    float      zm=0.0f;
    const bool refine=(l!=0) && vertex_morph(&zm, vv + SPLIT(i, j, k), vdp, zl, zr);

    // Recursively refine and morph the mesh.
    if (refine) submesh_morph(vv, vdp, l-1, CHILD_L(i, j, k), zm, zl, za); tstrip_vector_append(vv, i, l & 1, za);
    if (refine) submesh_morph(vv, vdp, l-1, CHILD_R(i, j, k), zm, za, zr);
}


// vv  - vertex array
// vdp - view-dependent parameters
// l   - refinement level
// i   - triangle apex
// j   - supplemental vertex #1
// k   - supplemental vertex #2
// za  - elevation of apex
// zl  - elevation of left corner
// zr  - elevation of right corner
// m   - visibility mask
static void submesh_morph_visible(const VERTEX* vv, const VDinfo& vdp, unsigned long l, TRIANGLE(VERTEXid i, VERTEXid j, VERTEXid k), float za, float zl, float zr, unsigned long m)
{
    // Recursively refine, morph, and cull the mesh.
    if (m==SPHERE_MASK_VISIBLE)
    {
        submesh_morph(vv, vdp, l, TRIANGLE(i, j, k), za, zl, zr);
        return;
    }

    m=sphere_visible(vv[SPLIT(i, j, k)], vdp, m);

    float      zm=0.0f;
    const bool refine=(l!=0) && vertex_morph(&zm, vv+SPLIT(i, j, k), vdp, zl, zr) && m;

    if (refine) submesh_morph_visible(vv, vdp, l-1, CHILD_L(i, j, k), zm, zl, za, m); tstrip_vector_append(vv, i, l & 1, za);
    if (refine) submesh_morph_visible(vv, vdp, l-1, CHILD_R(i, j, k), zm, za, zr, m);
}


// vdp - view-dependent parameters
// m   - initial visibility mask
ArrayT<VectorT>& mesh_morph(const TerrainOldT& Terrain, const VDinfo& vdp, unsigned long m)
{
    const unsigned long n  =Terrain.Levels;
    const VERTEX*       vv =&Terrain.Vertices[0];
    const float         zc =float(vv[I_C (n/2)].p.z);
    const float         zsw=float(vv[I_SW(n/2)].p.z);
    const float         zse=float(vv[I_SE(n/2)].p.z);
    const float         zne=float(vv[I_NE(n/2)].p.z);
    const float         znw=float(vv[I_NW(n/2)].p.z);

    // Initialize vector strip.
    VectorStrip.Overwrite();
    VectorStrip.PushBack(vv[I_SW(n/2)].p); VectorStripTail=I_SW(n/2);
    VectorStrip.PushBack(vv[I_SW(n/2)].p); VectorStripHead=I_SW(n/2);
    VectorStripParity=1;

    // Top-level function for constructing a morphed mesh.
    submesh_morph_visible(vv, vdp, n-1, ROOT_S(n/2), zc, zsw, zse, m); tstrip_vector_append(vv, I_SE(n/2), 0, zse);
    submesh_morph_visible(vv, vdp, n-1, ROOT_E(n/2), zc, zse, zne, m); tstrip_vector_append(vv, I_NE(n/2), 0, zne);
    submesh_morph_visible(vv, vdp, n-1, ROOT_N(n/2), zc, zne, znw, m); tstrip_vector_append(vv, I_NW(n/2), 0, znw);
    submesh_morph_visible(vv, vdp, n-1, ROOT_W(n/2), zc, znw, zsw, m); VectorStrip.PushBack(vv[I_SW(n/2)].p);

    return VectorStrip;
}

// ******************************** END MORPH ****************************************************


//@p-r-i-v-a-t-e---f-u-n-c-t-i-o-n-s-----------------------------------------

ArrayT<VERTEXid> IndexStrip;
bool             IndexStripParity;


// vp  - pointer to vertex
// vdp - view-dependent parameters
static bool vertex_active(const VERTEX& vp, const VDinfo& vdp)
{
    const float d =float(length(vp.p-vdp.viewpoint));
    const float d2=d*d;

    return SQR(vdp.nu*vp.e+vp.r) > d2;
}


// v   - index of vertex to append
// p   - parity of vertex
void tstrip_index_append(VERTEXid v, bool Parity)
{
    const VERTEXid tail=IndexStrip[IndexStrip.Size()-2];
    const VERTEXid head=IndexStrip[IndexStrip.Size()-1];

    // Add vertex to end of triangle strip vertex buffer.
    if (v!=tail && v!=head)
    {
        if (Parity==IndexStripParity) IndexStrip.PushBack(tail);    // turn corner; duplicate vertex
        IndexStrip.PushBack(v);                                     // append new vertex
        IndexStripParity=Parity;                                    // store parity here for easy access

        // TriangleCount++;                                         // Count triangles for statistics.
    }
}


// vv  - vertex array
// vdp - view-dependent parameters
// l   - refinement level
// i   - triangle apex
// j   - supplemental vertex #1
// k   - supplemental vertex #2
void submesh_refine(const VERTEX* vv, const VDinfo& vdp, unsigned long l, TRIANGLE(VERTEXid i, VERTEXid j, VERTEXid k))
{
    const bool refine=(l!=0) && vertex_active(vv[SPLIT(i, j, k)], vdp);

    // Recursively refine the mesh.  Since the refinement condition is the
    // same for both branches and can be somewhat expensive to evaluate,
    // it is evaluated and tested *before* making the recursive calls.
    if (refine) submesh_refine(vv, vdp, l-1, CHILD_L(i, j, k)); tstrip_index_append(i, l & 1);
    if (refine) submesh_refine(vv, vdp, l-1, CHILD_R(i, j, k));
}


// vv  - vertex array
// vdp - view-dependent parameters
// l   - refinement level
// i   - triangle apex
// j   - supplemental vertex #1
// k   - supplemental vertex #2
// m   - visibility mask
void submesh_refine_visible(const VERTEX* vv, const VDinfo& vdp, unsigned long l, TRIANGLE(VERTEXid i, VERTEXid j, VERTEXid k), unsigned long m)
{
    // If the sphere is contained inside the view volume, then transition to
    // submesh_refine() and make no further view culling tests.  Otherwise, continue culling.
    if (m==SPHERE_MASK_VISIBLE)
    {
        submesh_refine(vv, vdp, l, TRIANGLE(i, j, k));
        return;
    }

    m=sphere_visible(vv[SPLIT(i, j, k)], vdp, m);
    const bool refine=(l!=0) && vertex_active(vv[SPLIT(i, j, k)], vdp) && m;

    if (refine) submesh_refine_visible(vv, vdp, l-1, CHILD_L(i, j, k), m); tstrip_index_append(i, l & 1);
    if (refine) submesh_refine_visible(vv, vdp, l-1, CHILD_R(i, j, k), m);
}


// vdp    - view-dependent parameters
// m      - initial visibility mask
ArrayT<VERTEXid>& mesh_refine(const TerrainOldT& Terrain, const VDinfo& vdp, unsigned long m)
{
    const unsigned long n =Terrain.Levels;
    const VERTEX*       vv=&Terrain.Vertices[0];

    // Initialize index strip.
    IndexStrip.Overwrite();
    IndexStrip.PushBack(I_SW(n/2));
    IndexStrip.PushBack(I_SW(n/2));
    IndexStripParity=1;

    // Top-level function for constructing an indexed mesh.
    submesh_refine_visible(vv, vdp, n-1, ROOT_S(n/2), m); tstrip_index_append(I_SE(n/2), 0);
    submesh_refine_visible(vv, vdp, n-1, ROOT_E(n/2), m); tstrip_index_append(I_NE(n/2), 0);
    submesh_refine_visible(vv, vdp, n-1, ROOT_N(n/2), m); tstrip_index_append(I_NW(n/2), 0);
    submesh_refine_visible(vv, vdp, n-1, ROOT_W(n/2), m); IndexStrip.PushBack(I_SW(n/2));

    return IndexStrip;
}


//@p-u-b-l-i-c---f-u-n-c-t-i-o-n-s-------------------------------------------

void Usage()
{
    printf("\nUSAGE: TerrainViewer TerrainName\n");
    printf("\n");

    exit(1);
}


int main(int ArgC, char* ArgV[])
{
    const char* TerrainName=NULL;
    const char* TextureName=NULL;

    // Initialize the FileMan by mounting the default file system.
    // Note that specifying "./" (instead of "") as the file system description effectively prevents the use of
    // absolute paths like "D:\abc\someDir\someFile.xy" or "/usr/bin/xy". This however should be fine for this application.
    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_LOCAL_PATH, "./", "");
    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/DeathMatch/Textures/TechDemo.zip", "Games/DeathMatch/Textures/TechDemo/", "Ca3DE");
    cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, "Games/DeathMatch/Textures/SkyDomes.zip", "Games/DeathMatch/Textures/SkyDomes/", "Ca3DE");

    for (int ArgNr=1; ArgNr<ArgC; ArgNr++)
    {
             if (ArgV[ArgNr][0]=='-') Usage();
        else if (TerrainName==NULL) TerrainName=ArgV[ArgNr];
        else if (TextureName==NULL) TextureName=ArgV[ArgNr];
        else Usage();
    }

    if (!TerrainName)
    {
        printf("\nHmm. Specifying a terrain name wouldn't hurt...\n");
        Usage();
    }


    try
    {
        TerrainOldT TerrainOld(TerrainName);
        TerrainT    TerrainNew(TerrainName, Vector3fT(160.0f, 160.0f, 25.5f*255.0f));


        // Compare the old and new terrains.
        const TerrainT::VertexT* NewVertices=TerrainNew.GetVertices();

        printf("Comparing terrains...\n");
        for (unsigned long VNr=0; VNr<TerrainOld.Vertices.Size(); VNr++)
        {
            if (TerrainOld.Vertices[VNr].p.x!=NewVertices[VNr].x) { printf("COMPARE 1 FAILED! %lu, %f != %f\n", VNr, TerrainOld.Vertices[VNr].p.x, NewVertices[VNr].x); return 1; };
            if (TerrainOld.Vertices[VNr].p.y!=NewVertices[VNr].y) { printf("COMPARE 2 FAILED! %lu, %f != %f\n", VNr, TerrainOld.Vertices[VNr].p.y, NewVertices[VNr].y); return 1; };
            if (TerrainOld.Vertices[VNr].p.z!=NewVertices[VNr].z) { printf("COMPARE 3 FAILED! %lu, %f != %f\n", VNr, TerrainOld.Vertices[VNr].p.z, NewVertices[VNr].z); return 1; };
            if (TerrainOld.Vertices[VNr].e  !=NewVertices[VNr].e) { printf("COMPARE 4 FAILED! %lu, %f != %f\n", VNr, TerrainOld.Vertices[VNr].e,   NewVertices[VNr].e); return 1; };
            if (TerrainOld.Vertices[VNr].r  !=NewVertices[VNr].r) { printf("COMPARE 5 FAILED! %lu, %f != %f\n", VNr, TerrainOld.Vertices[VNr].r,   NewVertices[VNr].r); return 1; };
        }
        printf("OK!\n");

        // Open OpenGL-Window.
        const char* ErrorMsg=SingleOpenGLWindow->Open("Cafu Terrain Viewer 1.0", 800, 600, 32, false);

        if (ErrorMsg)
        {
            printf("\nUnable to open OpenGL window: %s\n", ErrorMsg);
            return 0;
        }

        glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

        if (TextureName)
        {
            try
            {
                const float res0=160.0;
                const float res1=160.0;

                // BBmin and BBmax are BBs for the xy-plane, z is not needed here.
                // Also note that the y-components have been multiplied by -1, or otherwise the texture gets flipped.
                const float BBmin[2]={ -res0*0.5f*(TerrainOld.Size-1),  res1*0.5f*(TerrainOld.Size-1) };
                const float BBmax[2]={  res0*0.5f*(TerrainOld.Size-1), -res1*0.5f*(TerrainOld.Size-1) };

                const float Plane1[4]={ 1.0f/(BBmax[0]-BBmin[0]), 0.0, 0.0, -BBmin[0]/(BBmax[0]-BBmin[0]) };
                const float Plane2[4]={ 0.0, 1.0f/(BBmax[1]-BBmin[1]), 0.0, -BBmin[1]/(BBmax[1]-BBmin[1]) };

                BitmapT Texture(TextureName);

                glEnable(GL_TEXTURE_GEN_S);
                glEnable(GL_TEXTURE_GEN_T);

                glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

                gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8, Texture.SizeX, Texture.SizeY, GL_RGBA, GL_UNSIGNED_BYTE, &Texture.Data[0]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                glTexGenf(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
                glTexGenf(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

                glTexGenfv(GL_S, GL_OBJECT_PLANE, Plane1);
                glTexGenfv(GL_T, GL_OBJECT_PLANE, Plane2);

                glEnable(GL_TEXTURE_2D);
                glColor3f(1.0, 1.0, 1.0);
            }
            catch (const BitmapT::LoadErrorT& /*E*/) {}
        }


        VDinfo VDI;

        VDI.cull =false;            // perform view culling when set
        VDI.morph=true;             // perform geomorphing when set

        // Master Game Loop
        TimerT  Timer;
        VectorT ViewerPos(0.0, -500.0, 50.0);
        float   Heading=0.0;
        float   Pitch  =0.0;

        while (true)
        {
            // Rufe die Nachrichten der Windows-Nachrichtenschlange ab.
            if (SingleOpenGLWindow->HandleWindowMessages()) break;

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glLoadIdentity();
            glRotatef(-90.0, 1.0, 0.0, 0.0);
            glRotatef(Pitch  , 1.0, 0.0, 0.0);
            glRotatef(Heading, 0.0, 0.0, 1.0);
            glTranslatef(float(-ViewerPos.x), float(-ViewerPos.y), float(-ViewerPos.z));

            if (TextureName==NULL) glColor3f(0.6f, 1.0f, 0.5f);
            if (TextureName==NULL) glDisable(GL_POLYGON_OFFSET_FILL);
            if (TextureName==NULL) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


            const float tau    =4.0;    // Error tolerance in pixel.
         // const float tau_min=tau;
         // const float tau_max=VDI.morph ? (3.0/2.0)*tau_min : tau_min;
            // The basic formula for fov_x is from soar/main.c, reshape_callback function, rearranged for fov_x.
            // The 67.5 is a fixed value from OpenGLWindow.cpp.
            const float fov_x  =float(2.0*atan(float(SingleOpenGLWindow->GetWidth())/float(SingleOpenGLWindow->GetHeight())*tan(DEG2RAD(67.5)/2.0)));
            const float kappa  =tau/SingleOpenGLWindow->GetWidth() * fov_x;

            VDI.nu    =kappa>0.0 ? 1.0f/kappa : FLT_MAX;    // inverse of error tolerance in radians
            VDI.nu_min=2.0f/3.0f*VDI.nu;                    // lower morph parameter
            VDI.nu_max=          VDI.nu;                    // upper morph parameter

            VDI.viewpoint=ViewerPos;

            // Set up VDI.viewplanes (clip planes) for view frustum culling.
            GLdouble mp[16]; glGetDoublev(GL_PROJECTION_MATRIX, mp);
            GLdouble mv[16]; glGetDoublev(GL_MODELVIEW_MATRIX, mv);
            double   mpv[4][4];

            // Multiply modelview and projection matrices.
            for (unsigned long i=0; i<4; i++)
                for (unsigned long j=0; j<4; j++)
                {
                    mpv[i][j]=0.0;

                    for (unsigned long k=0; k<4; k++)
                        mpv[i][j]+=mp[4*k+i] * mv[4*j+k];
                }

            // Compute view frustum planes.
            for (unsigned long i=0; i<5; i++)
            {
                // m can be used to easily minify / shrink the view frustum.
                // The values should be between 0 and 8: 0 is the default (no minification), 8 is the reasonable maximum.
                const char    m=0;
                const double  d=(i<4 && m>0) ? 1.0-0.75*m/8.0 : 1.0;
                double        plane[4];

                for (unsigned long j=0; j<4; j++)
                    plane[j]=((i & 1) ? mpv[i/2][j] : -mpv[i/2][j]) - d*mpv[3][j];

                const double l=sqrt(SQR(plane[0])+SQR(plane[1])+SQR(plane[2]));

                VDI.viewplanes[i]=Plane3T<double>(VectorT(plane[0]/l, plane[1]/l, plane[2]/l), -plane[3]/l);
            }


            TerrainT::ViewInfoT VI;
            VI.cull     =VDI.cull;
            VI.nu       =VDI.nu;
            VI.nu_min   =VDI.nu_min;
            VI.nu_max   =VDI.nu_max;
            VI.viewpoint=VDI.viewpoint.AsVectorOfFloat();
            for (unsigned long i=0; i<5; i++)
                VI.viewplanes[i]=Plane3fT(VDI.viewplanes[i].Normal.AsVectorOfFloat(), float(VDI.viewplanes[i].Dist));

            glBegin(GL_TRIANGLE_STRIP);
            if (VDI.morph)
            {
                ArrayT<Vector3fT>& VectorStripNew=TerrainNew.ComputeVectorStripByMorphing(VI);

                // Compare the old and new vector strips!
                {
                    ArrayT<VectorT>& VectorStrip=mesh_morph(TerrainOld, VDI, VDI.cull ? SPHERE_MASK_UNDECIDED : SPHERE_MASK_VISIBLE);

                    if (VectorStrip.Size()!=VectorStripNew.Size())
                    {
                        printf("Compare of vstrips sizes failed!  %lu %lu\n", VectorStrip.Size(), VectorStripNew.Size());
                    }
                    else
                    {
                        for (unsigned long v=0; v<VectorStrip.Size(); v++)
                        {
                            if (VectorStrip[v].x!=VectorStripNew[v].x) { printf("Compare of vstrips 1 failed! %lu %f %f\n", v, VectorStrip[v].x, VectorStripNew[v].x); }
                            if (VectorStrip[v].y!=VectorStripNew[v].y) { printf("Compare of vstrips 2 failed! %lu %f %f\n", v, VectorStrip[v].y, VectorStripNew[v].y); }
                            if (fabs(VectorStrip[v].z-VectorStripNew[v].z)>0.001) { printf("Compare of vstrips 3 failed!  diff==%.10f\n", VectorStrip[v].z-VectorStripNew[v].z); }
                        }
                    }
                }

                // Note that the first VectorT at VectorStrip[0] must be skipped!
                for (unsigned long VNr=1; VNr<VectorStripNew.Size(); VNr++) glVertex3f(float(VectorStripNew[VNr].x), float(VectorStripNew[VNr].y), float(VectorStripNew[VNr].z));
            }
            else
            {
                ArrayT<VERTEXid>& IdxStripNew=TerrainNew.ComputeIndexStripByRefinement(VI);

                // Compare the old and new index strips!
                {
                    ArrayT<VERTEXid>& IdxStrip=mesh_refine(TerrainOld, VDI, VDI.cull ? SPHERE_MASK_UNDECIDED : SPHERE_MASK_VISIBLE);

                    if (IdxStrip.Size()!=IdxStripNew.Size()) { printf("Compare of istrips sizes failed!\n"); return 1; }
                    for (unsigned long v=0; v<IdxStrip.Size(); v++)
                        if (IdxStrip[v]!=IdxStripNew[v]) { printf("Compare of istrips failed!\n"); return 1; }
                }

                // Note that the first index at IdxStrip[0] must be skipped!
                const TerrainT::VertexT* Vertices=TerrainNew.GetVertices();

                for (unsigned long IdxNr=1; IdxNr<IdxStripNew.Size(); IdxNr++) glVertex3f(float(Vertices[IdxStripNew[IdxNr]].x), float(Vertices[IdxStripNew[IdxNr]].y), float(Vertices[IdxStripNew[IdxNr]].z));
            }
            glEnd();

            if (TextureName==NULL) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            if (TextureName==NULL) glEnable(GL_POLYGON_OFFSET_FILL);


            double DeltaTime=Timer.GetSecondsSinceLastCall();

            float   MoveSpeed=float(1000.0*DeltaTime);
            float   RotSpeed =float(  90.0*DeltaTime);


            SingleOpenGLWindow->SwapBuffers();

            CaKeyboardEventT KE;
            bool QuitProgram=false;

            while (SingleOpenGLWindow->GetNextKeyboardEvent(KE)>0)
            {
                if (KE.Type!=CaKeyboardEventT::CKE_KEYDOWN) continue;
                if (KE.Key==CaKeyboardEventT::CK_ESCAPE) QuitProgram=true;
                if (KE.Key==CaKeyboardEventT::CK_C     ) { VDI.cull =!VDI.cull;  printf("View frustum culling is %s.\n", VDI.cull ? "ON" : "OFF"); }
                if (KE.Key==CaKeyboardEventT::CK_M     ) { VDI.morph=!VDI.morph; printf("Geo-morphing is %s\n", VDI.morph ? "ON" : "OFF"); }
            }

            if (QuitProgram) break;


            const float vx=float(MoveSpeed*sin(Heading/180.0*3.1415926));
            const float vy=float(MoveSpeed*cos(Heading/180.0*3.1415926));

            if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_UP    ] || SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_W]) ViewerPos=ViewerPos+VectorT( vx,  vy, 0);
            if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_DOWN  ] || SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_S]) ViewerPos=ViewerPos+VectorT(-vx, -vy, 0);
            if (                                                                       SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_A]) ViewerPos=ViewerPos+VectorT(-vy,  vx, 0);
            if (                                                                       SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_D]) ViewerPos=ViewerPos+VectorT( vy, -vx, 0);
            if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_INSERT] || SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_R]) ViewerPos.z+=MoveSpeed;
            if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_DELETE] || SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_F]) ViewerPos.z-=MoveSpeed;
            if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_LEFT  ]                                                                  ) Heading-=RotSpeed;
            if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_RIGHT ]                                                                  ) Heading+=RotSpeed;
            if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_PGUP  ]                                                                  ) Pitch-=RotSpeed;
            if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_PGDN  ]                                                                  ) Pitch+=RotSpeed;
            if (SingleOpenGLWindow->GetKeyboardState()[CaKeyboardEventT::CK_END   ]                                                                  ) Pitch=0.0;
        }

        SingleOpenGLWindow->Close();
    }
    catch (const BitmapT::LoadErrorT& /*E*/)
    {
        printf("\nEither \"%s\" could not be found, not be read,\n", TerrainName);
        printf("is not square, is smaller than 3 pixels, or not of size 2^n+1.  Sorry.\n");
    }

    return 0;
}
