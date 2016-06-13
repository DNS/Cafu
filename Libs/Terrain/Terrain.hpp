/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_TERRAIN_HPP_INCLUDED
#define CAFU_TERRAIN_HPP_INCLUDED

#include "Templates/Array.hpp"
#include "Math3D/BoundingBox.hpp"
#include "Math3D/Brush.hpp"
#include "Math3D/Plane3.hpp"


/// This class represents terrains, offering methods for LoD rendering and collision detection.
class TerrainT
{
    public:

    // TODO 1: Rename "e" to "error" and "r" to "radius".
    // TODO 2: Should we really derive from Vector3fT? Can we omit x and y (and instead compute it on the fly when required)?
    struct VertexT : public Vector3fT
    {
        float e;                ///< Error.
        float r;                ///< Bounding sphere radius.
    };

    struct ViewInfoT
    {
        bool      cull;             ///< Perform view culling when set.
        float     nu;               ///< Inverse of error tolerance in radians. Note that this is equivalent to the "magic" number in the original SOARX class by Andras Balogh.
        float     nu_min;           ///< Lower morph parameter.
        float     nu_max;           ///< Upper morph parameter.
        Vector3fT viewpoint;        ///< The viewpoint. Corresponds to Camera::m_position in the original code by Andras Balogh.
        Plane3fT  viewplanes[5];    ///< View frustum planes (without the "far" plane). Corresponds to Camera::m_frustum_planes in the original code by Andras Balogh.
    };

    /// An error class that is thrown upon init errors in the constructor.
    class InitError {};


    /// Constructor for creating a simple dummy terrain.
    /// The main purpose is to be able to store TerrainTs in container classes.
    TerrainT();

    /// The constructor for creating and initializing the terrain from the file specified by FileNameHeightMap.
    /// FileNameHeightMap should be the file name of a gray-scale heightmap in bmp, tga, or png file format.
    /// The heightmap must be square, and the side lengths be of the form (2^n)+1, where n is in range 1...15.
    /// (Thus, the smallest permissible heightmap has size 3x3, the largest 32769x32769.)
    /// The bounding box BB determines the lateral dimensions of the terrain.
    /// If FailSafe is set to false, InitError is thrown on problems reading the file or violation of the
    /// above constraints. Otherwise, a simple default heightmap is substituted.
    TerrainT(const char* FileNameHeightMap, const BoundingBox3fT& BB_, bool FailSafe=true) /*throw (InitError)*/;

    /// Constructor as above, except that instead of the bounding box, a Resolution vector is passed in.
    /// The resolution describes the distance between two elements of the height map in x- and y-direction,
    /// as well as the distance between two height values (z-direction). The resulting terrain is centered at the origin.
    TerrainT(const char* FileNameHeightMap, const Vector3fT& Resolution, bool FailSafe=true) /*throw (InitError)*/;

    /// The constructor for creating and initializing the terrain from the data specified in HeightData.
    /// The heightmap must be square, and the side lengths be of the form (2^n)+1, where n is in range 1...15.
    /// (Thus, the smallest permissible heightmap has size 3x3, the largest 32769x32769.)
    /// The bounding box BB determines the lateral dimensions of the terrain.
    TerrainT(const unsigned short* HeightData, unsigned long SideLength, const BoundingBox3fT& BB_) /*throw (InitError)*/;

    /// Updates the vertices in the given rectangle with new heights from the given height field.
    /// Note that this method is a HACK, because it INVALIDATES the auxiliary data that is computed in the constructor;
    /// the nested error metrics, radii and collision data in the quad-tree all become INVALID after a call to this method.
    /// The purpose of this method is to allow for quick but temporary changes in terrain heights of small magnitude,
    /// e.g. while the mouse button is held down for a single tool application in a terrain editor.
    /// @param HeightData   A pointer to a new heightmap of dimensions GetSize()*GetSize() from which the new heights are taken.
    /// @param PosX         The x-origin of the rectangle in which the vertices are updated.
    /// @param PosY         The y-origin of the rectangle in which the vertices are updated.
    /// @param SizeX        The width    of the rectangle in which the vertices are updated.
    /// @param SizeY        The height   of the rectangle in which the vertices are updated.
    void UpdateHeights(const unsigned short* HeightData, unsigned long PosX, unsigned long PosY, unsigned long SizeX, unsigned long SizeY);

    /// This functions returns a pointer to the vertices of the terrain,
    /// intended for use with the ComputeIndexStripByRefinement() function.
    const VertexT* GetVertices() const;

    /// Returns the number of vertices along one side of the terrain/heightmap, e.g. 257, 513, 1025, ...
    unsigned long GetSize() const { return Size; }

    /// Returns the bounding-box (i.e. the lateral dimensions) of the terrain/heightmap.
    const BoundingBox3fT& GetBB() const { return BB; }

    /// This function computes a triangle strip of vertices that approximates the terrain according to the parameters in VI.
    /// For this purpose, geo-morphing is employed in order to prevent sudden popping and to simultaneously allow for greater
    /// error tolerances.
    /// The returned array is global and thus mutable. It will change upon the next call to this function of *any* terrain object.
    ArrayT<Vector3fT>& ComputeVectorStrip(const TerrainT::ViewInfoT& VI) const;

    /// This function computes a triangle strip that approximates the terrain according to the parameters in VI.
    /// For this purpose, the method of refinement is employed, and there is no geo-morphing to prevent sudden popping.
    /// The returned triangle strip consists of indices into the array of terrain vertices, which in turn can be obtained
    /// by calling the GetVertices() function. Note that the real strip index data starts at index 1 of the returned array,
    /// and thus the first entry (at array index 0) should be skipped or ignored.
    /// Also, the returned array is global and thus mutable. It will change upon the next call to this function of *any* terrain object.
    ArrayT<unsigned long>& ComputeIndexStripByRefinement(const ViewInfoT& VI) const;

    /// This function computes a triangle strip of vectors that approximates the terrain according to the parameters in VI.
    /// For this purpose, geo-morphing is employed in order to prevent sudden popping and simultaneously allow for greater
    /// error tolerances. Note that the real strip vector data starts at index 1 of the returned array, and thus the first
    /// entry (at array index 0) should be skipped or ignored.
    /// Also, the returned array is global and thus mutable. It will change upon the next call to this function of *any* terrain object.
    ArrayT<Vector3fT>& ComputeVectorStripByMorphing(const ViewInfoT& VI) const;

    /// Traces the (relative) bounding box TraceBB from the (absolute) Origin along Dir towards the end position Origin+VectorScale(Dir, Trace.Fraction).
    /// The result is returned in Trace, indicating if and where the trace was stopped.
    void TraceBoundingBox(const BoundingBox3dT& TraceBB, const VectorT& Origin, const VectorT& Dir, VB_Trace3T<double>& Trace) const;


    private:

    class QuadTreeT
    {
        private:

        unsigned long Child00Index;             ///< Lower left  child index into a given array of QuadTreeT.
        unsigned long Child01Index;             ///< Upper left  child index into a given array of QuadTreeT.
        unsigned long Child10Index;             ///< Lower right child index into a given array of QuadTreeT.
        unsigned long Child11Index;             ///< Upper right child index into a given array of QuadTreeT.

        float         HeightMin;
        float         HeightMax;

        unsigned long LowerLeftVertexIndex;     ///< If this node is a child, this is the index of its lower left  vertex.
        unsigned long UpperRightVertexIndex;    ///< If this node is a child, this is the index of its upper right vertex.


        public:

        QuadTreeT() { }                         ///< Default constructor, required for use with ArrayT.
        QuadTreeT(TerrainT& Terrain, unsigned long LowerLeftVertIdx, unsigned long UpperRightVertIdx, unsigned long Level);
        void TraceBoundingBox(const TerrainT& Terrain, double BBMinX, double BBMinY, double BBMaxX, double BBMaxY, const BoundingBox3T<double>& BB, const VectorT& Origin, const VectorT& Dir, VB_Trace3T<double>& Trace) const;
    };

    friend class QuadTreeT;


    // This is the actual internal representation of the terrain.
    ArrayT<VertexT>   Vertices; ///< Array of all terrain vertices.
    unsigned long     Size;     ///< Lateral dimensions of the terrain/heightmap, e.g. 257, 513, 1025, ...
    unsigned long     Levels;   ///< Size==2^(Levels/2)+1
    BoundingBox3fT    BB;       ///< Lateral dimensions in world coordinates.
    ArrayT<QuadTreeT> QuadTree; ///< All nodes of the QuadTree for this terrain (the last element is the root), used for collision detection (that is, TraceBoundingBox()).

    // Const data used in ComputeVectorStrip().
    /*const*/ bool    mCVS_Side;
    /*const*/ bool    mCVS_Bottom;

    // Working data used in ComputeVectorStrip().
    mutable bool          mCVS_LeftOnly;
    mutable bool          mCVS_First;
    mutable unsigned long mCVS_CurrentLevel;


    // Helper functions for the constructor.
    const VertexT& GetVertex(unsigned long i, unsigned long j) const { return Vertices[i+Size*j]; }
    VertexT& GetVertex(unsigned long i, unsigned long j) { return Vertices[i+Size*j]; }
    void ComputeVertexLoD(unsigned long i, unsigned long j, int di, int dj, unsigned long n);
    void Init(const char* FileNameHeightMap, const BoundingBox3fT* BB_, const Vector3fT* Resolution, bool FailSafe);

    // Helper functions for ComputeVectorStrip().
    class CVS_VertexT;
    void CVS_GetVertex(CVS_VertexT& v) const;
    bool CVS_Active(CVS_VertexT& v, float zEdge, unsigned long& planes) const;
    void CVS_Append(const CVS_VertexT& v) const;
    void CVS_TurnCorner() const;
    void CVS_RefineTop() const;
    void CVS_Refine(const CVS_VertexT& i, const CVS_VertexT& j, float zA, float zB, bool in, bool out, unsigned long planes) const;
    void CVS_RefineL(const CVS_VertexT& i, const CVS_VertexT& j, float zA, bool in, unsigned long planes) const;
    void CVS_RefineR(CVS_VertexT i, CVS_VertexT j, float zB, bool out, unsigned long planes) const;

    // Helper functions for ComputeIndexStripByRefinement().
    void Refine_AppendStripIndex(unsigned long VertexIndex, bool Parity) const;
    void Refine_SubMesh(unsigned long l, /* TRIANGLE( */unsigned long i, unsigned long j, unsigned long k/*)*/) const;
    void Refine_SubMeshVisible(unsigned long l, /* TRIANGLE( */unsigned long i, unsigned long j, unsigned long k/*)*/, unsigned long m) const;

    // Helper functions for ComputeVectorStripByMorphing().
    void Morph_AppendStripVector(unsigned long VertexIndex, bool Parity, float z) const;
    void Morph_SubMesh(unsigned long l, /* TRIANGLE( */unsigned long i, unsigned long j, unsigned long k/*)*/, float za, float zl, float zr) const;
    void Morph_SubMeshVisible(unsigned long l, /* TRIANGLE( */unsigned long i, unsigned long j, unsigned long k/*)*/, float za, float zl, float zr, unsigned long m) const;
};

#endif
