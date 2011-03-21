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

#include "CollisionModel_static_BulletAdapter.hpp"
#include "Terrain/Terrain.hpp"


using namespace cf::ClipSys;


CollisionModelStaticT::BulletAdapterT::BulletAdapterT(const CollisionModelStaticT& CollMdl)
    : m_CollMdl(CollMdl),
      m_LocalScale(1, 1, 1)
{
    m_shapeType=CUSTOM_CONCAVE_SHAPE_TYPE;
}


void CollisionModelStaticT::BulletAdapterT::getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const
{
    const BoundingBox3dT& BB=m_CollMdl.m_BB;

    const btVector3 localBBMin      =conv(BB.Min);
    const btVector3 localBBMax      =conv(BB.Max);
    const btVector3 localHalfExtents=(localBBMax-localBBMin)*0.5f + btVector3(getMargin(), getMargin(), getMargin());
    const btVector3 localCenter     =(localBBMax+localBBMin)*0.5f;

    const btMatrix3x3 abs_b=t.getBasis().absolute();

    const btVector3 center=t(localCenter);
    const btVector3 extent=btVector3(abs_b[0].dot(localHalfExtents),
                                     abs_b[1].dot(localHalfExtents),
                                     abs_b[2].dot(localHalfExtents));

    aabbMin=center-extent;
    aabbMax=center+extent;
}


void CollisionModelStaticT::BulletAdapterT::calculateLocalInertia(btScalar /*mass*/, btVector3& inertia) const
{
    // Moving concave objects are not supported.
    inertia.setValue(btScalar(0), btScalar(0), btScalar(0));
}


void CollisionModelStaticT::BulletAdapterT::processAllTriangles(btTriangleCallback* callback, const btVector3& aabbMin, const btVector3& aabbMax) const
{
    BoundingBox3dT ProcBB(Vector3dT(aabbMin[0], aabbMin[1], aabbMin[2])*1000.0,
                          Vector3dT(aabbMax[0], aabbMax[1], aabbMax[2])*1000.0);

    if (ProcBB.Intersects(m_CollMdl.m_BB))
    {
        CollisionModelStaticT::s_CheckCount++;

        ProcessTriangles(m_CollMdl.m_RootNode, callback, ProcBB);
    }
}


void CollisionModelStaticT::BulletAdapterT::ProcessTriangles(NodeT* Node, btTriangleCallback* callback, const BoundingBox3dT& ProcBB) const
{
    btVector3 Triangle[3];

    while (true)
    {
        for (unsigned long PolyNr=0; PolyNr<Node->Polygons.Size(); PolyNr++)
        {
            const PolygonT* Poly=Node->Polygons[PolyNr];

            // Processed this polygon already?
            if (Poly->CheckCount==s_CheckCount) continue;
            Poly->CheckCount=s_CheckCount;

            assert(Poly->Parent==&m_CollMdl);

            // if (!ProcBB.Intersects(Poly->GetBB())) continue;

            Triangle[0]=conv(m_CollMdl.m_Vertices[Poly->Vertices[0]]/1000.0);
            Triangle[1]=conv(m_CollMdl.m_Vertices[Poly->Vertices[1]]/1000.0);
            Triangle[2]=conv(m_CollMdl.m_Vertices[Poly->Vertices[2]]/1000.0);

            callback->processTriangle(Triangle, 0, PolyNr);

            if (!Poly->IsTriangle())
            {
                Triangle[1]=Triangle[2];
                Triangle[2]=conv(m_CollMdl.m_Vertices[Poly->Vertices[3]]/1000.0);

                callback->processTriangle(Triangle, 1, PolyNr);
            }
        }

        for (unsigned long BrushNr=0; BrushNr<Node->Brushes.Size(); BrushNr++)
        {
            const BrushT* Brush=Node->Brushes[BrushNr];

            // Processed this brush already?
            if (Brush->CheckCount==s_CheckCount) continue;
            Brush->CheckCount=s_CheckCount;

            assert(Brush->Parent==&m_CollMdl);

            if (!ProcBB.Intersects(Brush->BB)) continue;

            for (unsigned long SideNr=0; SideNr<Brush->NrOfSides; SideNr++)
            {
                const BrushT::SideT& Side=Brush->Sides[SideNr];

                if (Side.NrOfVertices<3) continue;  // This can happen if Side is a precomputed bevel plane.

                Triangle[0]=conv(m_CollMdl.m_Vertices[Side.Vertices[0]]/1000.0);
             // Triangle[1]=...;
                Triangle[2]=conv(m_CollMdl.m_Vertices[Side.Vertices[1]]/1000.0);

                for (unsigned long VertexNr=2; VertexNr<Side.NrOfVertices; VertexNr++)
                {
                    Triangle[1]=Triangle[2];
                    Triangle[2]=conv(m_CollMdl.m_Vertices[Side.Vertices[VertexNr]]/1000.0);

                    callback->processTriangle(Triangle, 2, SideNr);
                }
            }
        }

        for (unsigned long TerrainNr=0; TerrainNr<Node->Terrains.Size(); TerrainNr++)
        {
            const TerrainRefT* Terrain=Node->Terrains[TerrainNr];

            // Processed this terrain already?
            if (Terrain->CheckCount==s_CheckCount) continue;
            Terrain->CheckCount=s_CheckCount;

            if (!ProcBB.Intersects(Terrain->BB)) continue;

            const Vector3dT          TerWrldSize=Terrain->BB.Max - Terrain->BB.Min;
            const int                TerGridSize=Terrain->Terrain->GetSize();
            const TerrainT::VertexT* TerVertices=Terrain->Terrain->GetVertices();

            const int GridMinX=std::max(            0, int( (ProcBB.Min.x-Terrain->BB.Min.x) / TerWrldSize.x * double(TerGridSize-1) ));
            const int GridMinY=std::max(            0, int( (ProcBB.Min.y-Terrain->BB.Min.y) / TerWrldSize.y * double(TerGridSize-1) ));
            const int GridMaxX=std::min(TerGridSize-1, int( (ProcBB.Max.x-Terrain->BB.Min.x) / TerWrldSize.x * double(TerGridSize-1) ) + 1);
            const int GridMaxY=std::min(TerGridSize-1, int( (ProcBB.Max.y-Terrain->BB.Min.y) / TerWrldSize.y * double(TerGridSize-1) ) + 1);

            for (int y=GridMinY; y<GridMaxY; y++)
            {
             // Triangle[0]=...;
                Triangle[1]=conv(TerVertices[(GridMinX)+TerGridSize*(y+1)]/1000.0f);
                Triangle[2]=conv(TerVertices[(GridMinX)+TerGridSize*(y  )]/1000.0f);

                for (int x=GridMinX; x<GridMaxX; x++)
                {
                    Triangle[0]=Triangle[2];
                    Triangle[2]=conv(TerVertices[(x+1)+TerGridSize*(y+1)]/1000.0f);

                    callback->processTriangle(Triangle, 3, x+TerGridSize*y);

                    Triangle[1]=Triangle[2];
                    Triangle[2]=conv(TerVertices[(x+1)+TerGridSize*(y  )]/1000.0f);

                    callback->processTriangle(Triangle, 4, x+TerGridSize*y);
                }
            }
        }


        if (Node->PlaneType==NodeT::NONE) break;

        if (ProcBB.Min[Node->PlaneType] >= Node->PlaneDist)
        {
            Node=Node->Children[0];
        }
        else if (ProcBB.Max[Node->PlaneType] <= Node->PlaneDist)
        {
            Node=Node->Children[1];
        }
        else
        {
            ProcessTriangles(Node->Children[1], callback, ProcBB);
            Node=Node->Children[0];
        }
    }
}
