/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MATH_BEZIERPATCH_HPP_INCLUDED
#define CAFU_MATH_BEZIERPATCH_HPP_INCLUDED

#include "Math3D/Vector3.hpp"
#include "Templates/Array.hpp"

#include <cassert>


namespace cf
{
    namespace math
    {
        /* class ControlDataT
        {
            public:

            unsigned long Width;
            unsigned long Height;

            ArrayT< Vector3T<T> > Coords;
            ArrayT< Vector3T<T> > TexCoords;
        }; */


        /// This class represents a mesh that approximates a Bezier patch.
        /// Two invariants are possible:
        /// a) Before one of the Subdivide() methods has been called, the mesh is considered to be a control mesh.
        ///    A control mesh consists alternatingly of interpolation and approximation vertices which are quasi a
        ///    composition of 3x3 sub-patches with shared borders. The width and height therefore must be odd numbers >= 3.
        /// b) After one of the Subdivide() methods has been called, the mesh is just an approximation of the Bezier patch.
        ///    Contrary to a), all it's vertices lie on the patch curve.
        template<class T>
        class BezierPatchT
        {
            public:

            /// Represents a single vertex.
            struct VertexT
            {
                Vector3T<T> Coord;    ///< Vertex coordinate.
                Vector3T<T> TexCoord; ///< Vertex texture coordinates.
                Vector3T<T> Normal;   ///< Vertex normal.
                Vector3T<T> TangentS; ///< DOCTODO.
                Vector3T<T> TangentT; ///< DOCTODO.

                /// Computes the values of this vertex as the average of the given vertices A and B.
                void Average(const VertexT& A, const VertexT& B);
            };


            /// Constructor for creating an empty Bezier patch.
            BezierPatchT();

            /// Constructor for creating a patch from given coordinates.
            BezierPatchT(unsigned long Width_, unsigned long Height_, const ArrayT< Vector3T<T> >& Coords_);

            /// Constructor for creating a patch from given coordinates and texture-coordinates.
            BezierPatchT(unsigned long Width_, unsigned long Height_, const ArrayT< Vector3T<T> >& Coords_, const ArrayT< Vector3T<T> >& TexCoords_);

            /// (Re-)computes the tangent space vectors (normal and tangents) for each vertex of the mesh.
            /// This method must only be called *before* any of the Subdivide() methods is called, it does not work thereafter.
            void ComputeTangentSpace();

            /// (Re-)computes the tangent space vectors (normal and tangents) for each vertex of the mesh.
            /// This method is supposed to be called before one of the Subdivide() methods is called, but it is actually independent of it.
            /// That is, it does not matter whether this method is called first and Subdivide() second, or vice versa - both orders are
            /// possible and valid, and they should even yield the same result.
            /// OBSOLETE NOTE: This method does not really work reliable, the tangent space axes that it generates are frequently questionable...
            ///                Instead of debugging its code, I rather provide a completely new implementation in a seperate method.
            void ComputeTangentSpace_Obsolete();

            /// This method subdivides the patch "automatically", that is, by the given maximal error and length bounds.
            /// @param MaxError     The maximum allowed spatial distance between the computed approximation mesh and the true mathematical curve.
            /// @param MaxLength    The maximum side length that one mesh element may have.
            /// @param OptimizeFlat If true unnecessary vertices from flat substrips are removed.
            void Subdivide(T MaxError, T MaxLength, bool OptimizeFlat=true);

            /// Subdivides the patch "manually", that is, as often as explicitly stated by the parameter values.
            /// @param SubDivsHorz  The number of horizontal subdivisions that each 3x3 sub-patch is subdivided into.
            /// @param SubDivsVert  The number of vertical   subdivisions that each 3x3 sub-patch is subdivided into.
            /// @param OptimizeFlat If true unnecessary vertices from flat substrips are removed.
            void Subdivide(unsigned long SubDivsHorz, unsigned long SubDivsVert, bool OptimizeFlat=true);

            /// Intended to be called *after* one of the Subdivide() methods has been called,
            /// this methods linearly subdivides the rows and columns of the mesh so that the MaxLength is never exceeded.
            /// This is useful/intended for obtaining meshes for lightmap computation purposes.
            /// Note that this method quasi does the very opposite from what OptimizeFlatRowAndColumnStrips() does:
            /// it inserts "flat" (linear) rows and columns of vertices.
            /// Q: Couldn't we just call   Subdivide(LargeNum, MaxLength, false);   to achieve the same result?
            /// A: No!! Two counter examples, both observed with the handrails in the TechDemo map:
            ///    a) The end pads are small 3x3 patches with sides shorter than MaxLength. However they are not
            ///       reduced to 2x2 meshes when Subdivide() is called with OptimizeFlat being set to false.
            ///    b) The rails themselves suffer from the same problem (at each of the four cylindrical sides),
            ///       *plus* they rely on their explicitly stated number of subdivisions for ignoring their central
            ///       interpolating control vertex, whose consideration would deform the handrail!
            /// @param MaxLength Maximum length of the subdivisions.
            void ForceLinearMaxLength(T MaxLength);

            /// Returns the area of the Bezier patch surface around vertex (i, j).
            T GetSurfaceAreaAtVertex(unsigned long i, unsigned long j) const;

            /// Returns the const mesh vertex at (i, j).
            const VertexT& GetVertex(unsigned long i, unsigned long j) const { assert(i<Width && j<Height); return Mesh[j*Width+i]; }

            /// Returns the (non-const) mesh vertex at (i, j).
            VertexT& GetVertex(unsigned long i, unsigned long j) { assert(i<Width && j<Height); return Mesh[j*Width+i]; }

            bool WrapsHorz() const;  ///< Returns whether the left and right borders are identical.
            bool WrapsVert() const;  ///< Returns whether the top and bottom borders are identical.


            // The mesh is a regular array of Width*Height components, stored as a series of rows.
            // The GetMesh() methods are provided for convenient access.
            // TODO: Make private and provide const get methods instead?
            unsigned long   Width;  ///< Number of vertices in width direction.
            unsigned long   Height; ///< Number of vertices in height direction.
            ArrayT<VertexT> Mesh;   ///< Array of Width*Heights vertices that build up the bezier patch.


            private:

            // Helper methods for the Subdivide() methods.
            void        SetMeshSize(unsigned long NewWidth, unsigned long NewHeight);
            VertexT     SampleSinglePatchPoint(const VertexT SubPatch[3][3], const T u, const T v) const;
            void        SampleSinglePatch(const VertexT SubPatch[3][3], unsigned long baseCol, unsigned long baseRow, unsigned long TargetWidth, unsigned long SubDivsHorz, unsigned long SubDivsVert, ArrayT<VertexT>& TargetMesh) const;
            Vector3T<T> ProjectPointOntoVector(const Vector3T<T>& Point, const Vector3T<T>& Start, const Vector3T<T>& End) const;
            void        OptimizeFlatRowAndColumnStrips();
            bool        ComputeTangentSpaceInSubPatch(unsigned long sp_i, unsigned long sp_j, const T s, const T t, Vector3T<T> Axes[3]) const;
        };
    }
}

#endif
