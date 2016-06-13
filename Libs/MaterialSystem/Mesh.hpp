/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/************/
/*** Mesh ***/
/************/

#ifndef CAFU_MATSYS_MESH_HPP_INCLUDED
#define CAFU_MATSYS_MESH_HPP_INCLUDED

#include "Templates/Array.hpp"
#include "Math3D/Vector3.hpp"


namespace MatSys
{
    /// This class represents a polygonal mesh.
    //
    // Current problems with meshes:
    // - Would like to have Origin, Normal, Tangent etc. as Vector3T<float>s.
    // - But Origin needs 4 components e.g. for stencil shadows.
    // - Don't really want to add a w-component for Vector3T<float>s (although realistically possible).
    // - Who says that the representation of a Vertex with Vector3T<float> members matches the packing and alignment requirements of vertex-arrays and VBOs?
    //   Don't want to (and can't) enforce a particular struct-alignment on Vector3T<float>s that's MatSys specific,
    //   because Vector3T<>s are intended for universal employment, not just for the MatSys.
    // - In the future, different Renderers might require different internal Mesh representations,
    //   e.g. as a GLfloat[24] for each vertex.
    // - The requirements of vertex-arrays or especially VBOs (locking, handles, etc.) cannot really be foreseen yet,
    //   e.g., do we have to have something like RendererI::GetVBOMesh() ?
    //
    // ==> Abstract meshes, hide their details behind a pure virtual interface, implemented by each Renderer.
    //     They might be constructed by more concrete meshes, similar to the current MeshT struct below,
    //     or "mesh builders" that are free to construct meshes in any way they like.
    //     I don't know yet whether the future mesh interface should have methods for manipulating its vertices or not
    //     (e.g. SetOrigin(VertexNr, const Vector3T<float>& Pos)). Having such methods might be complicated to implement (e.g. with VBOs).
    //     Not having such methods would leave us with a handler-type pointer to mesh interfaces, similar to RenderMaterials.
    //
    // ==> TODO: 1. Clarify the exact requirements of vertex-arrays for each Renderer, inclusive OpenGL 2.0.
    //           2. Clarify the exact requirements of VBOs          for each Renderer, inclusive OpenGL 2.0.
    //           3. Keep the differences/requirements/existance between static and animated meshes in mind.
    //           4. Armed with that knowledge, revise the meshes.
    class MeshT
    {
        public:

        enum TypeT    { Points, Lines, LineStrip, LineLoop, Triangles, TriangleStrip, TriangleFan, Quads, QuadStrip, Polygon };
        enum WindingT { CW, CCW };

        struct VertexT
        {
            double Origin[4];           // The position (xyzw) in model space.
            float  Color[4];            // The color (rgba).

            float  TextureCoord[2];
            float  LightMapCoord[2];
            float  SHLMapCoord[2];

            float  Normal[3];           // The normal vector in model space.
            float  Tangent[3];
            float  BiNormal[3];

            float  UserAttribF[4];
            int    UserAttribI[4];


            // Methods for conveniently setting the components.
            void SetOrigin(double x=0.0, double y=0.0, double z=0.0, double w=1.0) { Origin[0]=x; Origin[1]=y; Origin[2]=z; Origin[3]=w; }
            void SetOrigin(const Vector3fT& Pos, float w=1.0) { Origin[0]=Pos.x; Origin[1]=Pos.y; Origin[2]=Pos.z; Origin[3]=w; }
            void SetColor(float r, float g, float b, float a=1.0) { Color[0]=r; Color[1]=g; Color[2]=b; Color[3]=a; }
            void SetTextureCoord(float s, float t) { TextureCoord[0]=s; TextureCoord[1]=t; }
            void SetLightMapCoord(float s, float t) { LightMapCoord[0]=s; LightMapCoord[1]=t; }
            void SetSHLMapCoord(float s, float t)  { SHLMapCoord[0]=s; SHLMapCoord[1]=t; }
            void SetNormal(float x, float y, float z) { Normal[0]=x; Normal[1]=y; Normal[2]=z; }
            void SetNormal(const Vector3T<float>& N) { Normal[0]=N.x; Normal[1]=N.y; Normal[2]=N.z; }
            void SetTangent(float x, float y, float z) { Tangent[0]=x; Tangent[1]=y; Tangent[2]=z; }
            void SetTangent(const Vector3T<float>& T) { Tangent[0]=T.x; Tangent[1]=T.y; Tangent[2]=T.z; }
            void SetBiNormal(float x, float y, float z) { BiNormal[0]=x; BiNormal[1]=y; BiNormal[2]=z; }
            void SetBiNormal(const Vector3T<float>& B) { BiNormal[0]=B.x; BiNormal[1]=B.y; BiNormal[2]=B.z; }
        };

        TypeT           Type;
        WindingT        Winding;    ///< The orientation (cw or ccw) of front faces.
        ArrayT<VertexT> Vertices;


        /// Constructor.
        MeshT(TypeT T=Points, WindingT W=CW) : Type(T), Winding(W)
        {
        }
    };
}

#endif
