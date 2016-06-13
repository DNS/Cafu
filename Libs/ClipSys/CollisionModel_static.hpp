/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CLIPSYS_COLLISION_MODEL_STATIC_HPP_INCLUDED
#define CAFU_CLIPSYS_COLLISION_MODEL_STATIC_HPP_INCLUDED

#include "CollisionModel_base.hpp"
#include "Templates/Array.hpp"
#include "Templates/Pool.hpp"


namespace cf { struct MapFileEntityT; }
class MaterialT;
class TerrainT;


namespace cf
{
    namespace ClipSys
    {
        /// This class represents a static collision model.
        class CollisionModelStaticT : public CollisionModelT
        {
            public:

            class InternalTraceSolidT;
            class TraceParamsT;


            class PolygonT
            {
                public:

                /// The default constructor.
                PolygonT();

                /// The constructor.
                PolygonT(CollisionModelStaticT* Parent_, MaterialT* Material_, unsigned long A, unsigned long B, unsigned long C, unsigned long D = 0xFFFFFFFF);


                bool IsTriangle() const { return Vertices[3] == 0xFFFFFFFF; }

                BoundingBox3dT GetBB() const;

                void TraceConvexSolid(const InternalTraceSolidT& TraceSolid, const Vector3dT& Start, const Vector3dT& Ray, unsigned long ClipMask, TraceResultT& Result) const;

                void TraceRay(const Vector3dT& Start, const Vector3dT& Ray, unsigned long ClipMask, TraceResultT& Result) const;


                CollisionModelStaticT* Parent;
                unsigned long          Vertices[4]; ///< The indices of the vertices that define the triangle or quad, respectively, referring into the m_Vertices list.
                MaterialT*             Material;    ///< The material on the surface of this polygon.
                mutable unsigned long  CheckCount;  ///< Used in order to avoid processing things twice.
            };


            /// This class describes a brush (convex polyhedron) within a CollisionModelStaticT collision shape.
            /// It is conceptually very similar to and has in fact been created from the Brush3T<double> class.
            ///
            /// Brushes participate in traces (collision tests) and are exclusively used for volume contents tests.
            ///
            /// Note that most members (all but CheckCount) could and should be declared "const" - if not for the
            /// assignment operator that is required when storing elements of this class in ArrayT<>s.
            class BrushT
            {
                public:

                struct SideT
                {
                    Plane3dT       Plane;           ///< The plane of this side of the brush.
                    unsigned long* Vertices;        ///< The vertex indices of this polygon, in clockwise (CW) order (same as in Polygon3T<T>). Vertices itself points into Parent->m_BrushSideVIs, which in turn gives indices into Parent->m_Vertices.
                    unsigned long  NrOfVertices;    ///< The number of vertices (or rather, vertex indices).
                    MaterialT*     Material;        ///< The material of this side of the brush.
                };

                struct EdgeT
                {
                    unsigned long A;        ///< Index of the first  vertex of this edge.
                    unsigned long B;        ///< Index of the second vertex of this edge.
                };


                /// The default constructor.
                BrushT();

                void CacheHullVerts() const;
                void CacheHullEdges() const;

                /// Traces the given convex polyhedron from Start along Ray (up to the input value of Result.Fraction)
                /// through the brush, and reports the first collision, if any.
                /// For more detailed documentation, see CollisionModelT::TraceConvexSolid(), which has the same signature.
                /// @see CollisionModelT::TraceConvexSolid()
                /// @see TraceResultT
                void TraceConvexSolid(const InternalTraceSolidT& TraceSolid, const Vector3dT& Start, const Vector3dT& Ray, unsigned long ClipMask, TraceResultT& Result) const;

                void TraceRay(const Vector3dT& Start, const Vector3dT& Ray, unsigned long ClipMask, TraceResultT& Result) const;

                void TraceBevelBB(const BoundingBox3dT& TraceBB, const Vector3dT& Start, const Vector3dT& Ray, unsigned long ClipMask, TraceResultT& Result) const;


                CollisionModelStaticT* Parent;
                SideT*                 Sides;           ///< The sides of the brush (i.e. the planes whose intersection forms the brush). This points into Parent->m_BrushSides, the array of all sides of the collision shape the brush is a part of.
                unsigned long          NrOfSides;       ///< How many sides the brush has.

                BoundingBox3dT         BB;              ///< The bounding box (of the vertices) of this brush.
                unsigned long          Contents;        ///< The contents of this brush.
                mutable unsigned long  CheckCount;      ///< Used in order to avoid processing things twice.

                // Dependent information that is allocated and computed only *on demand*.
                // The "Hull" prefix is not to be taken literally, we could have used "Unique" as well.
                mutable unsigned long* HullVerts;       ///< The union of the vertex indices of the sides of this brush (i.e. the unique vertices whose convex hull forms the brush), referring into the m_Vertices list.
                mutable unsigned long  NrOfHullVerts;   ///< The number of unique vertices (or rather, vertex indices).
                mutable EdgeT*         HullEdges;       ///< The unique edges of this brush (e.g. edge <A, B> is not stored again as edge <B, A>).
                mutable unsigned long  NrOfHullEdges;   ///< The number of unique edges (or rather, edge indices).
            };


            class TerrainRefT
            {
                public:

                TerrainRefT();
                TerrainRefT(const TerrainT* Terrain_, MaterialT* Material_, const BoundingBox3dT& BB_);


                const TerrainT*        Terrain;         ///< The pointer to the actual terrain instance this class is referring to. The instances are kept "outside", as they are shared with the graphics world (scene graph).
                MaterialT*             Material;        ///< The material on the surface of this terrain.
                BoundingBox3dT         BB;              ///< The bounding box of this terrain.
                mutable unsigned long  CheckCount;      ///< Used in order to avoid processing things twice.
            };


            class NodeT
            {
                public:

                /// As the nodes of an octree are not subdivided by arbitrary planes, but only by planes that are parallel
                /// to the three major axes, we do not store a full Plane3dT member with the nodes, but only a "compacted"
                /// representation:
                /// The type expresses which axes the plane is parallel to, the distance is the offset from the origin.
                enum PlaneTypeE
                {
                    NONE = -1,  ///< No plane at all. Used for nodes that are actually leaves.
                    ALONG_X,    ///< A plane with normal vector (1, 0, 0), parallel to the y- and z-axis.
                    ALONG_Y,    ///< A plane with normal vector (0, 1, 0), parallel to the x- and z-axis.
                    ALONG_Z     ///< A plane with normal vector (0, 0, 1), parallel to the x- and y-axis.
                };


                /// The default constructor.
                /// Note that NodeTs are managed by cf::PoolTs, and thus it makes no sense to have anything but a default ctor.
                NodeT() : PlaneType(NONE), PlaneDist(0.0), Parent(NULL)
                {
                    Children[0] = NULL;
                    Children[1] = NULL;
                }

                /// Returns the bounding box of all contents (polygons, brushes, terrains) of this node.
                BoundingBox3dT GetBB() const;

                /// Returns the contents (the union of the contents of all polygons, brushes and terrains) of this node.
                unsigned long GetContents() const;

                /// Determines an axis-aligned split plane for further BSP partitioning of the contents of this node.
                /// For choosing the split plane, the method considers the bounding-box planes of all objects (polygons, brushes, terrains)
                /// of this node and all its ancestors, provided that they are wholly or partially in BB.
                /// When a split plane was found, the PlaneType and PlaneDist members are appropriately set and true is returned,
                /// otherwise they are initialized with NONE and 0, respectively, and the return value is false.
                ///
                /// @param NodeBB   The relevant bounds in which a split plane is to be found from the contents of this node (plus ancestors).
                /// @param MIN_NODE_SIZE   The minimum size (side length) that a node should not fall below.
                ///
                /// @returns whether a split plane has successfully been determined.
                bool DetermineSplitPlane(const BoundingBox3dT& NodeBB, const double MIN_NODE_SIZE);

                /// Determines whether the given BB intersects (is partly inside) each child of this node.
                /// @param BB   The bounding box that is tested for intersection.
                bool IntersectsAllChildren(const BoundingBox3dT& BB) const;

                /// Traces an object along a line segment through the tree that is rooted at this node.
                /// The line segment is defined by the points Start and Start+Ray == End.
                /// The parameters to this method specify a sub-segment of the line through Start and End as follows:
                ///     A = Start + Ray*FracA
                ///     B = Start + Ray*FracB
                /// This is mostly important for the recursive implementation of this method, you typically just call it like:
                ///     Trace(Start, End, 0, 1, Params);
                void Trace(const Vector3dT& A, const Vector3dT& B, double FracA, double FracB, const TraceParamsT& Params) const;

                /// Recursively inserts the given polygon into the (sub-)tree at and below this node.
                void Insert(const PolygonT* Poly);

                /// Recursively inserts the given brush into the (sub-)tree at and below this node.
                void Insert(const BrushT* Brush);

                /// Recursively inserts the given terrain into the (sub-)tree at and below this node.
                void Insert(const TerrainRefT* Terrain);


                PlaneTypeE                 PlaneType;       ///< The type of the plane that subdivides this node (no plane at all, normal vector along the x-, y- or z-axis).
                double                     PlaneDist;       ///< The distance of the plane to the origin. Corresponds to the Plane3fT::Dist member in a full plane.
                NodeT*                     Parent;          ///< The parent of this node, NULL if this is the root node.
                NodeT*                     Children[2];     ///< The children of this node at each side of the plane (NULL if there is no plane / the node is a leaf).
                ArrayT<const PolygonT*>    Polygons;        ///< The list of polygons in this node (used for traces, never for contents tests).
                ArrayT<const BrushT*>      Brushes;         ///< The list of brushes in this node (brushes are both for traces and contents tests).
                ArrayT<const TerrainRefT*> Terrains;        ///< The list of terrains in this node (used for traces, never for contents tests).
            };


            public:

            // CollisionModelStaticT(const std::string& Name_="");

            /// Constructor for creating a collision model by loading it from a file.
            /// TODO 1: Name should probably be a part of the InFile data...?
            /// TODO 2: Review serialization/deser. of class hierarchies (e.g. compare to cf::SceneGraph)!
            ///         Right now this is fixed and works for CollisionModelStaticTs only!!!
            CollisionModelStaticT(std::istream& InFile, cf::SceneGraph::aux::PoolT& Pool, const ArrayT<TerrainRefT>& Terrains);

            /// Constructor for creating a collision model from the brushes and patches of a MapFileEntityT.
            ///
            /// @param Entity              The entity to create the collision model from.
            /// @param Terrains            The pool of references to shared terrain instances.
            /// @param UseGenericBrushes   Whether generic brushes should be used, or variants with precomputed bevel planes.
            ///     When false, internally the same code that has been used in the Cafu engine for years
            ///     (in the cf::SceneGraph::BspTreeNodeT class) is used, and is thus *very* fast, rock solid and battle proven.
            ///     It also means that internally, only bounding-boxes (but not the true TraceSolidT objects) are traced against
            ///     such created brushes with the BrushT::TraceBevelBB() method.
            /// @param MAP_ROUND_EPSILON    The epsilon value used in roundoff error checks.
            /// @param MAP_MIN_VERTEX_DIST  The minimum distance between two vertices.
            /// @param BP_MAX_CURVE_ERROR   The maximum deviation of a segment against its true shape in an auto-segmented curve.
            /// @param BP_MAX_CURVE_LENGTH  The maximum length of a segment in an auto-segmented curve.
            /// @param MIN_NODE_SIZE        The minimum size (side length) that a node should not fall below.
            CollisionModelStaticT(const MapFileEntityT& Entity, const ArrayT<TerrainRefT>& Terrains, bool UseGenericBrushes,
                                  const double MAP_ROUND_EPSILON, const double MAP_MIN_VERTEX_DIST, const double BP_MAX_CURVE_ERROR, const double BP_MAX_CURVE_LENGTH, const double MIN_NODE_SIZE);

            /// Constructor for creating a collision model from a regular mesh.
            CollisionModelStaticT(unsigned long Width, unsigned long Height, const ArrayT<Vector3dT>& Mesh, MaterialT* Material, const double MIN_NODE_SIZE);

            /// The destructor.
            ~CollisionModelStaticT();


            // The CollisionModelT interface.
            BoundingBox3dT GetBoundingBox() const override;
            unsigned long GetContents() const override;
            void SaveToFile(std::ostream& OutFile, SceneGraph::aux::PoolT& Pool) const override;
            void TraceConvexSolid(const TraceSolidT& TraceSolid, const Vector3dT& Start, const Vector3dT& Ray, unsigned long ClipMask, TraceResultT& Result) const override;
            unsigned long GetContents(const Vector3dT& Point, double BoxRadius, unsigned long ContMask) const override;
            btCollisionShape* GetBulletAdapter() const override;


            private:

            // Forward-declaration of an adapter class that allows the Bullet Physics Library to use this class as a btConcaveShape.
            class BulletAdapterT;

            static unsigned long s_CheckCount;      ///< Used in order to avoid checking things more than once.

            CollisionModelStaticT(const CollisionModelStaticT&);    ///< Use of the Copy Constructor    is not allowed.
            void operator = (const CollisionModelStaticT&);         ///< Use of the Assignment Operator is not allowed.

            /// Auxiliary method for constructing a CollisionModelStaticT instance (called by the constructors).
            void BuildAxialBSPTree(NodeT* Node, const BoundingBox3dT& NodeBB, const double MIN_NODE_SIZE);

            // Memory pools from which we allocate our objects.
            // Memory pools allow the *quick* allocation of unknown many but large quantities of *individual* objects.
            cf::PoolNoFreeT<NodeT>         m_NodesPool;     ///< All the nodes of the orthogonal BSP tree of this model.
            cf::PoolNoFreeT<unsigned long> m_VerticesPool;  ///< The pool of vertex indices for the brushes.
            cf::PoolNoFreeT<BrushT::EdgeT> m_EdgesPool;     ///< The pool of edge indices for the brushes.

            // The data members that define a collision model.
            std::string           m_Name;           ///< The name of this model.
            BoundingBox3dT        m_BB;             ///< The spatial bounds of this model, caches the result of m_RootNode->GetBB().
            unsigned long         m_Contents;       ///< The contents flags of all model surfaces or'ed together, caches the result of m_RootNode->GetContents().
            NodeT*                m_RootNode;       ///< The root node of the orthogonal BSP tree of this model.
            BulletAdapterT*       m_BulletAdapter;  ///< The bullet adapter class instance that allows this class to be used as a btCollisionShape.

            ArrayT<PolygonT>      m_Polygons;       ///< The list of all polygons in this collision shape. Allocated (sized) only once, then never changed throughout the lifetime of this collision shape, so that pointers into it don't become invalid.
            bool                  m_GenericBrushes; ///< Whether our brushes are generic (the "normal" case), or whether they have precomputed bevel planes and can be used with bounding-boxes only.
            ArrayT<BrushT>        m_Brushes;        ///< The list of all brushes in this collision shape. Allocated (sized) only once, then never changed throughout the lifetime of this collision shape, so that pointers into it don't become invalid.
            ArrayT<BrushT::SideT> m_BrushSides;     ///< The list of all brush sides used in the brushes of this collision shape (BrushT::Sides points into this array). Allocated (sized) only once, then never changed throughout the lifetime of this collision shape, so that pointers into it don't become invalid.
            ArrayT<unsigned long> m_BrushSideVIs;   ///< The list of all vertex indices used in the brushes sides (BrushT::SideT::Vertices points into this array). Allocated (sized) only once, then never changed throughout the lifetime of this collision shape, so that pointers into it don't become invalid.
            ArrayT<Vector3dT>     m_Vertices;       ///< The list of all vertices in this model, shared by brushes and polygons.
            ArrayT<TerrainRefT>   m_Terrains;       ///< The list of all terrains in this model.
        };
    }
}

#endif
