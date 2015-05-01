/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2014 Carsten Fuchs Software.

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

#include "TraceSolid.hpp"


using namespace cf::ClipSys;


TraceSolidT::TraceSolidT()
{
}


TraceSolidT::TraceSolidT(const BoundingBox3dT& BB)
{
    SetBB(BB);
}


void TraceSolidT::SetBB(const BoundingBox3dT& BB)
{
    m_Vertices.Overwrite();
    m_Vertices.PushBackEmptyExact(8);

    BB.GetCornerVertices(&m_Vertices[0]);

    m_Planes.Overwrite();
    m_Planes.PushBackEmptyExact(6);

    m_Planes[0] = Plane3dT(Vector3dT( 1.0,  0.0,  0.0),  BB.Max.x);
    m_Planes[1] = Plane3dT(Vector3dT(-1.0,  0.0,  0.0), -BB.Min.x);
    m_Planes[2] = Plane3dT(Vector3dT( 0.0,  1.0,  0.0),  BB.Max.y);
    m_Planes[3] = Plane3dT(Vector3dT( 0.0, -1.0,  0.0), -BB.Min.y);
    m_Planes[4] = Plane3dT(Vector3dT( 0.0,  0.0,  1.0),  BB.Max.z);
    m_Planes[5] = Plane3dT(Vector3dT( 0.0,  0.0, -1.0), -BB.Min.z);

    m_Edges.Overwrite();
    m_Edges.PushBackEmptyExact(12);

    m_Edges[ 0].A = 1; m_Edges[ 0].B = 5;
    m_Edges[ 1].A = 5; m_Edges[ 1].B = 7;
    m_Edges[ 2].A = 7; m_Edges[ 2].B = 3;
    m_Edges[ 3].A = 3; m_Edges[ 3].B = 1;
    m_Edges[ 4].A = 0; m_Edges[ 4].B = 4;
    m_Edges[ 5].A = 4; m_Edges[ 5].B = 6;
    m_Edges[ 6].A = 6; m_Edges[ 6].B = 2;
    m_Edges[ 7].A = 2; m_Edges[ 7].B = 0;
    m_Edges[ 8].A = 0; m_Edges[ 8].B = 1;
    m_Edges[ 9].A = 4; m_Edges[ 9].B = 5;
    m_Edges[10].A = 6; m_Edges[10].B = 7;
    m_Edges[11].A = 2; m_Edges[11].B = 3;
}


void TraceSolidT::AssignInvTransformed(const TraceSolidT& Other, const cf::math::Matrix3x3dT& Mat)
{
    // Transform the vertices.
    m_Vertices.Overwrite();
    m_Vertices.PushBackEmpty(Other.GetNumVertices());

    for (unsigned int i = 0; i < Other.GetNumVertices(); i++)
        m_Vertices[i] = Mat.MulTranspose(Other.GetVertices()[i]);

    // Transform the planes.
    m_Planes.Overwrite();
    m_Planes.PushBackEmpty(Other.GetNumPlanes());

    for (unsigned int i = 0; i < Other.GetNumPlanes(); i++)
    {
        m_Planes[i].Normal = Mat.MulTranspose(Other.GetPlanes()[i].Normal);
        m_Planes[i].Dist   = Other.GetPlanes()[i].Dist;
    }

    // The edges are not transformed, but must still be copied.
    m_Edges.Overwrite();
    m_Edges.PushBackEmpty(Other.GetNumEdges());

    for (unsigned int i = 0; i < Other.GetNumEdges(); i++)
        m_Edges[i] = Other.GetEdges()[i];
}
