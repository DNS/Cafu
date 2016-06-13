/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CLIPSYS_TRACESOLID_HPP_INCLUDED
#define CAFU_CLIPSYS_TRACESOLID_HPP_INCLUDED

#include "Math3D/BoundingBox.hpp"
#include "Math3D/Matrix3x3.hpp"
#include "Math3D/Plane3.hpp"


namespace cf
{
    namespace ClipSys
    {
        /// This class represents a solid object that can be traced through collision worlds, models and shapes.
        /// The shape of the object is defined as a convex polyhedron of arbitrary complexity.
        /// However, polyhedrons of the least possible complexity (regarding the number of vertices, planes and edges)
        /// are strongly preferable regarding performance considerations.
        ///
        /// Moreover, the solids must be "well defined" without degeneracies. In particular, this means:
        ///   - there must be no duplicate or colinear vertices,
        ///   - there must be no duplicate or redundant planes, and
        ///   - the solid must have a true positive volume.
        /// The only exception to these rules is a trace solid with exactly one vertex, no planes and no edges.
        /// Such trace solids are treated equivalently to point traces, so that special-case optimized code is employed.
        class TraceSolidT
        {
            public:

            /// This struct describes an edge of a TraceSolidT.
            struct EdgeT
            {
                unsigned int A;   ///< Index of the first  vertex of this edge.
                unsigned int B;   ///< Index of the second vertex of this edge.
            };


            /// The constructor.
            TraceSolidT() { }

            /// The virtual destructor.
            virtual ~TraceSolidT() { }

            /// Returns the bounding-box of (the vertices of) this solid.
            BoundingBox3dT GetBB() const
            {
                BoundingBox3dT BB;

                for (unsigned int i = 0; i < GetNumVertices(); i++)
                    BB += GetVertices()[i];

                return BB;
            }

            /// Returns the number of vertices of this solid.
            virtual unsigned int GetNumVertices() const = 0;

            /// Returns the vertices of this solid.
            virtual const Vector3dT* GetVertices() const = 0;

            /// Returns the number of planes of this solid.
            virtual unsigned int GetNumPlanes() const = 0;

            /// Returns the planes of this solid.
            virtual const Plane3dT* GetPlanes() const = 0;

            /// Returns the number of edges of this solid.
            virtual unsigned int GetNumEdges() const = 0;

            /// Returns the edges of this solid.
            virtual const EdgeT* GetEdges() const = 0;
        };


        /// This class represents a convex solid in the shape of a point.
        /// Traces of TracePointT solids are effectively "ray" traces, for which the
        /// collision model implementations apply special optimizations.
        class TracePointT : public TraceSolidT
        {
            public:

            /// Creates a convex solid in the shape of a point.
            TracePointT() { }

            // Base class overrides.
            unsigned int GetNumVertices() const override { return 1; }
            const Vector3dT* GetVertices() const override { return &s_Vertex; }
            unsigned int GetNumPlanes() const override { return 0; }
            const Plane3dT* GetPlanes() const override { return NULL; }
            unsigned int GetNumEdges() const override { return 0; }
            const EdgeT* GetEdges() const override { return NULL; }


            private:

            const static Vector3dT s_Vertex;
        };


        /// This class represents a convex solid in the shape of a (bounding-)box.
        class TraceBoxT : public TraceSolidT
        {
            public:

            /// Creates a trace solid from (in the shape of) the given axis-aligned bounding-box.
            TraceBoxT(const BoundingBox3dT& BB);

            // Base class overrides.
            unsigned int GetNumVertices() const override { return 8; }
            const Vector3dT* GetVertices() const override { return m_Vertices; }
            unsigned int GetNumPlanes() const override { return 6; }
            const Plane3dT* GetPlanes() const override { return m_Planes; }
            unsigned int GetNumEdges() const override { return 12; }
            const EdgeT* GetEdges() const override { return s_Edges; }


            private:

            Vector3dT m_Vertices[8];
            Plane3dT  m_Planes[6];

            const static EdgeT s_Edges[12];
        };


        /// This class represents a generic convex solid of arbitrary shape.
        class TraceGenericT : public TraceSolidT
        {
            public:

            /// Creates an empty (invalid) trace model.
            TraceGenericT();

            /// Assigns the given solid to this one, transformed by the *transpose* of the given matrix.
            /// Compared to creating a new trace solid, this can significantly reduce or even
            /// completely eliminate the required memory (re-)allocations.
            void AssignInvTransformed(const TraceSolidT& Other, const math::Matrix3x3dT& Mat);

            // Base class overrides.
            unsigned int GetNumVertices() const override { return m_Vertices.Size(); }
            const Vector3dT* GetVertices() const override { return &m_Vertices[0]; }
            unsigned int GetNumPlanes() const override { return m_Planes.Size(); }
            const Plane3dT* GetPlanes() const override { return &m_Planes[0]; }
            unsigned int GetNumEdges() const override { return m_Edges.Size(); }
            const EdgeT* GetEdges() const override { return &m_Edges[0]; }


            private:

            ArrayT<Vector3dT> m_Vertices;
            ArrayT<Plane3dT>  m_Planes;
            ArrayT<EdgeT>     m_Edges;
        };
    }
}

#endif
