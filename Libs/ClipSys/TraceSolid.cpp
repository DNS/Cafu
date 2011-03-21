/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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
    Vertices.PushBackEmptyExact(8);
    BB.GetCornerVertices(&Vertices[0]);

    Planes.PushBackEmptyExact(6);
    Planes[0]=Plane3dT(Vector3dT( 1.0,  0.0,  0.0),  BB.Max.x);
    Planes[1]=Plane3dT(Vector3dT(-1.0,  0.0,  0.0), -BB.Min.x);
    Planes[2]=Plane3dT(Vector3dT( 0.0,  1.0,  0.0),  BB.Max.y);
    Planes[3]=Plane3dT(Vector3dT( 0.0, -1.0,  0.0), -BB.Min.y);
    Planes[4]=Plane3dT(Vector3dT( 0.0,  0.0,  1.0),  BB.Max.z);
    Planes[5]=Plane3dT(Vector3dT( 0.0,  0.0, -1.0), -BB.Min.z);

    Edges.PushBackEmptyExact(12);
    Edges[ 0].A=1; Edges[ 0].B=5;
    Edges[ 1].A=5; Edges[ 1].B=7;
    Edges[ 2].A=7; Edges[ 2].B=3;
    Edges[ 3].A=3; Edges[ 3].B=1;
    Edges[ 4].A=0; Edges[ 4].B=4;
    Edges[ 5].A=4; Edges[ 5].B=6;
    Edges[ 6].A=6; Edges[ 6].B=2;
    Edges[ 7].A=2; Edges[ 7].B=0;
    Edges[ 8].A=0; Edges[ 8].B=1;
    Edges[ 9].A=4; Edges[ 9].B=5;
    Edges[10].A=6; Edges[10].B=7;
    Edges[11].A=2; Edges[11].B=3;
}
