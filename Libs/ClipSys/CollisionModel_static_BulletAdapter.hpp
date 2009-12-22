/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#ifndef _CF_CLIPSYS_COLLISION_MODEL_STATIC_BULLET_ADAPTER_HPP_
#define _CF_CLIPSYS_COLLISION_MODEL_STATIC_BULLET_ADAPTER_HPP_

#include "CollisionModel_static.hpp"
#include "BulletCollision/CollisionShapes/btConcaveShape.h"


namespace cf
{
    namespace ClipSys
    {
        /// This class provides an adapter for CollisionModelStaticT instances to be used as btConcaveShape instances.
        ///
        /// Design notes:
        /// Its implementation must know and be able to access the private implementation details of class CollisionModelStaticT,
        /// so it is a friend class of CollisionModelStaticT.
        /// It is self-suggesting and logical to have each CollisionModelStaticT instance keep its related CollisionModelStaticT::BulletAdapterT
        /// instance, as everything else is burdening and confusing to the user (though possible).
        class CollisionModelStaticT::BulletAdapterT : public btConcaveShape
        {
            public:

            /// Constructor.
            BulletAdapterT(const CollisionModelStaticT& CollMdl);


            // The btCollisionShape interface.
            void getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const;
            void setLocalScaling(const btVector3& scaling) { m_LocalScale=scaling; }
            const btVector3& getLocalScaling() const { return m_LocalScale; }
            void calculateLocalInertia(btScalar mass, btVector3& inertia) const;
            const char* getName() const { return "CollisionModelStaticT::BulletAdapterT"; }

            // The btConcaveShape interface.
            void processAllTriangles(btTriangleCallback* callback, const btVector3& aabbMin, const btVector3& aabbMax) const;


            private:

            btVector3 conv(const Vector3dT& v) const { return btVector3(btScalar(v.x), btScalar(v.y), btScalar(v.z)); }   ///< Inline conversion from Vector3dT to btVector3.
            btVector3 conv(const Vector3fT& v) const { return btVector3(btScalar(v.x), btScalar(v.y), btScalar(v.z)); }   ///< Inline conversion from Vector3fT to btVector3.
            void      ProcessTriangles(NodeT* Node, btTriangleCallback* callback, const BoundingBox3dT& BB) const;

            const CollisionModelStaticT& m_CollMdl;     ///< The static collision model (our "parent") we provide an adapter for.
            btVector3                    m_LocalScale;  ///< A member required in order to implement the btCollisionShape interface (this class does not actually support local scaling).
        };
    }
}

#endif
