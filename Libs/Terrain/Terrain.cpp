/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/*
 * The SOAR terrain algorithm has been invented by Peter Lindstrom and Valerio Pascucci.
 * This implementation is based on the implementation by Andras Balogh and has been rewritten, modified and customized by
 * Carsten Fuchs Software (for example, we added the very important geo-morphing feature that was missing from Andras' code).
 * Andras Baloghs website is at <http://www.andrasbalogh.com/RangerMk2/>, and the license for his code is, verbatim:
 * "You can do whatever you want with the code, free of charge, but if you use it for something cool, I'd really love to know about it!"
 */
#include "Terrain.hpp"
#include "Bitmap/Bitmap.hpp"
#include "TextParser/TextParser.hpp"
#include "String.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>


#if __GNUC__>3
// This is utterly annoying, see http://gcc.gnu.org/bugzilla/show_bug.cgi?id=25509 for details.
static size_t fread_no_warn(void* buffer, size_t size, size_t items, FILE* file)
{
    return fread(buffer, size, items, file);
}

#define fread fread_no_warn
#endif


/// Index datastructure that stores the logical position on the grid.
class CVS_IndexT
{
    public:

    /// Constructor for creating an index instance from a given x_ and y_.
    CVS_IndexT(int x_, int y_)
        : x(x_),
          y(y_)
    {
    }

    /// Named constructor.
    /// It takes an index i that is the tip (apex) of a triangle, and an index j that is the center of the base of the triangle,
    /// and returns, depending on parity and direction, the index of the left or right vertex at the center of the split edge.
    static CVS_IndexT GetSplit(CVS_IndexT i, CVS_IndexT j, bool parity, bool direction)
    {
        if (!parity ^ direction)
        {
            return CVS_IndexT((i.x+j.y + (j.x-i.y))/2,
                              (i.x+j.y - (j.x-i.y))/2);
        }
        else
        {
            return CVS_IndexT((j.x+i.y + (i.x-j.y))/2,
                              (j.x+i.y - (i.x-j.y))/2);
        }
    }

    /// Named constructor.
    /// It takes an index i that is the tip (apex) of a triangle, and an index j that is the center of the base of the triangle,
    /// and returns, depending on parity and direction, the index of the left or right base vertex.
    static CVS_IndexT GetBase(CVS_IndexT i, CVS_IndexT j, bool parity, bool direction)
    {
        if (!parity ^ direction)
        {
            return CVS_IndexT(j.x - (i.y-j.y),
                              j.y + (i.x-j.x));
        }
        else
        {
            return CVS_IndexT(j.x + (i.y-j.y),
                              j.y - (i.x-j.x));
        }
    }


    public:

    int x;
    int y;
};


/// This is the processed, ready to use vertex data.
class TerrainT::CVS_VertexT
{
    public:

    CVS_IndexT index;
    Vector3fT  position;
    float      radius;
    float      error;

    CVS_VertexT() : index(0, 0), radius(0.0f), error(0.0f) {}
    CVS_VertexT(CVS_IndexT i) : index(i), radius(0.0f), error(0.0f) {}
};


/**********************************/
/*** Common data and functions. ***/
/**********************************/

static const TerrainT::ViewInfoT* ViewInfo=NULL;
static ArrayT<Vector3fT>          VectorStrip;

static inline float  Max(const float  a, const float  b) { return a>b ? a : b; }    // TODO: Replace with std::min() and std::max().
static inline double Max(const double a, const double b) { return a>b ? a : b; }
static inline float  Min(const float  a, const float  b) { return a<b ? a : b; }
static inline double Min(const double a, const double b) { return a<b ? a : b; }

static inline int Sign(const double a)
{
    if (a> 0.00000001) return  1;
    if (a<-0.00000001) return -1;

    return 0;
}


const TerrainT::VertexT* TerrainT::GetVertices() const
{
    return &Vertices[0];
}


/**************************************/
/*** Constructor related functions. ***/
/**************************************/

/// @param i    Column index.
/// @param j    Row index.
/// @param di   Non-negative col offset to bisected edge endpoint.
/// @param dj   Row offset to bisected edge endpoint.
/// @param n    One less array width/height (zero for leaves).
inline void TerrainT::ComputeVertexLoD(unsigned long i, unsigned long j, int di, int dj, unsigned long n)
{
    VertexT& Vertex=GetVertex(i, j);

    // Compute actual error and initialize radius to zero. The error is simply the vertical difference between the vertex and the bisected edge,
    // i.e. the error between two consecutive levels of refinement. This object-space error can be replaced with any measure of error,
    // as long as the nesting step below is performed.
    Vertex.e=fabs(Vertex.z-0.5f*(GetVertex(i-di, j-dj).z+GetVertex(i+di, j+dj).z));
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
            const VertexT& Child=GetVertex(i+di, j+dj);

            Vertex.e=Max(Vertex.e, Child.e);
            Vertex.r=Max(Vertex.r, length(Vertex-Child)+Child.r);
        }

        // di' = -dj
        // dj' = +di
        dj+=di;
        di-=dj;
        dj+=di;
    }
}


/// This method loads the heightmap from file FileName.
/// It returns its width and height, and writes the normalized (0 to 1) height values into the z-components of the Vertices.
/// The Vertices array is appropriately sized, but the x-, y- and other components of the Vertices are not filled in.
/// On failure, an object of type BitmapT::LoadErrorT is thrown and the return values are undefined.
static void LoadHeightmap(const char* FileName, unsigned long& SizeX, unsigned long& SizeY, ArrayT<TerrainT::VertexT>& Vertices)
{
    if (cf::String::EndsWith(FileName, ".ter"))
    {
        // Load terrain from Terragen file.
        FILE* FilePtr=fopen(FileName, "rb");
        if (FilePtr==NULL) throw BitmapT::LoadErrorT();

        // Read header.
        char Header[20];
        fread(Header, sizeof(char), 16, FilePtr);
        Header[16] = 0;
        if (strncmp(Header, "TERRAGENTERRAIN ", 16)!=0) { fclose(FilePtr); throw BitmapT::LoadErrorT(); }   // Header=="TERRAGENTERRAIN "

        // Reset the return values.
        SizeX=0;
        SizeY=0;

        // Other values from the file.
        VectorT MetersPerPixel(30.0, 30.0, 30.0);           // The distance in meters between two neighboured pixels.

        // Read the chunks.
        while (true)
        {
            char ChunkMarker[10];
            fread(ChunkMarker, sizeof(char), 4, FilePtr);
            ChunkMarker[4] = 0;

            if (feof(FilePtr)) break;
            if (strncmp(ChunkMarker, "EOF ", 4)==0) break;

            if (strncmp(ChunkMarker, "XPTS", 4)==0)
            {
                short int XPts;
                fread(&XPts, sizeof(XPts), 1, FilePtr);     // Read two bytes.
                SizeX=XPts;                                 // Let the XPTS chunk always override previous values (it appears only after SIZE anyway).
                fread(&XPts, sizeof(XPts), 1, FilePtr);     // Overread padding bytes.
            }
            else if (strncmp(ChunkMarker, "YPTS", 4)==0)
            {
                short int YPts;
                fread(&YPts, sizeof(YPts), 1, FilePtr);     // Read two bytes.
                SizeY=YPts;                                 // Let the YPTS chunk always override previous values (it appears only after SIZE anyway).
                fread(&YPts, sizeof(YPts), 1, FilePtr);     // Overread padding bytes.
            }
            else if (strncmp(ChunkMarker, "SIZE", 4)==0)
            {
                short int Size;
                fread(&Size, sizeof(Size), 1, FilePtr);     // Read two bytes.
                SizeX=Size+1;                               // Note that the XPTS and YPTS chunks appear *after* the SIZE chunk.
                SizeY=Size+1;
                fread(&Size, sizeof(Size), 1, FilePtr);     // Overread padding bytes.
            }
            else if (strncmp(ChunkMarker, "SCAL", 4)==0)
            {
                float Scale;
                fread(&Scale, sizeof(Scale), 1, FilePtr); MetersPerPixel.x=Scale;
                fread(&Scale, sizeof(Scale), 1, FilePtr); MetersPerPixel.y=Scale;
                fread(&Scale, sizeof(Scale), 1, FilePtr); MetersPerPixel.z=Scale;
            }
            else if (strncmp(ChunkMarker, "CRAD", 4)==0)
            {
                float PlanetRadius;
                fread(&PlanetRadius, sizeof(PlanetRadius), 1, FilePtr);     // This value is ignored.
            }
            else if (strncmp(ChunkMarker, "CRVM", 4)==0)
            {
                unsigned long RenderMode;
                fread(&RenderMode, sizeof(RenderMode), 1, FilePtr);         // This value is ignored.
            }
            else if (strncmp(ChunkMarker, "ALTW", 4)==0)
            {
                short int HeightScale;
                fread(&HeightScale, sizeof(HeightScale), 1, FilePtr);
                short int BaseHeight;
                fread(&BaseHeight, sizeof(BaseHeight), 1, FilePtr);

                Vertices.Overwrite();
                Vertices.PushBackEmptyExact(SizeX*SizeY);

                for (unsigned long y=0; y<SizeY; y++)
                    for (unsigned long x=0; x<SizeX; x++)
                    {
                        short int Elevation;
                        fread(&Elevation, sizeof(Elevation), 1, FilePtr);

                     // Vertices[y*SizeX+x].z=double(BaseHeight)+double(Elevation)*double(HeightScale)/65536.0;
                        Vertices[y*SizeX+x].z=(float(Elevation)+32768.0f)/65536.0f; // Just bring between 0 and 1, no need to relate this to the per-pixel distance.
                    }

                // Break here, because we're done *and* unspecified chunks like the THMB thumbnail-chunk might follow.
                // As I don't know how to deal with such chunks (where are the specs anyway?), just stop here.
                break;
            }
            else
            {
                // Unknown chunk?
                fclose(FilePtr);
                throw BitmapT::LoadErrorT();
            }
        }

        fclose(FilePtr);
    }
    else if (cf::String::EndsWith(FileName, ".pgm"))
    {
        try
        {
            // Load terrain from Portable Gray-Map file (only the ASCII variant).
            TextParserT TP(FileName, "", true, '#');

            std::string MagicNumber=TP.GetNextToken();
            bool        IsAscii    =(MagicNumber=="P2");

            // If it's not P2 and not P5, something is wrong.
            if (!IsAscii && MagicNumber!="P5") throw TextParserT::ParseError();

            SizeX             =atoi(TP.GetNextToken().c_str());
            SizeY             =atoi(TP.GetNextToken().c_str());
            const float MaxVal=float(atof(TP.GetNextToken().c_str()));

            Vertices.Overwrite();
            Vertices.PushBackEmptyExact(SizeX*SizeY);

            if (IsAscii)
            {
                // It's the P2 ascii type.
                for (unsigned long y=0; y<SizeY; y++)
                    for (unsigned long x=0; x<SizeX; x++)
                        Vertices[(SizeY-y-1)*SizeX+x].z=float(atof(TP.GetNextToken().c_str()))/MaxVal;
            }
            else
            {
                // It's the P5 binary type.
                FILE* FilePtr=fopen(FileName, "rb");
                if (FilePtr==NULL) throw BitmapT::LoadErrorT();

                // Assume that after the last text token (the maximum value) exactly *one* byte of white-space (e.g. '\r' or '\n') follows.
                fseek(FilePtr, TP.GetReadPosByte()+1, SEEK_SET);

                for (unsigned long y=0; y<SizeY; y++)
                    for (unsigned long x=0; x<SizeX; x++)
                    {
                        unsigned char Value;
                        fread(&Value, sizeof(Value), 1, FilePtr);   // This probably slow as hell, but alas! Who cares?

                        Vertices[(SizeY-y-1)*SizeX+x].z=Value/MaxVal;
                    }

                fclose(FilePtr);
            }
        }
        catch (const TextParserT::ParseError& /*PE*/)
        {
            throw BitmapT::LoadErrorT();
        }
    }
    else
    {
        // Load terrain from image file.
        BitmapT HeightMap=BitmapT(FileName);

        SizeX=HeightMap.SizeX;
        SizeY=HeightMap.SizeY;

        Vertices.Overwrite();
        Vertices.PushBackEmptyExact(SizeX*SizeY);

        // Note that we pick the red channel of the RBG HeightMap.Data as the relevant channel.
        // Moreover, note that the y-axis of the HeightMap.Data points down in screen-space and thus towards us in world space.
        // Our world-space y-axis points opposite (away from us), though, and therefore we access the HeightMap.Data at (i, Size-j-1).
        for (unsigned long y=0; y<SizeY; y++)
            for (unsigned long x=0; x<SizeX; x++)
                Vertices[y*SizeX+x].z=float(HeightMap.Data[(SizeY-y-1)*SizeX+x] & 0xFF)/255.0f;
    }
}


void TerrainT::Init(const char* FileName, const BoundingBox3fT* BB_, const Vector3fT* Resolution, bool FailSafe)
{
    if (FileName!=NULL)
    {
        // The try-block properly sizes the Vertices array, fills in the z-values in range 0 to 1,
        // and initializes the Levels and Size members.
        try
        {
            unsigned long SizeX;
            unsigned long SizeY;

            LoadHeightmap(FileName, SizeX, SizeY, Vertices);

            // Compute Levels and Size, and thereby perform some validity checks.
            if (SizeX!=SizeY) throw BitmapT::LoadErrorT();
            if (SizeX<3) throw BitmapT::LoadErrorT();
            if (SizeX>(1UL << 30/2)+1) throw BitmapT::LoadErrorT();

            Levels=1;
            while ((1UL << Levels)+1<SizeX) Levels++;
            Levels*=2;

            Size=(1UL << (Levels/2))+1;

            if (Size!=SizeX) throw BitmapT::LoadErrorT();
        }
        catch (const BitmapT::LoadErrorT& /*LE*/)
        {
            if (!FailSafe) throw InitError();

            Levels=2;
            Size  =3;

            Vertices.Clear();
            Vertices.PushBackEmptyExact(Size*Size);
            for (unsigned long VertexNr=0; VertexNr<Vertices.Size(); VertexNr++) Vertices[VertexNr].z=0.0;
        }
    }


    // The LoadHeightmap() function above has already properly sized the Vertices array and filled in the z-components.
    // Now set the BB and compute the other components (x, y, e, and r).
    for (unsigned long j=0; j<Size; j++)
        for (unsigned long i=0; i<Size; i++)
            if (BB_)
            {
                if (i==0 && j==0) BB=*BB_;

                GetVertex(i, j).x=BB.Min.x+(BB.Max.x-BB.Min.x)*float(i)/float(Size-1);
                GetVertex(i, j).y=BB.Min.y+(BB.Max.y-BB.Min.y)*float(j)/float(Size-1);
                GetVertex(i, j).z=BB.Min.z+(BB.Max.z-BB.Min.z)*GetVertex(i, j).z;
            }
            else
            {
                GetVertex(i, j).x=Resolution->x*(i-0.5f*(Size-1));
                GetVertex(i, j).y=Resolution->y*(j-0.5f*(Size-1));
                GetVertex(i, j).z=Resolution->z*GetVertex(i, j).z;

                if (i==0 && j==0) BB=BoundingBox3fT(GetVertex(i, j));
                             else BB.Insert(GetVertex(i, j));
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
    GetVertex(0, 0).e=GetVertex(n, 0).e=GetVertex(0, n).e=GetVertex(n, n).e=GetVertex(n/2, n/2).e=float(HUGE_VAL);   // Fixme: Use FLT_MAX instead??
    GetVertex(0, 0).r=GetVertex(n, 0).r=GetVertex(0, n).r=GetVertex(n, n).r=GetVertex(n/2, n/2).r=float(HUGE_VAL);   // Fixme: Use FLT_MAX instead??


    // Build the quad-tree structure.
#if 1   // This is entirely optional!
    const unsigned long NrOfNodes=(std::min(4*(Size-1)*(Size-1), 256ul*256ul)-1)/3;   // For the rationale of this expression see below.

    // Allocate all QuadTree memory at once, saving the all the reallocs and copies that are associated with growing it in steps.
    QuadTree.PushBackEmptyExact(NrOfNodes);
    QuadTree.Overwrite();
#endif

    // The last element of the array will become the root of the tree.
    QuadTree.PushBack(QuadTreeT(*this, 0, (Size-1)*(Size-1)-1, 0));

    // Q: How is the number of nodes of the QuadTree computed?
    // A: First, note that the number of cells of our QuadTree in each direction is one less than the number of heightmap elements.
    //    E.g. if there are 257*257 heightmap elements, the QuadTree will have 256*256 cells, i.e. Size-1.
    //    Next, the number of levels in the QuadTree is n+1, with 2^n == Size-1, that is, n == log2(Size-1).
    //    The number of nodes therefore is 4^0 + 4^1 + 4^2 + 4^3 + ... + 4^n.
    //    The closed form for such a sum from 0 to n is (4^(n+1) - 1)/3. More generally, see
    //      http://de.wikipedia.org/wiki/Formelsammlung_Algebra#Summenformeln and
    //      http://de.wikipedia.org/wiki/Geometrische_Reihe
    //    Substituting log2(Size-1) for n then yields the above expression for NrOfNodes.
    //    (Note that the QuadTreeT ctor has a built-in maximum-number-of-levels limit that is taken into account by the (256*256-1)/3 subexpression.)
    assert(QuadTree.Size()==NrOfNodes);
}


TerrainT::TerrainT()
    : mCVS_Side(true),
      mCVS_Bottom(false),
      mCVS_LeftOnly(false),
      mCVS_First(true),
      mCVS_CurrentLevel(0)
{
    const Vector3fT Resolution(1.0f, 1.0f, 1.0f);

    Init("", NULL, &Resolution, true);
}


TerrainT::TerrainT(const char* FileName, const BoundingBox3fT& BB_, bool FailSafe)
    : mCVS_Side(true),
      mCVS_Bottom(false),
      mCVS_LeftOnly(false),
      mCVS_First(true),
      mCVS_CurrentLevel(0)
{
    Init(FileName, &BB_, NULL, FailSafe);
}


TerrainT::TerrainT(const char* FileName, const Vector3fT& Resolution, bool FailSafe)
    : mCVS_Side(true),
      mCVS_Bottom(false),
      mCVS_LeftOnly(false),
      mCVS_First(true),
      mCVS_CurrentLevel(0)
{
    Init(FileName, NULL, &Resolution, FailSafe);
}


TerrainT::TerrainT(const unsigned short* HeightData, unsigned long SideLength, const BoundingBox3fT& BB_) /*throw (InitError)*/
    : mCVS_Side(true),
      mCVS_Bottom(false),
      mCVS_LeftOnly(false),
      mCVS_First(true),
      mCVS_CurrentLevel(0)
{
    assert(HeightData!=NULL);
    Vertices.PushBackEmptyExact(SideLength*SideLength);

    for (unsigned long VertexNr=0; VertexNr<Vertices.Size(); VertexNr++)
        Vertices[VertexNr].z=float(HeightData[VertexNr])/65535.0f;

    Levels=1;
    while ((1UL << Levels)+1<SideLength) Levels++;
    Levels*=2;

    Size=(1UL << (Levels/2))+1;

    if (Size!=SideLength) throw InitError();

    // Set the BB and compute other components.
    Init(NULL, &BB_, NULL, false);
}


void TerrainT::UpdateHeights(const unsigned short* HeightData, unsigned long PosX, unsigned long PosY, unsigned long SizeX, unsigned long SizeY)
{
    const unsigned long EndX =PosX+SizeX;
    const unsigned long EndY =PosY+SizeY;
    const float         Scale=(BB.Max.z-BB.Min.z)/65535.0f;

    for (unsigned long y=PosY; y<EndY; y++)
        for (unsigned long x=PosX; x<EndX; x++)
            GetVertex(x, y).z=BB.Min.z+float(HeightData[x+Size*y])*Scale;
}


/**********************************************/
/*** Terrain simplification implementation. ***/
/**********************************************/

ArrayT<Vector3fT>& TerrainT::ComputeVectorStrip(const TerrainT::ViewInfoT& VI) const
{
    VectorStrip.Overwrite();
    ViewInfo=&VI;

    CVS_RefineTop();

    ViewInfo=NULL;
    return VectorStrip;
}


void TerrainT::CVS_GetVertex(CVS_VertexT& v) const
{
    const TerrainT::VertexT& v0=GetVertex(v.index.x, v.index.y);

    v.position.x=BB.Min.x + (BB.Max.x-BB.Min.x)*float(v.index.x)/float(Size-1);
    v.position.y=BB.Min.y + (BB.Max.y-BB.Min.y)*float(v.index.y)/float(Size-1);
    v.position.z=v0.z;
    v.radius    =v0.r;
    v.error     =v0.e;
}


// Insert a vertex into the vertex array.
void TerrainT::CVS_Append(const CVS_VertexT& v) const
{
    VectorStrip.PushBack(v.position);
}


// Inserts a degenerate triange into the strip.
// This only requires sending one index.
void TerrainT::CVS_TurnCorner() const
{
    // index_array[ia_index++] = index_array[ia_index-2];
    assert(VectorStrip.Size()>=2);
    VectorStrip.PushBack(VectorStrip[VectorStrip.Size()-2]);
}


// Decides if CVS_VertexT v should be in the mesh, or not.
// In order to make this decision, it has to calculate its position and the 2D/3D projected error.
// Hierarchical view-frustum culling is also performed here.
bool TerrainT::CVS_Active(CVS_VertexT& v, float zEdge, unsigned long& planes) const
{
    const unsigned long clip_planes[]={ 1, 2, 4, 8, 16, 32 };

    // Reject vertices after max level (end recursion).
    if (mCVS_CurrentLevel+1==Levels) return false;

    // Calculate real 2D position of vertex.
    const TerrainT::VertexT& v0=GetVertex(v.index.x, v.index.y);

    v.position.x=BB.Min.x + (BB.Max.x-BB.Min.x)*float(v.index.x)/float(Size-1);
    v.position.y=BB.Min.y + (BB.Max.y-BB.Min.y)*float(v.index.y)/float(Size-1);
    v.position.z=v0.z;
    v.radius    =v0.r;
    v.error     =v0.e;

    // Avoid culling if bounding sphere is already outside frustum.
    if (planes)
    {
        const float radius_boost=450.0f;    // Ad hoc constant for lazy frustum culling.
        const float radius=v.radius + radius_boost;

        // For each frustum plane.
        for (int i=0; i<5; i++)     // CF: 5 planes only, ViewInfoT doesn't come with the "far" plane.
        {
            // Cull against plane only if not outside.
            if (planes & clip_planes[i])
            {
                const float dp = -ViewInfo->viewplanes[i].GetDistance(v.position);

                if (dp < -radius) return false;
                if (dp >  radius) planes ^= clip_planes[i];
            }
        }
    }

#if 0
    /* Refinement only - no geo-morphing. */
    // Check projected error.
    const Vector3fT Delta=ViewInfo->viewpoint - v.position;
    const float     dist2=dot(Delta, Delta);    // 3D distance squared.
    const float     o    =ViewInfo->nu * v.error + v.radius;

    return o*o>dist2;
#else
    /* Employ geo-morphing for eliminating the popping effect. */
    // Compute the elevation of the morphed vertex.
    const Vector3fT View =v.position - ViewInfo->viewpoint;
    const float     d    =dot(View, View);  // d=length(v.position - ViewInfo->viewpoint)^2
    const float     dmax_=ViewInfo->nu_max * v.error + v.radius;
    const float     dmax =dmax_*dmax_;

    if (dmax<=d) return false;

    const float     dmin_=ViewInfo->nu_min * v.error + v.radius;
    const float     dmin =dmin_*dmin_;

    v.position.z=dmin>d ? v0.z : ((dmax-d)*v0.z + (d-dmin)*zEdge)/(dmax-dmin);
    return true;
#endif
}


// Refines the top level of the mesh.
void TerrainT::CVS_RefineTop() const
{
    CVS_VertexT BaseVertices[13];

    const int c4=int(Size-1);
    const int c2=c4 >> 1;
    const int c1=c2 >> 1;
    const int c3=c2 + c1;

    BaseVertices[ 0].index=CVS_IndexT( 0, c4);
    BaseVertices[ 1].index=CVS_IndexT(c4, c4);
    BaseVertices[ 2].index=CVS_IndexT(c4,  0);
    BaseVertices[ 3].index=CVS_IndexT( 0,  0);
    BaseVertices[ 4].index=CVS_IndexT(c2, c2);
    BaseVertices[ 5].index=CVS_IndexT(c2,  0);
    BaseVertices[ 6].index=CVS_IndexT(c4, c2);
    BaseVertices[ 7].index=CVS_IndexT(c2, c4);
    BaseVertices[ 8].index=CVS_IndexT( 0, c2);
    BaseVertices[ 9].index=CVS_IndexT(c3, c1);
    BaseVertices[10].index=CVS_IndexT(c1, c1);
    BaseVertices[11].index=CVS_IndexT(c1, c3);
    BaseVertices[12].index=CVS_IndexT(c3, c3);

    for (unsigned long i=0; i<13; i++)
        CVS_GetVertex(BaseVertices[i]);


    mCVS_First = true;

    mCVS_CurrentLevel = 1;
    CVS_Append(BaseVertices[3]);
    CVS_Append(BaseVertices[3]);

    mCVS_LeftOnly = false;
    CVS_Refine(BaseVertices[8], BaseVertices[10], BaseVertices[3].position.z, BaseVertices[4].position.z, mCVS_Bottom, mCVS_Side, 63);
    CVS_Append(BaseVertices[4]);
    mCVS_LeftOnly = false;
    CVS_Refine(BaseVertices[8], BaseVertices[11], BaseVertices[4].position.z, BaseVertices[0].position.z, mCVS_Side, mCVS_Bottom, 63);
    CVS_Append(BaseVertices[0]);

    mCVS_LeftOnly = false;
    CVS_Refine(BaseVertices[7], BaseVertices[11], BaseVertices[0].position.z, BaseVertices[4].position.z, mCVS_Bottom, mCVS_Side, 63);
    CVS_Append(BaseVertices[4]);
    mCVS_LeftOnly = false;
    CVS_Refine(BaseVertices[7], BaseVertices[12], BaseVertices[4].position.z, BaseVertices[1].position.z, mCVS_Side, mCVS_Bottom, 63);
    CVS_Append(BaseVertices[1]);

    mCVS_LeftOnly = false;
    CVS_Refine(BaseVertices[6], BaseVertices[12], BaseVertices[1].position.z, BaseVertices[4].position.z, mCVS_Bottom, mCVS_Side, 63);
    CVS_Append(BaseVertices[4]);
    mCVS_LeftOnly = false;
    CVS_Refine(BaseVertices[6], BaseVertices[ 9], BaseVertices[4].position.z, BaseVertices[2].position.z, mCVS_Side, mCVS_Bottom, 63);
    CVS_Append(BaseVertices[2]);

    mCVS_LeftOnly = false;
    CVS_Refine(BaseVertices[5], BaseVertices[ 9], BaseVertices[2].position.z, BaseVertices[4].position.z, mCVS_Bottom, mCVS_Side, 63);
    CVS_Append(BaseVertices[4]);
    mCVS_LeftOnly = false;
    CVS_Refine(BaseVertices[5], BaseVertices[10], BaseVertices[4].position.z, BaseVertices[3].position.z, mCVS_Side, mCVS_Bottom, 63);
    CVS_Append(BaseVertices[3]);
}


// Left-only refinement is not tail-recursive.
void TerrainT::CVS_RefineL(const CVS_VertexT& i, const CVS_VertexT& j, float zA, bool in, unsigned long planes) const
{
    ++mCVS_CurrentLevel;
    CVS_VertexT left(CVS_IndexT::GetSplit(i.index, j.index, (mCVS_CurrentLevel & 1), false));

    // The assert() in the next line can only succeed when geo-morphing in CVS_Active() is *not* used!
    // const CVS_IndexT A=CVS_IndexT::GetBase(i.index, j.index, (mCVS_CurrentLevel & 1), false); assert(GetVertex(A.x, A.y).z == zA);

    if (CVS_Active(left, (i.position.z+zA)/2.0f, planes))
    {
        CVS_RefineL(j, left, zA, !in, planes);
    }
    else
    {
        if (mCVS_First)
        {
            CVS_Append(in==mCVS_Side ? i : j);
            mCVS_First = false;
        }
        else
        {
            CVS_TurnCorner();
        }

        if (in==mCVS_Side)
        {
            CVS_Append(j);
        }
    }

    CVS_Append(i);
    --mCVS_CurrentLevel;
}


// Right-only refinement is tail-recursive, so it can be converted to a simple loop.
// This eliminates function call and stack maintenance overhead.
void TerrainT::CVS_RefineR(CVS_VertexT i, CVS_VertexT j, float zB, bool out, unsigned long planes) const
{
    const unsigned long level_save=mCVS_CurrentLevel;
    CVS_VertexT right;
    bool is_right;

    do
    {
        ++mCVS_CurrentLevel;
        CVS_Append(j);
        right.index = CVS_IndexT::GetSplit(i.index, j.index, (mCVS_CurrentLevel & 1), true);

        // The assert() in the next line can only succeed when geo-morphing in CVS_Active() is *not* used!
        // const CVS_IndexT B=CVS_IndexT::GetBase(i.index, j.index, (mCVS_CurrentLevel & 1), true ); assert(GetVertex(B.x, B.y).z == zB);

        is_right = CVS_Active(right, (i.position.z+zB)/2.0f, planes);
        i  =j;
        j  =right;
     // zB =zB;
        out=!out;
    } while (is_right);

    if (out==mCVS_Bottom)
    {
        CVS_TurnCorner();
    }

    mCVS_CurrentLevel = level_save;
}


// General refinement procedure.
void TerrainT::CVS_Refine(const CVS_VertexT& i, const CVS_VertexT& j, float zA, float zB, bool in, bool out, unsigned long planes) const
{
    ++mCVS_CurrentLevel;

    // Check both left and right child nodes.
    CVS_VertexT left(CVS_IndexT::GetSplit(i.index, j.index, (mCVS_CurrentLevel & 1), false));
    CVS_VertexT right(CVS_IndexT::GetSplit(i.index, j.index, (mCVS_CurrentLevel & 1), true));
    unsigned long left_planes = planes;
    unsigned long right_planes = planes;
    const bool is_left =CVS_Active(left,  (i.position.z+zA)/2.0f, left_planes );
    const bool is_right=CVS_Active(right, (i.position.z+zB)/2.0f, right_planes);

    // The assert() in the next lines can only succeed when geo-morphing in CVS_Active() is *not* used!
    // const CVS_IndexT A=CVS_IndexT::GetBase(i.index, j.index, (mCVS_CurrentLevel & 1), false); assert(GetVertex(A.x, A.y).z == zA);
    // const CVS_IndexT B=CVS_IndexT::GetBase(i.index, j.index, (mCVS_CurrentLevel & 1), true ); assert(GetVertex(B.x, B.y).z == zB);

    if (is_left)
    {
        if (is_right)
        {
            // left - right
            CVS_Refine(j, left, zA, i.position.z, !in, mCVS_Side, left_planes);
            CVS_Append(i);

            if (mCVS_LeftOnly)
            {
                CVS_Append(j);
                CVS_RefineR(j, right, zB, !out, right_planes);
            }
            else
            {
                CVS_Refine(j, right, i.position.z, zB, mCVS_Side, !out, right_planes);
            }
            mCVS_LeftOnly = false;
        }
        else
        {
            // left only
            mCVS_LeftOnly = true;
            CVS_RefineL(j, left, zA, !in, left_planes);
            CVS_Append(i);

            if (out==mCVS_Bottom)
            {
                CVS_Append(j);
            }
        }
    }
    else
    {
        if (mCVS_First)
        {
            CVS_Append(in==mCVS_Side ? i : j);
            mCVS_First = false;
        }
        else
        {
            CVS_TurnCorner();
        }

        if (is_right)
        {
            // right only
            if (in==mCVS_Bottom)
            {
                // in::bottom - out:side
                CVS_Append(i);
            }

            CVS_Append(j);
            CVS_RefineR(j, right, zB, !out, right_planes);
        }
        else
        {
            // none
            if (in==mCVS_Side)
            {
                // in:side - out:?
                CVS_Append(j);

                if (out==mCVS_Side)
                {
                    // in:side - out:side
                    CVS_Append(i);
                }
            }
            else
            {
                // in:bottom - out:side
                CVS_Append(i);
            }
        }
    }

    --mCVS_CurrentLevel;
}


/*************************************************************/
/*** Global stuff used both by Refine and Morph functions. ***/
/*************************************************************/

#define LINEAR_INDEX(i, j, m)   ((i) + (j) + ((j) << (m)))
#define LINEAR_SPLIT(i, j, k)   (((j) + (k)) / 2)

// Standard row-major (linear) matrix layout.
#define I_SW(m)             LINEAR_INDEX(0 << (m), 0 << (m), m)
#define I_SE(m)             LINEAR_INDEX(1 << (m), 0 << (m), m)
#define I_NE(m)             LINEAR_INDEX(1 << (m), 1 << (m), m)
#define I_NW(m)             LINEAR_INDEX(0 << (m), 1 << (m), m)
#define I_C(m)              LINEAR_INDEX(1 << ((m) - 1), 1 << ((m) - 1), m)
#define ROOT_S(m)           I_C(m), I_SW(m), I_SE(m)
#define ROOT_E(m)           I_C(m), I_SE(m), I_NE(m)
#define ROOT_N(m)           I_C(m), I_NE(m), I_NW(m)
#define ROOT_W(m)           I_C(m), I_NW(m), I_SW(m)
#define TRIANGLE(i, j, k)   i, j, k
#define SPLIT(i, j, k)      LINEAR_SPLIT(i, j, k)
#define CHILD_L(i, j, k)    LINEAR_SPLIT(i, j, k), j, i
#define CHILD_R(i, j, k)    LINEAR_SPLIT(i, j, k), i, k

const unsigned long SPHERE_MASK_UNDECIDED=0x20;     // 0010 0000   initial visibility mask
const unsigned long SPHERE_MASK_VISIBLE  =0x3F;     // 0011 1111   guaranteed visible

typedef unsigned long VERTEXid;


/// @param Vertex Vertex.
/// @param mask   Visibility mask.
static unsigned long IsSphereVisible(const TerrainT::VertexT& Vertex, unsigned long mask)
{
    // Compare the radius of the vertex's bounding sphere against the signed distance to each plane of the view volume.
    // If the sphere is completely on the exterior side of a plane, it is invisible.
    // Otherwise, if it is entirely on the interior side, then a flag is set for this plane, and no further
    // tests are made between this plane and the vertex's descendants since the bounding spheres are nested.
    for (unsigned long i=0; i<5; i++)
        if (!(mask & (1u << i)))
        {
            const float d=float(ViewInfo->viewplanes[i].GetDistance(Vertex));

            if (d> Vertex.r) return 0;       // Completely outside view volume.
            if (d<-Vertex.r) mask|=1u << i;  // Completely on interior side of view plane.
        }

    return mask;
}


/*********************************/
/*** Refinement related stuff. ***/
/*********************************/

static bool Refine_IsVertexActive(const TerrainT::VertexT& Vertex)
{
    const float d=float(length(Vertex-ViewInfo->viewpoint));
    const float f=ViewInfo->nu*Vertex.e+Vertex.r;

    return f*f>d*d;
}


static ArrayT<VERTEXid> IndexStrip;
static bool             IndexStripParity;


/// @param VertexIndex   Index of vertex to append.
/// @param Parity        Parity of vertex.
void TerrainT::Refine_AppendStripIndex(unsigned long VertexIndex, bool Parity) const
{
    const VERTEXid tail=IndexStrip[IndexStrip.Size()-2];
    const VERTEXid head=IndexStrip[IndexStrip.Size()-1];

    // Add vertex to end of triangle strip vertex buffer.
    if (VertexIndex!=tail && VertexIndex!=head)
    {
        if (Parity==IndexStripParity) IndexStrip.PushBack(tail);    // turn corner; duplicate vertex
        IndexStrip.PushBack(VertexIndex);                           // append new vertex
        IndexStripParity=Parity;                                    // store parity here for easy access

        // TriangleCount++;                                         // Count triangles for statistics.
    }
}


/// @param l   Refinement level.
/// @param i   Triangle apex.
/// @param j   Supplemental vertex #1.
/// @param k   Supplemental vertex #2.
void TerrainT::Refine_SubMesh(unsigned long l, TRIANGLE(VERTEXid i, VERTEXid j, VERTEXid k)) const
{
    const bool refine=(l!=0) && Refine_IsVertexActive(Vertices[SPLIT(i, j, k)]);

    // Recursively refine the mesh.  Since the refinement condition is the
    // same for both branches and can be somewhat expensive to evaluate,
    // it is evaluated and tested *before* making the recursive calls.
    if (refine) Refine_SubMesh(l-1, CHILD_L(i, j, k));
    Refine_AppendStripIndex(i, l & 1);
    if (refine) Refine_SubMesh(l-1, CHILD_R(i, j, k));
}


/// @param l   Refinement level.
/// @param i   Triangle apex.
/// @param j   Supplemental vertex #1.
/// @param k   Supplemental vertex #2.
/// @param m   Visibility mask.
void TerrainT::Refine_SubMeshVisible(unsigned long l, TRIANGLE(VERTEXid i, VERTEXid j, VERTEXid k), unsigned long m) const
{
    // Recursively refine and cull the mesh.
    // If the sphere is contained inside the view volume, then transition to
    // Refine_SubMesh() and make no further view culling tests. Otherwise, continue culling.
    if (m==SPHERE_MASK_VISIBLE)
    {
        Refine_SubMesh(l, TRIANGLE(i, j, k));
        return;
    }

    m=IsSphereVisible(Vertices[SPLIT(i, j, k)], m);

    const bool refine=(l!=0) && Refine_IsVertexActive(Vertices[SPLIT(i, j, k)]) && m;

    if (refine) Refine_SubMeshVisible(l-1, CHILD_L(i, j, k), m);
    Refine_AppendStripIndex(i, l & 1);
    if (refine) Refine_SubMeshVisible(l-1, CHILD_R(i, j, k), m);
}


ArrayT<unsigned long>& TerrainT::ComputeIndexStripByRefinement(const ViewInfoT& VI) const
{
    const unsigned long m=VI.cull ? SPHERE_MASK_UNDECIDED : SPHERE_MASK_VISIBLE;

    ViewInfo=&VI;

    // Initialize index strip.
    IndexStrip.Overwrite();
    IndexStrip.PushBack(I_SW(Levels/2));
    IndexStrip.PushBack(I_SW(Levels/2));
    IndexStripParity=1;

    // Top-level function for constructing an indexed mesh.
    Refine_SubMeshVisible(Levels-1, ROOT_S(Levels/2), m); Refine_AppendStripIndex(I_SE(Levels/2), 0);
    Refine_SubMeshVisible(Levels-1, ROOT_E(Levels/2), m); Refine_AppendStripIndex(I_NE(Levels/2), 0);
    Refine_SubMeshVisible(Levels-1, ROOT_N(Levels/2), m); Refine_AppendStripIndex(I_NW(Levels/2), 0);
    Refine_SubMeshVisible(Levels-1, ROOT_W(Levels/2), m); IndexStrip.PushBack    (I_SW(Levels/2)   );

    return IndexStrip;
}


/*******************************/
/*** Morphing related stuff. ***/
/*******************************/

/// @param zmp    Pointer to morphed elevation.
/// @param Vertex Vertex.
/// @param zl     Elevation of left endpoint of split edge.
/// @param zr     Elevation of right endpoint of split edge.
static bool Morph_IsVertexActive(float& zmp, const TerrainT::VertexT& Vertex, const float zl, const float zr)
{
    // Compute the elevation of the morphed vertex. The return value indicates whether the vertex is active or not.
    const Vector3fT View =Vertex-ViewInfo->viewpoint;
    const float     d    =dot(View, View);  // d=length(Vertex-ViewInfo->viewpoint)^2
    const float     dmax_=ViewInfo->nu_max*Vertex.e+Vertex.r;
    const float     dmax =dmax_*dmax_;

    if (dmax<=d) return false;

    const float     dmin_=ViewInfo->nu_min*Vertex.e+Vertex.r;
    const float     dmin =dmin_*dmin_;

    zmp=dmin>d ? Vertex.z : ((dmax-d)*Vertex.z+(d-dmin)*0.5f*(zl+zr))/(dmax-dmin);
    return true;
}


static VERTEXid VectorStripHead;       // ID of most recent vertex.
static VERTEXid VectorStripTail;       // ID of second most recent vertex.
static bool     VectorStripParity;     // Parity of most recent vertex.


/// @param VertexIndex   Index of vertex to append.
/// @param Parity        Parity of vertex.
/// @param z             Elevation of morphed vertex.
void TerrainT::Morph_AppendStripVector(unsigned long VertexIndex, bool Parity, float z) const
{
    if (VertexIndex!=VectorStripTail && VertexIndex!=VectorStripHead)
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

        VectorStripHead=VertexIndex;

        // append new vertex
        VectorStrip.PushBack(Vector3fT(Vertices[VertexIndex].x, Vertices[VertexIndex].y, z));

        // TriangleCount++;                                         // Count triangles for statistics.
    }
}


/// @param l    Refinement level.
/// @param i    Triangle apex.
/// @param j    Supplemental vertex #1.
/// @param k    Supplemental vertex #2.
/// @param za   Elevation of apex.
/// @param zl   Elevation of left corner.
/// @param zr   Elevation of right corner.
void TerrainT::Morph_SubMesh(unsigned long l, TRIANGLE(unsigned long i, unsigned long j, unsigned long k), float za, float zl, float zr) const
{
    float      zm=0.0f;
    const bool refine=(l!=0) && Morph_IsVertexActive(zm, Vertices[SPLIT(i, j, k)], zl, zr);

    // Recursively refine and morph the mesh.
    if (refine) Morph_SubMesh(l-1, CHILD_L(i, j, k), zm, zl, za);
    Morph_AppendStripVector(i, l & 1, za);
    if (refine) Morph_SubMesh(l-1, CHILD_R(i, j, k), zm, za, zr);
}


/// @param l    Refinement level.
/// @param i    Triangle apex.
/// @param j    Supplemental vertex #1.
/// @param k    Supplemental vertex #2.
/// @param za   Elevation of apex.
/// @param zl   Elevation of left corner.
/// @param zr   Elevation of right corner.
/// @param m    Visibility mask.
void TerrainT::Morph_SubMeshVisible(unsigned long l, TRIANGLE(unsigned long i, unsigned long j, unsigned long k), float za, float zl, float zr, unsigned long m) const
{
    // Recursively refine, morph, and cull the mesh.
    // If the sphere is contained inside the view volume, then transition to
    // Morph_SubMesh() and make no further view culling tests. Otherwise, continue culling.
    if (m==SPHERE_MASK_VISIBLE)
    {
        Morph_SubMesh(l, TRIANGLE(i, j, k), za, zl, zr);
        return;
    }

    m=IsSphereVisible(Vertices[SPLIT(i, j, k)], m);

    float      zm=0.0f;
    const bool refine=(l!=0) && Morph_IsVertexActive(zm, Vertices[SPLIT(i, j, k)], zl, zr) && m;

    if (refine) Morph_SubMeshVisible(l-1, CHILD_L(i, j, k), zm, zl, za, m);
    Morph_AppendStripVector(i, l & 1, za);
    if (refine) Morph_SubMeshVisible(l-1, CHILD_R(i, j, k), zm, za, zr, m);
}


ArrayT<Vector3fT>& TerrainT::ComputeVectorStripByMorphing(const ViewInfoT& VI) const
{
    const unsigned long m=VI.cull ? SPHERE_MASK_UNDECIDED : SPHERE_MASK_VISIBLE;

    ViewInfo=&VI;

    const float zc =Vertices[I_C (Levels/2)].z;
    const float zsw=Vertices[I_SW(Levels/2)].z;
    const float zse=Vertices[I_SE(Levels/2)].z;
    const float zne=Vertices[I_NE(Levels/2)].z;
    const float znw=Vertices[I_NW(Levels/2)].z;

    // Initialize vector strip.
    VectorStrip.Overwrite();
    VectorStrip.PushBack(Vertices[I_SW(Levels/2)]); VectorStripTail=I_SW(Levels/2);
    VectorStrip.PushBack(Vertices[I_SW(Levels/2)]); VectorStripHead=I_SW(Levels/2);
    VectorStripParity=1;

    // Top-level function for constructing a morphed mesh.
    Morph_SubMeshVisible(Levels-1, ROOT_S(Levels/2), zc, zsw, zse, m); Morph_AppendStripVector(I_SE(Levels/2), 0, zse);
    Morph_SubMeshVisible(Levels-1, ROOT_E(Levels/2), zc, zse, zne, m); Morph_AppendStripVector(I_NE(Levels/2), 0, zne);
    Morph_SubMeshVisible(Levels-1, ROOT_N(Levels/2), zc, zne, znw, m); Morph_AppendStripVector(I_NW(Levels/2), 0, znw);
    Morph_SubMeshVisible(Levels-1, ROOT_W(Levels/2), zc, znw, zsw, m); VectorStrip.PushBack(Vertices[I_SW(Levels/2)]);

    return VectorStrip;
}


/***************************/
/*** Clipping functions. ***/
/***************************/

void TerrainT::TraceBoundingBox(const BoundingBox3T<double>& TraceBB, const VectorT& Origin, const VectorT& Dir, VB_Trace3T<double>& Trace) const
{
    QuadTree[QuadTree.Size()-1].TraceBoundingBox(*this, BB.Min.x, BB.Min.y, BB.Max.x, BB.Max.y, TraceBB, Origin, Dir, Trace);
}


TerrainT::QuadTreeT::QuadTreeT(TerrainT& Terrain, unsigned long LowerLeftVertIdx, unsigned long UpperRightVertIdx, unsigned long Level)
{
    const unsigned long CellMinX=LowerLeftVertIdx  % (Terrain.Size-1);
    const unsigned long CellMinY=LowerLeftVertIdx  / (Terrain.Size-1);
    const unsigned long CellMaxX=UpperRightVertIdx % (Terrain.Size-1);
    const unsigned long CellMaxY=UpperRightVertIdx / (Terrain.Size-1);


    // We will obtain (2^MAX_LEVEL * 2^MAX_LEVEL) == 4^MAX_LEVEL leaves.
    if (Level==7 /*MAX_LEVEL*/ || LowerLeftVertIdx==UpperRightVertIdx)
    {
        // We reached a leaf.
        Child00Index=0xFFFFFFFF;
        Child01Index=0xFFFFFFFF;
        Child10Index=0xFFFFFFFF;
        Child11Index=0xFFFFFFFF;

        HeightMin=Terrain.GetVertex(CellMinX, CellMinY).z;
        HeightMax=Terrain.GetVertex(CellMinX, CellMinY).z;

        for (unsigned long y=CellMinY; y<=CellMaxY; y++)
            for (unsigned long x=CellMinX; x<=CellMaxX; x++)
            {
                if (Terrain.GetVertex(x  , y  ).z<HeightMin) HeightMin=Terrain.GetVertex(x  , y  ).z;
                if (Terrain.GetVertex(x+1, y  ).z<HeightMin) HeightMin=Terrain.GetVertex(x+1, y  ).z;
                if (Terrain.GetVertex(x  , y+1).z<HeightMin) HeightMin=Terrain.GetVertex(x  , y+1).z;
                if (Terrain.GetVertex(x+1, y+1).z<HeightMin) HeightMin=Terrain.GetVertex(x+1, y+1).z;

                if (Terrain.GetVertex(x  , y  ).z>HeightMax) HeightMax=Terrain.GetVertex(x  , y  ).z;
                if (Terrain.GetVertex(x+1, y  ).z>HeightMax) HeightMax=Terrain.GetVertex(x+1, y  ).z;
                if (Terrain.GetVertex(x  , y+1).z>HeightMax) HeightMax=Terrain.GetVertex(x  , y+1).z;
                if (Terrain.GetVertex(x+1, y+1).z>HeightMax) HeightMax=Terrain.GetVertex(x+1, y+1).z;
            }

        LowerLeftVertexIndex =LowerLeftVertIdx;
        UpperRightVertexIndex=UpperRightVertIdx;
    }
    else
    {
        // Subdivide this QuadTreeT node into four.
        const unsigned long dX=(CellMaxX-CellMinX+1)/2;
        const unsigned long dY=(CellMaxY-CellMinY+1)/2;

        const unsigned long MinX00=CellMinX;    const unsigned long MinY00=CellMinY;
        const unsigned long MinX01=CellMinX;    const unsigned long MinY01=CellMinY+dY;
        const unsigned long MinX10=CellMinX+dX; const unsigned long MinY10=CellMinY;
        const unsigned long MinX11=CellMinX+dX; const unsigned long MinY11=CellMinY+dY;

        const unsigned long MaxX00=MinX00+dX-1; const unsigned long MaxY00=MinY00+dX-1;
        const unsigned long MaxX01=MinX01+dX-1; const unsigned long MaxY01=MinY01+dX-1;
        const unsigned long MaxX10=MinX10+dX-1; const unsigned long MaxY10=MinY10+dX-1;
        const unsigned long MaxX11=MinX11+dX-1; const unsigned long MaxY11=MinY11+dX-1;

        Terrain.QuadTree.PushBack(QuadTreeT(Terrain, MinX00+(Terrain.Size-1)*MinY00, MaxX00+(Terrain.Size-1)*MaxY00, Level+1)); Child00Index=Terrain.QuadTree.Size()-1;
        Terrain.QuadTree.PushBack(QuadTreeT(Terrain, MinX01+(Terrain.Size-1)*MinY01, MaxX01+(Terrain.Size-1)*MaxY01, Level+1)); Child01Index=Terrain.QuadTree.Size()-1;
        Terrain.QuadTree.PushBack(QuadTreeT(Terrain, MinX10+(Terrain.Size-1)*MinY10, MaxX10+(Terrain.Size-1)*MaxY10, Level+1)); Child10Index=Terrain.QuadTree.Size()-1;
        Terrain.QuadTree.PushBack(QuadTreeT(Terrain, MinX11+(Terrain.Size-1)*MinY11, MaxX11+(Terrain.Size-1)*MaxY11, Level+1)); Child11Index=Terrain.QuadTree.Size()-1;

        if (true)                                               HeightMin=Terrain.QuadTree[Child00Index].HeightMin;
        if (Terrain.QuadTree[Child01Index].HeightMin<HeightMin) HeightMin=Terrain.QuadTree[Child01Index].HeightMin;
        if (Terrain.QuadTree[Child10Index].HeightMin<HeightMin) HeightMin=Terrain.QuadTree[Child10Index].HeightMin;
        if (Terrain.QuadTree[Child11Index].HeightMin<HeightMin) HeightMin=Terrain.QuadTree[Child11Index].HeightMin;

        if (true)                                               HeightMax=Terrain.QuadTree[Child00Index].HeightMax;
        if (Terrain.QuadTree[Child01Index].HeightMax>HeightMax) HeightMax=Terrain.QuadTree[Child01Index].HeightMax;
        if (Terrain.QuadTree[Child10Index].HeightMax>HeightMax) HeightMax=Terrain.QuadTree[Child10Index].HeightMax;
        if (Terrain.QuadTree[Child11Index].HeightMax>HeightMax) HeightMax=Terrain.QuadTree[Child11Index].HeightMax;

        LowerLeftVertexIndex =0;
        UpperRightVertexIndex=0;
    }
}


/// @todo Optimize by reducing parameter passing!
void TerrainT::QuadTreeT::TraceBoundingBox(const TerrainT& Terrain, double BBMinX, double BBMinY, double BBMaxX, double BBMaxY, const BoundingBox3T<double>& BB, const VectorT& Origin, const VectorT& Dir, VB_Trace3T<double>& Trace) const
{
    if (Child00Index==0xFFFFFFFF /*IsLeaf*/)
    {
        static Brush3T<double> CaseBrushes[3];

        if (CaseBrushes[0].Planes.Size()==0)
        {
            CaseBrushes[0].Planes.PushBack(Plane3T<double>(VectorT(-1.0,  0.0,  0.0), 0.0));    // Left     plane.
            CaseBrushes[0].Planes.PushBack(Plane3T<double>(VectorT( 1.0,  0.0,  0.0), 0.0));    // Right    plane.
            CaseBrushes[0].Planes.PushBack(Plane3T<double>(VectorT( 0.0, -1.0,  0.0), 0.0));    // Near     plane.
            CaseBrushes[0].Planes.PushBack(Plane3T<double>(VectorT( 0.0,  1.0,  0.0), 0.0));    // Far      plane.
            CaseBrushes[0].Planes.PushBack(Plane3T<double>(VectorT( 0.0,  0.0, -1.0), 0.0));    // Bottom   plane.
            CaseBrushes[0].Planes.PushBack(Plane3T<double>(VectorT( 0.0,  0.0,  1.0), 0.0));    // Top      plane.
            CaseBrushes[0].Planes.PushBack(Plane3T<double>());                                  // Diagonal plane.
            CaseBrushes[0].Planes.PushBack(Plane3T<double>());                                  // Triangle plane.

            CaseBrushes[1]=CaseBrushes[0]; CaseBrushes[1].Planes.PushBack(Plane3T<double>());    // First  auxiliary plane.
            CaseBrushes[2]=CaseBrushes[1]; CaseBrushes[2].Planes.PushBack(Plane3T<double>());    // Second auxiliary plane.
        }


        // We are in a leaf. Do an exact test.
        // (For temp. tests, some brute-force would be in order...).
        const unsigned long CellMinX=LowerLeftVertexIndex  % (Terrain.Size-1);
        const unsigned long CellMinY=LowerLeftVertexIndex  / (Terrain.Size-1);
        const unsigned long CellMaxX=UpperRightVertexIndex % (Terrain.Size-1);
        const unsigned long CellMaxY=UpperRightVertexIndex / (Terrain.Size-1);

        // const double SpacingX=(BBMaxX-BBMinX)/(CellMaxX-CellMinX+1);
        // const double SpacingY=(BBMaxY-BBMinY)/(CellMaxY-CellMinY+1);

        for (unsigned long y=CellMinY; y<=CellMaxY; y++)
            for (unsigned long x=CellMinX; x<=CellMaxX; x++)
            {
                // If not for the cast to doubles, V1 to V4 could just be references instead of instances.
                const VectorT V1=Terrain.GetVertex(x  , y  ).AsVectorOfDouble();
                const VectorT V2=Terrain.GetVertex(x  , y+1).AsVectorOfDouble();
                const VectorT V3=Terrain.GetVertex(x+1, y+1).AsVectorOfDouble();
                const VectorT V4=Terrain.GetVertex(x+1, y  ).AsVectorOfDouble();

                double BBMinZ=Min(Min(V1.z, V2.z), Min(V3.z, V4.z));
                double BBMaxZ=Max(Max(V1.z, V2.z), Max(V3.z, V4.z));

                // We now have to compute the trace against this cell.
                // Each quadratic cell decomposes into two triangles, and thus into two brushes.
                // As there are two ways in which a quadratic cell can be decomposed into triangles
                // (the diagonal can be from near left to far right, or from far left to near right),
                // there are four possible triangles that we need to be able to handle,
                // but only two of them (one pair) needs to be considered for each cell.

                // As the triangles are all in a fixed grid, we may exploit their special structure
                // in order to construct four special-case brushes from them.

                // Each of the four possible cases must further be sub-cased though:
                // ...

                // Construct two brushes for this cell: one brush for each triangle.
                // Each brush consists of 8 planes: The six planes of the bounding box (where the top and bottom plane serve as "bevel" planes),
                // plus the actual plane of the triangle, plus a mirrored copy of the triangle plane for collisions "from below".
                // First, complete the common part:
                const static double Sqrt2Div2=sqrt(2.0)/2.0;

                if ((x & 1)==(y & 1))
                {
                    // The diagonal goes from lower left to upper right ( (0, 0) to (1, 1) ),
                    // yielding triangles (V1, V2, V3) and (V3, V4, V1).

                    // Clip against the first triangle.
                    {
                        const Plane3T<double> TrianglePlane(V1, V2, V3, 0.0);
                        const bool   NeedAuxPlaneAtLeftSide=Sign(TrianglePlane.Normal.x)== 1;   // Sign(TrianglePlane.Normal.x)*Sign(-1.0)==-1
                        const bool   NeedAuxPlaneAtFar_Side=Sign(TrianglePlane.Normal.y)==-1;   // Sign(TrianglePlane.Normal.y)*Sign( 1.0)==-1
                        const bool   NeedAuxPlan1AtDiagSide=Sign(TrianglePlane.Normal.x)==-1;   // Sign(TrianglePlane.Normal.x)*Sign( 0.7)==-1
                        const bool   NeedAuxPlan2AtDiagSide=Sign(TrianglePlane.Normal.y)== 1;   // Sign(TrianglePlane.Normal.y)*Sign(-0.7)==-1

                        // Determine the number of extra / additionally required bevel planes.
                        unsigned long NumExtraPlanes=0;

                        if (NeedAuxPlaneAtLeftSide) NumExtraPlanes++;
                        if (NeedAuxPlaneAtFar_Side) NumExtraPlanes++;
                        if (NeedAuxPlan1AtDiagSide) NumExtraPlanes++;
                        if (NeedAuxPlan2AtDiagSide) NumExtraPlanes++;

                        // Note that NumExtraPlanes is *always* in [0..2] !
                        Brush3T<double>& ClipBrush=CaseBrushes[NumExtraPlanes];

                        ClipBrush.Planes[0].Dist=-V1.x;             // Left   plane.
                        ClipBrush.Planes[1].Dist= V3.x;             // Right  plane.
                        ClipBrush.Planes[2].Dist=-V1.y;             // Near   plane.
                        ClipBrush.Planes[3].Dist= V3.y;             // Far    plane.
                        ClipBrush.Planes[4].Dist=-BBMinZ+100.0;     // Bottom plane.
                        ClipBrush.Planes[5].Dist= BBMaxZ;           // Top    plane.

                        ClipBrush.Planes[6].Normal.x= Sqrt2Div2;    // Diagonal plane (through V3, V1).
                        ClipBrush.Planes[6].Normal.y=-Sqrt2Div2;
                        ClipBrush.Planes[6].Dist    = Sqrt2Div2*(V1.x-V1.y);

                        ClipBrush.Planes[7]=TrianglePlane;          // Triangle plane.

                        unsigned long NextPlaneNr=8;
                        if (NeedAuxPlaneAtLeftSide) ClipBrush.Planes[NextPlaneNr++]=Plane3T<double>(V1, V2, VectorT(V2.x+100.0, V2.y, V2.z), 0.0);
                        if (NeedAuxPlaneAtFar_Side) ClipBrush.Planes[NextPlaneNr++]=Plane3T<double>(V2, V3, VectorT(V3.x, V3.y-100.0, V3.z), 0.0);
                        if (NeedAuxPlan1AtDiagSide) ClipBrush.Planes[NextPlaneNr++]=Plane3T<double>(V3, V1, VectorT(V1.x-100.0, V1.y, V1.z), 0.0);
                        if (NeedAuxPlan2AtDiagSide) ClipBrush.Planes[NextPlaneNr++]=Plane3T<double>(V3, V1, VectorT(V1.x, V1.y+100.0, V1.z), 0.0);

                        ClipBrush.TraceBoundingBox(BB, Origin, Dir, Trace);
                    }

                    // Clip against the second triangle.
                    {
                        const Plane3T<double> TrianglePlane(V3, V4, V1, 0.0);
                        const bool   NeedAuxPlaneAtRghtSide=Sign(TrianglePlane.Normal.x)==-1;   // Sign(TrianglePlane.Normal.x)*Sign( 1.0)==-1
                        const bool   NeedAuxPlaneAtNearSide=Sign(TrianglePlane.Normal.y)== 1;   // Sign(TrianglePlane.Normal.y)*Sign(-1.0)==-1
                        const bool   NeedAuxPlan1AtDiagSide=Sign(TrianglePlane.Normal.x)== 1;   // Sign(TrianglePlane.Normal.x)*Sign(-0.7)==-1
                        const bool   NeedAuxPlan2AtDiagSide=Sign(TrianglePlane.Normal.y)==-1;   // Sign(TrianglePlane.Normal.y)*Sign( 0.7)==-1

                        // Determine the number of extra / additionally required bevel planes.
                        unsigned long NumExtraPlanes=0;

                        if (NeedAuxPlaneAtRghtSide) NumExtraPlanes++;
                        if (NeedAuxPlaneAtNearSide) NumExtraPlanes++;
                        if (NeedAuxPlan1AtDiagSide) NumExtraPlanes++;
                        if (NeedAuxPlan2AtDiagSide) NumExtraPlanes++;

                        // Note that NumExtraPlanes is *always* in [0..2] !
                        Brush3T<double>& ClipBrush=CaseBrushes[NumExtraPlanes];

                        ClipBrush.Planes[0].Dist=-V1.x;             // Left   plane.
                        ClipBrush.Planes[1].Dist= V3.x;             // Right  plane.
                        ClipBrush.Planes[2].Dist=-V1.y;             // Near   plane.
                        ClipBrush.Planes[3].Dist= V3.y;             // Far    plane.
                        ClipBrush.Planes[4].Dist=-BBMinZ+100.0;     // Bottom plane.
                        ClipBrush.Planes[5].Dist= BBMaxZ;           // Top    plane.

                        ClipBrush.Planes[6].Normal.x=-Sqrt2Div2;    // Diagonal plane (through V1, V3).
                        ClipBrush.Planes[6].Normal.y= Sqrt2Div2;
                        ClipBrush.Planes[6].Dist    = Sqrt2Div2*(V1.y-V1.x);

                        ClipBrush.Planes[7]=TrianglePlane;          // Triangle plane.

                        unsigned long NextPlaneNr=8;
                        if (NeedAuxPlaneAtRghtSide) ClipBrush.Planes[NextPlaneNr++]=Plane3T<double>(V3, V4, VectorT(V4.x-100.0, V4.y, V4.z), 0.0);
                        if (NeedAuxPlaneAtNearSide) ClipBrush.Planes[NextPlaneNr++]=Plane3T<double>(V4, V1, VectorT(V1.x, V1.y+100.0, V1.z), 0.0);
                        if (NeedAuxPlan1AtDiagSide) ClipBrush.Planes[NextPlaneNr++]=Plane3T<double>(V1, V3, VectorT(V3.x+100.0, V3.y, V3.z), 0.0);
                        if (NeedAuxPlan2AtDiagSide) ClipBrush.Planes[NextPlaneNr++]=Plane3T<double>(V1, V3, VectorT(V3.x, V3.y-100.0, V3.z), 0.0);

                        ClipBrush.TraceBoundingBox(BB, Origin, Dir, Trace);
                    }
                }
                else
                {
                    // The diagonal goes from upper left to lower right ( (0, 1) to (1, 0) ),
                    // yielding triangles (V1, V2, V4) and (V2, V3, V4).

                    // Clip against the first triangle.
                    {
                        const Plane3T<double> TrianglePlane(V1, V2, V4, 0.0);
                        const bool   NeedAuxPlaneAtLeftSide=Sign(TrianglePlane.Normal.x)== 1;   // Sign(TrianglePlane.Normal.x)*Sign(-1.0)==-1
                        const bool   NeedAuxPlaneAtNearSide=Sign(TrianglePlane.Normal.y)== 1;   // Sign(TrianglePlane.Normal.y)*Sign(-1.0)==-1
                        const bool   NeedAuxPlan1AtDiagSide=Sign(TrianglePlane.Normal.x)==-1;   // Sign(TrianglePlane.Normal.x)*Sign( 0.7)==-1
                        const bool   NeedAuxPlan2AtDiagSide=Sign(TrianglePlane.Normal.y)==-1;   // Sign(TrianglePlane.Normal.y)*Sign( 0.7)==-1

                        // Determine the number of extra / additionally required bevel planes.
                        unsigned long NumExtraPlanes=0;

                        if (NeedAuxPlaneAtLeftSide) NumExtraPlanes++;
                        if (NeedAuxPlaneAtNearSide) NumExtraPlanes++;
                        if (NeedAuxPlan1AtDiagSide) NumExtraPlanes++;
                        if (NeedAuxPlan2AtDiagSide) NumExtraPlanes++;

                        // Note that NumExtraPlanes is *always* in [0..2] !
                        Brush3T<double>& ClipBrush=CaseBrushes[NumExtraPlanes];

                        ClipBrush.Planes[0].Dist=-V1.x;             // Left   plane.
                        ClipBrush.Planes[1].Dist= V3.x;             // Right  plane.
                        ClipBrush.Planes[2].Dist=-V1.y;             // Near   plane.
                        ClipBrush.Planes[3].Dist= V3.y;             // Far    plane.
                        ClipBrush.Planes[4].Dist=-BBMinZ+100.0;     // Bottom plane.
                        ClipBrush.Planes[5].Dist= BBMaxZ;           // Top    plane.

                        ClipBrush.Planes[6].Normal.x=Sqrt2Div2;     // Diagonal plane (through V2, V4).
                        ClipBrush.Planes[6].Normal.y=Sqrt2Div2;
                        ClipBrush.Planes[6].Dist    =Sqrt2Div2*(V2.x+V2.y);

                        ClipBrush.Planes[7]=TrianglePlane;          // Triangle plane.

                        unsigned long NextPlaneNr=8;
                        if (NeedAuxPlaneAtLeftSide) ClipBrush.Planes[NextPlaneNr++]=Plane3T<double>(V1, V2, VectorT(V2.x+100.0, V2.y, V2.z), 0.0);
                        if (NeedAuxPlaneAtNearSide) ClipBrush.Planes[NextPlaneNr++]=Plane3T<double>(V4, V1, VectorT(V1.x, V1.y+100.0, V1.z), 0.0);
                        if (NeedAuxPlan1AtDiagSide) ClipBrush.Planes[NextPlaneNr++]=Plane3T<double>(V2, V4, VectorT(V4.x-100.0, V4.y, V4.z), 0.0);
                        if (NeedAuxPlan2AtDiagSide) ClipBrush.Planes[NextPlaneNr++]=Plane3T<double>(V2, V4, VectorT(V4.x, V4.y-100.0, V4.z), 0.0);

                        ClipBrush.TraceBoundingBox(BB, Origin, Dir, Trace);
                    }

                    // Clip against the second triangle.
                    {
                        const Plane3T<double> TrianglePlane(V2, V3, V4, 0.0);
                        const bool   NeedAuxPlaneAtRghtSide=Sign(TrianglePlane.Normal.x)==-1;   // Sign(TrianglePlane.Normal.x)*Sign( 1.0)==-1
                        const bool   NeedAuxPlaneAtFar_Side=Sign(TrianglePlane.Normal.y)==-1;   // Sign(TrianglePlane.Normal.y)*Sign( 1.0)==-1
                        const bool   NeedAuxPlan1AtDiagSide=Sign(TrianglePlane.Normal.x)== 1;   // Sign(TrianglePlane.Normal.x)*Sign(-0.7)==-1
                        const bool   NeedAuxPlan2AtDiagSide=Sign(TrianglePlane.Normal.y)== 1;   // Sign(TrianglePlane.Normal.y)*Sign(-0.7)==-1

                        // Determine the number of extra / additionally required bevel planes.
                        unsigned long NumExtraPlanes=0;

                        if (NeedAuxPlaneAtRghtSide) NumExtraPlanes++;
                        if (NeedAuxPlaneAtFar_Side) NumExtraPlanes++;
                        if (NeedAuxPlan1AtDiagSide) NumExtraPlanes++;
                        if (NeedAuxPlan2AtDiagSide) NumExtraPlanes++;

                        // Note that NumExtraPlanes is *always* in [0..2] !
                        Brush3T<double>& ClipBrush=CaseBrushes[NumExtraPlanes];

                        ClipBrush.Planes[0].Dist=-V1.x;             // Left   plane.
                        ClipBrush.Planes[1].Dist= V3.x;             // Right  plane.
                        ClipBrush.Planes[2].Dist=-V1.y;             // Near   plane.
                        ClipBrush.Planes[3].Dist= V3.y;             // Far    plane.
                        ClipBrush.Planes[4].Dist=-BBMinZ+100.0;     // Bottom plane.
                        ClipBrush.Planes[5].Dist= BBMaxZ;           // Top    plane.

                        ClipBrush.Planes[6].Normal.x=-Sqrt2Div2;    // Diagonal plane (through V4, V2).
                        ClipBrush.Planes[6].Normal.y=-Sqrt2Div2;
                        ClipBrush.Planes[6].Dist    =-Sqrt2Div2*(V2.x+V2.y);

                        ClipBrush.Planes[7]=TrianglePlane;          // Triangle plane.

                        unsigned long NextPlaneNr=8;
                        if (NeedAuxPlaneAtRghtSide) ClipBrush.Planes[NextPlaneNr++]=Plane3T<double>(V3, V4, VectorT(V4.x-100.0, V4.y, V4.z), 0.0);
                        if (NeedAuxPlaneAtFar_Side) ClipBrush.Planes[NextPlaneNr++]=Plane3T<double>(V2, V3, VectorT(V3.x, V3.y-100.0, V3.z), 0.0);
                        if (NeedAuxPlan1AtDiagSide) ClipBrush.Planes[NextPlaneNr++]=Plane3T<double>(V4, V2, VectorT(V2.x+100.0, V2.y, V2.z), 0.0);
                        if (NeedAuxPlan2AtDiagSide) ClipBrush.Planes[NextPlaneNr++]=Plane3T<double>(V4, V2, VectorT(V2.x, V2.y+100.0, V2.z), 0.0);

                        ClipBrush.TraceBoundingBox(BB, Origin, Dir, Trace);
                    }
                }
            }
    }
    else
    {
        // It's not a leaf. It's a node.
        // First, figure out if our bounding box brush "BBB" is intersected by BB at all, that is, if there is a *potential* hit.
        static Brush3T<double> BBB;

        if (BBB.Planes.Size()==0)
        {
            BBB.Planes.PushBack(Plane3T<double>(VectorT(-1.0,  0.0,  0.0), 0.0));     // Left   plane.
            BBB.Planes.PushBack(Plane3T<double>(VectorT( 1.0,  0.0,  0.0), 0.0));     // Right  plane.
            BBB.Planes.PushBack(Plane3T<double>(VectorT( 0.0, -1.0,  0.0), 0.0));     // Near   plane.
            BBB.Planes.PushBack(Plane3T<double>(VectorT( 0.0,  1.0,  0.0), 0.0));     // Far    plane.
            BBB.Planes.PushBack(Plane3T<double>(VectorT( 0.0,  0.0, -1.0), 0.0));     // Bottom plane.
            BBB.Planes.PushBack(Plane3T<double>(VectorT( 0.0,  0.0,  1.0), 0.0));     // Top    plane.
        }

        BBB.Planes[0].Dist=-BBMinX;
        BBB.Planes[1].Dist= BBMaxX;
        BBB.Planes[2].Dist=-BBMinY;
        BBB.Planes[3].Dist= BBMaxY;
        BBB.Planes[4].Dist=-HeightMin;
        BBB.Planes[5].Dist= HeightMax;

        VB_Trace3T<double> PotentialHitTrace(Trace);

        BBB.TraceBoundingBox(BB, Origin, Dir, PotentialHitTrace);

        // This test does also work fine if Trace.Fraction is 0.0. Then PotentialHitTrace.Fraction will be 0.0, too - done.
        // In this case, PotentialHitTrace.StartSolid==true could possibly go undetected, but then Trace.Fraction is 0.0 anyway, so why bother?
        if (PotentialHitTrace.Fraction<Trace.Fraction)
        {
            const double BBHalfX=(BBMinX+BBMaxX)/2.0;
            const double BBHalfY=(BBMinY+BBMaxY)/2.0;

            // Potential hit found! Check the children.
            Terrain.QuadTree[Child00Index].TraceBoundingBox(Terrain, BBMinX , BBMinY , BBHalfX, BBHalfY, BB, Origin, Dir, Trace);
            Terrain.QuadTree[Child01Index].TraceBoundingBox(Terrain, BBMinX , BBHalfY, BBHalfX, BBMaxY , BB, Origin, Dir, Trace);
            Terrain.QuadTree[Child10Index].TraceBoundingBox(Terrain, BBHalfX, BBMinY , BBMaxX , BBHalfY, BB, Origin, Dir, Trace);
            Terrain.QuadTree[Child11Index].TraceBoundingBox(Terrain, BBHalfX, BBHalfY, BBMaxX , BBMaxY , BB, Origin, Dir, Trace);
        }
    }
}
