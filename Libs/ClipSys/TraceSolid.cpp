/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "TraceSolid.hpp"


using namespace cf::ClipSys;


const Vector3dT TracePointT::s_Vertex = Vector3dT(0, 0, 0);


TraceBoxT::TraceBoxT(const BoundingBox3dT& BB)
{
    // No dynamic memory allocs from the heap!
    BB.GetCornerVertices(m_Vertices);

    m_Planes[0] = Plane3dT(Vector3dT( 1.0,  0.0,  0.0),  BB.Max.x);
    m_Planes[1] = Plane3dT(Vector3dT(-1.0,  0.0,  0.0), -BB.Min.x);
    m_Planes[2] = Plane3dT(Vector3dT( 0.0,  1.0,  0.0),  BB.Max.y);
    m_Planes[3] = Plane3dT(Vector3dT( 0.0, -1.0,  0.0), -BB.Min.y);
    m_Planes[4] = Plane3dT(Vector3dT( 0.0,  0.0,  1.0),  BB.Max.z);
    m_Planes[5] = Plane3dT(Vector3dT( 0.0,  0.0, -1.0), -BB.Min.z);
}


const TraceSolidT::EdgeT TraceBoxT::s_Edges[12] =
{
    { 1, 5 },
    { 5, 7 },
    { 7, 3 },
    { 3, 1 },
    { 0, 4 },
    { 4, 6 },
    { 6, 2 },
    { 2, 0 },
    { 0, 1 },
    { 4, 5 },
    { 6, 7 },
    { 2, 3 }
};


TraceGenericT::TraceGenericT()
{
}


void TraceGenericT::AssignInvTransformed(const TraceSolidT& Other, const cf::math::Matrix3x3dT& Mat)
{
    // Transform the vertices.
    {
        const unsigned int NumVerts = Other.GetNumVertices();
        const Vector3dT*   Verts    = Other.GetVertices();

        m_Vertices.Overwrite();
        m_Vertices.PushBackEmpty(NumVerts);

        for (unsigned int i = 0; i < NumVerts; i++)
            m_Vertices[i] = Mat.MulTranspose(Verts[i]);
    }

    // Transform the planes.
    {
        const unsigned int NumPlanes = Other.GetNumPlanes();
        const Plane3dT*    Planes    = Other.GetPlanes();

        m_Planes.Overwrite();
        m_Planes.PushBackEmpty(NumPlanes);

        for (unsigned int i = 0; i < NumPlanes; i++)
        {
            m_Planes[i].Normal = Mat.MulTranspose(Planes[i].Normal);
            m_Planes[i].Dist   = Planes[i].Dist;
        }
    }

    // The edges are not transformed, but must still be copied.
    {
        const unsigned int NumEdges = Other.GetNumEdges();
        const EdgeT*       Edges    = Other.GetEdges();

        m_Edges.Overwrite();
        m_Edges.PushBackEmpty(NumEdges);

        for (unsigned int i = 0; i < NumEdges; i++)
            m_Edges[i] = Edges[i];
    }
}
