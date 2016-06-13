/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CLIPSYS_COLLISION_MODEL_STATIC_BULLET_ADAPTER_HPP_INCLUDED
#define CAFU_CLIPSYS_COLLISION_MODEL_STATIC_BULLET_ADAPTER_HPP_INCLUDED

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
            void getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const override;
            void setLocalScaling(const btVector3& scaling) override { m_LocalScale = scaling; }
            const btVector3& getLocalScaling() const override { return m_LocalScale; }
            void calculateLocalInertia(btScalar mass, btVector3& inertia) const override;
            const char* getName() const override { return "CollisionModelStaticT::BulletAdapterT"; }

            // The btConcaveShape interface.
            void processAllTriangles(btTriangleCallback* callback, const btVector3& aabbMin, const btVector3& aabbMax) const override;


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
